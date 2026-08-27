/// @file
/// HttpsService: a TLS listener that speaks either HTTP/1.1 or SPDY/3.0,
/// selected by NPN (Next Protocol Negotiation). It backs the tst_spdy test:
/// it serves a small set of routes (an Apache-looking root page, echo/md5sum/
/// multipart CGIs, and files from a --folder) over whichever protocol the
/// client negotiates. The SPDY layer mirrors, in reverse, the framing and the
/// level-0/no-dictionary zlib header compression of Qt's own client-side
/// qspdyprotocolhandler.cpp.
#ifndef HTTPS_SPDY_H
#define HTTPS_SPDY_H

/// Http2Connection, for the "h2" ALPN branch below.
#include "https_http2.h"

/// QThread::msleep for the server-config.ini responseDelayMs hold.
#include <QtCore/QThread>
#include <QtNetwork/QSslSocket>
#include <QtNetwork/QSslConfiguration>
#include <QtNetwork/QSslKey>
#include <QtNetwork/QSslCertificate>
#include <QtNetwork/QSslPreSharedKeyAuthenticator>
#include <QtNetwork/QTcpServer>
#include <QtCore/QFile>
#include <QtCore/QFileInfo>
#include <QtCore/QCryptographicHash>
#include <QtCore/QDateTime>
#include <QtCore/QMap>
#include <QtCore/QtEndian>
#include <QtCore/QLocale>
#include <zlib.h>

/// TestService.
#include "service_base.h"
/// TestServer::port().
#include "../testserver.h"
#include "spdy3_dictionary.h"

/// The TLS listeners' cert+key, loaded once at startup from the --cert/--key files TestServer hands
/// us (see TestServer::serverCert()). No certificate is compiled into this binary or committed to the
/// tree -- main.cpp fills these in before any service starts.
extern QByteArray g_serverCertPem;
extern QByteArray g_serverKeyPem;

/// A protocol-agnostic reply: the same routing serves HTTP/1.1 and SPDY.
#include "http_reply.h"

// MARK: SPDY/3.0 connection.

class SpdyConnection : public QObject
{
    Q_OBJECT
public:
    SpdyConnection(QSslSocket *socket, const QString &root, QObject *parent = Q_NULLPTR)
        : QObject(parent), m_socket(socket), m_root(root)
    {
        memset(&m_inflate, 0, sizeof(m_inflate));
        memset(&m_deflate, 0, sizeof(m_deflate));
        inflateInit(&m_inflate);
        deflateInit(&m_deflate, 0); // level 0: no compression (matches the client, CRIME-safe)
        connect(m_socket, &QSslSocket::readyRead, this, &SpdyConnection::onReadyRead);
        connect(m_socket, &QSslSocket::disconnected, this, &QObject::deleteLater);
        onReadyRead(); // bytes may already be buffered
    }
    ~SpdyConnection()
    {
        inflateEnd(&m_inflate);
        deflateEnd(&m_deflate);
    }

private:
    struct Stream {
        QByteArray method, path, query, scheme, host, contentType;
        QByteArray body;
        qint64 sendWindow;
        Stream() : sendWindow(65536) {}
    };

    static quint32 be32(const char *p)
    {
        return (quint32(quint8(p[0])) << 24) | (quint32(quint8(p[1])) << 16)
             | (quint32(quint8(p[2])) << 8) | quint32(quint8(p[3]));
    }
    static void put32(QByteArray &b, quint32 v)
    {
        char c[4] = { char(v >> 24), char(v >> 16), char(v >> 8), char(v) };
        b.append(c, 4);
    }

    void onReadyRead()
    {
        m_buf += m_socket->readAll();
        forever {
            if (m_buf.size() < 8)
                return;
            const uchar b0 = uchar(m_buf.at(0));
            const quint32 length = (quint32(uchar(m_buf.at(5))) << 16)
                                 | (quint32(uchar(m_buf.at(6))) << 8) | quint32(uchar(m_buf.at(7)));
            const int frameSize = 8 + int(length);
            if (m_buf.size() < frameSize)
                return;
            const QByteArray frame = m_buf.left(frameSize);
            m_buf = m_buf.mid(frameSize);

            if (b0 & 0x80) { // control frame
                const int type = (int(uchar(frame.at(2))) << 8) | int(uchar(frame.at(3)));
                const uchar flags = uchar(frame.at(4));
                handleControl(type, flags, frame.mid(8));
            } else { // data frame
                const quint32 streamID = be32(frame.constData()) & 0x7fffffff;
                const uchar flags = uchar(frame.at(4));
                handleData(streamID, flags, frame.mid(8));
            }
        }
    }

    /// Inflate a SPDY header block into (name,value) pairs.
    bool inflateHeaders(const QByteArray &in, QMap<QByteArray, QByteArray> *out)
    {
        QByteArray raw;
        char chunk[2048];
        m_inflate.next_in = reinterpret_cast<Bytef *>(const_cast<char *>(in.constData()));
        m_inflate.avail_in = in.size();
        forever {
            m_inflate.next_out = reinterpret_cast<Bytef *>(chunk);
            m_inflate.avail_out = sizeof(chunk);
            int ret = inflate(&m_inflate, Z_SYNC_FLUSH);
            if (ret == Z_NEED_DICT)
                ret = inflateSetDictionary(&m_inflate, reinterpret_cast<const Bytef *>(spdyDictionary),
                                           spdyDictionaryLength), inflate(&m_inflate, Z_SYNC_FLUSH);
            const int produced = int(sizeof(chunk)) - int(m_inflate.avail_out);
            if (produced > 0)
                raw.append(chunk, produced);
            if (ret == Z_STREAM_END)
                break;
            if (m_inflate.avail_in == 0 && produced == 0)
                break;
            if (ret != Z_OK && ret != Z_BUF_ERROR)
                return false;
        }
        // parse: count(4) then [nameLen(4) name valueLen(4) value] x count
        if (raw.size() < 4)
            return false;
        int pos = 0;
        const quint32 count = be32(raw.constData()); pos += 4;
        for (quint32 i = 0; i < count; ++i) {
            if (pos + 4 > raw.size()) return false;
            const quint32 nlen = be32(raw.constData() + pos); pos += 4;
            if (pos + int(nlen) + 4 > raw.size()) return false;
            const QByteArray name = raw.mid(pos, nlen); pos += nlen;
            const quint32 vlen = be32(raw.constData() + pos); pos += 4;
            if (pos + int(vlen) > raw.size()) return false;
            const QByteArray value = raw.mid(pos, vlen); pos += vlen;
            out->insert(name, value);
        }
        return true;
    }

    /// Deflate a response header block (count + pairs) with the running stream.
    QByteArray deflateHeaders(const QList<QPair<QByteArray, QByteArray> > &pairs)
    {
        QByteArray raw;
        put32(raw, quint32(pairs.size()));
        for (int i = 0; i < pairs.size(); ++i) {
            put32(raw, quint32(pairs.at(i).first.size()));
            raw += pairs.at(i).first;
            put32(raw, quint32(pairs.at(i).second.size()));
            raw += pairs.at(i).second;
        }
        QByteArray out(raw.size() + 128, Qt::Uninitialized);
        m_deflate.next_in = reinterpret_cast<Bytef *>(raw.data());
        m_deflate.avail_in = raw.size();
        m_deflate.next_out = reinterpret_cast<Bytef *>(out.data());
        m_deflate.avail_out = out.size();
        deflate(&m_deflate, Z_SYNC_FLUSH);
        out.truncate(out.size() - int(m_deflate.avail_out));
        return out;
    }

    void sendControlFrame(int type, uchar flags, const QByteArray &body)
    {
        QByteArray f;
        f.append(char(0x80)); f.append(char(0x03));           // control, version 3
        f.append(char(type >> 8)); f.append(char(type & 0xff));
        f.append(char(flags));
        const quint32 len = body.size();
        f.append(char(len >> 16)); f.append(char(len >> 8)); f.append(char(len));
        f += body;
        m_socket->write(f);
    }

    void sendDataFrame(quint32 streamID, uchar flags, const QByteArray &data)
    {
        QByteArray f;
        put32(f, streamID & 0x7fffffff);
        f.append(char(flags));
        const quint32 len = data.size();
        f.append(char(len >> 16)); f.append(char(len >> 8)); f.append(char(len));
        f += data;
        m_socket->write(f);
    }

    void handleControl(int type, uchar flags, const QByteArray &body)
    {
        switch (type) {
        case 1: { // SYN_STREAM
            if (body.size() < 10)
                return;
            const quint32 streamID = be32(body.constData()) & 0x7fffffff;
            QMap<QByteArray, QByteArray> h;
            if (!inflateHeaders(body.mid(10), &h))
                return;
            Stream st;
            st.method = h.value(":method");
            st.scheme = h.value(":scheme");
            st.host = h.value(":host");
            st.contentType = h.value("content-type");
            splitTarget(h.value(":path"), &st.path, &st.query);
            m_streams.insert(streamID, st);
            if (flags & 0x01) // FLAG_FIN: no request body follows
                respond(streamID);
            break;
        }
        case 6: // PING -> echo back verbatim
            sendControlFrame(6, 0, body);
            break;
        case 9: { // WINDOW_UPDATE: streamID(4) + delta(4)
            if (body.size() >= 8) {
                const quint32 streamID = be32(body.constData()) & 0x7fffffff;
                const quint32 delta = be32(body.constData() + 4) & 0x7fffffff;
                if (m_streams.contains(streamID)) {
                    m_streams[streamID].sendWindow += delta;
                    flushPending(streamID);
                }
            }
            break;
        }
        default: // SETTINGS, RST_STREAM, GOAWAY, HEADERS: nothing to do here
            break;
        }
    }

    void handleData(quint32 streamID, uchar flags, const QByteArray &data)
    {
        if (!m_streams.contains(streamID))
            return;
        m_streams[streamID].body += data;
        // Replenish the peer's flow-control window (per-stream and session-wide),
        // or a client uploading more than the 64 KB initial window would stall.
        if (!data.isEmpty()) {
            sendWindowUpdate(streamID, data.size());
            sendWindowUpdate(0, data.size());
        }
        if (flags & 0x01) // FLAG_FIN
            respond(streamID);
    }

    void sendWindowUpdate(quint32 streamID, quint32 delta)
    {
        QByteArray body;
        put32(body, streamID & 0x7fffffff);
        put32(body, delta & 0x7fffffff);
        sendControlFrame(9 /* WINDOW_UPDATE */, 0, body);
    }

    void respond(quint32 streamID)
    {
        if (!m_streams.contains(streamID))
            return;
        const Stream st = m_streams.value(streamID);
        const HttpReply reply = buildReply(st.method, st.path, st.query, st.contentType, st.body, m_root);

        QList<QPair<QByteArray, QByteArray> > hp;
        hp.append(qMakePair(QByteArray(":status"),
                            QByteArray::number(reply.code) + ' ' + reply.reason));
        hp.append(qMakePair(QByteArray(":version"), QByteArray("HTTP/1.1")));
        for (int i = 0; i < reply.headers.size(); ++i)
            hp.append(reply.headers.at(i));
        hp.append(qMakePair(QByteArray("content-length"), QByteArray::number(reply.body.size())));

        const uchar synFlags = reply.body.isEmpty() ? 0x01 : 0x00; // FIN now if no body
        QByteArray syn;
        put32(syn, streamID & 0x7fffffff);
        syn += deflateHeaders(hp);
        sendControlFrame(2 /* SYN_REPLY */, synFlags, syn);

        m_pending.insert(streamID, reply.body);
        flushPending(streamID);
    }

    /// Send as much of the stream's pending body as the flow-control window allows.
    void flushPending(quint32 streamID)
    {
        if (!m_pending.contains(streamID) || !m_streams.contains(streamID))
            return;
        QByteArray &data = m_pending[streamID];
        qint64 &window = m_streams[streamID].sendWindow;
        while (!data.isEmpty() && window > 0) {
            const int n = int(qMin<qint64>(qMin<qint64>(data.size(), window), 8192));
            const QByteArray chunk = data.left(n);
            data = data.mid(n);
            window -= n;
            const uchar flags = data.isEmpty() ? 0x01 : 0x00; // FIN with the last chunk
            sendDataFrame(streamID, flags, chunk);
        }
        if (data.isEmpty()) {
            m_pending.remove(streamID);
            m_streams.remove(streamID);
        }
    }

    QSslSocket *m_socket;
    QString m_root;
    QByteArray m_buf;
    z_stream m_inflate;
    z_stream m_deflate;
    QMap<quint32, Stream> m_streams;
    QMap<quint32, QByteArray> m_pending;
};

// MARK: HTTP/1.1 over the same TLS socket.

class Http1Connection : public QObject
{
    Q_OBJECT
public:
    /// Takes a QTcpSocket so the same HTTP/1.1 responder serves both the plain
    /// listener and (via the QSslSocket subclass) the TLS listener's http/1.1 path.
    Http1Connection(QTcpSocket *socket, const QString &root, QObject *parent = Q_NULLPTR)
        : QObject(parent), m_socket(socket), m_root(root), m_continued(false)
    {
        connect(m_socket, &QTcpSocket::readyRead, this, &Http1Connection::onReadyRead);
        connect(m_socket, &QTcpSocket::disconnected, this, &QObject::deleteLater);
        onReadyRead();
    }

private:
    static QByteArray header(const QByteArray &head, const char *name)
    {
        const QList<QByteArray> lines = head.split('\n');
        const QByteArray key = QByteArray(name).toLower();
        for (int i = 1; i < lines.size(); ++i) {
            const int c = lines.at(i).indexOf(':');
            if (c > 0 && lines.at(i).left(c).trimmed().toLower() == key)
                return lines.at(i).mid(c + 1).trimmed();
        }
        return QByteArray();
    }

    void onReadyRead()
    {
        m_buf += m_socket->readAll();
        // A real HTTP request line begins with an uppercase method token. Anything else (most
        // usefully a TLS ClientHello, 0x16, from a client that spoke https:// to this plain-HTTP
        // port) is not HTTP. Answer it the way a real HTTP server does -- a plain-text "400 Bad
        // Request" -- then close: a TLS client reads that as a malformed record and fails the
        // handshake (tst_QNetworkReply::ioGetFromHttpsWithSslHandshakeError expects exactly that),
        // instead of the server blocking forever waiting for a request terminator.
        if (!m_buf.isEmpty() && (m_buf.at(0) < 'A' || m_buf.at(0) > 'Z')) {
            m_socket->write("HTTP/1.1 400 Bad Request\r\nContent-Length: 0\r\nConnection: close\r\n\r\n");
            m_socket->disconnectFromHost();
            return;
        }
        forever {
            // HTTP/0.9 simple request: one line "METHOD path" with no version and no
            // headers. The client expects the raw body back and the socket then closed.
            const int firstNl = m_buf.indexOf('\n');
            if (firstNl >= 0) {
                const QList<QByteArray> firstLine = m_buf.left(firstNl).simplified().split(' ');
                if (firstLine.size() == 2) {
                    QByteArray p09, q09;
                    splitTarget(firstLine.at(1), &p09, &q09);
                    const HttpReply reply = buildReply(firstLine.at(0).toUpper(), p09, q09,
                                                       QByteArray(), QByteArray(), m_root);
                    m_socket->write(reply.body);
                    m_socket->disconnectFromHost();
                    return;
                }
            }
            // Accept both CRLF and bare-LF header terminators (some tests send LF only).
            int headEnd = m_buf.indexOf("\r\n\r\n");
            int sepLen = 4;
            const int lfEnd = m_buf.indexOf("\n\n");
            if (lfEnd >= 0 && (headEnd < 0 || lfEnd < headEnd)) {
                headEnd = lfEnd;
                sepLen = 2;
            }
            if (headEnd < 0)
                return;
            const QByteArray head = m_buf.left(headEnd);
            const int bodyStart = headEnd + sepLen;

            // A client that announces "Expect: 100-continue" holds its body back until
            // it sees the interim response, so answer once before waiting for the body.
            if (!m_continued && header(head, "expect").toLower().contains("100-continue")) {
                m_socket->write("HTTP/1.1 100 Continue\r\n\r\n");
                m_continued = true;
            }

            // Request body: chunked (Transfer-Encoding) or Content-Length delimited.
            // A streamed upload (POST from a socket) arrives chunked with no length.
            QByteArray body;
            int consumed;
            if (header(head, "transfer-encoding").toLower().contains("chunked")) {
                if (!decodeChunkedBody(m_buf, bodyStart, &body, &consumed))
                    return; // wait for the rest of the chunks
            } else {
                const int clen = header(head, "content-length").toInt();
                consumed = bodyStart + clen;
                if (m_buf.size() < consumed)
                    return;
                body = m_buf.mid(bodyStart, clen);
            }
            m_buf = m_buf.mid(consumed);
            m_continued = false; // ready for the next pipelined request on this socket

            // The request line is head's first line. When the request carries no extra headers
            // (e.g. "GET / HTTP/1.0\r\n\r\n"), head has no embedded '\n' and head.left(-1) would be
            // empty -- take the whole head in that case, or the request would parse as a bogus 400.
            const int reqLineNl = head.indexOf('\n');
            const QByteArray reqLineRaw = reqLineNl >= 0 ? head.left(reqLineNl) : head;
            const QList<QByteArray> reqLine = reqLineRaw.simplified().split(' ');
            if (reqLine.size() < 2) {
                m_socket->write("HTTP/1.1 400 Bad Request\r\nContent-Length: 0\r\n\r\n");
                continue;
            }
            const QByteArray method = reqLine.at(0).toUpper();
            QByteArray path, query;
            splitTarget(reqLine.at(1), &path, &query);
            const HttpReply reply = buildReply(method, path, query,
                                               header(head, "content-type"), body, m_root, head);
            QByteArray resp = "HTTP/1.1 " + QByteArray::number(reply.code) + ' ' + reply.reason + "\r\n";
            resp += "Server: Apache\r\n"; // tst_QNetworkReply::getFromHttp checks the Server header names Apache
            for (int i = 0; i < reply.headers.size(); ++i)
                resp += reply.headers.at(i).first + ": " + reply.headers.at(i).second + "\r\n";
            resp += "Content-Length: " + QByteArray::number(reply.body.size()) + "\r\n\r\n";
            if (method != "HEAD") // a HEAD reply carries the headers and length, but no body
                resp += reply.body;
            // Honour server-config.ini [IO] responseDelayMs before answering. server-dummy is
            // single-threaded, so a blocking hold SERIALISES otherwise-concurrent exchanges -- the
            // parallel bad-credential auth rows need the first request's 401->retry to settle the
            // per-host auth cache before the second's 401 is processed. Default 0 = no delay.
            const int delayMs = IniWatcher::shared(g_configIniPath)->load()->responseDelayMs();
            if (delayMs > 0)
                QThread::msleep(uint(delayMs));
            m_socket->write(resp);

            // Close the connection when the client asked to ("Connection: close") or when it is an
            // HTTP/1.0 request that did not opt into keep-alive (HTTP/1.0 is non-persistent by
            // default) -- tst_QTcpSocket::zeroAndMinusOneReturns sends a trailing "GET / HTTP/1.0"
            // and waits for the server to hang up. HTTP/1.1 stays open (pipelining/keep-alive tests).
            const QByteArray connHdr = header(head, "connection").toLower();
            const bool http10 = reqLine.size() > 2 && reqLine.at(2).contains("HTTP/1.0");
            if (connHdr == "close" || (http10 && connHdr != "keep-alive")) {
                m_socket->disconnectFromHost();
                return;
            }
        }
    }

    QTcpSocket *m_socket;
    QString m_root;
    QByteArray m_buf;
    /// Sent a "100 Continue" for the in-flight request already.
    bool m_continued;
};

// MARK: plain HTTP/1.1 listener (the http-url row, port 8080).

class PlainHttpService : public TestService
{
public:
    PlainHttpService(const ServiceConfig &c, QObject *parent = Q_NULLPTR)
        : TestService(c, parent), m_root(c.folder) {}

    void start() Q_DECL_OVERRIDE
    {
        QTcpServer *server = new QTcpServer(this);
        listenTcp(server);
        connect(server, &QTcpServer::newConnection, this, &PlainHttpService::onNewConnection);
    }

private:
    void onNewConnection()
    {
        QTcpServer *server = qobject_cast<QTcpServer *>(sender());
        while (server->hasPendingConnections()) {
            QTcpSocket *s = server->nextPendingConnection();
            new Http1Connection(s, m_root, s);
        }
    }
    QString m_root;
};

/// Daytime service (port 13): on connect, write the time and close, so a client's
/// waitForDisconnected() succeeds. connectToHostEncrypted's tail connects here in
/// plain mode to check the socket drops back to UnencryptedMode.
class DaytimeService : public TestService
{
public:
    DaytimeService(const ServiceConfig &c, QObject *parent = Q_NULLPTR)
        : TestService(c, parent) {}

    void start() Q_DECL_OVERRIDE
    {
        QTcpServer *server = new QTcpServer(this);
        listenTcp(server);
        connect(server, &QTcpServer::newConnection, this, &DaytimeService::onNewConnection);
    }

private:
    void onNewConnection()
    {
        QTcpServer *server = qobject_cast<QTcpServer *>(sender());
        while (server->hasPendingConnections()) {
            QTcpSocket *c = server->nextPendingConnection();
            c->write(QDateTime::currentDateTime().toString(Qt::ISODate).toLatin1() + "\r\n");
            connect(c, &QTcpSocket::disconnected, c, &QObject::deleteLater);
            c->disconnectFromHost();
        }
    }
};

/// Map a proxy CONNECT / SOCKS target onto the local listeners: a proxied request
/// names the service by its public host and port (qt-test-server:443/80), but the
/// fleet actually serves it on loopback at the port TestServer::port() picks, so the proxy
/// (a separate process) rewrites the target to 127.0.0.1:4433/8080.
static inline void remapTarget(QString *host, quint16 *port)
{
    // Only the fleet's OWN hosts live on loopback stand-in ports; map those to where the fleet
    // actually serves them (127.0.0.1 at TestServer::port()). A proxied request that names some
    // OTHER host must be attempted as-is so its upstream lookup fails and the proxy reports the
    // error -- otherwise every target would resolve to the local fleet and a real proxy failure
    // (tst_QTcpSocket::hostNotFound through an HTTP proxy) could never surface.
    const bool known = *host == QLL("127.0.0.1")
                       || *host == QLL("::1")
                       || *host == QLL("localhost")
                       || host->endsWith(QLL("qt-local-server.test"))
                       || host->endsWith(QLL("qt-test-net"));
    if (!known)
        return;
    *port = TestServer::port(*port);
    *host = QString::fromLatin1("127.0.0.1");
}

// MARK: HTTP CONNECT proxy (ports 3128 no-auth, 3129 Basic-auth).

class ConnectProxyService : public TestService
{
public:
    ConnectProxyService(const ServiceConfig &c, QObject *parent = Q_NULLPTR)
        : TestService(c, parent), m_requireAuth(c.requireAuth) {}

    void start() Q_DECL_OVERRIDE
    {
        QTcpServer *server = new QTcpServer(this);
        listenTcp(server);
        connect(server, &QTcpServer::newConnection, this, &ConnectProxyService::onNewConnection);
    }

private:
    void onNewConnection()
    {
        QTcpServer *server = qobject_cast<QTcpServer *>(sender());
        while (server->hasPendingConnections()) {
            QTcpSocket *c = server->nextPendingConnection();
            connect(c, &QTcpSocket::readyRead, this, &ConnectProxyService::onReadyRead);
            connect(c, &QTcpSocket::disconnected, c, &QObject::deleteLater);
        }
    }
    void onReadyRead() { onData(qobject_cast<QTcpSocket *>(sender())); }

    void onData(QTcpSocket *client)
    {
        if (client->property("tunnel").toBool() || client->property("forwarding").toBool())
            return; // a CONNECT tunnel or an http forward is already relaying
        QByteArray buf = client->property("buf").toByteArray() + client->readAll();
        const int headEnd = buf.indexOf("\r\n\r\n");
        if (headEnd < 0) { client->setProperty("buf", buf); return; }
        const QByteArray head = buf.left(headEnd);
        const QList<QByteArray> lines = head.split('\n');
        const QList<QByteArray> reqLine = lines.value(0).simplified().split(' ');
        if (reqLine.size() < 2) {
            client->write("HTTP/1.1 400 Bad Request\r\nContent-Length: 0\r\n\r\n");
            client->disconnectFromHost();
            return;
        }
        const QByteArray method = reqLine.at(0).toUpper();

        // Proxy auth (Basic) gates BOTH the CONNECT tunnel and the http forward.
        if (m_requireAuth) {
            QByteArray auth;
            for (int i = 1; i < lines.size(); ++i) {
                if (lines.at(i).toLower().startsWith("proxy-authorization:"))
                    auth = lines.at(i).mid(lines.at(i).indexOf(':') + 1).trimmed();
            }
            // Validate the credential, do not merely require its presence: the auth tests send a
            // wrong password first (qsockstest:badpassword) and assert the proxy re-challenges, then
            // the right one (qsockstest:password) and assert it passes. Accepting any "Basic ..."
            // would let the wrong password through and break that sequence.
            bool proxyAuthed = false;
            if (auth.startsWith("Basic "))
                proxyAuthed = QByteArray::fromBase64(auth.mid(6)) == "qsockstest:password";
            if (!proxyAuthed) {
                // Wait for the whole unauthenticated body to arrive, then discard it and
                // challenge on the SAME connection. The client then retries with credentials
                // and a fresh body. Draining first avoids two failure modes: splicing leftover
                // body bytes onto the retry (they would parse as a bogus request line), and
                // tearing the connection down mid-upload for a large body (QTBUG-33180).
                int clen = 0;
                bool chunkedReq = false;
                for (int i = 1; i < lines.size(); ++i) {
                    const QByteArray low = lines.at(i).toLower();
                    if (low.startsWith("content-length:"))
                        clen = lines.at(i).mid(lines.at(i).indexOf(':') + 1).trimmed().toInt();
                    else if (low.startsWith("transfer-encoding:") && low.contains("chunked"))
                        chunkedReq = true;
                }
                if (chunkedReq) {
                    QByteArray drained;
                    int consumed;
                    if (!decodeChunkedBody(buf, headEnd + 4, &drained, &consumed)) {
                        client->setProperty("buf", buf);
                        return; // await the rest of the chunks before challenging
                    }
                } else if (buf.size() < headEnd + 4 + clen) {
                    client->setProperty("buf", buf);
                    return; // await the body before challenging
                }
                client->write("HTTP/1.1 407 Proxy Authentication Required\r\n"
                              "Proxy-Authenticate: Basic realm=\"test\"\r\n"
                              "Content-Length: 0\r\n\r\n");
                client->setProperty("buf", QByteArray());
                return; // client retries with credentials on the same connection
            }
        }

        if (method == "CONNECT") {
            const QByteArray hostPort = reqLine.at(1);
            const int colon = hostPort.lastIndexOf(':');
            QString host = QString::fromLatin1(colon < 0 ? hostPort : hostPort.left(colon));
            quint16 port = colon < 0 ? 80 : quint16(hostPort.mid(colon + 1).toUShort());
            remapTarget(&host, &port);
            // remapTarget only rewrites the fleet's own hosts to loopback; a target it left alone is
            // not part of the test fleet, i.e. the "does not exist" host tst_QTcpSocket::hostNotFound
            // asks for through a proxy. Answer 404 (-> HostNotFoundError) deterministically rather
            // than attempting a real DNS lookup, whose outcome depends on the box's resolver (an ISP
            // wildcard would resolve it and turn the expected not-found into a connection-refused).
            if (host != QLL("127.0.0.1")) {
                client->write("HTTP/1.1 404 Not Found\r\nContent-Length: 0\r\n\r\n");
                client->disconnectFromHost();
                return;
            }

            QTcpSocket *upstream = new QTcpSocket(client);
            client->setProperty("tunnel", true);
            linkPeers(client, upstream);
            upstream->setProperty("connectReply", QByteArray("HTTP/1.1 200 Connection established\r\n\r\n"));
            upstream->setProperty("errorReply", QByteArray("HTTP/1.1 503 Service Unavailable\r\n\r\n"));
            upstream->setProperty("closeOnError", true);
            connect(upstream, &QTcpSocket::connected, this, &ConnectProxyService::onTunnelConnected);
            connect(upstream, static_cast<void (QTcpSocket::*)(QAbstractSocket::SocketError)>(&QTcpSocket::error),
                    this, &ConnectProxyService::onTunnelError);
            connect(upstream, &QTcpSocket::readyRead, this, &ConnectProxyService::relayReadyRead);
            connect(client, &QTcpSocket::readyRead, this, &ConnectProxyService::relayReadyRead);
            connect(upstream, &QTcpSocket::disconnected, this, &ConnectProxyService::relayDisconnected);
            // Relay the client->upstream close gracefully too (see the SocksProxyService note): a
            // client-initiated close must flush and cleanly FIN the upstream, not abort it.
            connect(client, &QTcpSocket::disconnected, this, &ConnectProxyService::relayDisconnected);
            upstream->connectToHost(host, port);
            return;
        }

        // Plain HTTP forward proxy: "<method> http://host[:port]/path HTTP/1.1".
        const QByteArray target = reqLine.at(1);
        if (!target.toLower().startsWith("http://")) {
            client->write("HTTP/1.1 405 Method Not Allowed\r\nContent-Length: 0\r\n\r\n");
            client->disconnectFromHost();
            return;
        }
        int clen = 0;
        bool chunked = false;
        for (int i = 1; i < lines.size(); ++i) {
            const QByteArray low = lines.at(i).toLower();
            if (low.startsWith("content-length:"))
                clen = lines.at(i).mid(lines.at(i).indexOf(':') + 1).trimmed().toInt();
            else if (low.startsWith("transfer-encoding:") && low.contains("chunked"))
                chunked = true;
        }
        QByteArray body;
        if (chunked) {
            int consumed;
            if (!decodeChunkedBody(buf, headEnd + 4, &body, &consumed)) {
                client->setProperty("buf", buf); return; // await the rest of the chunks
            }
        } else {
            const int total = headEnd + 4 + clen;
            if (buf.size() < total) { client->setProperty("buf", buf); return; } // await the body
            body = buf.mid(headEnd + 4, clen);
        }

        QByteArray rest = target.mid(7); // strip "http://"
        const int slash = rest.indexOf('/');
        const QByteArray hostPort = slash < 0 ? rest : rest.left(slash);
        const QByteArray path = slash < 0 ? QByteArray("/") : rest.mid(slash);
        const int colon = hostPort.lastIndexOf(':');
        QString host = QString::fromLatin1(colon < 0 ? hostPort : hostPort.left(colon));
        quint16 port = colon < 0 ? 80 : quint16(hostPort.mid(colon + 1).toUShort());
        remapTarget(&host, &port);

        // Re-emit in origin-form, drop hop-by-hop/proxy headers, force a clean close.
        QByteArray fwd = method + ' ' + path + " HTTP/1.1\r\n";
        for (int i = 1; i < lines.size(); ++i) {
            const QByteArray l = lines.at(i).trimmed();
            const QByteArray low = l.toLower();
            if (l.isEmpty() || low.startsWith("proxy-") || low.startsWith("connection:")
                || low.startsWith("keep-alive") || low.startsWith("content-length:")
                || low.startsWith("transfer-encoding:"))
                continue;
            fwd += l + "\r\n";
        }
        fwd += "Content-Length: " + QByteArray::number(body.size()) + "\r\n";
        fwd += "Connection: close\r\n\r\n";
        fwd += body;

        client->setProperty("forwarding", true);
        QTcpSocket *upstream = new QTcpSocket(client);
        linkPeers(client, upstream);
        upstream->setProperty("forward", fwd);
        upstream->setProperty("errorReply", QByteArray("HTTP/1.1 503 Service Unavailable\r\nContent-Length: 0\r\n\r\n"));
        upstream->setProperty("closeOnError", true);
        connect(upstream, &QTcpSocket::connected, this, &ConnectProxyService::onForwardConnected);
        connect(upstream, &QTcpSocket::readyRead, this, &ConnectProxyService::relayReadyRead);
        connect(upstream, &QTcpSocket::disconnected, this, &ConnectProxyService::relayDisconnected);
        connect(upstream, static_cast<void (QTcpSocket::*)(QAbstractSocket::SocketError)>(&QTcpSocket::error),
                this, &ConnectProxyService::onTunnelError);
        upstream->connectToHost(host, port);
    }

    /// The forward proxy writes the rewritten request to the upstream once connected (and
    /// marks it "up" so a later close is not mistaken for a connect failure).
    void onForwardConnected()
    {
        QTcpSocket *up = qobject_cast<QTcpSocket *>(sender());
        up->setProperty("up", true);
        up->write(up->property("forward").toByteArray());
    }

    bool m_requireAuth;
};

// MARK: SOCKS5 proxy with target remap (ports 1080 no-auth, 1081 user/pass).

class SocksProxyService : public TestService
{
public:
    SocksProxyService(const ServiceConfig &c, QObject *parent = Q_NULLPTR)
        : TestService(c, parent), m_requireAuth(c.requireAuth) {}

    void start() Q_DECL_OVERRIDE
    {
        QTcpServer *server = new QTcpServer(this);
        listenTcp(server);
        connect(server, &QTcpServer::newConnection, this, &SocksProxyService::onNewConnection);
    }

private:
    void onNewConnection()
    {
        QTcpServer *server = qobject_cast<QTcpServer *>(sender());
        while (server->hasPendingConnections()) {
            QTcpSocket *c = server->nextPendingConnection();
            connect(c, &QTcpSocket::readyRead, this, &SocksProxyService::onReadyRead);
            connect(c, &QTcpSocket::disconnected, c, &QObject::deleteLater);
        }
    }
    void onReadyRead() { onSocks(qobject_cast<QTcpSocket *>(sender())); }

    void onSocks(QTcpSocket *client)
    {
        if (client->property("tunnel").toBool())
            return;
        if (!client->property("greeted").toBool()) {
            if (client->bytesAvailable() < 2)
                return;
            const QByteArray peek = client->peek(int(client->bytesAvailable()));
            if (quint8(peek.at(0)) != 0x05)
                return;
            const int nmethods = int(quint8(peek.at(1)));
            if (peek.size() < 2 + nmethods)
                return; // await all the method bytes before replying
            client->read(2);
            const QByteArray methods = client->read(nmethods);
            char method = m_requireAuth ? char(0x02) : char(0x00);
            if (m_requireAuth && !methods.contains(char(0x02)))
                method = char(0xff); // no acceptable method
            client->write(QByteArray(1, char(0x05)) + QByteArray(1, method));
            client->setProperty("greeted", true);
            client->setProperty("needauth", m_requireAuth);
            return;
        }
        if (client->property("needauth").toBool() && !client->property("authed").toBool()) {
            if (client->bytesAvailable() < 2)
                return;
            const QByteArray peek = client->peek(int(client->bytesAvailable()));
            const int ulen = int(quint8(peek.at(1)));           // VER ULEN uname PLEN pass
            if (peek.size() < 2 + ulen + 1)
                return; // need at least through the password-length byte
            const int plen = int(quint8(peek.at(2 + ulen)));
            if (peek.size() < 2 + ulen + 1 + plen)
                return; // await the whole username/password request
            const QByteArray uname = peek.mid(2, ulen);
            const QByteArray passwd = peek.mid(2 + ulen + 1, plen);
            client->read(2 + ulen + 1 + plen); // consume it all at once
            // Validate the credential (RFC 1929): the auth tests send a wrong password first and
            // assert the proxy rejects it, then the right one. Accepting any pair would break that.
            if (uname == "qsockstest" && passwd == "password") {
                client->write(QByteArray::fromRawData("\x01\x00", 2)); // status 0 = success
                client->setProperty("authed", true);
            } else {
                client->write(QByteArray::fromRawData("\x01\x01", 2)); // non-zero = failure
                client->disconnectFromHost();                          // server MUST close on auth failure
            }
            return;
        }
        if (client->property("target").isNull()) {
            // Buffer until the WHOLE CONNECT request has arrived: it may come in pieces, and
            // reading past what is present would parse a garbage target -> a bogus upstream
            // connect the client sees as "connection refused". Peek first, consume only when complete.
            if (client->bytesAvailable() < 4)
                return;
            const QByteArray peek = client->peek(int(client->bytesAvailable()));
            const quint8 cmd = quint8(peek.at(1));
            const quint8 atyp = quint8(peek.at(3));
            int need = 0;
            if (atyp == 0x01) need = 4 + 4 + 2;        // VER CMD RSV ATYP + IPv4 + port
            else if (atyp == 0x04) need = 4 + 16 + 2;  // ... + IPv6 + port
            else if (atyp == 0x03) {
                if (peek.size() < 5) return;           // need the domain-length byte first
                need = 4 + 1 + int(quint8(peek.at(4))) + 2; // ... + len + name + port
            } else {
                return;
            }
            if (peek.size() < need)
                return; // await the rest of the CONNECT request
            client->read(4); // consume VER CMD RSV ATYP
            QString hostStr;
            if (atyp == 0x01) {
                const QByteArray a = client->read(4);
                hostStr = QHostAddress(qFromBigEndian<quint32>((const uchar *)a.constData())).toString();
            } else if (atyp == 0x04) {
                const QByteArray a = client->read(16);
                hostStr = QHostAddress(reinterpret_cast<const quint8 *>(a.constData())).toString();
            } else { // 0x03 domain name
                const quint8 len = quint8(client->read(1).at(0));
                hostStr = QString::fromLatin1(client->read(len));
            }
            quint16 dport = qFromBigEndian<quint16>((const uchar *)client->read(2).constData());
            remapTarget(&hostStr, &dport);
            client->setProperty("target", true);
            // Further client bytes are relay data, not SOCKS, whichever command this is.
            client->setProperty("tunnel", true);
            if (cmd == 0x02) { // BIND: QTcpServer::listen() over SOCKS5. DST is advisory; ignore it.
                startBind(client);
                return;
            }
            QTcpSocket *upstream = new QTcpSocket(client);
            linkPeers(client, upstream);
            upstream->setProperty("connectReply", QByteArray("\x05\x00\x00\x01\x00\x00\x00\x00\x00\x00", 10));
            upstream->setProperty("errorReply", QByteArray("\x05\x05\x00\x01\x00\x00\x00\x00\x00\x00", 10));
            upstream->setProperty("closeOnError", true);
            connect(upstream, &QTcpSocket::connected, this, &SocksProxyService::onTunnelConnected);
            connect(upstream, static_cast<void (QTcpSocket::*)(QAbstractSocket::SocketError)>(&QTcpSocket::error),
                    this, &SocksProxyService::onTunnelError);
            connect(upstream, &QTcpSocket::readyRead, this, &SocksProxyService::relayReadyRead);
            connect(client, &QTcpSocket::readyRead, this, &SocksProxyService::relayReadyRead);
            connect(upstream, &QTcpSocket::disconnected, this, &SocksProxyService::relayDisconnected);
            // Relay BOTH close directions gracefully. Without the client->upstream half, a client-
            // initiated close (how FTP STOR signals end-of-upload) would only deleteLater the client
            // and abort/RST the child upstream, dropping buffered bytes so the FTP server never emits
            // its "226 transfer complete" and QFtp times out on the control connection. (startBind
            // already wires both directions; the CONNECT path was missing this half.)
            connect(client, &QTcpSocket::disconnected, this, &SocksProxyService::relayDisconnected);
            upstream->connectToHost(hostStr, dport);
        }
    }

    /// Build a SOCKS5 reply "05 REP 00 01 <IPv4> <port>".
    static QByteArray socksReply(quint8 rep, const QHostAddress &addr, quint16 port)
    {
        const quint32 v4 = addr.toIPv4Address();
        QByteArray b;
        b.append(char(0x05)).append(char(rep)).append(char(0x00)).append(char(0x01));
        b.append(char((v4 >> 24) & 0xff)).append(char((v4 >> 16) & 0xff));
        b.append(char((v4 >> 8) & 0xff)).append(char(v4 & 0xff));
        b.append(char((port >> 8) & 0xff)).append(char(port & 0xff));
        return b;
    }

    /// SOCKS5 BIND: listen on an ephemeral port and hand the client that port in the first
    /// reply; on the first incoming connection send the second reply (its peer) and relay
    /// over the same control socket. This is what QTcpServer::listen() through a SOCKS5
    /// proxy needs -- without it the command was misparsed as CONNECT and listen() failed.
    void startBind(QTcpSocket *control)
    {
        QTcpServer *bindServer = new QTcpServer(control);
        bindServer->listen(control->localAddress(), 0);
        bindServer->setProperty("bindControl", QVariant::fromValue<QObject *>(control));
        control->write(socksReply(0x00, bindServer->serverAddress(), bindServer->serverPort()));
        connect(bindServer, &QTcpServer::newConnection, this, &SocksProxyService::onBindConnection);
    }

    void onBindConnection()
    {
        QTcpServer *bindServer = qobject_cast<QTcpServer *>(sender());
        QTcpSocket *control = qobject_cast<QTcpSocket *>(bindServer->property("bindControl").value<QObject *>());
        QTcpSocket *incoming = bindServer->nextPendingConnection();
        // BIND relays a single connection, so stop listening.
        bindServer->close();
        control->write(socksReply(0x00, incoming->peerAddress(), incoming->peerPort()));
        linkPeers(control, incoming);
        connect(incoming, &QTcpSocket::readyRead, this, &SocksProxyService::relayReadyRead);
        connect(control, &QTcpSocket::readyRead, this, &SocksProxyService::relayReadyRead);
        connect(incoming, &QTcpSocket::disconnected, this, &SocksProxyService::relayDisconnected);
        connect(control, &QTcpSocket::disconnected, this, &SocksProxyService::relayDisconnected);
    }

    /// Map an upstream connect failure to the SOCKS5 CONNECT failure reply the client expects (NOT an
    /// HTTP response -- this is a SOCKS proxy): VER=05, REP, RSV=00, ATYP=01(IPv4), 0.0.0.0:0. REP is
    /// 0x04 (host unreachable) for an unresolvable target so QSocks5SocketEngine reports HostNotFoundError,
    /// else 0x05 (connection refused). Overrides TestService::onTunnelError's fixed errorReply so the
    /// reason code tracks the actual failure.
    void onTunnelError()
    {
        QTcpSocket *up = qobject_cast<QTcpSocket *>(sender());
        if (!up || up->property("up").toBool())
            return; // a close once tunnelling has begun is normal, not a handshake failure
        char reply[10] = { '\x05', '\x05', '\x00', '\x01', 0, 0, 0, 0, 0, 0 };
        if (up->error() == QAbstractSocket::HostNotFoundError)
            reply[1] = '\x04';
        if (QTcpSocket *client = peerOf(up)) {
            client->write(QByteArray(reply, 10));
            client->disconnectFromHost();
        }
    }

    bool m_requireAuth;
};

// MARK: the TLS listener that dispatches by negotiated protocol.

/// A QTcpServer that turns each accepted descriptor straight into a server-side
/// QSslSocket. Building the QSslSocket from the raw descriptor (rather than
/// wrapping a QTcpSocket handed out by nextPendingConnection) avoids two socket
/// notifiers fighting over the same fd.
class SslServer : public QTcpServer
{
    Q_OBJECT
public:
    SslServer(const QSslConfiguration &cfg, const QString &root, QObject *parent)
        : QTcpServer(parent), m_cfg(cfg), m_root(root) {}

protected:
    void incomingConnection(qintptr socketDescriptor) Q_DECL_OVERRIDE
    {
        QSslSocket *s = new QSslSocket(this);
        if (!s->setSocketDescriptor(socketDescriptor)) {
            delete s;
            return;
        }
        QSslConfiguration cfg = m_cfg;
        // server-config.ini [TLS] fixedSessionTicketKey (default OFF): install a FIXED RFC-5077
        // session-ticket key only while a test opts in, so the tickets this server issues resume
        // across the separate sockets QNAM opens (sslSessionSharing / ...FromPersistentSession).
        // Off by default -> each connection keeps OpenSSL's random per-context key (no cross-socket
        // resumption), so this key-pinning is never the standing behaviour. Read live per connection.
        if (IniWatcher::shared(g_configIniPath)->load()->fixedSessionTicketKey())
            cfg.setSessionTicketKey(QByteArray(80, '\x2a')); // 16 name + 32 HMAC + 32 AES (OpenSSL 1.1.1)
        s->setSslConfiguration(cfg);
        connect(s, &QSslSocket::encrypted, this, &SslServer::onEncrypted);
        connect(s, static_cast<void (QSslSocket::*)(const QList<QSslError> &)>(&QSslSocket::sslErrors),
                this, &SslServer::onSslErrors);
        connect(s, &QSslSocket::preSharedKeyAuthenticationRequired, this, &SslServer::onPsk);
        connect(s, &QSslSocket::disconnected, s, &QObject::deleteLater);
        s->startServerEncryption();
    }

private:
    void onEncrypted()
    {
        QSslSocket *s = qobject_cast<QSslSocket *>(sender());
        const QByteArray proto = s->sslConfiguration().nextNegotiatedProtocol();
        if (proto == "h2")
            new Http2Connection(s, m_root, s);
        else if (proto == "spdy/3")
            new SpdyConnection(s, m_root, s);
        else
            new Http1Connection(s, m_root, s);
    }
    /// A self-signed cert is expected; nothing to do.
    void onSslErrors() {}
    /// Server side of a PSK handshake (tst_QSslSocket::simplePskConnect): hand back the shared
    /// key the client expects. The matching identity hint is advertised via m_cfg below.
    void onPsk(QSslPreSharedKeyAuthenticator *auth)
    {
        // Genuine server-side PSK: hand back the shared key ONLY for the expected client
        // identity. A wrong identity gets no key, so OpenSSL aborts the handshake --
        // tst_QSslSocket::simplePskConnect's WrongIdentity row depends on that rejection
        // (accepting any identity would let a wrong-identity handshake wrongly succeed).
        if (auth->identity() == QByteArrayLiteral("Client_identity"))
            auth->setPreSharedKey(QByteArrayLiteral("\x1a\x2b\x3c\x4d\x5e\x6f"));
        // else: leave the key empty -> handshake fails.
    }

    QSslConfiguration m_cfg;
    QString m_root;
};

/// A minimal TLS greeting server (imaps on 993): completes the handshake and writes
/// a one-line greeting, so a client's waitForReadyRead() sees data. Cert-verify is the
/// client's business (these tests ignore SSL errors).
class GreetingSslServer : public QTcpServer
{
    Q_OBJECT
public:
    GreetingSslServer(const QSslConfiguration &cfg, const QByteArray &greeting, QObject *parent)
        : QTcpServer(parent), m_cfg(cfg), m_greeting(greeting) {}

protected:
    void incomingConnection(qintptr socketDescriptor) Q_DECL_OVERRIDE
    {
        QSslSocket *s = new QSslSocket(this);
        if (!s->setSocketDescriptor(socketDescriptor)) {
            delete s;
            return;
        }
        s->setSslConfiguration(m_cfg);
        connect(s, &QSslSocket::encrypted, this, &GreetingSslServer::onEncrypted);
        connect(s, static_cast<void (QSslSocket::*)(const QList<QSslError> &)>(&QSslSocket::sslErrors),
                this, &GreetingSslServer::onSslErrors);
        connect(s, &QSslSocket::disconnected, s, &QObject::deleteLater);
        s->startServerEncryption();
    }

private:
    void onEncrypted() { qobject_cast<QSslSocket *>(sender())->write(m_greeting); }
    void onSslErrors() {}

    QSslConfiguration m_cfg;
    QByteArray m_greeting;
};

class ImapsService : public TestService
{
public:
    ImapsService(const ServiceConfig &c, QObject *parent = Q_NULLPTR)
        : TestService(c, parent) {}

    void start() Q_DECL_OVERRIDE
    {
        QSslConfiguration cfg = QSslConfiguration::defaultConfiguration();
        cfg.setLocalCertificate(QSslCertificate(g_serverCertPem, QSsl::Pem));
        cfg.setPrivateKey(QSslKey(g_serverKeyPem, QSsl::Rsa, QSsl::Pem));
        // compareReplyIMAP() requires the greeting to start with "* OK " and end with
        // "server ready\r\n".
        GreetingSslServer *server = new GreetingSslServer(
            cfg, "* OK [CAPABILITY IMAP4rev1] server-dummy IMAP4 server ready\r\n", this);
        listenTcp(server);
    }
};

class HttpsService : public TestService
{
public:
    /// The cert comes from the ServiceConfig (certPem/keyPem): main.cpp's --second-port serves a
    /// DISTINCT cert so a client can observe a different certificate chain than the primary port.
    /// Empty config cert => fall back to the process-wide g_serverCertPem/g_serverKeyPem (start()).
    HttpsService(const ServiceConfig &c, QObject *parent = Q_NULLPTR)
        : TestService(c, parent), m_root(c.folder), m_certPem(c.certPem), m_keyPem(c.keyPem) {}

    void start() Q_DECL_OVERRIDE
    {
        QSslConfiguration cfg = QSslConfiguration::defaultConfiguration();
        // NPN (which SPDY/3 negotiates over) is undefined for TLS 1.3, and OpenSSL
        // 1.1.1 would otherwise pick 1.3 and skip the NPN callbacks. Cap at TLS 1.2,
        // as the SPDY-era servers this test targets did.
        cfg.setProtocol(QSsl::TlsV1_2);
        const QByteArray &certPem = m_certPem.isEmpty() ? g_serverCertPem : m_certPem;
        const QByteArray &keyPem  = m_keyPem.isEmpty()  ? g_serverKeyPem  : m_keyPem;
        cfg.setLocalCertificate(QSslCertificate(certPem, QSsl::Pem));
        cfg.setPrivateKey(QSslKey(keyPem, QSsl::Rsa, QSsl::Pem));
        QList<QByteArray> npn;
        // Offer HTTP/2 (ALPN "h2"), SPDY/3 (NPN "spdy/3") and HTTP/1.1. The client's
        // offer decides: an HTTP/2 client picks h2 over ALPN, a SPDY client spdy/3.
        npn << QByteArray("h2") << QByteArray("spdy/3") << QByteArray("http/1.1");
        cfg.setAllowedNextProtocols(npn);
        // Advertise the PSK identity hint tst_QSslSocket::simplePskConnect's client verifies. It
        // only affects a PSK handshake, so it is inert for the normal certificate handshakes above.
        cfg.setPreSharedKeyIdentityHint(QByteArrayLiteral("QtTestServerHint"));
        // NB: the fixed session-ticket key is NOT set here (that would pin it for the process's whole
        // lifetime, defeating ticket-key rotation as a standing default). SslServer installs it per
        // connection only while server-config.ini [TLS] fixedSessionTicketKey is on -- see there.

        SslServer *server = new SslServer(cfg, m_root, this);
        listenTcp(server);
    }

private:
    QString m_root;
    QByteArray m_certPem;
    QByteArray m_keyPem;
};

#endif // HTTPS_SPDY_H
