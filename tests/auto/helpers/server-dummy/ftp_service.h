#ifndef FTP_SERVICE_H
#define FTP_SERVICE_H

#include "service_base.h"

#include <QtCore/QByteArray>
#include <QtCore/QCoreApplication>
#include <QtCore/QDir>
#include <QtCore/QFile>
#include <QtCore/QFileInfo>
#include <QtCore/QList>
#include <QtCore/QStringList>

/// FtpService: a small FTP server over a writable in-memory filesystem.
class FtpService : public TestService
{
public:
    /// When qtestAtRoot is true a leading "/qtest" in an FTP path maps onto m_root
    /// itself, so a flat --folder (no qtest/ subdir) still answers the ftp://.../qtest/...
    /// URLs the tests use. A server given a fixture that already has a real qtest/ dir
    /// (e.g. the qftp fixture) leaves it false and serves that directory verbatim.
    FtpService(const ServiceConfig &c, QObject *parent = Q_NULLPTR)
        : TestService(c, parent), m_root(c.folder), m_qtestAtRoot(c.qtestAtRoot), m_ftpProxy(c.ftpProxy)
    { m_config = watcher()->load(); }

    /// Re-read the live config when server-config.ini changes (a test may edit [Unreachable]
    /// between cases via TestServer::edit()). FtpService opts in to reloading; the base default is
    /// to ignore the change.
    void onIniChange() Q_DECL_OVERRIDE { m_config = watcher()->load(); }

    /// Out-of-source scratch for STOR uploads. The read --folder (m_root) is the test's fixture
    /// directory, which lives in the SOURCE TREE -- writing uploads there pollutes the checkout (and
    /// they used to get git-added). So the only writable subtree (/qtest/upload) is redirected here,
    /// under the system temp dir; server-dummy removes it on exit (see main.cpp). One per process.
    /// Public so main.cpp can delete it on shutdown.
    static QString uploadScratchDir()
    {
        return QDir::tempPath() + QLL("/server-dummy-ftp-")
             + QString::number(QCoreApplication::applicationPid());
    }

    void start() Q_DECL_OVERRIDE
    {
        QTcpServer *server = new QTcpServer(this);
        listenTcp(server);
        connect(server, &QTcpServer::newConnection, this, &FtpService::onNewConnection);
    }

private:
    void onNewConnection()
    {
        QTcpSocket *c = qobject_cast<QTcpServer *>(sender())->nextPendingConnection();
        FtpState *st = new FtpState(c);
        c->setProperty("ftpState", QVariant::fromValue<QObject *>(st));
        connect(c, &QTcpSocket::readyRead, this, &FtpService::onControlReadyRead);
        connect(c, &QTcpSocket::disconnected, c, &QObject::deleteLater);
        // Greeting must read exactly "220 (vsFTPd X.Y.Z)" -- tst_QTcpSocket::atEnd checks the line
        // length and socketInAThread/QtNetworkSettings::compareReplyFtp regex-match the vsFTPd form.
        reply(c, 220, "(vsFTPd 2.3.5)");
    }

    void onControlReadyRead()
    {
        QTcpSocket *c = qobject_cast<QTcpSocket *>(sender());
        onFtp(c, stateOf(c));
    }

    /// Per-control-connection state, kept alive as a child of the control socket.
    struct FtpState : QObject {
        explicit FtpState(QObject *p)
            : QObject(p), pasv(Q_NULLPTR), data(Q_NULLPTR), pending(false),
              loggedIn(false), rest(0), active(false), portPort(0), ascii(false),
              cwd(QLL("/")) {}
        /// The current PASV listener.
        QTcpServer *pasv;
        /// The data connection, once established.
        QTcpSocket *data;
        /// Bytes queued for the next download.
        QByteArray payload;
        /// A download is queued.
        bool pending;
        /// Real path of an upload target, or the rename-from path.
        QString storName;
        bool loggedIn;
        QString user;
        qint64 rest;
        /// PORT (active) mode selected for the next transfer.
        bool active;
        QHostAddress portAddr;
        quint16 portPort;
        /// TYPE A (CRLF text mode).
        bool ascii;
        /// FTP-space current directory.
        QString cwd;
    };

    /// FtpState is a plain QObject (no Q_OBJECT), so recover it with static_cast from the
    /// QObject* stashed on the control/data/pasv object; the stored object is always one.
    static FtpState *stateOf(QObject *o)
    {
        return static_cast<FtpState *>(o->property("ftpState").value<QObject *>());
    }
    static QTcpSocket *ctrlOf(QObject *o)
    {
        return qobject_cast<QTcpSocket *>(o->property("ftpControl").value<QObject *>());
    }

    /// Filesystem directory this FTP server exposes (from --folder).
    QString m_root;
    /// Treat a leading "/qtest" as m_root (flat-folder fleet use).
    bool m_qtestAtRoot;
    /// FTP-proxy mode: strip the "@host" suffix Qt appends to USER, serve locally.
    bool m_ftpProxy;
    /// Live config (refreshed in onIniChange).
    QSharedPointer<ServiceConfigDynamic> m_config;

    /// Map an FTP path (absolute, /var/ftp-aliased, or relative to cwd) into FTP space.
    static QString ftpNorm(const QString &cwd, QString p)
    {
        p = p.trimmed();
        if (p == QLL("/var/ftp") || p.startsWith(QLL("/var/ftp/")))
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
        // Uploads (reads and writes of /qtest/upload/...) go to the out-of-source scratch dir, never
        // the source-tree fixture folder. A round-trip (STOR then RETR) still works: both land here.
        if (ftp == QLL("/qtest/upload") || ftp.startsWith(QLL("/qtest/upload/"))) {
            const QString rel = ftp.mid(int(sizeof("/qtest/upload") - 1)); // "" or "/name"
            return QDir::cleanPath(uploadScratchDir() + rel);
        }
        if (m_qtestAtRoot) {
            if (ftp == QLL("/qtest"))
                ftp = QLL("/");
            else if (ftp.startsWith(QLL("/qtest/")))
                ftp = ftp.mid(6); // drop "/qtest", keep the leading slash
        }
        return QDir::cleanPath(m_root + ftp);
    }
    /// Only /qtest/upload (and below) is writable; writes elsewhere must fail, as the
    /// tests expect (mkdir/put at the root is rejected).
    static bool writable(const QString &cwd, const QString &p)
    {
        return ftpNorm(cwd, p).startsWith(QLL("/qtest/upload"));
    }

    /// Non-zero when this resolved (absolute) path is configured "unreachable" in the live config
    /// (listed directly or under a listed directory), returning its error code. Such a path answers
    /// SIZE/MDTM (exists) but denies RETR, so a client sees content-access-denied.
    int unreachableCode(const QString &absPath) const
    {
        return m_config ? m_config->unreachableCode(absPath) : 0;
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
                // FTP-proxy mode: Qt's FTP backend sends "USER user@host[:port]" through the proxy.
                // Strip the origin suffix and serve locally -- a terminating proxy, which is enough
                // for the tests (they only assert the fetched content/listing, not a real second hop).
                st->user = (m_ftpProxy && arg.contains(QLatin1Char('@')))
                           ? arg.left(arg.indexOf(QLatin1Char('@'))) : arg;
                reply(c, 331, "need password");
            } else if (verb == "PASS") {
                // Anonymous and a couple of fixed accounts log in; a bad login is rejected with
                // 530 -> QNAM maps to AuthenticationRequiredError. "foo"/"invalidUser" are always
                // rejected (qftp's rejected-login row; putToFtpWithInvalidCredentials). The
                // "ftptest" account must present password "password" (ftpAuthentication checks that
                // a wrong password fails); anonymous and others log in with any password.
                if (st->user == QLL("foo") || st->user == QLL("invalidUser")
                    || (st->user == QLL("ftptest") && arg != QLL("password")))
                    reply(c, 530, "login incorrect");
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
                st->cwd = ftpNorm(st->cwd, QLL(".."));
                reply(c, 200, "ok");
            } else if (verb == "REST") {
                st->rest = arg.toLongLong();
                reply(c, 350, "restarting");
            } else if (verb == "SIZE") {
                // A configured-unreachable path stands for a file that exists but cannot be read:
                // SIZE succeeds (so QNAM's backend leaves the Statting state) and the later RETR is
                // denied, yielding ContentAccessDenied rather than ContentNotFoundError
                // (tst_QNetworkReply::getErrors ftp-file-not-readable / ftp-dir-not-readable). Which
                // paths are unreachable comes from server-config.ini [Unreachable].
                if (unreachableCode(realPath(st->cwd, arg))) {
                    reply(c, 213, "512");
                } else {
                    const QFileInfo fi(realPath(st->cwd, arg));
                    if (fi.isFile()) reply(c, 213, QByteArray::number(fi.size()).constData());
                    else reply(c, 550, "no such file");
                }
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
                reply(c, 221, "Goodbye."); // compareReplyFtp expects "221 Goodbye.\r\n"
                c->disconnectFromHost();
            } else if (verb == "HELP") {
                // QNAM's FTP backend scans the HELP reply for "SIZE"/"MDTM" (case-sensitive) to
                // decide whether to issue those commands; SIZE is what populates a download's
                // Content-Length. List them so the backend probes SIZE (RFC 3659 commands).
                reply(c, 214, "Recognized commands: TYPE PASV PORT RETR STOR LIST NLST "
                              "SIZE MDTM CWD CDUP PWD REST DELE MKD RMD RNFR RNTO QUIT");
            } else if (verb == "FEAT") {
                reply(c, 211, "no features");
            } else if (verb == "OPTS") {
                reply(c, 200, "ok");
            } else if (verb == "MDTM") {
                // A configured-unreachable path must stat OK here too: QNAM's Statting phase issues
                // both SIZE and MDTM, and if EITHER fails it maps to ContentNotFound. So report a
                // fixed timestamp (the file "exists"); only RETR then denies -> ContentAccessDenied.
                if (unreachableCode(realPath(st->cwd, arg))) {
                    reply(c, 213, "20200101000000");
                } else {
                    const QFileInfo fi(realPath(st->cwd, arg));
                    if (fi.isFile())
                        reply(c, 213, fi.lastModified().toUTC().toString(QLL("yyyyMMddhhmmss")).toLatin1().constData());
                    else
                        reply(c, 550, "no such file");
                }
            } else {
                std::fprintf(stderr, "server-dummy ftp: unimplemented verb '%s'\n", verb.constData());
                std::fflush(stderr);
                reply(c, 502, "not implemented");
            }
        }
    }

    // MARK: data channel: PASV (we listen) or PORT (we connect back to the client).

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
        st->pasv->setProperty("ftpState", QVariant::fromValue<QObject *>(st));
        st->pasv->setProperty("ftpControl", QVariant::fromValue<QObject *>(c));
        connect(st->pasv, &QTcpServer::newConnection, this, &FtpService::onPasvConnection);
        const quint16 p = st->pasv->serverPort();
        reply(c, 227, QByteArray("Entering Passive Mode (127,0,0,1," +
                                 QByteArray::number(p >> 8) + ',' +
                                 QByteArray::number(p & 0xff) + ")").constData());
    }

    void onPasvConnection()
    {
        QTcpServer *pasv = qobject_cast<QTcpServer *>(sender());
        FtpState *st = stateOf(pasv);
        st->data = pasv->nextPendingConnection();
        st->data->setParent(st);
        onDataReady(ctrlOf(pasv), st);
    }

    /// "PORT h1,h2,h3,h4,p1,p2": remember where to connect for the next transfer.
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

    /// Called once a data connection exists: run whichever transfer was queued.
    void onDataReady(QTcpSocket *c, FtpState *st)
    {
        if (st->pending) sendData(c, st);
        else if (!st->storName.isEmpty()) receiveUpload(c, st);
    }

    /// Establish the data connection: connect out in PORT mode, use/await it in PASV mode.
    void ensureData(QTcpSocket *c, FtpState *st)
    {
        if (st->active) {
            QTcpSocket *d = new QTcpSocket(st);
            st->data = d;
            d->setProperty("ftpState", QVariant::fromValue<QObject *>(st));
            d->setProperty("ftpControl", QVariant::fromValue<QObject *>(c));
            connect(d, &QTcpSocket::connected, this, &FtpService::onActiveDataConnected);
            d->connectToHost(st->portAddr, st->portPort);
        } else if (st->data) {
            onDataReady(c, st);
        }
        // else PASV, still waiting: openPasv's newConnection handler calls onDataReady.
    }

    void onActiveDataConnected()
    {
        QObject *d = sender();
        onDataReady(ctrlOf(d), stateOf(d));
    }

    // MARK: listing: a real directory in the ls -l form QFtp's parser understands.

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

    // MARK: RETR / STOR against real files.

    void retr(QTcpSocket *c, FtpState *st, const QString &name)
    {
        // A configured-unreachable path exists (SIZE succeeded) but cannot be read -> 550 permission
        // denied, which QNAM maps to ContentAccessDenied (getErrors ftp-file-not-readable). The set
        // comes from server-config.ini [Unreachable] (see unreachableCode).
        if (unreachableCode(realPath(st->cwd, name))) { reply(c, 550, "permission denied"); return; }
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
        // Ensure the upload directory exists -- a document root handed in by a test
        // (e.g. tst_QNetworkReply's testDataDir) usually has no qtest/upload/ subtree yet, and
        // QFile::open(Append) silently fails on a missing dir, storing 0 bytes.
        QDir().mkpath(QFileInfo(st->storName).path());
        if (!append)
            QFile::remove(st->storName);
        st->pending = false;
        ensureData(c, st);
    }

    void receiveUpload(QTcpSocket *c, FtpState *st)
    {
        reply(c, 150, "opening data connection");
        QTcpSocket *data = st->data;
        QFile *f = new QFile(st->storName, data);
        f->open(QIODevice::Append);
        data->setProperty("uploadFile", QVariant::fromValue<QObject *>(f));
        data->setProperty("uploadAscii", st->ascii);
        data->setProperty("ftpState", QVariant::fromValue<QObject *>(st));
        data->setProperty("ftpControl", QVariant::fromValue<QObject *>(c));
        connect(data, &QTcpSocket::readyRead, this, &FtpService::onUploadReadyRead);
        connect(data, &QTcpSocket::disconnected, this, &FtpService::onUploadDone);
    }

    void onUploadReadyRead()
    {
        QObject *data = sender();
        QFile *f = qobject_cast<QFile *>(data->property("uploadFile").value<QObject *>());
        QByteArray d = qobject_cast<QTcpSocket *>(data)->readAll();
        // ASCII store: CRLF -> LF.
        if (data->property("uploadAscii").toBool()) d.replace("\r\n", "\n");
        f->write(d);
    }

    void onUploadDone()
    {
        QObject *data = sender();
        QFile *f = qobject_cast<QFile *>(data->property("uploadFile").value<QObject *>());
        QByteArray d = qobject_cast<QTcpSocket *>(data)->readAll();
        if (data->property("uploadAscii").toBool()) d.replace("\r\n", "\n");
        f->write(d);
        f->close();
        reply(ctrlOf(data), 226, "transfer complete");
        resetData(stateOf(data));
    }
};

#endif // FTP_SERVICE_H
