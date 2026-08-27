/****************************************************************************
**
** Copyright (C) 2015 The XD Company Ltd.
**
** This file is part of the test suite of the XD Toolkit.
**
** $QT_BEGIN_LICENSE:APACHE2$
**
** Licensed under the Apache License, Version 2.0 (the "License");
** you may not use this file except in compliance with the License.
** You may obtain a copy of the License at
**
**     http://www.apache.org/licenses/LICENSE-2.0
**
** Unless required by applicable law or agreed to in writing, software
** distributed under the License is distributed on an "AS IS" BASIS,
** WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
** See the License for the specific language governing permissions and
** limitations under the License.
**
** $QT_END_LICENSE$
**
****************************************************************************/

// The bundled test server, run as its own process. A test does not link this; it
// launches it through TestServer (helpers/testserver.h), which is handed this binary's
// absolute path by TestEnv and runs it as a QProcess. It implements each "way" the
// network tests need, selected by the first argument, as a set of protocol services:
//
//   ftp  -> FtpService  (control/PASV over a small writable in-memory FS)
//           EchoService (a TCP + UDP echo, on the secondary port)
//   web  -> HttpService (HTTP/1.1 responder, and an HTTP CONNECT proxy)
//           SocksService(SOCKS5 CONNECT proxy, on the secondary port)
//   dns  -> DnsService  (DNS A-record responder -> 127.0.0.1)
//
// Each service subclasses TestService, which owns its host and wanted port, binds with an
// ephemeral fallback (so a privileged or taken port degrades gracefully), and remembers
// the port it actually got. The primary and secondary ports are passed as argv[2]/argv[3]
// (TestServer has already checked they are bindable) and echoed back in the READY line, so
// the handle -- and thus the test, via TestServer::port()/echoPort() -- learns where the
// listeners really are. Connections use the async newConnection/nextPendingConnection pair,
// never the synchronous QTcpServer::waitForNewConnection, which does not accept reliably
// under Fil-C. The whole thing is header-only in one file (lambda-wired, no moc).

#include <cstdio>

#include <QtCore/QByteArray>
#include <QtCore/QCoreApplication>
#include <QtCore/QDir>
#include <QtCore/QFile>
#include <QtCore/QFileInfo>
#include <QtCore/QHash>
#include <QtCore/QtEndian>
#include <QtNetwork/QHostAddress>
#include <QtNetwork/QTcpServer>
#include <QtNetwork/QTcpSocket>
#include <QtNetwork/QUdpSocket>

#include "../testenv.h"
#include "../test-ports.h"
#include "https_spdy.h"

// TestService, the base for every protocol listener, now lives in service_base.h
// so https_spdy.h can share it (included above, via https_spdy.h).

// ---------------------------------------------------------------------------
// FtpService: a small FTP server over a writable in-memory filesystem.
// ---------------------------------------------------------------------------

class FtpService : public TestService
{
public:
    // When qtestAtRoot is true a leading "/qtest" in an FTP path maps onto m_root
    // itself, so a flat --folder (no qtest/ subdir) still answers the ftp://.../qtest/...
    // URLs the tests use. A server given a fixture that already has a real qtest/ dir
    // (e.g. the qftp fixture) leaves it false and serves that directory verbatim.
    FtpService(const QHostAddress &host, quint16 port, const QString &folder,
               QObject *parent = Q_NULLPTR, bool qtestAtRoot = false)
        : TestService(host, port, parent), m_root(folder), m_qtestAtRoot(qtestAtRoot) {}

    void start() Q_DECL_OVERRIDE
    {
        QTcpServer *server = new QTcpServer(this);
        listenTcp(server);
        connect(server, &QTcpServer::newConnection, server, [this, server]() {
            QTcpSocket *c = server->nextPendingConnection();
            FtpState *st = new FtpState(c);
            connect(c, &QTcpSocket::readyRead, c, [this, c, st]() { onFtp(c, st); });
            connect(c, &QTcpSocket::disconnected, c, &QObject::deleteLater);
            reply(c, 220, "server-dummy ready");
        });
    }

private:
    // Per-control-connection state, kept alive as a child of the control socket.
    struct FtpState : QObject {
        explicit FtpState(QObject *p)
            : QObject(p), pasv(Q_NULLPTR), data(Q_NULLPTR), pending(false),
              loggedIn(false), rest(0), active(false), portPort(0), ascii(false),
              cwd(QStringLiteral("/")) {}
        QTcpServer *pasv;    // the current PASV listener
        QTcpSocket *data;    // the data connection, once established
        QByteArray payload;  // bytes queued for the next download
        bool pending;        // a download is queued
        QString storName;    // real path of an upload target, or the rename-from path
        bool loggedIn;
        QString user;
        qint64 rest;
        bool active;         // PORT (active) mode selected for the next transfer
        QHostAddress portAddr;
        quint16 portPort;
        bool ascii;          // TYPE A (CRLF text mode)
        QString cwd;         // FTP-space current directory
    };

    QString m_root; // filesystem directory this FTP server exposes (from --folder)
    bool m_qtestAtRoot; // treat a leading "/qtest" as m_root (flat-folder fleet use)

    // Map an FTP path (absolute, /var/ftp-aliased, or relative to cwd) into FTP space.
    static QString ftpNorm(const QString &cwd, QString p)
    {
        p = p.trimmed();
        if (p == QLatin1String("/var/ftp") || p.startsWith(QLatin1String("/var/ftp/")))
            p = p.mid(8);
        if (p.isEmpty())
            p = cwd;
        QString ftp = p.startsWith(QLatin1Char('/')) ? p : (cwd + QLatin1Char('/') + p);
        ftp = QDir::cleanPath(ftp);
        if (!ftp.startsWith(QLatin1Char('/')))
            ftp = QLatin1Char('/') + ftp;
        return ftp;
    }
    QString realPath(const QString &cwd, const QString &p) const
    {
        QString ftp = ftpNorm(cwd, p);
        if (m_qtestAtRoot) {
            if (ftp == QLatin1String("/qtest"))
                ftp = QStringLiteral("/");
            else if (ftp.startsWith(QLatin1String("/qtest/")))
                ftp = ftp.mid(6); // drop "/qtest", keep the leading slash
        }
        return QDir::cleanPath(m_root + ftp);
    }
    // Only /qtest/upload (and below) is writable; writes elsewhere must fail, as the
    // tests expect (mkdir/put at the root is rejected).
    static bool writable(const QString &cwd, const QString &p)
    {
        return ftpNorm(cwd, p).startsWith(QLatin1String("/qtest/upload"));
    }

    void onFtp(QTcpSocket *c, FtpState *st)
    {
        while (c->canReadLine()) {
            const QByteArray trimmed = c->readLine().trimmed();
            const int sp = trimmed.indexOf(' ');
            const QByteArray verb = (sp < 0 ? trimmed : trimmed.left(sp)).toUpper();
            const QByteArray rawArg = sp < 0 ? QByteArray() : trimmed.mid(sp + 1);
            const QString arg = QString::fromLatin1(rawArg);

            if (verb == "USER") {
                st->user = arg;
                reply(c, 331, "need password");
            } else if (verb == "PASS") {
                // Anonymous and a couple of fixed accounts log in; login "foo" is rejected.
                if (st->user == QLatin1String("foo")) reply(c, 530, "login incorrect");
                else { st->loggedIn = true; reply(c, 230, "logged in"); }
            } else if (!st->loggedIn && verb != "QUIT" && verb != "NOOP") {
                reply(c, 530, "please login with USER and PASS");
            } else if (verb == "SYST") {
                reply(c, 215, "UNIX Type: L8");
            } else if (verb == "PWD" || verb == "XPWD") {
                reply(c, 257, ('"' + st->cwd.toLatin1() + "\" is current directory").constData());
            } else if (verb == "TYPE") {
                st->ascii = arg.toUpper().startsWith(QLatin1Char('A'));
                reply(c, 200, "ok");
            } else if (verb == "MODE" || verb == "STRU" || verb == "NOOP") {
                reply(c, 200, "ok");
            } else if (verb == "CWD") {
                if (QFileInfo(realPath(st->cwd, arg)).isDir()) {
                    st->cwd = ftpNorm(st->cwd, arg);
                    reply(c, 250, "ok");
                } else {
                    reply(c, 550, "no such directory");
                }
            } else if (verb == "CDUP") {
                st->cwd = ftpNorm(st->cwd, QStringLiteral(".."));
                reply(c, 200, "ok");
            } else if (verb == "REST") {
                st->rest = arg.toLongLong();
                reply(c, 350, "restarting");
            } else if (verb == "SIZE") {
                const QFileInfo fi(realPath(st->cwd, arg));
                if (fi.isFile()) reply(c, 213, QByteArray::number(fi.size()).constData());
                else reply(c, 550, "no such file");
            } else if (verb == "MKD" || verb == "XMKD") {
                const QString rp = realPath(st->cwd, arg);
                if (!writable(st->cwd, arg)) reply(c, 550, "permission denied");
                else if (QFileInfo::exists(rp)) reply(c, 550, "already exists");
                else if (QDir().mkpath(rp)) reply(c, 257, ('"' + arg.toLatin1() + "\" created").constData());
                else reply(c, 550, "cannot create");
            } else if (verb == "RMD" || verb == "XRMD") {
                if (writable(st->cwd, arg) && QDir().rmdir(realPath(st->cwd, arg))) reply(c, 250, "ok");
                else reply(c, 550, "cannot remove");
            } else if (verb == "DELE") {
                if (writable(st->cwd, arg) && QFile::remove(realPath(st->cwd, arg))) reply(c, 250, "ok");
                else reply(c, 550, "no such file");
            } else if (verb == "RNFR") {
                if (QFileInfo::exists(realPath(st->cwd, arg))) {
                    st->storName = realPath(st->cwd, arg);
                    reply(c, 350, "ready for RNTO");
                } else reply(c, 550, "no such file");
            } else if (verb == "RNTO") {
                if (!st->storName.isEmpty() && writable(st->cwd, arg)
                    && QFile::rename(st->storName, realPath(st->cwd, arg)))
                    reply(c, 250, "renamed");
                else reply(c, 550, "rename failed");
                st->storName.clear();
            } else if (verb == "PASV") {
                openPasv(c, st);
            } else if (verb == "PORT") {
                openPort(c, st, rawArg);
            } else if (verb == "LIST" || verb == "NLST") {
                startDownload(c, st, listing(st, arg, verb == "NLST"));
            } else if (verb == "RETR") {
                retr(c, st, arg);
            } else if (verb == "STOR" || verb == "APPE") {
                beginUpload(c, st, arg, verb == "APPE");
            } else if (verb == "QUIT") {
                reply(c, 221, "bye");
                c->disconnectFromHost();
            } else if (verb == "HELP") {
                reply(c, 214, "no help available");
            } else if (verb == "FEAT") {
                reply(c, 211, "no features");
            } else if (verb == "OPTS") {
                reply(c, 200, "ok");
            } else if (verb == "MDTM") {
                const QFileInfo fi(realPath(st->cwd, arg));
                if (fi.isFile())
                    reply(c, 213, fi.lastModified().toUTC().toString(QStringLiteral("yyyyMMddhhmmss")).toLatin1().constData());
                else
                    reply(c, 550, "no such file");
            } else {
                std::fprintf(stderr, "server-dummy ftp: unimplemented verb '%s'\n", verb.constData());
                std::fflush(stderr);
                reply(c, 502, "not implemented");
            }
        }
    }

    // --- data channel: PASV (we listen) or PORT (we connect back to the client) ---

    void resetData(FtpState *st)
    {
        if (st->pasv) { st->pasv->deleteLater(); st->pasv = Q_NULLPTR; }
        st->data = Q_NULLPTR;
        st->pending = false;
        st->storName.clear();
        st->active = false;
    }

    void openPasv(QTcpSocket *c, FtpState *st)
    {
        resetData(st);
        st->pasv = new QTcpServer(st);
        st->pasv->listen(m_host, 0);
        connect(st->pasv, &QTcpServer::newConnection, st->pasv, [this, c, st]() {
            st->data = st->pasv->nextPendingConnection();
            st->data->setParent(st);
            onDataReady(c, st);
        });
        const quint16 p = st->pasv->serverPort();
        reply(c, 227, QByteArray("Entering Passive Mode (127,0,0,1," +
                                 QByteArray::number(p >> 8) + ',' +
                                 QByteArray::number(p & 0xff) + ")").constData());
    }

    // "PORT h1,h2,h3,h4,p1,p2": remember where to connect for the next transfer.
    void openPort(QTcpSocket *c, FtpState *st, const QByteArray &arg)
    {
        const QList<QByteArray> n = arg.split(',');
        if (n.size() != 6) { reply(c, 501, "bad PORT"); return; }
        resetData(st);
        st->active = true;
        st->portAddr = QHostAddress(QString::fromLatin1(n[0].trimmed() + '.' + n[1].trimmed()
                                    + '.' + n[2].trimmed() + '.' + n[3].trimmed()));
        st->portPort = quint16((n[4].trimmed().toUInt() << 8) | n[5].trimmed().toUInt());
        reply(c, 200, "PORT ok");
    }

    // Called once a data connection exists: run whichever transfer was queued.
    void onDataReady(QTcpSocket *c, FtpState *st)
    {
        if (st->pending) sendData(c, st);
        else if (!st->storName.isEmpty()) receiveUpload(c, st);
    }

    // Establish the data connection: connect out in PORT mode, use/await it in PASV mode.
    void ensureData(QTcpSocket *c, FtpState *st)
    {
        if (st->active) {
            QTcpSocket *d = new QTcpSocket(st);
            st->data = d;
            connect(d, &QTcpSocket::connected, c, [this, c, st]() { onDataReady(c, st); });
            d->connectToHost(st->portAddr, st->portPort);
        } else if (st->data) {
            onDataReady(c, st);
        }
        // else PASV, still waiting: openPasv's newConnection handler calls onDataReady.
    }

    // --- listing: a real directory in the ls -l form QFtp's parser understands ---

    QByteArray listing(FtpState *st, const QString &arg, bool namesOnly)
    {
        const QString target = realPath(st->cwd, arg);
        QByteArray out;
        const QFileInfo fi(target);
        if (fi.isFile()) { out += fileLine(fi, namesOnly); return out; } // LIST <file>
        const QDir d(target);
        foreach (const QFileInfo &e, d.entryInfoList(QDir::AllEntries | QDir::NoDotAndDotDot, QDir::Name))
            out += fileLine(e, namesOnly);
        return out;
    }
    static QByteArray fileLine(const QFileInfo &fi, bool namesOnly)
    {
        if (namesOnly)
            return fi.fileName().toLatin1() + "\r\n";
        const QByteArray perm = fi.isDir() ? "drwxr-xr-x" : "-rw-r--r--";
        return perm + "    1 ftp      ftp      "
               + QByteArray::number(fi.size()).rightJustified(12) + " Jan 01 00:00 "
               + fi.fileName().toLatin1() + "\r\n";
    }

    // --- RETR / STOR against real files ---

    void retr(QTcpSocket *c, FtpState *st, const QString &name)
    {
        QFile f(realPath(st->cwd, name));
        if (!f.open(QIODevice::ReadOnly)) { reply(c, 550, "no such file"); return; }
        QByteArray payload = f.readAll();
        f.close();
        if (st->rest > 0 && st->rest <= payload.size()) { payload = payload.mid(int(st->rest)); st->rest = 0; }
        startDownload(c, st, payload);
    }

    void startDownload(QTcpSocket *c, FtpState *st, const QByteArray &payload)
    {
        st->payload = payload;
        st->pending = true;
        ensureData(c, st);
    }

    void sendData(QTcpSocket *c, FtpState *st)
    {
        reply(c, 150, "opening data connection");
        st->data->write(st->payload);
        st->data->flush();
        st->data->disconnectFromHost();
        reply(c, 226, "transfer complete");
        resetData(st);
    }

    void beginUpload(QTcpSocket *c, FtpState *st, const QString &name, bool append)
    {
        if (!writable(st->cwd, name)) { reply(c, 550, "permission denied"); return; }
        st->storName = realPath(st->cwd, name);
        if (!append)
            QFile::remove(st->storName);
        st->pending = false;
        ensureData(c, st);
    }

    void receiveUpload(QTcpSocket *c, FtpState *st)
    {
        reply(c, 150, "opening data connection");
        const bool ascii = st->ascii;
        QTcpSocket *data = st->data;
        QFile *f = new QFile(st->storName, data);
        f->open(QIODevice::Append);
        connect(data, &QTcpSocket::readyRead, data, [f, data, ascii]() {
            QByteArray d = data->readAll();
            if (ascii) d.replace("\r\n", "\n"); // ASCII store: CRLF -> LF
            f->write(d);
        });
        connect(data, &QTcpSocket::disconnected, c, [this, c, st, f, data, ascii]() {
            QByteArray d = data->readAll();
            if (ascii) d.replace("\r\n", "\n");
            f->write(d);
            f->close();
            reply(c, 226, "transfer complete");
            resetData(st);
        });
    }
};

// ---------------------------------------------------------------------------
// EchoService: a plain TCP + UDP echo on one port (the FileTransfer secondary).
// ---------------------------------------------------------------------------

class EchoService : public TestService
{
public:
    EchoService(const QHostAddress &host, quint16 port, QObject *parent = Q_NULLPTR)
        : TestService(host, port, parent) {}

    void start() Q_DECL_OVERRIDE
    {
        QTcpServer *tcp = new QTcpServer(this);
        listenTcp(tcp); // records m_port
        connect(tcp, &QTcpServer::newConnection, tcp, [tcp]() {
            QTcpSocket *s = tcp->nextPendingConnection();
            connect(s, &QTcpSocket::readyRead, s, [s]() { s->write(s->readAll()); });
            connect(s, &QTcpSocket::disconnected, s, &QObject::deleteLater);
        });
        QUdpSocket *udp = new QUdpSocket(this);
        udp->bind(m_host, m_port); // match the TCP echo's port
        connect(udp, &QUdpSocket::readyRead, udp, [udp]() {
            while (udp->hasPendingDatagrams()) {
                QByteArray data(int(udp->pendingDatagramSize()), Qt::Uninitialized);
                QHostAddress from; quint16 fromPort;
                udp->readDatagram(data.data(), data.size(), &from, &fromPort);
                udp->writeDatagram(data, from, fromPort);
            }
        });
    }
};

// ---------------------------------------------------------------------------
// HttpService: an HTTP/1.1 responder that is also an HTTP CONNECT proxy.
// ---------------------------------------------------------------------------

class HttpService : public TestService
{
public:
    HttpService(const QHostAddress &host, quint16 port, QObject *parent = Q_NULLPTR)
        : TestService(host, port, parent) {}

    void start() Q_DECL_OVERRIDE
    {
        QTcpServer *server = new QTcpServer(this);
        listenTcp(server);
        connect(server, &QTcpServer::newConnection, server, [this, server]() {
            QTcpSocket *s = server->nextPendingConnection();
            connect(s, &QTcpSocket::readyRead, s, [this, s]() { onRequest(s); });
            connect(s, &QTcpSocket::disconnected, s, &QObject::deleteLater);
        });
    }

private:
    // Keep-alive request pump: buffer bytes, and while a full request (head + any
    // Content-Length body) is present, dispatch it and keep the connection open. This is
    // what lets the pipelining/keep-alive tests reuse one socket for many requests.
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
            const int clen = headerValue(head, "content-length").toInt();
            const int total = headEnd + 4 + clen;
            if (buf.size() < total)
                break; // wait for the body
            const QByteArray body = buf.mid(headEnd + 4, clen);
            buf = buf.mid(total);
            const QList<QByteArray> reqLine = head.left(head.indexOf('\n')).simplified().split(' ');
            if (reqLine.size() < 2) { // malformed request line -> 400
                send(s, 400, "Bad Request", "bad request\n");
                continue;
            }
            const QByteArray method = reqLine.at(0).toUpper();
            const QByteArray path = reqLine.at(1);
            if (method == "CONNECT") {
                s->setProperty("buf", QByteArray());
                startConnectTunnel(s, path);
                return;
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

    // Write a response, honouring keep-alive unless the request asked to close.
    void send(QTcpSocket *s, int code, const char *reason, const QByteArray &body,
              const QByteArray &extraHeaders = QByteArray())
    {
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

        if (p == "/qtest/cgi-bin/echo.cgi") {
            send(s, 200, "OK", body);                       // echo the request body
        } else if (p == "/cgi-bin/echo.cgi") {
            send(s, 200, "OK", QByteArray());               // empty body (pipelining test)
        } else if (p == "/qtest/rfc3252.txt") {
            send(s, 200, "OK", QByteArray(25962, 'x'));      // exact size 25962
        } else if (p == "/qtest/bigfile") {
            send(s, 200, "OK", QByteArray(512 * 1024, 'x')); // large enough to stream
        } else if (p.startsWith("/qtest/rfcs-auth/")) {
            requireBasic(s, head, "httptest:httptest", "httptest login");
        } else if (p.startsWith("/qtest/auth-digest/")) {
            requireDigest(s, head);
        } else if (p.startsWith("/dav/")) {
            send(s, (method == "PUT") ? 201 : 200, "Created", QByteArray());
        } else if (p.startsWith("/dav2/")) {
            send(s, 405, "Method Not Allowed", QByteArray()); // read-only WebDAV area
        } else if (p == "/t") {
            send(s, 404, "Not Found", QByteArray());
        } else {
            send(s, 200, "OK", "hello from server-dummy\n");
        }
    }

    // HTTP Basic auth against "user:pass"; 401 with a WWW-Authenticate challenge otherwise.
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

    // Minimal HTTP Digest: challenge with a nonce, then accept any response whose
    // Authorization header carries username="httptest" (enough to tell the tests' valid
    // account from the wrong one without doing the full MD5 dance).
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
        connect(upstream, &QTcpSocket::connected, client, [client, upstream]() {
            upstream->setProperty("up", true);
            client->write("HTTP/1.1 200 Connection established\r\n\r\n");
        });
        connect(upstream, static_cast<void (QTcpSocket::*)(QAbstractSocket::SocketError)>(&QTcpSocket::error),
                client, [client, upstream]() {
            // Only a failure to establish the tunnel is a proxy error; a normal close once
            // relaying has begun must not inject a 503 into the tunnelled stream.
            if (!upstream->property("up").toBool()) {
                client->write("HTTP/1.1 503 Service Unavailable\r\n\r\n");
                client->disconnectFromHost();
            }
        });
        connect(upstream, &QTcpSocket::readyRead, client, [client, upstream]() { client->write(upstream->readAll()); });
        connect(client, &QTcpSocket::readyRead, upstream, [client, upstream]() { upstream->write(client->readAll()); });
        connect(upstream, &QTcpSocket::disconnected, client, [client, upstream]() {
            client->write(upstream->readAll()); // relay any final bytes, then close so the client sees EOF
            client->disconnectFromHost();
        });
        upstream->connectToHost(host, port);
    }
};

// ---------------------------------------------------------------------------
// SocksService: a minimal SOCKS5 CONNECT proxy (no-auth and user/pass).
// ---------------------------------------------------------------------------

class SocksService : public TestService
{
public:
    SocksService(const QHostAddress &host, quint16 port, QObject *parent = Q_NULLPTR)
        : TestService(host, port, parent) {}

    void start() Q_DECL_OVERRIDE
    {
        QTcpServer *server = new QTcpServer(this);
        listenTcp(server);
        connect(server, &QTcpServer::newConnection, server, [this, server]() {
            QTcpSocket *client = server->nextPendingConnection();
            connect(client, &QTcpSocket::readyRead, client, [this, client]() { onSocks(client); });
            connect(client, &QTcpSocket::disconnected, client, &QObject::deleteLater);
        });
    }

private:
    void onSocks(QTcpSocket *client)
    {
        // Stage 0: greeting "05 nmethods methods..." -> pick no-auth (00) if offered,
        // else user/pass (02); reply "05 <method>".
        if (!client->property("greeted").toBool()) {
            if (client->bytesAvailable() < 2)
                return;
            QByteArray g = client->read(2);
            if (quint8(g.at(0)) != 0x05)
                return;
            const QByteArray methods = client->read(quint8(g.at(1)));
            char method = char(0x00);
            if (!methods.contains(char(0x00)) && methods.contains(char(0x02)))
                method = char(0x02);
            client->write(QByteArray(1, char(0x05)) + QByteArray(1, method));
            client->setProperty("greeted", true);
            client->setProperty("needauth", method == char(0x02));
            return;
        }
        // Stage 0.5: username/password auth "01 ulen user plen pass" -> "01 00" (ok).
        if (client->property("needauth").toBool() && !client->property("authed").toBool()) {
            if (client->bytesAvailable() < 2)
                return;
            QByteArray v = client->read(1);
            const quint8 ulen = quint8(client->read(1).at(0));
            client->read(ulen);
            if (client->bytesAvailable() < 1)
                return;
            const quint8 plen = quint8(client->read(1).at(0));
            client->read(plen);
            Q_UNUSED(v);
            client->write(QByteArray::fromRawData("\x01\x00", 2)); // accept any credentials
            client->setProperty("authed", true);
            return;
        }
        // Stage 1: request "05 CMD 00 ATYP addr port" -> connect upstream and relay.
        if (client->property("target").isNull()) {
            if (client->bytesAvailable() < 4)
                return;
            QByteArray hdr = client->read(4);
            const quint8 atyp = quint8(hdr.at(3));
            QString hostStr;
            if (atyp == 0x01) {
                QByteArray a = client->read(4);
                hostStr = QHostAddress(qFromBigEndian<quint32>((const uchar *)a.constData())).toString();
            } else if (atyp == 0x03) {
                quint8 len = quint8(client->read(1).at(0));
                hostStr = QString::fromLatin1(client->read(len));
            }
            const quint16 dport = qFromBigEndian<quint16>((const uchar *)client->read(2).constData());
            QTcpSocket *upstream = new QTcpSocket(client);
            client->setProperty("target", true);
            connect(upstream, &QTcpSocket::connected, client, [client, upstream]() {
                upstream->setProperty("up", true);
                client->write(QByteArray::fromRawData("\x05\x00\x00\x01\x00\x00\x00\x00\x00\x00", 10));
            });
            connect(upstream, static_cast<void (QTcpSocket::*)(QAbstractSocket::SocketError)>(&QTcpSocket::error),
                    client, [client, upstream]() {
                // Only a failure to establish the tunnel is a SOCKS error; a normal close
                // once relaying has begun must NOT inject an error reply into the stream.
                if (!upstream->property("up").toBool())
                    client->write(QByteArray::fromRawData("\x05\x05\x00\x01\x00\x00\x00\x00\x00\x00", 10));
            });
            connect(upstream, &QTcpSocket::readyRead, client, [client, upstream]() { client->write(upstream->readAll()); });
            connect(client, &QTcpSocket::readyRead, upstream, [client, upstream]() { upstream->write(client->readAll()); });
            connect(upstream, &QTcpSocket::disconnected, client, [client, upstream]() {
                client->write(upstream->readAll()); // relay any final bytes, then close so the client sees EOF
                client->disconnectFromHost();
            });
            upstream->connectToHost(hostStr, dport);
        }
    }
};

// ---------------------------------------------------------------------------
// DnsService: a DNS responder for tst_QDnsLookup's zone. It answers the exact records the
// test expects (A/AAAA/MX/NS/SRV/TXT), so the test can run offline by pointing QDnsLookup at
// this server with setNameserver(host, port). Any name whose base label is not in the zone
// answers NXDOMAIN. Only the base label (and, for SRV, the label after _service._proto)
// selects the record; the trailing zone labels of the query are echoed onto relative targets,
// so the same table serves both the plain and the IDN zone the test uses.
// ---------------------------------------------------------------------------

class DnsService : public TestService
{
public:
    DnsService(const QHostAddress &host, quint16 port, QObject *parent = Q_NULLPTR)
        : TestService(host, port, parent) {}

    void start() Q_DECL_OVERRIDE
    {
        QUdpSocket *dns = new QUdpSocket(this);
        bindUdp(dns);
        connect(dns, &QUdpSocket::readyRead, dns, [this, dns]() {
            while (dns->hasPendingDatagrams()) {
                QByteArray query(int(dns->pendingDatagramSize()), Qt::Uninitialized);
                QHostAddress from; quint16 fromPort;
                dns->readDatagram(query.data(), query.size(), &from, &fromPort);
                const QByteArray reply = buildReply(query);
                if (!reply.isEmpty())
                    dns->writeDatagram(reply, from, fromPort);
            }
        });
    }

private:
    enum { T_A = 1, T_NS = 2, T_PTR = 12, T_MX = 15, T_TXT = 16, T_AAAA = 28, T_SRV = 33, T_ANY = 255 };

    static QByteArray u16(quint16 v) { QByteArray b; b.append(char((v >> 8) & 0xff)); b.append(char(v & 0xff)); return b; }
    static QByteArray u32(quint32 v)
    {
        QByteArray b;
        b.append(char((v >> 24) & 0xff)); b.append(char((v >> 16) & 0xff));
        b.append(char((v >> 8) & 0xff)); b.append(char(v & 0xff));
        return b;
    }

    static QByteArray encodeName(const QList<QByteArray> &labels)
    {
        QByteArray out;
        for (int i = 0; i < labels.size(); ++i) {
            out.append(char(labels.at(i).size() & 0x3f));
            out.append(labels.at(i));
        }
        out.append(char(0));
        return out;
    }

    // One answer resource record whose owner name is a compression pointer to the question
    // (offset 12) -- every record the test asks for is owned by the queried name.
    static QByteArray rr(quint16 type, const QByteArray &rdata)
    {
        QByteArray out;
        out.append(char(0xc0)); out.append(char(0x0c));
        out += u16(type); out += u16(1); out += u32(60);
        out += u16(quint16(rdata.size())); out += rdata;
        return out;
    }

    static QByteArray aRdata(const char *ip) { return u32(QHostAddress(QString::fromLatin1(ip)).toIPv4Address()); }
    static QByteArray aaaaRdata(const char *ip)
    {
        const Q_IPV6ADDR addr = QHostAddress(QString::fromLatin1(ip)).toIPv6Address();
        return QByteArray(reinterpret_cast<const char *>(addr.c), 16);
    }
    static QByteArray txtChunk(const QByteArray &s) { QByteArray out; out.append(char(s.size() & 0xff)); out += s; return out; }

    QByteArray buildReply(const QByteArray &query)
    {
        if (query.size() < 12)
            return QByteArray();

        // Read the question name into labels, then qtype.
        int off = 12;
        QList<QByteArray> labels;
        while (off < query.size()) {
            const int len = quint8(query.at(off));
            if (len == 0) { ++off; break; }
            if ((len & 0xc0) == 0xc0) { off += 2; break; }
            ++off;
            labels << query.mid(off, len);
            off += len;
        }
        if (off + 4 > query.size())
            return QByteArray();
        const quint16 qtype = (quint16(quint8(query.at(off))) << 8) | quint8(query.at(off + 1));
        const int questionEnd = off + 4;

        // Pick the base label and the trailing zone labels (echoed onto relative targets).
        QByteArray base;
        QList<QByteArray> zone;
        if (labels.size() >= 3 && labels.at(0).startsWith('_')) {
            base = labels.at(2); zone = labels.mid(3);   // _service._proto.<base>.<zone>
        } else if (!labels.isEmpty()) {
            base = labels.at(0); zone = labels.mid(1);
        }

        QByteArray answers;
        int answerCount = 0;
        const bool known = appendRecords(base, qtype, zone, answers, answerCount);

        QByteArray reply = query.left(2);                 // copy the query id
        reply += u16(known ? 0x8580 : 0x8583);            // QR + AA + RD + RA; rcode 3 when unknown
        reply += u16(1);                                  // one question
        reply += u16(quint16(answerCount));
        reply += u16(0); reply += u16(0);                 // no authority / additional
        reply += query.mid(12, questionEnd - 12);         // echo the question
        reply += answers;
        return reply;
    }

    // Appends the resource records for base+qtype; returns false for an unknown base so the
    // caller answers NXDOMAIN. zone is the trailing labels appended to relative targets.
    bool appendRecords(const QByteArray &base, quint16 qtype, const QList<QByteArray> &zone,
                       QByteArray &out, int &count)
    {
        const bool any = (qtype == T_ANY);
        // A relative target ("multi") becomes "multi.<zone>"; an absolute one keeps its labels.
        const QList<QByteArray> zoneLabels = zone;
        auto relName = [zoneLabels](const char *n) {
            QList<QByteArray> ls = QByteArray(n).split('.');
            ls += zoneLabels;
            return encodeName(ls);
        };
        auto absName = [](const char *n) { return encodeName(QByteArray(n).split('.')); };
        auto addA = [&](const char *ip) { out += rr(T_A, aRdata(ip)); ++count; };
        auto addAAAA = [&](const char *ip) { out += rr(T_AAAA, aaaaRdata(ip)); ++count; };
        auto addMX = [&](quint16 pref, const char *tgt) { out += rr(T_MX, u16(pref) + relName(tgt)); ++count; };
        auto addNS = [&](const char *tgt) { out += rr(T_NS, absName(tgt)); ++count; };
        auto addPTR = [&](const char *tgt) { out += rr(T_PTR, relName(tgt)); ++count; };
        auto addSRV = [&](quint16 p, quint16 w, quint16 port, const char *tgt) {
            out += rr(T_SRV, u16(p) + u16(w) + u16(port) + relName(tgt)); ++count;
        };
        auto addTXT = [&](const QByteArray &s) { out += rr(T_TXT, txtChunk(s)); ++count; };

        if (base == "a-single") { if (any || qtype == T_A) addA("192.0.2.1"); }
        else if (base == "a-multi") { if (any || qtype == T_A) { addA("192.0.2.1"); addA("192.0.2.2"); addA("192.0.2.3"); } }
        else if (base == "aaaa-single") { if (any || qtype == T_AAAA) addAAAA("2001:db8::1"); }
        else if (base == "aaaa-multi") { if (any || qtype == T_AAAA) { addAAAA("2001:db8::1"); addAAAA("2001:db8::2"); addAAAA("2001:db8::3"); } }
        else if (base == "a-plus-aaaa") { if (any || qtype == T_A) addA("198.51.100.1"); if (any || qtype == T_AAAA) addAAAA("2001:db8::1:1"); }
        else if (base == "multi") {
            if (any || qtype == T_A) { addA("198.51.100.1"); addA("198.51.100.2"); addA("198.51.100.3"); }
            if (any || qtype == T_AAAA) { addAAAA("2001:db8::1:1"); addAAAA("2001:db8::1:2"); }
        }
        else if (base == "mx-single") { if (any || qtype == T_MX) addMX(10, "multi"); }
        else if (base == "mx-single-cname") { if (any || qtype == T_MX) addMX(10, "cname"); }
        else if (base == "mx-multi") { if (any || qtype == T_MX) { addMX(10, "multi"); addMX(20, "a-single"); } }
        else if (base == "mx-multi-sameprio") { if (any || qtype == T_MX) { addMX(10, "multi"); addMX(10, "a-single"); } }
        else if (base == "ptr-single") { if (any || qtype == T_PTR) addPTR("a-single"); }
        else if (base == "ns-single") { if (any || qtype == T_NS) addNS("ns11.cloudns.net"); }
        else if (base == "ns-multi") { if (any || qtype == T_NS) { addNS("ns11.cloudns.net"); addNS("ns12.cloudns.net"); } }
        else if (base == "srv-single") { if (any || qtype == T_SRV) addSRV(5, 0, 7, "multi"); }
        else if (base == "srv-prio") { if (any || qtype == T_SRV) { addSRV(1, 0, 7, "multi"); addSRV(2, 0, 7, "a-plus-aaaa"); } }
        else if (base == "srv-weighted") { if (any || qtype == T_SRV) { addSRV(5, 75, 7, "multi"); addSRV(5, 25, 7, "a-plus-aaaa"); } }
        else if (base == "srv-multi") {
            if (any || qtype == T_SRV) {
                addSRV(1, 50, 7, "multi"); addSRV(2, 50, 7, "a-single");
                addSRV(2, 50, 7, "aaaa-single"); addSRV(3, 50, 7, "a-multi");
            }
        }
        else if (base == "txt-single") { if (any || qtype == T_TXT) addTXT("Hello"); }
        else if (base == "txt-multi-onerr") { if (any || qtype == T_TXT) { out += rr(T_TXT, txtChunk("Hello") + txtChunk("World")); ++count; } }
        else if (base == "txt-multi-multirr") { if (any || qtype == T_TXT) { addTXT("Hello"); addTXT("World"); } }
        else { return false; } // unknown base -> NXDOMAIN

        return true;
    }
};

// ---------------------------------------------------------------------------
// ImapService: an IMAP-ish greeter. Enough for the socket-engine tests, which only
// drive the greeting + NOOP + LOGOUT and echo the client's command tag.
// ---------------------------------------------------------------------------

class ImapService : public TestService
{
public:
    ImapService(const QHostAddress &host, quint16 port, QObject *parent = Q_NULLPTR)
        : TestService(host, port, parent) {}

    void start() Q_DECL_OVERRIDE
    {
        QTcpServer *server = new QTcpServer(this);
        listenTcp(server);
        connect(server, &QTcpServer::newConnection, server, [server]() {
            QTcpSocket *c = server->nextPendingConnection();
            connect(c, &QTcpSocket::readyRead, c, [c]() { onImap(c); });
            connect(c, &QTcpSocket::disconnected, c, &QObject::deleteLater);
            // Greeting: must start with "* OK " and end with "server ready\r\n".
            c->write("* OK [CAPABILITY IMAP4rev1] server-dummy IMAP server ready\r\n");
        });
    }

private:
    static void onImap(QTcpSocket *c)
    {
        while (c->canReadLine()) {
            const QByteArray line = c->readLine().trimmed();
            const int sp = line.indexOf(' ');
            const QByteArray tag = sp < 0 ? line : line.left(sp);
            const QByteArray cmd = (sp < 0 ? QByteArray() : line.mid(sp + 1)).toUpper();
            if (cmd == "LOGOUT") {
                // "* BYE LOGOUT received\r\n<tag> OK Completed\r\n", then close, so the
                // client observes RemoteHostClosedError.
                c->write("* BYE LOGOUT received\r\n" + tag + " OK Completed\r\n");
                c->flush();
                c->disconnectFromHost();
            } else {
                // NOOP and anything else: "<tag> OK Completed\r\n".
                c->write(tag + " OK Completed\r\n");
            }
        }
    }
};

// ---------------------------------------------------------------------------
// main: build the services for the requested "way" and report the real ports.
// ---------------------------------------------------------------------------

int main(int argc, char **argv)
{
    QCoreApplication app(argc, argv);

    // Positional args are: <way> <port> <echoPort>; options (e.g. --folder=<dir>, which
    // makes the FTP server expose a real directory) may appear anywhere.
    QStringList positional;
    QString folder;
    for (int i = 1; i < argc; ++i) {
        const QString a = QString::fromLocal8Bit(argv[i]);
        if (a.startsWith(QLatin1String("--folder=")))
            folder = a.mid(9);
        else
            positional << a;
    }

    const QString which = positional.value(0).toLower();

    if (which == QLatin1String("spdy")) {
        // The tst_spdy harness needs a whole fleet of services. Each binds its
        // CORRECT (privileged) port when this process is elevated -- https on 443,
        // http on 80, ftp on 21, imap(s) on 143/993, echo on 7, daytime on 13 --
        // and an unprivileged stand-in otherwise (testPort(): 4433/8080/2100/...).
        // A client that asks testPort() for the same service meets it on the same
        // port, so an unelevated run needs no port-remap layer at all. The HTTP
        // CONNECT proxies (3128/3129 auth) and SOCKS5 proxies (1080/1081 auth) are
        // already unprivileged, so they keep their fixed ports.
        const QHostAddress host = QHostAddress::LocalHost;
        (new HttpsService(host, testPort(443), folder, &app))->start();
        (new ImapsService(host, testPort(993), &app))->start(); // imaps
        (new ImapService(host, testPort(143), &app))->start(); // plain imap
        (new EchoService(host, testPort(7), &app))->start(); // tcp+udp echo
        (new FtpService(host, testPort(21), folder, &app, true))->start(); // ftp control (flat folder as /qtest)
        (new DaytimeService(host, testPort(13), &app))->start(); // daytime
        (new PlainHttpService(host, testPort(80), folder, &app))->start();
        (new ConnectProxyService(host, 3128, false, &app))->start();
        (new ConnectProxyService(host, 3129, true, &app))->start();
        (new SocksProxyService(host, 1080, false, &app))->start();
        (new SocksProxyService(host, 1081, true, &app))->start();
        std::fputs("READY spdy\n", stdout);
        std::fflush(stdout);
        return app.exec();
    }

    TestServer::Type type = TestServer::FileTransfer;
    if (which == QLatin1String("web"))
        type = TestServer::WebProxy;
    else if (which == QLatin1String("dns"))
        type = TestServer::NameLookup;
    else if (which == QLatin1String("imap"))
        type = TestServer::MailProxy;

    // Ports come from the arguments TestServer passes (it has already checked they are
    // bindable); fall back to the shared test-env.ini values when run by hand with none.
    const quint16 wantPort = positional.size() > 1 ? quint16(positional.at(1).toUShort())
                                                   : TestEnv::serverPort();
    const quint16 wantEcho = positional.size() > 2 ? quint16(positional.at(2).toUShort())
                                                   : quint16(wantPort + 1);

    const QHostAddress host = TestEnv::serverAddress();

    TestService *primary = Q_NULLPTR;   // FTP / HTTP / DNS -- on wantPort
    TestService *secondary = Q_NULLPTR;  // echo / SOCKS     -- on wantEcho
    switch (type) {
    case TestServer::FileTransfer:
        primary = new FtpService(host, wantPort, folder, &app);
        secondary = new EchoService(host, wantEcho, &app);
        break;
    case TestServer::WebProxy:
        primary = new HttpService(host, wantPort, &app);
        secondary = new SocksService(host, wantEcho, &app);
        break;
    case TestServer::NameLookup:
        primary = new DnsService(host, wantPort, &app);
        break;
    case TestServer::MailProxy:
        primary = new ImapService(host, wantPort, &app);
        secondary = new SocksService(host, wantEcho, &app);
        break;
    }
    if (primary)
        primary->start();
    if (secondary)
        secondary->start();

    // Let TestServer (or a human) know the listeners are bound and it is safe to connect,
    // and on which ports they actually landed (they may have fallen back). Kept on its own
    // line and flushed so a reader can wait for it.
    char ready[64];
    std::snprintf(ready, sizeof(ready), "READY %u %u\n",
                  unsigned(primary ? primary->port() : wantPort),
                  unsigned(secondary ? secondary->port() : wantEcho));
    std::fputs(ready, stdout);
    std::fflush(stdout);

    return app.exec();
}
