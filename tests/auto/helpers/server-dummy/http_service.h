#ifndef HTTP_SERVICE_H
#define HTTP_SERVICE_H

#include "service_base.h"
#include "http_reply.h" // shared buildReply(): CGIs, cache headers, WebDAV, auth, files

#include <cstring>
#include <zlib.h>

#include <QtCore/QByteArray>
#include <QtCore/QList>
#include <QtCore/QThread>

/// HttpService: an HTTP/1.1 responder that is also an HTTP CONNECT proxy.
class HttpService : public TestService
{
public:
    HttpService(const ServiceConfig &c, QObject *parent = Q_NULLPTR)
        : TestService(c, parent) {}

    void start() Q_DECL_OVERRIDE
    {
        QTcpServer *server = new QTcpServer(this);
        listenTcp(server);
        connect(server, &QTcpServer::newConnection, this, &HttpService::onNewConnection);
    }

private:
    void onNewConnection()
    {
        QTcpSocket *s = qobject_cast<QTcpServer *>(sender())->nextPendingConnection();
        connect(s, &QTcpSocket::readyRead, this, &HttpService::onReadyRead);
        connect(s, &QTcpSocket::disconnected, s, &QObject::deleteLater);
    }
    void onReadyRead() { onRequest(qobject_cast<QTcpSocket *>(sender())); }

    /// Keep-alive request pump: buffer bytes, and while a full request (head + any
    /// Content-Length body) is present, dispatch it and keep the connection open. This is
    /// what lets the pipelining/keep-alive tests reuse one socket for many requests.
    void onRequest(QTcpSocket *s)
    {
        if (s->property("tunnel").toBool()) // already CONNECT-tunnelled: relay is elsewhere
            return;
        QByteArray buf = s->property("buf").toByteArray() + s->readAll();
        forever {
            const int headEnd = buf.indexOf("\r\n\r\n");
            if (headEnd < 0)
                break; // wait for the full head
            const QByteArray head = buf.left(headEnd);
            QByteArray body;
            if (headerValue(head, "transfer-encoding").toLower().contains("chunked")) {
                // A body POSTed from a sequential device (e.g. a socket) arrives chunked,
                // with no Content-Length: decode the chunks before routing.
                int consumed = 0;
                if (!decodeChunkedBody(buf, headEnd + 4, &body, &consumed))
                    break; // wait for the rest of the chunks
                buf = buf.mid(consumed);
            } else {
                const int clen = headerValue(head, "content-length").toInt();
                const int total = headEnd + 4 + clen;
                if (buf.size() < total)
                    break; // wait for the body
                body = buf.mid(headEnd + 4, clen);
                buf = buf.mid(total);
            }
            const QList<QByteArray> reqLine = head.left(head.indexOf('\n')).simplified().split(' ');
            if (reqLine.size() < 2) { // malformed request line -> 400
                send(s, 400, "Bad Request", "bad request\n");
                continue;
            }
            const QByteArray method = reqLine.at(0).toUpper();
            const QByteArray path = reqLine.at(1);
            if (method == "CONNECT") {
                // A well-formed proxy CONNECT names an authority (host:port). The custom-verb
                // test sends CONNECT to an origin path ("/"), which is not a tunnel target ->
                // 400 Bad Request (QNetworkReply::ProtocolInvalidOperationError).
                if (path.contains(':') && !path.startsWith('/')) {
                    s->setProperty("buf", QByteArray());
                    startConnectTunnel(s, path);
                    return;
                }
                send(s, 400, "Bad Request", QByteArray());
                continue;
            }
            route(s, method, path, head, body);
        }
        s->setProperty("buf", buf);
    }

    static QByteArray headerValue(const QByteArray &head, const char *name)
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

    /// Honour server-config.ini [IO] responseDelayMs: hold this reply for the configured latency
    /// before writing. The server process is single-threaded, so a non-zero delay also serialises
    /// otherwise-concurrent requests (each is read + answered in turn) -- which is the point for the
    /// parallel-auth rows, where the first 401->retry must settle the per-host credential cache
    /// before the second challenge is processed. Default 0 = no delay, so every other test is unaffected.
    void applyResponseDelay()
    {
        const int delayMs = watcher()->load()->responseDelayMs();
        if (delayMs > 0)
            QThread::msleep(uint(delayMs));
    }

    /// Write a response, honouring keep-alive unless the request asked to close.
    void send(QTcpSocket *s, int code, const char *reason, const QByteArray &body,
              const QByteArray &extraHeaders = QByteArray())
    {
        applyResponseDelay();
        // Error responses carry a short generated page, so a client that waits for body
        // bytes (e.g. QHttpNetworkConnection::get) sees data instead of blocking on an
        // empty 4xx/5xx. The tests do not require a fixed error-page size.
        QByteArray b = body;
        if (b.isEmpty() && code >= 400)
            b = QByteArray(reason) + "\n";
        QByteArray resp = "HTTP/1.1 " + QByteArray::number(code) + ' ' + reason + "\r\n"
                          "Content-Type: text/plain\r\n" + extraHeaders +
                          "Content-Length: " + QByteArray::number(b.size()) + "\r\n\r\n" + b;
        s->write(resp);
    }

    /// A fixed, moderately compressible page standing in for the RFC 2616 fixture. Only its
    /// length (and its gzipped length) matter to the compression rows, which assert whatever
    /// sizes this server produces rather than a foreign server's fixture sizes.
    static QByteArray rfc2616Body()
    {
        QByteArray b;
        b.reserve(40000);
        while (b.size() < 40000)
            b += "The Hypertext Transfer Protocol (HTTP) is an application-level protocol "
                 "for distributed, collaborative, hypermedia information systems.\n";
        return b;
    }

    /// gzip-wrap bytes with the bundled zlib (windowBits 15+16 selects the gzip container).
    static QByteArray gzipBody(const QByteArray &in)
    {
        z_stream zs;
        memset(&zs, 0, sizeof(zs));
        deflateInit2(&zs, Z_DEFAULT_COMPRESSION, Z_DEFLATED, 15 + 16, 8, Z_DEFAULT_STRATEGY);
        QByteArray out(int(deflateBound(&zs, uLong(in.size()))), Qt::Uninitialized);
        zs.next_in = reinterpret_cast<Bytef *>(const_cast<char *>(in.constData()));
        zs.avail_in = uInt(in.size());
        zs.next_out = reinterpret_cast<Bytef *>(out.data());
        zs.avail_out = uInt(out.size());
        deflate(&zs, Z_FINISH);
        out.truncate(int(zs.total_out));
        deflateEnd(&zs);
        return out;
    }

    void route(QTcpSocket *s, const QByteArray &method, const QByteArray &path,
               const QByteArray &head, const QByteArray &body)
    {
        // Strip an absolute-URI down to its path (some tests send "host/path").
        QByteArray p = path;
        const int schemeSlash = p.indexOf("//");
        if (schemeSlash >= 0) {
            const int slash = p.indexOf('/', schemeSlash + 2);
            p = slash < 0 ? QByteArray("/") : p.mid(slash);
        }
        if (!p.startsWith('/')) { // a request line with no scheme and no leading slash
            send(s, 400, "Bad Request", QByteArray(), "Connection: close\r\n");
            s->disconnectFromHost(); // a malformed request ends the connection
            return;
        }

        // Split the query off ONCE and match routes on the path alone: a target may carry a query
        // (e.g. putWithRateLimiting POSTs to ".../echo.cgi?"), and an exact "== path" test would
        // otherwise miss it and fall through to the shared responder (which echoes the query, not
        // the body). rpath/query are reused by the shared-responder branch below.
        QByteArray rpath, query;
        splitTarget(p, &rpath, &query);

        // Reject verbs the server does not implement (RFC 7231 -> 501). The known set flows on to
        // the routes below; OPTIONS/TRACE fall through to a 200 like a permissive origin, while an
        // unrecognised verb such as the custom-request test's "NONSENSE" must yield 501.
        {
            static const char *const known[] = { "GET", "HEAD", "POST", "PUT", "DELETE", "OPTIONS", "TRACE" };
            bool methodKnown = false;
            for (unsigned i = 0; i < sizeof(known) / sizeof(known[0]); ++i)
                if (method == known[i]) { methodKnown = true; break; }
            if (!methodKnown) {
                send(s, 501, "Not Implemented", QByteArray());
                return;
            }
        }

        if (rpath == "/qtest/cgi-bin/echo.cgi") {
            send(s, 200, "OK", body);                       // echo the request body
        } else if (rpath == "/cgi-bin/echo.cgi") {
            send(s, 200, "OK", QByteArray());               // empty body (pipelining test)
        } else if (rpath == "/qtest/rfc3252.txt") {
            send(s, 200, "OK", QByteArray(25962, 'x'));      // exact size 25962
        } else if (rpath == "/qtest/bigfile") {
            send(s, 200, "OK", QByteArray(512 * 1024, 'x')); // large enough to stream
        } else if (rpath == "/qtest/mediumfile") {
            // Exactly 10 MB of NUL bytes for downloadBigFile. The zero body matters: that
            // test finds the body by the first NUL past the ASCII headers, then counts from
            // there, so the payload must be all-NUL to total exactly 10000000.
            send(s, 200, "OK", QByteArray(10000000, '\0'));
        } else if (rpath == "/qtest/rfcs/rfc2616.html") {
            // Identity body; the compression rows assert the exact byte length served here.
            send(s, 200, "OK", rfc2616Body());
        } else if (rpath == "/qtest/deflate/rfc2616.html") {
            // The pre-gzipped variant, served with its Content-Encoding so the client keeps the
            // compressed bytes; the compression row asserts that gzipped length.
            send(s, 200, "OK", gzipBody(rfc2616Body()), "Content-Encoding: gzip\r\n");
        } else if (rpath == "/qtest/rfcs-auth" || rpath.startsWith("/qtest/rfcs-auth/")) {
            requireBasic(s, head, "httptest:httptest", "httptest login");
        } else if (rpath.startsWith("/qtest/auth-digest/")) {
            requireDigest(s, head);
        } else if (rpath.startsWith("/dav/")) {
            send(s, (method == "PUT") ? 201 : 200, "Created", QByteArray());
        } else if (rpath.startsWith("/dav2/")) {
            send(s, 405, "Method Not Allowed", QByteArray()); // read-only WebDAV area
        } else if (rpath == "/t") {
            send(s, 404, "Not Found", QByteArray());
        } else {
            // Everything else (md5sum/multipart CGIs, the httpcachetest cache CGIs with
            // Expires/ETag/304, /qtest file serving, PUT) is answered by the shared
            // responder, so this plain listener serves the same routes as the TLS/SPDY one.
            sendReply(s, method, head, buildReply(method, rpath, query,
                                                  headerValue(head, "content-type"),
                                                  body, QString(), head));
        }
    }

    /// Serialise an HttpReply from the shared responder onto the socket. Unlike send(), this
    /// emits the reply's own headers verbatim (it carries its own Content-Type), and omits the
    /// body for a HEAD request the way an HTTP server must.
    void sendReply(QTcpSocket *s, const QByteArray &method, const QByteArray &head, const HttpReply &r)
    {
        applyResponseDelay();
        // Honour a request's "Connection: close" (a forward proxy adds it): echo it back and
        // hang up after the response, so the client sees EOF and reconnects for a retry
        // (e.g. after a 401) rather than reusing a connection the proxy is done relaying.
        // Close on an explicit "Connection: close", OR on an HTTP/1.0 request that did not ask to
        // keep the connection alive (HTTP/1.0 defaults to non-persistent) -- tst_QTcpSocket::
        // zeroAndMinusOneReturns sends a trailing "GET / HTTP/1.0" and waits for the server to hang up.
        const QByteArray connHdr = headerValue(head, "connection").toLower();
        const bool http10 = head.left(head.indexOf('\n')).contains("HTTP/1.0");
        const bool close = connHdr == "close" || (http10 && connHdr != "keep-alive");
        QByteArray resp = "HTTP/1.1 " + QByteArray::number(r.code) + ' ' + r.reason + "\r\n";
        for (int i = 0; i < r.headers.size(); ++i)
            resp += r.headers.at(i).first + ": " + r.headers.at(i).second + "\r\n";
        if (close)
            resp += "Connection: close\r\n";
        resp += "Content-Length: " + QByteArray::number(r.body.size()) + "\r\n\r\n";
        if (method != "HEAD")
            resp += r.body;
        s->write(resp);
        if (close)
            s->disconnectFromHost();
    }

    /// HTTP Basic auth against "user:pass"; 401 with a WWW-Authenticate challenge otherwise.
    void requireBasic(QTcpSocket *s, const QByteArray &head, const QByteArray &userPass,
                      const QByteArray &okBody)
    {
        const QByteArray auth = headerValue(head, "authorization");
        const QByteArray want = "Basic " + userPass.toBase64();
        if (auth == want)
            send(s, 200, "OK", okBody);
        else
            send(s, 401, "Unauthorized", QByteArray(),
                 "WWW-Authenticate: Basic realm=\"test\"\r\n");
    }

    /// Minimal HTTP Digest: challenge with a nonce, then accept any response whose
    /// Authorization header carries username="httptest" (enough to tell the tests' valid
    /// account from the wrong one without doing the full MD5 dance).
    void requireDigest(QTcpSocket *s, const QByteArray &head)
    {
        const QByteArray auth = headerValue(head, "authorization");
        if (auth.startsWith("Digest ") && auth.contains("username=\"httptest\""))
            send(s, 200, "OK", "httptest login");
        else
            send(s, 401, "Unauthorized", QByteArray(),
                 "WWW-Authenticate: Digest realm=\"test\", "
                 "nonce=\"abc123nonce\", qop=\"auth\"\r\n");
    }

    void startConnectTunnel(QTcpSocket *client, const QByteArray &hostPort)
    {
        // Drain the rest of the CONNECT request head.
        while (client->canReadLine()) {
            const QByteArray h = client->readLine();
            if (h == "\r\n" || h == "\n")
                break;
        }
        const int colon = hostPort.lastIndexOf(':');
        const QString host = QString::fromLatin1(colon < 0 ? hostPort : hostPort.left(colon));
        const quint16 port = colon < 0 ? 80 : quint16(hostPort.mid(colon + 1).toUShort());

        QTcpSocket *upstream = new QTcpSocket(client);
        client->setProperty("tunnel", true);
        linkPeers(client, upstream);
        upstream->setProperty("connectReply", QByteArray("HTTP/1.1 200 Connection established\r\n\r\n"));
        upstream->setProperty("errorReply", QByteArray("HTTP/1.1 503 Service Unavailable\r\n\r\n"));
        // A failed CONNECT must also close the client.
        upstream->setProperty("closeOnError", true);
        connect(upstream, &QTcpSocket::connected, this, &HttpService::onTunnelConnected);
        connect(upstream, static_cast<void (QTcpSocket::*)(QAbstractSocket::SocketError)>(&QTcpSocket::error),
                this, &HttpService::onTunnelError);
        connect(upstream, &QTcpSocket::readyRead, this, &HttpService::relayReadyRead);
        connect(client, &QTcpSocket::readyRead, this, &HttpService::relayReadyRead);
        connect(upstream, &QTcpSocket::disconnected, this, &HttpService::relayDisconnected);
        upstream->connectToHost(host, port);
    }
};

#endif // HTTP_SERVICE_H
