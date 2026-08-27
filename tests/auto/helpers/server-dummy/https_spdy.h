// ---------------------------------------------------------------------------
// HttpsService: a TLS listener that speaks either HTTP/1.1 or SPDY/3.0,
// selected by NPN (Next Protocol Negotiation). It backs the tst_spdy test:
// it serves a small set of routes (an Apache-looking root page, echo/md5sum/
// multipart CGIs, and files from a --folder) over whichever protocol the
// client negotiates. The SPDY layer mirrors, in reverse, the framing and the
// level-0/no-dictionary zlib header compression of Qt's own client-side
// qspdyprotocolhandler.cpp.
// ---------------------------------------------------------------------------
#ifndef HTTPS_SPDY_H
#define HTTPS_SPDY_H

#include "https_http2.h" // Http2Connection, for the "h2" ALPN branch below

#include <QtNetwork/QSslSocket>
#include <QtNetwork/QSslConfiguration>
#include <QtNetwork/QSslKey>
#include <QtNetwork/QSslCertificate>
#include <QtNetwork/QTcpServer>
#include <QtCore/QFile>
#include <QtCore/QFileInfo>
#include <QtCore/QCryptographicHash>
#include <QtCore/QDateTime>
#include <QtCore/QMap>
#include <QtCore/QtEndian>
#include <QtCore/QLocale>
#include <zlib.h>

#include "service_base.h"     // TestService
#include "../test-ports.h"    // testPort()
#include "spdy_certs.h"
#include "spdy3_dictionary.h"

// A protocol-agnostic reply: the same routing serves HTTP/1.1 and SPDY.
struct HttpReply
{
    int code;
    QByteArray reason;
    QList<QPair<QByteArray, QByteArray> > headers; // lower-case names, minus status/version
    QByteArray body;
    HttpReply() : code(200), reason("OK") {}
    void setHeader(const QByteArray &name, const QByteArray &value)
    {
        headers.append(qMakePair(name.toLower(), value));
    }
};

// ---- shared routing --------------------------------------------------------

// Split a raw request target into decoded path and (raw) query.
static inline void splitTarget(const QByteArray &target, QByteArray *path, QByteArray *query)
{
    QByteArray p = target;
    const int schemeSlash = p.indexOf("//"); // strip an absolute-form URI
    if (schemeSlash >= 0) {
        const int slash = p.indexOf('/', schemeSlash + 2);
        p = slash < 0 ? QByteArray("/") : p.mid(slash);
    }
    const int q = p.indexOf('?');
    if (q >= 0) {
        *query = p.mid(q + 1);
        *path = p.left(q);
    } else {
        *query = QByteArray();
        *path = p;
    }
}

static inline QByteArray httpDate(const QDateTime &dt)
{
    return QLocale::c().toString(dt.toUTC(), QLatin1String("ddd, dd MMM yyyy HH:mm:ss")).toLatin1() + " GMT";
}

// Compute the multipart.cgi response: one md5 line per form-data part.
static inline QByteArray multipartSummary(const QByteArray &contentType, const QByteArray &body)
{
    QByteArray out = "content type: " + contentType + "\n";
    const int bpos = contentType.toLower().indexOf("boundary=");
    if (bpos < 0)
        return out;
    QByteArray boundary = contentType.mid(bpos + 9);
    if (boundary.startsWith('"') && boundary.endsWith('"'))
        boundary = boundary.mid(1, boundary.size() - 2);
    const QByteArray sep = "--" + boundary;
    const QList<QByteArray> chunks = body.split('\n'); // reassembled below; split on the separator instead
    Q_UNUSED(chunks);
    int pos = 0;
    while (true) {
        const int start = body.indexOf(sep, pos);
        if (start < 0)
            break;
        int hdrStart = start + sep.size();
        if (body.mid(hdrStart, 2) == "--") // closing boundary
            break;
        if (body.mid(hdrStart, 2) == "\r\n")
            hdrStart += 2;
        const int hdrEnd = body.indexOf("\r\n\r\n", hdrStart);
        if (hdrEnd < 0)
            break;
        const QByteArray partHead = body.mid(hdrStart, hdrEnd - hdrStart);
        const int contentStart = hdrEnd + 4;
        const int next = body.indexOf(sep, contentStart);
        if (next < 0)
            break;
        int contentEnd = next;
        if (body.mid(contentEnd - 2, 2) == "\r\n") // trailing CRLF before the separator
            contentEnd -= 2;
        const QByteArray content = body.mid(contentStart, contentEnd - contentStart);

        // pull the form-data name out of Content-Disposition
        QByteArray name;
        const int nkey = partHead.toLower().indexOf("name=\"");
        if (nkey >= 0) {
            const int ns = nkey + 6;
            const int ne = partHead.indexOf('"', ns);
            if (ne > ns)
                name = partHead.mid(ns, ne - ns);
        }
        const QByteArray md5 = QCryptographicHash::hash(content, QCryptographicHash::Md5).toHex();
        out += "key: " + name + ", value: " + md5 + "\n";
        pos = next;
    }
    return out;
}

// The route table shared by both protocols. `root` is the --folder to read
// files from; `reqContentType` is the request's Content-Type (for multipart).
// Case-insensitive lookup of a request header value in a raw request-head block
// (the text before the blank line). Line 0 is the request line, so skip it.
static inline QByteArray reqHeaderValue(const QByteArray &head, const char *name)
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

// A tiny in-memory WebDAV store shared across requests. server-dummy is a single
// process serving requests serially, so a function-local static is enough: a PUT
// under /dav/ stores the body, a later GET reads it back, a DELETE drops it.
static inline QMap<QByteArray, QByteArray> &davStore()
{
    static QMap<QByteArray, QByteArray> store;
    return store;
}

// Decode a chunked message body beginning at `from` in `buf`. On success sets *body
// to the reassembled payload and *consumed to how many bytes the whole chunked body
// (including its terminating 0-chunk) spans, and returns true; returns false when more
// bytes are still needed. Trailers are skipped.
static inline bool decodeChunkedBody(const QByteArray &buf, int from, QByteArray *body, int *consumed)
{
    QByteArray out;
    int p = from;
    forever {
        const int nl = buf.indexOf("\r\n", p);
        if (nl < 0)
            return false;
        bool ok = false;
        const int sz = buf.mid(p, nl - p).trimmed().toInt(&ok, 16);
        if (!ok)
            return false;
        const int dataStart = nl + 2;
        if (sz == 0) {
            const int endTrailer = buf.indexOf("\r\n", dataStart);
            if (endTrailer < 0)
                return false;
            *body = out;
            *consumed = endTrailer + 2;
            return true;
        }
        if (buf.size() < dataStart + sz + 2)
            return false;
        out += buf.mid(dataStart, sz);
        p = dataStart + sz + 2;
    }
}

// Serve a file that lives under `root`, named by `name` (no leading slash). Fills
// `r` and returns true on a hit; leaves `r` untouched and returns false on a miss.
static inline bool serveFile(HttpReply *r, const QString &root, const QByteArray &name)
{
    if (root.isEmpty())
        return false;
    QFileInfo fi(root + QLatin1Char('/') + QString::fromLatin1(name));
    if (!fi.isFile())
        return false;
    QFile f(fi.absoluteFilePath());
    if (!f.open(QIODevice::ReadOnly))
        return false;
    r->body = f.readAll();
    r->setHeader("content-type", name.endsWith(".html") ? "text/html" : "text/plain");
    r->setHeader("last-modified", httpDate(fi.lastModified()));
    return true;
}

static inline HttpReply buildReply(const QByteArray &method, const QByteArray &path,
                                   const QByteArray &query, const QByteArray &reqContentType,
                                   const QByteArray &reqBody, const QString &root,
                                   const QByteArray &reqHead = QByteArray())
{
    HttpReply r;

    // WebDAV under /dav/: PUT stores (201 Created), GET reads back, DELETE removes.
    if (path.startsWith("/dav/")) {
        if (method == "PUT") {
            davStore().insert(path, reqBody);
            r.code = 201; r.reason = "Created";
            r.setHeader("content-type", "text/plain");
            return r;
        }
        if (method == "DELETE") {
            if (davStore().remove(path) > 0) { r.code = 204; r.reason = "No Content"; }
            else { r.code = 404; r.reason = "Not Found"; r.body = "not found\n"; }
            return r;
        }
        if (davStore().contains(path)) {
            r.body = davStore().value(path);
            r.setHeader("content-type", "text/plain");
            return r;
        }
        r.code = 404; r.reason = "Not Found"; r.body = "not found\n";
        return r;
    }

    // DELETE against a plain static resource is not allowed.
    if (method == "DELETE" && (path == "/index.html" || path == "/")) {
        r.code = 405; r.reason = "Method Not Allowed";
        r.setHeader("allow", "GET, HEAD");
        return r;
    }
    // http-delete.cgi?<tag>: return the status the query names.
    if (path == "/qtest/cgi-bin/http-delete.cgi") {
        r.setHeader("content-type", "text/plain");
        if (query.startsWith("200")) { r.code = 200; r.reason = "OK"; r.body = "ok\n"; }
        else if (query.startsWith("202")) { r.code = 202; r.reason = "Accepted"; r.body = "accepted\n"; }
        else if (query.startsWith("204")) { r.code = 204; r.reason = "No Content"; }
        else { r.code = 404; r.reason = "Not Found"; r.body = "not found\n"; }
        return r;
    }

    if (path == "/") {
        r.setHeader("content-type", "text/html");
        r.setHeader("server", "Apache/2.2.22 (Unix)");
        r.setHeader("last-modified", httpDate(QDateTime::fromMSecsSinceEpoch(Q_INT64_C(1400000000000))));
        r.body = "<html><head><title>server-dummy</title></head><body>"
                 "<h1>It works!</h1></body></html>\n";
        return r;
    }

    // Basic-auth-protected content under /qtest/protected/<...>: same credentials,
    // then serve the underlying resource by re-dispatching without the segment.
    if (path.startsWith("/qtest/protected/")) {
        const QByteArray got = reqHeaderValue(reqHead, "authorization");
        const QByteArray want = "Basic " + QByteArray("httptest:httptest").toBase64();
        if (got != want) {
            r.code = 401; r.reason = "Authorization Required";
            r.setHeader("www-authenticate", "Basic realm=\"QAuthenticator\"");
            r.setHeader("content-type", "text/plain");
            r.body = "authorization required\n";
            return r;
        }
        return buildReply(method, "/qtest/" + path.mid(sizeof("/qtest/protected/") - 1),
                          query, reqContentType, reqBody, root, reqHead);
    }

    // Basic auth under /qtest/rfcs-auth/<file>: credentials httptest:httptest.
    if (path.startsWith("/qtest/rfcs-auth/")) {
        const QByteArray got = reqHeaderValue(reqHead, "authorization");
        const QByteArray want = "Basic " + QByteArray("httptest:httptest").toBase64();
        if (got != want) {
            r.code = 401; r.reason = "Authorization Required";
            r.setHeader("www-authenticate", "Basic realm=\"QAuthenticator\"");
            r.setHeader("content-type", "text/plain");
            r.body = "authorization required\n";
            return r;
        }
        if (serveFile(&r, root, path.mid(sizeof("/qtest/rfcs-auth/") - 1)))
            return r;
        r.code = 404; r.reason = "Not Found"; r.body = "not found\n";
        return r;
    }

    // Digest auth under /qtest/auth-digest/: accept a Digest naming user httptest.
    if (path.startsWith("/qtest/auth-digest")) {
        const QByteArray got = reqHeaderValue(reqHead, "authorization");
        if (!got.startsWith("Digest ") || !got.contains("username=\"httptest\"")) {
            r.code = 401; r.reason = "Authorization Required";
            r.setHeader("www-authenticate",
                        "Digest realm=\"QAuthenticator\", nonce=\"aaaaaaaaaaaaaaaa\", algorithm=MD5, qop=\"auth\"");
            r.setHeader("content-type", "text/plain");
            r.body = "authorization required\n";
            return r;
        }
        r.setHeader("content-type", "text/plain");
        r.body = "digest authentication successful\n";
        return r;
    }

    // HTTP cache tests: httpcachetest_*.cgi. The response headers drive whether the
    // client's QNetworkDiskCache stores and/or revalidates the entry.
    if (path.startsWith("/qtest/cgi-bin/httpcachetest_")) {
        const QByteArray file = path.mid(sizeof("/qtest/cgi-bin/") - 1);
        const QByteArray fixedLM = httpDate(QDateTime::fromMSecsSinceEpoch(Q_INT64_C(1400000000000)));
        const QByteArray etag = "\"server-dummy-cache\"";
        const QByteArray ims = reqHeaderValue(reqHead, "if-modified-since");
        const QByteArray inm = reqHeaderValue(reqHead, "if-none-match");
        r.setHeader("date", httpDate(QDateTime::currentDateTimeUtc()));
        r.setHeader("content-type", "text/html");
        if (file.startsWith("httpcachetest_cachecontrol")) {
            const QByteArray cc = QByteArray::fromPercentEncoding(QByteArray(query).replace('+', ' '));
            if (!cc.isEmpty())
                r.setHeader("cache-control", cc);
            r.setHeader("last-modified", fixedLM);
            if (!ims.isEmpty() && ims == fixedLM) { r.code = 304; r.reason = "Not Modified"; return r; }
            r.body = "cachecontrol\n";
            return r;
        }
        if (file.startsWith("httpcachetest_etag")) {
            r.setHeader("etag", etag);
            if (file.contains("304") && inm == etag) { r.code = 304; r.reason = "Not Modified"; return r; }
            r.body = "etag\n";
            return r;
        }
        if (file.startsWith("httpcachetest_lastModified")) {
            r.setHeader("last-modified", fixedLM);
            if (file.contains("304") && ims == fixedLM) { r.code = 304; r.reason = "Not Modified"; return r; }
            r.body = "lastmodified\n";
            return r;
        }
        // expires200 / expires304: cacheable via a future Expires plus a Last-Modified.
        r.setHeader("expires", httpDate(QDateTime::currentDateTimeUtc().addSecs(3600)));
        r.setHeader("last-modified", fixedLM);
        if (file.contains("304") && ims == fixedLM) { r.code = 304; r.reason = "Not Modified"; return r; }
        r.body = "expires\n";
        return r;
    }

    if (path == "/qtest/cgi-bin/echo.cgi") {
        r.setHeader("content-type", "text/plain");
        r.body = query; // the negotiation rows fetch "?1" and expect the body "1"
        return r;
    }
    if (path == "/qtest/cgi-bin/md5sum.cgi") {
        r.setHeader("content-type", "text/plain");
        r.body = QCryptographicHash::hash(reqBody, QCryptographicHash::Md5).toHex() + "\n";
        return r;
    }
    if (path == "/qtest/cgi-bin/multipart.cgi") {
        r.setHeader("content-type", "text/plain");
        r.body = multipartSummary(reqContentType, reqBody);
        return r;
    }

    // The /qtest/ directory index.
    if (path == "/qtest/" || path == "/qtest/index.html") {
        if (serveFile(&r, root, "index.html"))
            return r;
        r.setHeader("content-type", "text/html");
        r.body = "<html><body>qtest</body></html>\n";
        return r;
    }

    if (path.startsWith("/qtest/") && serveFile(&r, root, path.mid(sizeof("/qtest/") - 1)))
        return r;

    r.code = 404;
    r.reason = "Not Found";
    r.setHeader("content-type", "text/plain");
    r.body = "not found\n";
    return r;
}

// ---- SPDY/3.0 connection ---------------------------------------------------

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

    // Inflate a SPDY header block into (name,value) pairs.
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

    // Deflate a response header block (count + pairs) with the running stream.
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

    // Send as much of the stream's pending body as the flow-control window allows.
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

// ---- HTTP/1.1 over the same TLS socket -------------------------------------

class Http1Connection : public QObject
{
    Q_OBJECT
public:
    // Takes a QTcpSocket so the same HTTP/1.1 responder serves both the plain
    // listener and (via the QSslSocket subclass) the TLS listener's http/1.1 path.
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

            const QList<QByteArray> reqLine = head.left(head.indexOf('\n')).simplified().split(' ');
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
            for (int i = 0; i < reply.headers.size(); ++i)
                resp += reply.headers.at(i).first + ": " + reply.headers.at(i).second + "\r\n";
            resp += "Content-Length: " + QByteArray::number(reply.body.size()) + "\r\n\r\n";
            if (method != "HEAD") // a HEAD reply carries the headers and length, but no body
                resp += reply.body;
            m_socket->write(resp);
        }
    }

    QTcpSocket *m_socket;
    QString m_root;
    QByteArray m_buf;
    bool m_continued; // sent a "100 Continue" for the in-flight request already
};

// ---- plain HTTP/1.1 listener (the http-url row, port 8080) ------------------

class PlainHttpService : public TestService
{
public:
    PlainHttpService(const QHostAddress &host, quint16 port, const QString &folder,
                     QObject *parent = Q_NULLPTR)
        : TestService(host, port, parent), m_root(folder) {}

    void start() Q_DECL_OVERRIDE
    {
        QTcpServer *server = new QTcpServer(this);
        listenTcp(server);
        connect(server, &QTcpServer::newConnection, server, [this, server]() {
            while (server->hasPendingConnections()) {
                QTcpSocket *s = server->nextPendingConnection();
                new Http1Connection(s, m_root, s);
            }
        });
    }

private:
    QString m_root;
};

// Daytime service (port 13): on connect, write the time and close, so a client's
// waitForDisconnected() succeeds. connectToHostEncrypted's tail connects here in
// plain mode to check the socket drops back to UnencryptedMode.
class DaytimeService : public TestService
{
public:
    DaytimeService(const QHostAddress &host, quint16 port, QObject *parent = Q_NULLPTR)
        : TestService(host, port, parent) {}

    void start() Q_DECL_OVERRIDE
    {
        QTcpServer *server = new QTcpServer(this);
        listenTcp(server);
        connect(server, &QTcpServer::newConnection, server, [server]() {
            while (server->hasPendingConnections()) {
                QTcpSocket *c = server->nextPendingConnection();
                c->write(QDateTime::currentDateTime().toString(Qt::ISODate).toLatin1() + "\r\n");
                connect(c, &QTcpSocket::disconnected, c, &QObject::deleteLater);
                c->disconnectFromHost();
            }
        });
    }
};

// Map a proxy CONNECT / SOCKS target onto the local listeners: a proxied request
// names the service by its public host and port (qt-test-server:443/80), but the
// fleet actually serves it on loopback at the port testPort() picks, so the proxy
// (a separate process) rewrites the target to 127.0.0.1:4433/8080.
static inline void remapTarget(QString *host, quint16 *port)
{
    // Send the proxied request to wherever the fleet actually serves that service:
    // its canonical port when elevated, the unprivileged stand-in otherwise --
    // exactly how the fleet binds them (testPort()).
    *port = testPort(*port);
    *host = QString::fromLatin1("127.0.0.1");
}

// ---- HTTP CONNECT proxy (ports 3128 no-auth, 3129 Basic-auth) ---------------

class ConnectProxyService : public TestService
{
public:
    ConnectProxyService(const QHostAddress &host, quint16 port, bool requireAuth,
                        QObject *parent = Q_NULLPTR)
        : TestService(host, port, parent), m_requireAuth(requireAuth) {}

    void start() Q_DECL_OVERRIDE
    {
        QTcpServer *server = new QTcpServer(this);
        listenTcp(server);
        connect(server, &QTcpServer::newConnection, server, [this, server]() {
            while (server->hasPendingConnections()) {
                QTcpSocket *c = server->nextPendingConnection();
                connect(c, &QTcpSocket::readyRead, c, [this, c]() { onData(c); });
                connect(c, &QTcpSocket::disconnected, c, &QObject::deleteLater);
            }
        });
    }

private:
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
            if (!auth.startsWith("Basic ")) {
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

            QTcpSocket *upstream = new QTcpSocket(client);
            client->setProperty("tunnel", true);
            connect(upstream, &QTcpSocket::connected, client, [client, upstream]() {
                upstream->setProperty("up", true);
                client->write("HTTP/1.1 200 Connection established\r\n\r\n");
            });
            connect(upstream, static_cast<void (QTcpSocket::*)(QAbstractSocket::SocketError)>(&QTcpSocket::error),
                    client, [client, upstream]() {
                if (!upstream->property("up").toBool()) {
                    client->write("HTTP/1.1 503 Service Unavailable\r\n\r\n");
                    client->disconnectFromHost();
                }
            });
            connect(upstream, &QTcpSocket::readyRead, client, [client, upstream]() { client->write(upstream->readAll()); });
            connect(client, &QTcpSocket::readyRead, upstream, [client, upstream]() { upstream->write(client->readAll()); });
            connect(upstream, &QTcpSocket::disconnected, client, [client, upstream]() {
                client->write(upstream->readAll());
                client->disconnectFromHost();
            });
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
        connect(upstream, &QTcpSocket::connected, upstream, [upstream, fwd]() { upstream->write(fwd); });
        connect(upstream, &QTcpSocket::readyRead, client, [client, upstream]() { client->write(upstream->readAll()); });
        connect(upstream, &QTcpSocket::disconnected, client, [client, upstream]() {
            client->write(upstream->readAll());
            client->disconnectFromHost();
        });
        connect(upstream, static_cast<void (QTcpSocket::*)(QAbstractSocket::SocketError)>(&QTcpSocket::error),
                client, [client]() {
            client->write("HTTP/1.1 503 Service Unavailable\r\nContent-Length: 0\r\n\r\n");
            client->disconnectFromHost();
        });
        upstream->connectToHost(host, port);
    }

    bool m_requireAuth;
};

// ---- SOCKS5 proxy with target remap (ports 1080 no-auth, 1081 user/pass) ----

class SocksProxyService : public TestService
{
public:
    SocksProxyService(const QHostAddress &host, quint16 port, bool requireAuth,
                      QObject *parent = Q_NULLPTR)
        : TestService(host, port, parent), m_requireAuth(requireAuth) {}

    void start() Q_DECL_OVERRIDE
    {
        QTcpServer *server = new QTcpServer(this);
        listenTcp(server);
        connect(server, &QTcpServer::newConnection, server, [this, server]() {
            while (server->hasPendingConnections()) {
                QTcpSocket *c = server->nextPendingConnection();
                connect(c, &QTcpSocket::readyRead, c, [this, c]() { onSocks(c); });
                connect(c, &QTcpSocket::disconnected, c, &QObject::deleteLater);
            }
        });
    }

private:
    void onSocks(QTcpSocket *client)
    {
        if (client->property("tunnel").toBool())
            return;
        if (!client->property("greeted").toBool()) {
            if (client->bytesAvailable() < 2)
                return;
            const QByteArray g = client->read(2);
            if (quint8(g.at(0)) != 0x05)
                return;
            const QByteArray methods = client->read(quint8(g.at(1)));
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
            client->read(1); // version
            const quint8 ulen = quint8(client->read(1).at(0));
            client->read(ulen);
            if (client->bytesAvailable() < 1)
                return;
            const quint8 plen = quint8(client->read(1).at(0));
            client->read(plen);
            client->write(QByteArray::fromRawData("\x01\x00", 2)); // accept any credentials
            client->setProperty("authed", true);
            return;
        }
        if (client->property("target").isNull()) {
            if (client->bytesAvailable() < 4)
                return;
            const QByteArray hdr = client->read(4);
            const quint8 atyp = quint8(hdr.at(3));
            QString hostStr;
            if (atyp == 0x01) {
                const QByteArray a = client->read(4);
                hostStr = QHostAddress(qFromBigEndian<quint32>((const uchar *)a.constData())).toString();
            } else if (atyp == 0x03) {
                const quint8 len = quint8(client->read(1).at(0));
                hostStr = QString::fromLatin1(client->read(len));
            }
            quint16 dport = qFromBigEndian<quint16>((const uchar *)client->read(2).constData());
            remapTarget(&hostStr, &dport);
            QTcpSocket *upstream = new QTcpSocket(client);
            client->setProperty("target", true);
            connect(upstream, &QTcpSocket::connected, client, [client, upstream]() {
                upstream->setProperty("up", true);
                client->setProperty("tunnel", true);
                client->write(QByteArray::fromRawData("\x05\x00\x00\x01\x00\x00\x00\x00\x00\x00", 10));
            });
            connect(upstream, static_cast<void (QTcpSocket::*)(QAbstractSocket::SocketError)>(&QTcpSocket::error),
                    client, [client, upstream]() {
                if (!upstream->property("up").toBool()) {
                    client->write(QByteArray::fromRawData("\x05\x05\x00\x01\x00\x00\x00\x00\x00\x00", 10));
                    client->disconnectFromHost();
                }
            });
            connect(upstream, &QTcpSocket::readyRead, client, [client, upstream]() { client->write(upstream->readAll()); });
            connect(client, &QTcpSocket::readyRead, upstream, [client, upstream]() { upstream->write(client->readAll()); });
            connect(upstream, &QTcpSocket::disconnected, client, [client, upstream]() {
                client->write(upstream->readAll());
                client->disconnectFromHost();
            });
            upstream->connectToHost(hostStr, dport);
        }
    }

    bool m_requireAuth;
};

// ---- the TLS listener that dispatches by negotiated protocol ---------------

// A QTcpServer that turns each accepted descriptor straight into a server-side
// QSslSocket. Building the QSslSocket from the raw descriptor (rather than
// wrapping a QTcpSocket handed out by nextPendingConnection) avoids two socket
// notifiers fighting over the same fd.
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
        s->setSslConfiguration(m_cfg);
        connect(s, &QSslSocket::encrypted, s, [this, s]() {
            const QByteArray proto = s->sslConfiguration().nextNegotiatedProtocol();
            if (proto == "h2")
                new Http2Connection(s, m_root, s);
            else if (proto == "spdy/3")
                new SpdyConnection(s, m_root, s);
            else
                new Http1Connection(s, m_root, s);
        });
        connect(s, static_cast<void (QSslSocket::*)(const QList<QSslError> &)>(&QSslSocket::sslErrors),
                s, [s](const QList<QSslError> &) { /* self-signed cert is expected */ });
        connect(s, &QSslSocket::disconnected, s, &QObject::deleteLater);
        s->startServerEncryption();
    }

private:
    QSslConfiguration m_cfg;
    QString m_root;
};

// A minimal TLS greeting server (imaps on 993): completes the handshake and writes
// a one-line greeting, so a client's waitForReadyRead() sees data. Cert-verify is the
// client's business (these tests ignore SSL errors).
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
        const QByteArray greeting = m_greeting;
        connect(s, &QSslSocket::encrypted, s, [s, greeting]() { s->write(greeting); });
        connect(s, static_cast<void (QSslSocket::*)(const QList<QSslError> &)>(&QSslSocket::sslErrors),
                s, [s](const QList<QSslError> &) {});
        connect(s, &QSslSocket::disconnected, s, &QObject::deleteLater);
        s->startServerEncryption();
    }

private:
    QSslConfiguration m_cfg;
    QByteArray m_greeting;
};

class ImapsService : public TestService
{
public:
    ImapsService(const QHostAddress &host, quint16 port, QObject *parent = Q_NULLPTR)
        : TestService(host, port, parent) {}

    void start() Q_DECL_OVERRIDE
    {
        QSslConfiguration cfg = QSslConfiguration::defaultConfiguration();
        cfg.setLocalCertificate(QSslCertificate(QByteArray(kServerCertPem), QSsl::Pem));
        cfg.setPrivateKey(QSslKey(QByteArray(kServerKeyPem), QSsl::Rsa, QSsl::Pem));
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
    HttpsService(const QHostAddress &host, quint16 port, const QString &folder,
                 QObject *parent = Q_NULLPTR)
        : TestService(host, port, parent), m_root(folder) {}

    void start() Q_DECL_OVERRIDE
    {
        QSslConfiguration cfg = QSslConfiguration::defaultConfiguration();
        // NPN (which SPDY/3 negotiates over) is undefined for TLS 1.3, and OpenSSL
        // 1.1.1 would otherwise pick 1.3 and skip the NPN callbacks. Cap at TLS 1.2,
        // as the SPDY-era servers this test targets did.
        cfg.setProtocol(QSsl::TlsV1_2);
        cfg.setLocalCertificate(QSslCertificate(QByteArray(kServerCertPem), QSsl::Pem));
        cfg.setPrivateKey(QSslKey(QByteArray(kServerKeyPem), QSsl::Rsa, QSsl::Pem));
        QList<QByteArray> npn;
        // Offer HTTP/2 (ALPN "h2"), SPDY/3 (NPN "spdy/3") and HTTP/1.1. The client's
        // offer decides: an HTTP/2 client picks h2 over ALPN, a SPDY client spdy/3.
        npn << QByteArray("h2") << QByteArray("spdy/3") << QByteArray("http/1.1");
        cfg.setAllowedNextProtocols(npn);

        SslServer *server = new SslServer(cfg, m_root, this);
        listenTcp(server);
    }

private:
    QString m_root;
};

#endif // HTTPS_SPDY_H
