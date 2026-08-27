#ifndef HTTP_REPLY_H
#define HTTP_REPLY_H

/// @file
/// The shared HTTP/1.1 responder: HttpReply + buildReply() and their helpers,
/// serving the route set the network tests expect (echo/md5sum/multipart CGIs,
/// cache headers, WebDAV, Basic/Digest auth, files from a folder).
/// Both the plain listener (HttpService, in http_service.h)
/// and the TLS/SPDY listener (Http1Connection, in https_spdy.h)
/// build their replies here, so the two serve identical routes.

#include <QtCore/QByteArray>
#include <QtCore/QCryptographicHash>
#include <QtCore/QDateTime>
#include <QtCore/QFile>
#include <QtCore/QFileInfo>
#include <QtCore/QList>
#include <QtCore/QLocale>
#include <QtCore/QMap>
#include <QtCore/QPair>
#include <QtCore/QString>

#include "./service_base.h"

// `std::sort` (name-sort the multipart form-data summary).
#include <algorithm>
// memset
#include <cstring>
// Gzip the /qtest/deflate/ fixtures (Content-Encoding: gzip).
#include <zlib.h>

/// gzip-wrap bytes with the bundled zlib (windowBits 15+16 selects the
/// gzip container). Used to serve the /qtest/deflate/ fixtures so QNAM
/// decodes them (tst_QNetworkReply::synchronousRequest http-gzip).
/// Mirrors HttpService::gzipBody(); kept here so the shared responder
/// is self-contained.
static inline QByteArray gzipEncode(const QByteArray &in)
{
    z_stream zs;
    memset(&zs, 0, sizeof(zs));
    deflateInit2(&zs, Z_DEFAULT_COMPRESSION, Z_DEFLATED, 15 + 16, 8, Z_DEFAULT_STRATEGY);
    QByteArray out(int(deflateBound(&zs, uLong(in.size()))), Qt::Uninitialized);
    zs.next_in = reinterpret_cast<Bytef * >(const_cast<char * >(in.constData()));
    zs.avail_in = uInt(in.size());
    zs.next_out = reinterpret_cast<Bytef * >(out.data());
    zs.avail_out = uInt(out.size());
    deflate(&zs, Z_FINISH);
    out.truncate(int(zs.total_out));
    deflateEnd(&zs);
    return out;
}

struct HttpReply
{
    int code;
    QByteArray reason;
    /// Lower-case names, minus status/version.
    QList<QPair<QByteArray, QByteArray > > headers;
    QByteArray body;
    HttpReply() : code(200), reason("OK") {}
    void setHeader(const QByteArray &name, const QByteArray &value)
    {
        headers.append(qMakePair(name.toLower(), value));
    }
    /// Replaces every header of `name` with one of `value`; an empty `value`
    /// leaves none.
    void replaceHeader(const QByteArray &name, const QByteArray &value)
    {
        const QByteArray key = name.toLower();
        for (int i = headers.size() - 1; i >= 0; --i) {
            if (headers.at(i).first == key) {
                headers.removeAt(i);
            }
        }
        if (!value.isEmpty()) {
            headers.append(qMakePair(key, value));
        }
    }
};

// MARK: shared routing.

/// Split a raw request target into decoded path and (raw) query.
static inline void splitTarget(const QByteArray &target, QByteArray *path,
                               QByteArray *query)
{
    QByteArray p = target;
    // Strip an absolute-form URI.
    const int schemeSlash = p.indexOf("//");
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
    return QLocale::c().toString(dt.toUTC(), QLL("ddd, dd MMM yyyy HH:mm:ss")).toLatin1() + " GMT";
}

/// Builds the header lines of @p reply, with `Date` first, the way a web
/// server sends it: the reply's own `date` header when it has one, else now.
/// @p leading goes right after it (such as a fixed `Server` line).
static inline QByteArray httpHeaderLines(const HttpReply &reply,
                                         const QByteArray &leading = QByteArray())
{
    QByteArray date;
    QByteArray rest;
    for (int i = 0; i < reply.headers.size(); ++i) {
        const QPair<QByteArray, QByteArray > &h = reply.headers.at(i);
        if (h.first == "date") {
            date = h.second;
        } else {
            rest += h.first + ": " + h.second + "\r\n";
        }
    }
    if (date.isEmpty()) {
        date = httpDate(QDateTime::currentDateTimeUtc());
    }
    return "Date: " + date + "\r\n" + leading + rest;
}

/// Compute the multipart.cgi response: one md5 line per form-data part.
static inline QByteArray multipartSummary(const QByteArray &contentType,
                                          const QByteArray &body)
{
    QByteArray out = "content type: " + contentType + "\n";
    const int bpos = contentType.toLower().indexOf("boundary=");
    if (bpos < 0) {
        return out;
    }
    QByteArray boundary = contentType.mid(bpos + 9);
    if (boundary.startsWith('"') && boundary.endsWith('"')) {
        boundary = boundary.mid(1, boundary.size() - 2);
    }
    const QByteArray sep = "--" + boundary;
    // Collected, then emitted name-sorted.
    QList<QPair<QByteArray, QByteArray > > formFields;
    int pos = 0;
    while (true) {
        const int start = body.indexOf(sep, pos);
        if (start < 0) {
            break;
        }
        int hdrStart = start + sep.size();
        // Closing boundary.
        if (body.mid(hdrStart, 2) == "--") {
            break;
        }
        if (body.mid(hdrStart, 2) == "\r\n") {
            hdrStart += 2;
        }
        const int hdrEnd = body.indexOf("\r\n\r\n", hdrStart);
        if (hdrEnd < 0) {
            break;
        }
        const QByteArray partHead = body.mid(hdrStart, hdrEnd - hdrStart);
        const int contentStart = hdrEnd + 4;
        const int next = body.indexOf(sep, contentStart);
        if (next < 0) {
            break;
        }
        int contentEnd = next;
        // Trailing CRLF before the separator.
        if (body.mid(contentEnd - 2, 2) == "\r\n") {
            contentEnd -= 2;
        }
        const QByteArray content = body.mid(contentStart, contentEnd - contentStart);

        const QList<QByteArray > hlines = partHead.split('\n');
        if (contentType.toLower().contains("multipart/form-data")) {
            // "key: <name>, value: ...": a text field (no type, or text/*)
            // echoes its raw value; a typed part (an uploaded file/image)
            // is summarised by its md5.
            QByteArray name, ctype;
            for (int i = 0; i < hlines.size(); ++i) {
                const QByteArray l = hlines.at(i).trimmed();
                const QByteArray low = l.toLower();
                if (low.startsWith("content-disposition:")) {
                    const int nk = l.indexOf("name=\"");
                    if (nk >= 0) {
                        const int ns = nk + 6;
                        const int ne = l.indexOf('"', ns);
                        if (ne > ns) {
                            name = l.mid(ns, ne - ns);
                        }
                    }
                } else if (low.startsWith("content-type:")) {
                    ctype = l.mid(l.indexOf(':') + 1).trimmed();
                }
            }
            const bool textual = ctype.isEmpty()
                || ctype.toLower().startsWith("text/");
            const QByteArray value = textual
                ? content
                    : QCryptographicHash::hash(content, QCryptographicHash::Md5).toHex();
            formFields.append(qMakePair(name, value));
        } else {
            // Any other multipart type is dumped verbatim: each header,
            // then the content, then a blank line, in the order
            // the parts arrived.
            for (int i = 0; i < hlines.size(); ++i) {
                const QByteArray l = hlines.at(i).trimmed();
                const int c = l.indexOf(':');
                if (c < 0) {
                    continue;
                }
                out += "header: " + l.left(c).trimmed() + ", value: '"
                    + l.mid(c + 1).trimmed() + "'\n";
            }
            out += "content: " + content + "\n\n";
        }
        pos = next;
    }
    // Emit the form-data fields in a deterministic order by name
    // (`QPair` compares by first, then second), so the summary is stable
    // regardless of the wire order the parts arrived in -- the client's
    // test fixtures assert this same sorted order.
    std::sort(formFields.begin(), formFields.end());
    for (int i = 0; i < formFields.size(); ++i) {
        out += "key: " + formFields.at(i).first + ", value: "
            + formFields.at(i).second + "\n";
    }
    return out;
}

/// The route table shared by both protocols. `root` is the --folder to
/// read files from; `reqContentType` is the request's Content-Type
/// (for multipart). Case-insensitive lookup of a request header value in a
/// raw request-head block (the text before the blank line).
/// Line 0 is the request line, so skip it.
static inline QByteArray reqHeaderValue(const QByteArray &head,
                                        const char *name)
{
    const QList<QByteArray > lines = head.split('\n');
    const QByteArray key = QByteArray(name).toLower();
    for (int i = 1; i < lines.size(); ++i) {
        const int c = lines.at(i).indexOf(':');
        if (c > 0 && lines.at(i).left(c).trimmed().toLower() == key) {
            return lines.at(i).mid(c + 1).trimmed();
        }
    }
    return QByteArray();
}

/// A tiny in-memory WebDAV store shared across requests.
/// server-dummy is a single process serving requests serially, so a
/// function-local static is enough: a PUT under /dav/ stores the body,
/// a later GET reads it back, a DELETE drops it.
static inline QMap<QByteArray, QByteArray > &davStore()
{
    static QMap<QByteArray, QByteArray > store;
    return store;
}

/// Decode a chunked message body beginning at `from` in `buf`.
/// On success sets *body to the reassembled payload and *consumed to how
/// many bytes the whole chunked body (including its terminating 0-chunk)
/// spans, and returns true; returns false when more bytes are still needed.
/// Trailers are skipped.
static inline bool decodeChunkedBody(const QByteArray &buf, int from,
                                     QByteArray *body, int *consumed)
{
    QByteArray out;
    int p = from;
    forever {
        const int nl = buf.indexOf("\r\n", p);
        if (nl < 0) {
            return false;
        }
        bool ok = false;
        const int sz = buf.mid(p, nl - p).trimmed().toInt(&ok, 16);
        if (!ok) {
            return false;
        }
        const int dataStart = nl + 2;
        if (sz == 0) {
            const int endTrailer = buf.indexOf("\r\n", dataStart);
            if (endTrailer < 0) {
                return false;
            }
            *body = out;
            *consumed = endTrailer + 2;
            return true;
        }
        if (buf.size() < dataStart + sz + 2) {
            return false;
        }
        out += buf.mid(dataStart, sz);
        p = dataStart + sz + 2;
    }
}

/// Serve a file that lives under `root`, named by `name` (no leading slash).
/// Fills `r` and returns true on a hit; leaves `r` untouched and
/// returns false on a miss.
static inline bool serveFile(HttpReply *r, const QString &root,
                             const QByteArray &name)
{
    if (root.isEmpty()) {
        return false;
    }
    QFileInfo fi(root + QLatin1Char('/') + QString::fromLatin1(name));
    if (!fi.isFile()) {
        return false;
    }
    QFile f(fi.absoluteFilePath());
    if (!f.open(QIODevice::ReadOnly)) {
        return false;
    }
    r->body = f.readAll();
    r->setHeader("content-type", name.endsWith(".html") ? "text/html" : "text/plain");
    r->setHeader("last-modified", httpDate(fi.lastModified()));
    return true;
}

/// Extract the value of `key=` from an RFC 2617 Digest Authorization header
/// (quoted or bare token), matching only at a token boundary so e.g.
/// "nc" is not found inside "cnonce".
static inline QByteArray digestField(const QByteArray &hdr, const char *key)
{
    const QByteArray needle = QByteArray(key) + "=";
    int i = 0;
    for (;;) {
        i = hdr.indexOf(needle, i);
        if (i < 0) {
            return QByteArray();
        }
        const char b = (i == 0) ? ' ' : hdr.at(i - 1);
        if (b == ' ' || b == ',' || b == '\t') {
            break;
        }
        i += needle.size();
    }
    int v = i + needle.size();
    if (v < hdr.size() && hdr.at(v) == '"') {
        const int end = hdr.indexOf('"', v + 1);
        return (end < 0) ? QByteArray() : hdr.mid(v + 1, end - v - 1);
    }
    int end = v;
    while (end < hdr.size() && hdr.at(end) != ',' && hdr.at(end) != ' ') {
        ++end;
    }
    return hdr.mid(v, end - v);
}

static inline QByteArray md5Hex(const QByteArray &in)
{ return QCryptographicHash::hash(in, QCryptographicHash::Md5).toHex(); }

/// Describes the request a route answers, as the listeners parse it.
struct HttpRequest
{
    QByteArray method;
    QByteArray path;
    QByteArray query;
    QByteArray contentType;
    QByteArray body;
    QByteArray head;
    /// Lets a route limit itself to a listener: `plain`, `secure`, or `http2`.
    QByteArray listener;
    /// Names the --folder that file routes read from.
    QString root;
};

static inline HttpReply buildReply(const HttpRequest &req);

// MARK: route types.

/// Fills `r` with a status reply, after the route's settings for the request's
/// method (`<method>.code` and the like) and then its plain ones.
///
/// A reply of 400 or above with no `body` setting at all carries its reason
/// phrase as its body, so that a client waiting for body bytes sees some.
static inline void replyStatic(const HttpRoute &route, const HttpRequest &req,
                               HttpReply *r)
{
    const QByteArray scoped = req.method.toLower() + '.';
    const QByteArray code = route.param(scoped + "code", route.param("code", "200"));
    r->code = code.toInt();
    r->reason = route.param(scoped + "reason", route.param("reason", "OK"));
    r->setHeader("content-type", "text/plain");
    if (route.has(scoped + "body")) {
        r->body = route.param(scoped + "body");
    } else if (route.has("body")) {
        r->body = route.param("body");
    } else if (r->code >= 400) {
        r->body = r->reason + '\n';
    }
}

/// Fills `r` with `size` bytes: the `head` bytes, then NUL bytes.
static inline void replyNullBytes(const HttpRoute &route, HttpReply *r)
{
    r->setHeader("content-type", "application/octet-stream");
    const QByteArray head = route.param("head");
    const qint64 size = qMax<qint64 >(route.size("size"), head.size());
    r->body = head + QByteArray(int(size - head.size()), '\0');
}

/// Fills `r` with `size` copies of the `byte` setting.
static inline void replyRepeatedByte(const HttpRoute &route, HttpReply *r)
{
    r->setHeader("content-type", "text/plain");
    const QByteArray byte = route.param("byte", "x");
    r->body = QByteArray(int(route.size("size")), byte.isEmpty() ? 'x' : byte.at(0));
}

/// Fills `r` with the `text` setting repeated up to at least `size` bytes, gzip
/// encoded when `content-encoding` says so.
static inline void replyRepeatedText(const HttpRoute &route, HttpReply *r)
{
    r->setHeader("content-type", "text/plain");
    const QByteArray text = route.param("text", "x");
    const qint64 size = route.size("size");
    QByteArray body;
    body.reserve(int(size + text.size()));
    while (body.size() < size) {
        body += text;
    }
    if (route.param("content-encoding") == "gzip") {
        body = gzipEncode(body);
        r->setHeader("content-encoding", "gzip");
    }
    r->body = body;
}

/// Fills `r` with what the request sent: its body always (`source = body`),
/// else the body of a POST or PUT, and the query of any other method.
static inline void replyEcho(const HttpRoute &route, const HttpRequest &req,
                             HttpReply *r)
{
    r->setHeader("content-type", "text/plain");
    const bool upload = req.method == "POST" || req.method == "PUT";
    r->body = (route.param("source") == "body" || upload) ? req.body : req.query;
}

/// Serves the file the request names below the route's `strip` prefix; tells
/// whether one was found.
static inline bool serveStrippedFile(const HttpRoute &route, const HttpRequest &req,
                                     HttpReply *r)
{
    const QByteArray strip = route.param("strip");
    if (!req.path.startsWith(strip)) {
        return false;
    }
    return serveFile(r, req.root, req.path.mid(strip.size()));
}

/// Fills `r` with a Basic challenge (or `challenge-text`) unless the request
/// carries the route's `user` and `password`; then answers per `serve`: the
/// `text` setting, a file below `strip`, or the route that answers the path
/// with `strip` replaced by `replace`.
static inline void replyBasicAuth(const HttpRoute &route,
                                  const HttpRequest &req, HttpReply *r)
{
    const QByteArray credentials = route.param("user") + ':'
        + route.param("password");
    const QByteArray want = "Basic " + credentials.toBase64();
    if (reqHeaderValue(req.head, "authorization") != want) {
        r->code = 401;
        r->reason = route.param("reason", "Authorization Required");
        r->setHeader("www-authenticate",
                     "Basic realm=\"" + route.param("realm", "QAuthenticator") + '"');
        r->setHeader("content-type", "text/plain");
        r->body = route.param("challenge-text", "authorization required\n");
        return;
    }
    const QByteArray serve = route.param("serve", "text");
    if (serve == "redispatch") {
        HttpRequest inner = req;
        inner.path = route.param("replace")
            + req.path.mid(route.param("strip").size());
        *r = buildReply(inner);
        return;
    }
    if (serve == "files") {
        if (!serveStrippedFile(route, req, r)) {
            r->code = 404;
            r->reason = "Not Found";
            r->body = "not found\n";
        }
        return;
    }
    r->setHeader("content-type", "text/plain");
    r->body = route.param("text");
}

/// Fills `r` with a Digest challenge unless the request answers it: by the
/// RFC 2617 response hash (`check = full`) or by its user name alone (`check =
/// user`); then with the `text` setting.
static inline void replyDigestAuth(const HttpRoute &route, const HttpRequest &req,
                                   HttpReply *r)
{
    const QByteArray got = reqHeaderValue(req.head, "authorization");
    const QByteArray user = route.param("user");
    bool ok = got.startsWith("Digest ");
    if (ok && route.param("check", "full") == "user") {
        ok = got.contains("username=\"" + user + '"');
    } else if (ok) {
        const QByteArray realm = route.param("realm", "QAuthenticator");
        const QByteArray nonce = digestField(got, "nonce");
        const QByteArray uri = digestField(got, "uri");
        const QByteArray qop = digestField(got, "qop");
        const QByteArray nc = digestField(got, "nc");
        const QByteArray cnonce = digestField(got, "cnonce");
        const QByteArray resp = digestField(got, "response");
        const QByteArray ha1 = md5Hex(digestField(got, "username") + ':' + realm + ':'
                                      + route.param("password"));
        const QByteArray ha2 = md5Hex(req.method + ':' + uri);
        const QByteArray expected = qop.isEmpty()
            ? md5Hex(ha1 + ':' + nonce + ':' + ha2)
            : md5Hex(ha1 + ':' + nonce + ':' + nc + ':' + cnonce + ':' + qop + ':' + ha2);
        ok = digestField(got, "username") == user && !resp.isEmpty()
            && resp == expected;
    }
    if (!ok) {
        QByteArray challenge = "Digest realm=\"" + route.param("realm", "QAuthenticator")
            + "\", nonce=\"" + route.param("nonce") + '"';
        if (route.has("algorithm")) {
            challenge += ", algorithm=" + route.param("algorithm");
        }
        if (route.has("qop")) {
            challenge += ", qop=\"" + route.param("qop") + '"';
        }
        r->code = 401;
        r->reason = route.param("reason", "Authorization Required");
        r->setHeader("www-authenticate", challenge);
        r->setHeader("content-type", "text/plain");
        r->body = route.param("challenge-text", "authorization required\n");
        return;
    }
    r->setHeader("content-type", "text/plain");
    r->body = route.param("text");
}

/// Keeps an in-memory WebDAV store: PUT stores (201 Created), GET reads back,
/// DELETE removes.
static inline void replyWebDav(const HttpRequest &req, HttpReply *r)
{
    if (req.method == "PUT") {
        davStore().insert(req.path, req.body);
        r->code = 201;
        r->reason = "Created";
        r->setHeader("content-type", "text/plain");
        return;
    }
    if (req.method == "DELETE") {
        if (davStore().remove(req.path) > 0) { r->code = 204; r->reason = "No Content"; }
        else { r->code = 404; r->reason = "Not Found"; r->body = "not found\n"; }
        return;
    }
    if (davStore().contains(req.path)) {
        r->body = davStore().value(req.path);
        r->setHeader("content-type", "text/plain");
        return;
    }
    r->code = 404;
    r->reason = "Not Found";
    r->body = "not found\n";
}

/// Answers with the status the query starts with, when the route lists it in
/// `codes`, with its `reason.<code>` and `body.<code>`; else with 404.
static inline void replyQueryStatus(const HttpRoute &route, const HttpRequest &req,
                                    HttpReply *r)
{
    r->setHeader("content-type", "text/plain");
    const QList<QByteArray > codes = route.param("codes").split(',');
    for (int i = 0; i < codes.size(); ++i) {
        const QByteArray code = codes.at(i).trimmed();
        if (!code.isEmpty() && req.query.startsWith(code)) {
            r->code = code.toInt();
            r->reason = route.param("reason." + code, "OK");
            r->body = route.param("body." + code);
            return;
        }
    }
    r->code = 404;
    r->reason = "Not Found";
    r->body = "not found\n";
}

/// Answers the httpcachetest_*.cgi family: the name below `strip` picks the
/// cache headers that drive whether the client's QNetworkDiskCache stores
/// and/or revalidates the entry.
static inline void replyCacheTest(const HttpRoute &route,
                                  const HttpRequest &req, HttpReply *r)
{
    const QByteArray file = req.path.mid(route.param("strip").size());
    const QByteArray fixedLM = httpDate(QDateTime::fromMSecsSinceEpoch(Q_INT64_C(1400000000000)));
    const QByteArray etag = "\"server-dummy-cache\"";
    const QByteArray ims = reqHeaderValue(req.head, "if-modified-since");
    const QByteArray inm = reqHeaderValue(req.head, "if-none-match");
    const QDateTime now = QDateTime::currentDateTimeUtc();
    r->setHeader("date", httpDate(now));
    r->setHeader("content-type", "text/html");
    // Fresh (future expiry).
    const bool is500 = file.contains("500");
    // Stale, but revalidation answers 304.
    const bool is304 = file.contains("304");

    // Cachecontrol[-expire][200].cgi?<directives>: echo the query
    // back as Cache-Control.
    if (file.startsWith("httpcachetest_cachecontrol")) {
        if (file.startsWith("httpcachetest_cachecontrol-expire")) {
            // Already expired, with no validator: never served from a cache.
            r->setHeader("expires", httpDate(now.addSecs(-3600)));
        } else {
            const QByteArray cc = QByteArray::fromPercentEncoding(QByteArray(req.query).replace('+', ' '));
            if (!cc.isEmpty()) {
                r->setHeader("cache-control", cc);
            }
            r->setHeader("last-modified", fixedLM);
            // no-cache means the stored copy must never be used as-is:
            // answer a fresh 200 even to a conditional GET so nothing is
            // served from the cache.
            if (!cc.contains("no-cache") && !ims.isEmpty() && ims == fixedLM) {
                r->code = 304;
                r->reason = "Not Modified";
                return;
            }
        }
        r->body = "cachecontrol\n";
        return;
    }

    if (file.startsWith("httpcachetest_etag")) {
        // No freshness lifetime, so every use revalidates.
        // etag304 answers 304 to a matching If-None-Match (cache hit);
        // etag200 always answers 200, so it is never cached.
        r->setHeader("etag", etag);
        if (is304 && inm == etag) { r->code = 304; r->reason = "Not Modified"; return; }
        r->body = "etag\n";
        return;
    }

    if (file.startsWith("httpcachetest_lastModified")) {
        // A Last-Modified far in the past yields a long heuristic freshness,
        // so lastModified200 is served fresh from cache with no revalidation.
        // lastModified304 forces revalidation with a past Expires and answers
        // 304 to the conditional GET.
        r->setHeader("last-modified", fixedLM);
        if (is304) {
            r->setHeader("expires", httpDate(now.addSecs(-3600)));
            if (ims == fixedLM) { r->code = 304; r->reason = "Not Modified"; return; }
        }
        r->body = "lastmodified\n";
        return;
    }

    // expires*.cgi: expires500 is fresh (future Expires -> cache hit
    // without revalidation); expires304 is stale but revalidates to 304
    // (cache hit); expires200 is stale with no 304 answer, so it is
    // always refetched fresh.
    r->setHeader("last-modified", fixedLM);
    if (is500) {
        r->setHeader("expires", httpDate(now.addSecs(500)));
    } else {
        r->setHeader("expires", httpDate(now.addSecs(-3600)));
        if (is304 && ims == fixedLM) { r->code = 304; r->reason = "Not Modified"; return; }
    }
    r->body = "expires\n";
}

/// Emits each line of the POST body ("<name>=<value>[; attrs]") as a Set-Cookie
/// header.
static inline void replySetCookie(const HttpRequest &req, HttpReply *r)
{
    r->setHeader("content-type", "text/plain");
    foreach (const QByteArray &line, req.body.split('\n')) {
        QByteArray cookie = line;
        while (cookie.endsWith('\r')) { cookie.chop(1); }
        if (!cookie.trimmed().isEmpty()) {
            r->setHeader("Set-Cookie", cookie);
        }
    }
    r->body = "ok\n";
}

/// Challenges with an authentication scheme Qt does not implement (`scheme`):
/// 407 with Proxy-Authenticate when the query starts with 407, else 401 with
/// WWW-Authenticate; the body is the `text` setting.
static inline void replyUnknownAuth(const HttpRoute &route,
                                    const HttpRequest &req, HttpReply *r)
{
    const QByteArray challenge = route.param("scheme", "Bogus") + " realm=\"\"";
    if (req.query.startsWith("407")) {
        r->code = 407;
        r->reason = "Proxy Authorization Required";
        r->setHeader("Proxy-Authenticate", challenge);
    } else {
        r->code = 401;
        r->reason = "Authorization Required";
        r->setHeader("WWW-Authenticate", challenge);
    }
    r->setHeader("content-type", "text/plain");
    r->body = route.param("text");
}

/// Answers `req` by `route`'s type; tells whether it did, since a `files`
/// route that finds no file lets the next route answer.
static inline bool answerRoute(const HttpRoute &route, const HttpRequest &req,
                               HttpReply *r)
{
    const QByteArray &type = route.type;
    if (type == "static") {
        replyStatic(route, req, r);
    } else if (type == "null-bytes") {
        replyNullBytes(route, r);
    } else if (type == "repeated-byte") {
        replyRepeatedByte(route, r);
    } else if (type == "repeated-text") {
        replyRepeatedText(route, r);
    } else if (type == "echo") {
        replyEcho(route, req, r);
    } else if (type == "basic-auth") {
        replyBasicAuth(route, req, r);
    } else if (type == "digest-auth") {
        replyDigestAuth(route, req, r);
    } else if (type == "webdav") {
        replyWebDav(req, r);
    } else if (type == "query-status") {
        replyQueryStatus(route, req, r);
    } else if (type == "cache-test") {
        replyCacheTest(route, req, r);
    } else if (type == "cookie-echo") {
        r->setHeader("content-type", "text/plain");
        r->body = reqHeaderValue(req.head, "cookie");
    } else if (type == "set-cookie") {
        replySetCookie(req, r);
    } else if (type == "unknown-auth") {
        replyUnknownAuth(route, req, r);
    } else if (type == "md5sum") {
        r->setHeader("content-type", "text/plain");
        r->body = md5Hex(req.body) + "\n";
    } else if (type == "multipart") {
        r->setHeader("content-type", "text/plain");
        r->body = multipartSummary(req.contentType, req.body);
    } else if (type == "directory-index") {
        if (!serveFile(r, req.root, route.param("file", "index.html"))) {
            r->setHeader("content-type", "text/html");
            r->body = route.param("text");
        }
    } else if (type == "gzip-files") {
        if (serveStrippedFile(route, req, r)) {
            // `serveFile()` already set the content type and last-modified.
            r->body = gzipEncode(r->body);
            r->setHeader("content-encoding", "gzip");
        } else {
            r->code = 404;
            r->reason = "Not Found";
            r->setHeader("content-type", "text/plain");
            r->body = "not found\n";
        }
    } else if (type == "files") {
        if (!serveStrippedFile(route, req, r)) {
            return false;
        }
    } else {
        return false;
    }

    // Sets the route's headers: `headers.<name>`, then, for the request's
    // method, `<method>.headers.<name>`; each replaces a header of its name.
    const QByteArray scoped = req.method.toLower() + ".headers.";
    QMap<QByteArray, QByteArray >::const_iterator it = route.params.constBegin();
    for (; it != route.params.constEnd(); ++it) {
        if (it.key().startsWith("headers.")) {
            r->replaceHeader(it.key().mid(8), it.value());
        }
    }
    for (it = route.params.constBegin(); it != route.params.constEnd(); ++it) {
        if (it.key().startsWith(scoped)) {
            r->replaceHeader(it.key().mid(scoped.size()), it.value());
        }
    }
    return true;
}

// MARK: shared responder.

/// Builds the reply to `req` from the route table: server-dummy's routes.ini,
/// with the routes of the --folder's server-config.ini in front.
static inline HttpReply buildReply(const HttpRequest &req)
{
    HttpReply r;

    // Verb handling shared by every HTTP listener
    // (plain + TLS go through here). A bare CONNECT to an origin is
    // invalid -> 400 Bad Request (QNAM: ProtocolInvalidOperationError);
    // a verb the server does not implement -> 501 Not Implemented
    // (QNAM: OperationNotImplementedError). Known methods (incl. OPTIONS/TRACE,
    // which fall through to a permissive 200) flow on to the routes below.
    // Drives
    // tst_QNetworkReply::sendCustomRequestToHttp("connect"/"nonsense").
    if (req.method == "CONNECT") {
        r.code = 400;
        r.reason = "Bad Request";
        r.setHeader("content-type", "text/plain");
        r.body = "bad request\n";
        return r;
    }
    {
        static const char *const knownVerbs[] = { "GET", "HEAD", "POST", "PUT", "DELETE", "OPTIONS", "TRACE" };
        bool verbKnown = false;
        for (unsigned i = 0; i < sizeof(knownVerbs) / sizeof(knownVerbs[0]); ++i)
            if (req.method == knownVerbs[i]) { verbKnown = true; break; }
        if (!verbKnown) {
            r.code = 501;
            r.reason = "Not Implemented";
            r.setHeader("content-type", "text/plain");
            r.body = "not implemented\n";
            return r;
        }
    }

    const QList<HttpRoute > routes = IniWatcher::shared(g_configIniPath)->load()->routes();
    for (int i = 0; i < routes.size(); ++i) {
        const HttpRoute &route = routes.at(i);
        if (route.matches(req.listener, req.path) && answerRoute(route, req, &r)) {
            return r;
        }
        r = HttpReply();
    }

    r.code = 404;
    r.reason = "Not Found";
    r.setHeader("content-type", "text/plain");
    r.body = "not found\n";
    return r;
}

/// Builds the reply of a request that a listener has split into its parts.
static inline HttpReply buildReply(const QByteArray &method, const QByteArray &path,
                                   const QByteArray &query, const QByteArray &reqContentType,
                                   const QByteArray &reqBody, const QString &root,
                                   const QByteArray &reqHead = QByteArray(),
                                   const QByteArray &listener = QByteArray("secure"))
{
    HttpRequest req;
    req.method = method;
    req.path = path;
    req.query = query;
    req.contentType = reqContentType;
    req.body = reqBody;
    req.head = reqHead;
    req.listener = listener;
    req.root = root;
    return buildReply(req);
}


#endif // HTTP_REPLY_H
