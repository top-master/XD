#ifndef HTTP_REPLY_H
#define HTTP_REPLY_H

/// @file
/// The shared HTTP/1.1 responder: HttpReply + buildReply() and their helpers, serving the
/// route set the network tests expect (echo/md5sum/multipart CGIs, cache headers, WebDAV,
/// Basic/Digest auth, files from a folder). Both the plain listener (HttpService, in
/// http_service.h) and the TLS/SPDY listener (Http1Connection, in https_spdy.h) build their
/// replies here, so the two serve identical routes.

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

#include <algorithm> // std::sort (name-sort the multipart form-data summary)
#include <cstring>   // memset
#include <zlib.h>    // gzip the /qtest/deflate/ fixtures (Content-Encoding: gzip)

/// gzip-wrap bytes with the bundled zlib (windowBits 15+16 selects the gzip container). Used to
/// serve the /qtest/deflate/ fixtures so QNAM decodes them (tst_QNetworkReply::synchronousRequest
/// http-gzip). Mirrors HttpService::gzipBody(); kept here so the shared responder is self-contained.
static inline QByteArray gzipEncode(const QByteArray &in)
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

struct HttpReply
{
    int code;
    QByteArray reason;
    /// Lower-case names, minus status/version.
    QList<QPair<QByteArray, QByteArray> > headers;
    QByteArray body;
    HttpReply() : code(200), reason("OK") {}
    void setHeader(const QByteArray &name, const QByteArray &value)
    {
        headers.append(qMakePair(name.toLower(), value));
    }
};

// MARK: shared routing.

/// Split a raw request target into decoded path and (raw) query.
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
    return QLocale::c().toString(dt.toUTC(), QLL("ddd, dd MMM yyyy HH:mm:ss")).toLatin1() + " GMT";
}

/// Compute the multipart.cgi response: one md5 line per form-data part.
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
    QList<QPair<QByteArray, QByteArray> > formFields; // collected, then emitted name-sorted
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

        const QList<QByteArray> hlines = partHead.split('\n');
        if (contentType.toLower().contains("multipart/form-data")) {
            // "key: <name>, value: ...": a text field (no type, or text/*) echoes its raw
            // value; a typed part (an uploaded file/image) is summarised by its md5.
            QByteArray name, ctype;
            for (int i = 0; i < hlines.size(); ++i) {
                const QByteArray l = hlines.at(i).trimmed();
                const QByteArray low = l.toLower();
                if (low.startsWith("content-disposition:")) {
                    const int nk = l.indexOf("name=\"");
                    if (nk >= 0) {
                        const int ns = nk + 6;
                        const int ne = l.indexOf('"', ns);
                        if (ne > ns)
                            name = l.mid(ns, ne - ns);
                    }
                } else if (low.startsWith("content-type:")) {
                    ctype = l.mid(l.indexOf(':') + 1).trimmed();
                }
            }
            const bool textual = ctype.isEmpty() || ctype.toLower().startsWith("text/");
            const QByteArray value = textual
                    ? content
                    : QCryptographicHash::hash(content, QCryptographicHash::Md5).toHex();
            formFields.append(qMakePair(name, value));
        } else {
            // Any other multipart type is dumped verbatim: each header, then the content,
            // then a blank line, in the order the parts arrived.
            for (int i = 0; i < hlines.size(); ++i) {
                const QByteArray l = hlines.at(i).trimmed();
                const int c = l.indexOf(':');
                if (c < 0)
                    continue;
                out += "header: " + l.left(c).trimmed() + ", value: '" + l.mid(c + 1).trimmed() + "'\n";
            }
            out += "content: " + content + "\n\n";
        }
        pos = next;
    }
    // Emit the form-data fields in a deterministic order by name (QPair compares by first,
    // then second), so the summary is stable regardless of the wire order the parts arrived
    // in -- the client's test fixtures assert this same sorted order.
    std::sort(formFields.begin(), formFields.end());
    for (int i = 0; i < formFields.size(); ++i)
        out += "key: " + formFields.at(i).first + ", value: " + formFields.at(i).second + "\n";
    return out;
}

/// The route table shared by both protocols. `root` is the --folder to read
/// files from; `reqContentType` is the request's Content-Type (for multipart).
/// Case-insensitive lookup of a request header value in a raw request-head block
/// (the text before the blank line). Line 0 is the request line, so skip it.
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

/// A tiny in-memory WebDAV store shared across requests. server-dummy is a single
/// process serving requests serially, so a function-local static is enough: a PUT
/// under /dav/ stores the body, a later GET reads it back, a DELETE drops it.
static inline QMap<QByteArray, QByteArray> &davStore()
{
    static QMap<QByteArray, QByteArray> store;
    return store;
}

/// Decode a chunked message body beginning at `from` in `buf`. On success sets *body
/// to the reassembled payload and *consumed to how many bytes the whole chunked body
/// (including its terminating 0-chunk) spans, and returns true; returns false when more
/// bytes are still needed. Trailers are skipped.
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

/// Serve a file that lives under `root`, named by `name` (no leading slash). Fills
/// `r` and returns true on a hit; leaves `r` untouched and returns false on a miss.
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

/// Extract the value of `key=` from an RFC 2617 Digest Authorization header (quoted or bare token),
/// matching only at a token boundary so e.g. "nc" is not found inside "cnonce".
static inline QByteArray digestField(const QByteArray &hdr, const char *key)
{
    const QByteArray needle = QByteArray(key) + "=";
    int i = 0;
    for (;;) {
        i = hdr.indexOf(needle, i);
        if (i < 0)
            return QByteArray();
        const char b = (i == 0) ? ' ' : hdr.at(i - 1);
        if (b == ' ' || b == ',' || b == '\t')
            break;
        i += needle.size();
    }
    int v = i + needle.size();
    if (v < hdr.size() && hdr.at(v) == '"') {
        const int end = hdr.indexOf('"', v + 1);
        return (end < 0) ? QByteArray() : hdr.mid(v + 1, end - v - 1);
    }
    int end = v;
    while (end < hdr.size() && hdr.at(end) != ',' && hdr.at(end) != ' ')
        ++end;
    return hdr.mid(v, end - v);
}

static inline QByteArray md5Hex(const QByteArray &in)
{ return QCryptographicHash::hash(in, QCryptographicHash::Md5).toHex(); }

static inline HttpReply buildReply(const QByteArray &method, const QByteArray &path,
                                   const QByteArray &query, const QByteArray &reqContentType,
                                   const QByteArray &reqBody, const QString &root,
                                   const QByteArray &reqHead = QByteArray())
{
    HttpReply r;

    // Verb handling shared by every HTTP listener (plain + TLS go through here). A bare CONNECT to
    // an origin is invalid -> 400 Bad Request (QNAM: ProtocolInvalidOperationError); a verb the
    // server does not implement -> 501 Not Implemented (QNAM: OperationNotImplementedError). Known
    // methods (incl. OPTIONS/TRACE, which fall through to a permissive 200) flow on to the routes
    // below. Drives tst_QNetworkReply::sendCustomRequestToHttp("connect"/"nonsense").
    if (method == "CONNECT") {
        r.code = 400; r.reason = "Bad Request";
        r.setHeader("content-type", "text/plain");
        r.body = "bad request\n";
        return r;
    }
    {
        static const char *const knownVerbs[] = { "GET", "HEAD", "POST", "PUT", "DELETE", "OPTIONS", "TRACE" };
        bool verbKnown = false;
        for (unsigned i = 0; i < sizeof(knownVerbs) / sizeof(knownVerbs[0]); ++i)
            if (method == knownVerbs[i]) { verbKnown = true; break; }
        if (!verbKnown) {
            r.code = 501; r.reason = "Not Implemented";
            r.setHeader("content-type", "text/plain");
            r.body = "not implemented\n";
            return r;
        }
    }

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

    // Root document: "/" and the explicit "/index.html" both serve the landing page. Several SSL
    // rows (ignoreSslErrorsList, sslConfiguration) GET https://<host>/index.html and expect 200.
    if (path == "/" || path == "/index.html") {
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

    // Basic auth under /qtest/rfcs-auth[/<file>]: credentials httptest:httptest. The bare
    // directory (no trailing slash) is used by tst_QNetworkReply::getErrors[http-authentication],
    // which expects the 401 challenge, so match it too.
    if (path == "/qtest/rfcs-auth" || path.startsWith("/qtest/rfcs-auth/")) {
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

    // Digest auth under /qtest/auth-digest/: user httptest / password httptest, realm QAuthenticator.
    // We fully VALIDATE the RFC 2617 response hash (not just the username) so a wrong password/user is
    // rejected with a fresh 401 -- tst_QNetworkReply::ioGetFromHttpWithAuth(digest-bad-*-in-url) relies
    // on the bad URL creds being challenged (then its authenticator supplies the correct ones).
    if (path.startsWith("/qtest/auth-digest")) {
        const QByteArray got = reqHeaderValue(reqHead, "authorization");
        bool ok = got.startsWith("Digest ");
        if (ok) {
            const QByteArray user  = digestField(got, "username");
            const QByteArray nonce = digestField(got, "nonce");
            const QByteArray uri   = digestField(got, "uri");
            const QByteArray qop   = digestField(got, "qop");
            const QByteArray nc    = digestField(got, "nc");
            const QByteArray cnonce= digestField(got, "cnonce");
            const QByteArray resp  = digestField(got, "response");
            const QByteArray ha1 = md5Hex(user + ":QAuthenticator:httptest");
            const QByteArray ha2 = md5Hex(method + ":" + uri);
            const QByteArray expected = qop.isEmpty()
                ? md5Hex(ha1 + ":" + nonce + ":" + ha2)
                : md5Hex(ha1 + ":" + nonce + ":" + nc + ":" + cnonce + ":" + qop + ":" + ha2);
            ok = (user == "httptest") && !resp.isEmpty() && (resp == expected);
        }
        if (!ok) {
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
        const QDateTime now = QDateTime::currentDateTimeUtc();
        r.setHeader("date", httpDate(now));
        r.setHeader("content-type", "text/html");
        const bool is500 = file.contains("500"); // fresh (future expiry)
        const bool is304 = file.contains("304"); // stale, but revalidation answers 304

        // cachecontrol[-expire][200].cgi?<directives>: echo the query back as Cache-Control.
        if (file.startsWith("httpcachetest_cachecontrol")) {
            if (file.startsWith("httpcachetest_cachecontrol-expire")) {
                // Already expired, with no validator: never served from a cache.
                r.setHeader("expires", httpDate(now.addSecs(-3600)));
            } else {
                const QByteArray cc = QByteArray::fromPercentEncoding(QByteArray(query).replace('+', ' '));
                if (!cc.isEmpty())
                    r.setHeader("cache-control", cc);
                r.setHeader("last-modified", fixedLM);
                // no-cache means the stored copy must never be used as-is: answer a fresh 200
                // even to a conditional GET so nothing is served from the cache.
                if (!cc.contains("no-cache") && !ims.isEmpty() && ims == fixedLM) {
                    r.code = 304; r.reason = "Not Modified"; return r;
                }
            }
            r.body = "cachecontrol\n";
            return r;
        }

        if (file.startsWith("httpcachetest_etag")) {
            // No freshness lifetime, so every use revalidates. etag304 answers 304 to a matching
            // If-None-Match (cache hit); etag200 always answers 200, so it is never cached.
            r.setHeader("etag", etag);
            if (is304 && inm == etag) { r.code = 304; r.reason = "Not Modified"; return r; }
            r.body = "etag\n";
            return r;
        }

        if (file.startsWith("httpcachetest_lastModified")) {
            // A Last-Modified far in the past yields a long heuristic freshness, so
            // lastModified200 is served fresh from cache with no revalidation. lastModified304
            // forces revalidation with a past Expires and answers 304 to the conditional GET.
            r.setHeader("last-modified", fixedLM);
            if (is304) {
                r.setHeader("expires", httpDate(now.addSecs(-3600)));
                if (ims == fixedLM) { r.code = 304; r.reason = "Not Modified"; return r; }
            }
            r.body = "lastmodified\n";
            return r;
        }

        // expires*.cgi: expires500 is fresh (future Expires -> cache hit without revalidation);
        // expires304 is stale but revalidates to 304 (cache hit); expires200 is stale with no
        // 304 answer, so it is always refetched fresh.
        r.setHeader("last-modified", fixedLM);
        if (is500) {
            r.setHeader("expires", httpDate(now.addSecs(500)));
        } else {
            r.setHeader("expires", httpDate(now.addSecs(-3600)));
            if (is304 && ims == fixedLM) { r.code = 304; r.reason = "Not Modified"; return r; }
        }
        r.body = "expires\n";
        return r;
    }

    if (path == "/qtest/cgi-bin/echo.cgi") {
        r.setHeader("content-type", "text/plain");
        // POST/PUT echo the request BODY back (putWithRateLimiting round-trips its whole upload);
        // a GET carries no body and the content-negotiation rows fetch "?1" expecting the body "1",
        // so those echo the query string instead.
        r.body = (method == "POST" || method == "PUT") ? reqBody : query;
        return r;
    }
    // get-cookie.cgi: echo the request's Cookie header value back as the body, so
    // tst_QNetworkReply::sendCookies can verify which cookies the jar sent.
    if (path == "/qtest/cgi-bin/get-cookie.cgi") {
        r.setHeader("content-type", "text/plain");
        r.body = reqHeaderValue(reqHead, "cookie");
        return r;
    }
    // set-cookie.cgi: the POST body carries cookie definition(s), one per line
    // ("<name>=<value>[; attrs]"). Emit each as a Set-Cookie response header, so
    // tst_QNetworkReply::receiveCookiesFromHttp sees them in SetCookieHeader / the jar.
    if (path == "/qtest/cgi-bin/set-cookie.cgi") {
        r.setHeader("content-type", "text/plain");
        foreach (const QByteArray &line, reqBody.split('\n')) {
            QByteArray cookie = line;
            while (cookie.endsWith('\r')) cookie.chop(1);
            if (!cookie.trimmed().isEmpty())
                r.setHeader("Set-Cookie", cookie);
        }
        r.body = "ok\n";
        return r;
    }
    // fluke.gif: a fixed-date image fixture. tst_QNetworkReply::lastModifiedHeaderForHttp HEADs it
    // and asserts Last-Modified == "Tue, 22 May 2007 12:04:57 GMT" (the historical webserver value).
    if (path == "/qtest/fluke.gif") {
        r.setHeader("content-type", "image/gif");
        r.setHeader("Last-Modified", "Tue, 22 May 2007 12:04:57 GMT");
        // Historical size 27906: lastModifiedHeaderForHttp only HEADs it (body ignored), but
        // httpRecursiveCreation GETs it and asserts bytesAvailable()==27906, so pad to that length.
        QByteArray gif("GIF89a\x01\x00\x01\x00\x00\x00\x00;", 14); // 1x1 stub prefix
        gif += QByteArray(27906 - gif.size(), '\0');
        r.body = gif;
        return r;
    }
    // http-unknown-authentication-method.cgi?<code>-...: challenge with a scheme Qt does not
    // implement (Bogus), so tst_QNetworkReply::authorizationError sees the request fail with the auth
    // kept unsatisfied. The query selects 401 (WWW-Authenticate -> AuthenticationRequiredError) vs
    // 407 (Proxy-Authenticate -> ProxyAuthenticationRequiredError); the body is exactly
    // "authorization required" (no trailing newline -- the test QCOMPAREs reply->size()).
    if (path == "/qtest/cgi-bin/http-unknown-authentication-method.cgi") {
        if (query.startsWith("407")) {
            r.code = 407; r.reason = "Proxy Authorization Required";
            r.setHeader("Proxy-Authenticate", "Bogus realm=\"\"");
        } else {
            r.code = 401; r.reason = "Authorization Required";
            r.setHeader("WWW-Authenticate", "Bogus realm=\"\"");
        }
        r.setHeader("content-type", "text/plain");
        r.body = "authorization required";
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

    // /qtest/deflate/<file>: the underlying fixture file gzip-encoded (Content-Encoding: gzip),
    // so QNAM transparently inflates it (tst_QNetworkReply::synchronousRequest http-gzip).
    if (path.startsWith("/qtest/deflate/")) {
        if (serveFile(&r, root, path.mid(sizeof("/qtest/deflate/") - 1))) {
            r.body = gzipEncode(r.body); // serveFile already set content-type + last-modified
            r.setHeader("content-encoding", "gzip");
            return r;
        }
        r.code = 404; r.reason = "Not Found"; r.setHeader("content-type", "text/plain");
        r.body = "not found\n";
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


#endif // HTTP_REPLY_H
