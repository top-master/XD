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

#ifndef TESTSERVER_H
#define TESTSERVER_H

#include <QtCore/QByteArray>
#include <QtCore/QCoreApplication>
#include <QtCore/QDateTime>
#include <QtCore/QDir>
#include <QtCore/QElapsedTimer>
#include <QtCore/QFile>
#include <QtCore/QFileInfo>
#include <QtCore/QList>
#include <QtCore/QMap>
#include <QtCore/QObject>
#include <QtCore/QSettings>
#include <QtCore/QPair>
#include <QtCore/QProcess>
#include <QtCore/QString>
#include <QtCore/QStringList>
#include <QtCore/QThread>
#include <QtCore/QUrl>
#include <QtNetwork/QHostAddress>
#include <QtNetwork/QTcpServer>
#include <QtNetwork/QTcpSocket>
#ifndef QT_NO_SSL
#  include <QtNetwork/QSslCertificate>
#endif

#include "../network-settings.h"
#include "test-ports.h"

#if defined(Q_OS_LINUX)
#  include <csignal>
#  include <unistd.h>
#  include <sys/prctl.h>
#endif

/// A QProcess that ties the launched child's lifetime to ours. QProcess terminates the
/// child on clean teardown (TestServer::stop() below), but if the *test* process itself
/// dies abnormally -- a crash, a harness timeout, an outright SIGKILL -- that path never
/// runs, and server-dummy would be reparented to init (PPID 1) and linger for as long as
/// the box stays up, still holding its listening ports and blocking the next run. Rather
/// than bake a raw prctl() into the server binary, we use QProcess's own child-setup hook
/// -- setupChildProcess() runs in the forked child, after chdir and just before exec --
/// which is exactly where XD's QProcess lets a caller adjust the about-to-be-exec'd child.
/// On Linux we ask the kernel to send it SIGTERM the instant we go away, and close the
/// race where we had already died before the call (getppid() == 1). setupChildProcess()
/// runs post-fork, so only async-signal-safe calls are allowed -- prctl/getppid/_exit all
/// qualify. Best-effort and Linux-only; every other platform relies on the stop() path.
class ParentDeathProcess : public QProcess
{
public:
    explicit ParentDeathProcess(QObject *parent = Q_NULLPTR) : QProcess(parent) {}

protected:
    void setupChildProcess() Q_DECL_OVERRIDE
    {
#if defined(Q_OS_LINUX)
        ::prctl(PR_SET_PDEATHSIG, SIGTERM);
        if (::getppid() == 1)
            ::_exit(0);
#endif
    }
};

/// A handle to the bundled test server (helpers/server-dummy). It does not implement
/// any protocol itself: TestEnv hands it the absolute path of the server-dummy binary,
/// and this class runs that binary as a QProcess for one "way" (Type), then terminates
/// it when the handle is destroyed, or when stop()/stopLater() is called. TestEnv::
/// getServer() owns one through a QRef, so the server lives exactly as long as the ref.
///
/// Modelled on the stable/testable server in QtRemote's tests (tests/auto/remote/
/// dummy-server.h): start() blocks only until the child says it is listening -- the
/// child prints "READY" once its listeners are bound -- so a test can then drive a
/// blocking client against it without racing the server's startup.
///
/// Builder that edits a TestServer's server-config.ini between test-cases (obtained from
/// TestServer::edit()). Chainable: edit().reset().add("Unreachable./pub/x.txt", "403").save().
///   reset()  -- restore the file to the pristine baseline captured before the first edit, so one
///               test-case's knobs never leak into the next.
///   add(k,v) -- upsert "group.key = value" (key is quoted when it holds a slash or space, matching
///               the [Unreachable] path form). The group is the part before the first '.'.
///   save()   -- write the file. The running server-dummy's IniWatcher picks the change up on its
///               next poll (no restart), and each service reacts in onIniChange().
class TestServerEditor
{
public:
    explicit TestServerEditor(const QString &iniPath) : m_path(iniPath), m_reset(false)
    {
        // Snapshot the pristine baseline once per path (the first edit sees the committed file).
        static QMap<QString, QByteArray> baseline;
        if (!baseline.contains(m_path)) {
            QFile f(m_path);
            baseline.insert(m_path, f.open(QIODevice::ReadOnly) ? f.readAll() : QByteArray());
        }
        m_baseline = baseline.value(m_path);
    }
    TestServerEditor &reset() { m_reset = true; return *this; }
    TestServerEditor &add(const QString &groupDotKey, const QString &value)
    { m_edits.append(qMakePair(groupDotKey, value)); return *this; }
    /// Write the edits, then (unless noWaiting) block until the running server-dummy's IniWatcher has
    /// actually picked the change up -- i.e. its [Watcher] lastPull ms-timestamp climbs past the moment
    /// this save() began. That is a precise handshake, not a fixed sleep: the pre-write lastPull is
    /// necessarily < lastSave, so only a FRESH pull (after the watcher sees our write) ends the wait.
    /// Give up after 10 s and warn -- the server is assumed to have crashed (the caller's own
    /// assertions then fail naturally). Pass noWaiting=true for fire-and-forget writes (e.g. a reset at
    /// the end of a test, where the next save() will synchronise anyway).
    void save(bool noWaiting = false)
    {
        const qint64 lastSave = QDateTime::currentMSecsSinceEpoch();
        QByteArray content = m_baseline;
        if (!m_reset) { QFile f(m_path); if (f.open(QIODevice::ReadOnly)) content = f.readAll(); }
        QStringList lines = QString::fromUtf8(content).split(QLatin1Char('\n'));
        for (int i = 0; i < m_edits.size(); ++i)
            upsert(lines, m_edits.at(i).first, m_edits.at(i).second);
        QFile out(m_path);
        if (out.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
            out.write(lines.join(QLatin1Char('\n')).toUtf8());
            out.close();
        }
        if (noWaiting)
            return;
        const qint64 deadline = lastSave + 10000;
        forever {
            QCoreApplication::processEvents(); // keep the caller's event loop breathing...
            QThread::msleep(30);               // ...without busy-spinning while we wait for the ack
            if (readLastPull() > lastSave)
                return;
            if (QDateTime::currentMSecsSinceEpoch() > deadline) {
                qWarning("TestServerEditor::save: server-dummy did not pull '%s' within 10 s (crashed?)",
                         qPrintable(m_path));
                return;
            }
        }
    }

    /// Read [Watcher] lastPull (ms since epoch) the server stamps on each pull; -1 if absent/unreadable.
    qint64 readLastPull() const
    {
        QFile f(m_path);
        if (!f.open(QIODevice::ReadOnly | QIODevice::Text))
            return -1;
        const QStringList lines = QString::fromUtf8(f.readAll()).split(QLatin1Char('\n'));
        f.close();
        QString section;
        for (int i = 0; i < lines.size(); ++i) {
            const QString t = lines.at(i).trimmed();
            if (t.startsWith(QLatin1Char('[')) && t.endsWith(QLatin1Char(']')))
                section = t.mid(1, t.size() - 2).trimmed().toLower();
            else if (section == QLL("watcher") && t.startsWith(QLL("lastPull"))) {
                const int eq = t.indexOf(QLatin1Char('='));
                if (eq >= 0)
                    return t.mid(eq + 1).trimmed().toLongLong();
            }
        }
        return -1;
    }

private:
    static QString sectionOf(const QString &line)
    {
        const QString t = line.trimmed();
        if (t.startsWith(QLatin1Char('[')) && t.endsWith(QLatin1Char(']')))
            return t.mid(1, t.size() - 2).trimmed().toLower();
        return QString();
    }
    static QString unquote(QString k)
    {
        if (k.size() >= 2 && ((k.startsWith(QLatin1Char('"')) && k.endsWith(QLatin1Char('"')))
                              || (k.startsWith(QLatin1Char('\'')) && k.endsWith(QLatin1Char('\'')))))
            k = k.mid(1, k.size() - 2);
        return k;
    }
    static QString keyOf(const QString &line)
    {
        const int eq = line.indexOf(QLatin1Char('='));
        return eq < 0 ? QString() : unquote(line.left(eq).trimmed());
    }
    static void upsert(QStringList &lines, const QString &groupDotKey, const QString &value)
    {
        const int dot = groupDotKey.indexOf(QLatin1Char('.'));
        const QString group = (dot < 0 ? groupDotKey : groupDotKey.left(dot)).trimmed();
        const QString key = dot < 0 ? QString() : groupDotKey.mid(dot + 1);
        const bool quote = key.contains(QLatin1Char('/')) || key.contains(QLatin1Char(' '));
        const QString kw = quote ? (QLatin1Char('"') + key + QLatin1Char('"')) : key;
        const QString newLine = kw + QLL(" = ") + value;
        const QString glow = group.toLower();
        int sec = -1, secEnd = lines.size();
        for (int i = 0; i < lines.size(); ++i) {
            if (sectionOf(lines.at(i)) == glow) {
                sec = i;
                secEnd = lines.size();
                for (int j = i + 1; j < lines.size(); ++j)
                    if (!sectionOf(lines.at(j)).isEmpty()) { secEnd = j; break; }
                break;
            }
        }
        if (sec < 0) {
            if (!lines.isEmpty() && !lines.last().trimmed().isEmpty())
                lines.append(QString());
            lines.append(QLatin1Char('[') + group + QLatin1Char(']'));
            lines.append(newLine);
            return;
        }
        for (int i = sec + 1; i < secEnd; ++i)
            if (keyOf(lines.at(i)) == key) { lines[i] = newLine; return; }
        lines.insert(secEnd, newLine);
    }

    QString m_path;
    bool m_reset;
    QByteArray m_baseline;
    QList<QPair<QString, QString> > m_edits;
};

/// Being a QObject, stop() is usable as a slot target from the Qt5 connect() syntax,
/// e.g. connect(someObject, &Some::signal, server.data(), &TestServer::stop).
class TestServer : public QObject
{
public:
    enum Type {
        /// FTP control/PASV + TCP/UDP echo (server-dummy "ftp").
        FileTransfer,
        /// HTTP/1.1 responder + SOCKS5 proxy (server-dummy "web").
        WebProxy,
        /// DNS A-record responder (server-dummy "dns").
        NameLookup,
        /// IMAP-ish greeter + SOCKS5 auth proxy (server-dummy "imap").
        MailProxy,
        /// HTTPS/1.1 responder, self-signed cert (server-dummy "https").
        WebSecure,
        /// The whole TLS/SPDY/proxy fleet at once (server-dummy "spdy").
        Fleet
    };

    explicit TestServer(const QString &serverDummyPath, Type type,
                        quint16 defaultPort = 45678, QObject *parent = Q_NULLPTR)
        : QObject(parent), m_program(serverDummyPath), m_type(type), m_proc(Q_NULLPTR),
          m_defaultPort(defaultPort), m_port(defaultPort), m_echoPort(quint16(defaultPort + 1)),
          m_secondSecure(false)
    {
    }

    ~TestServer() { stop(); }

    Type type() const { return m_type; }
    QString program() const { return m_program; }
    bool isRunning() const { return m_proc && m_proc->state() != QProcess::NotRunning; }

    /// Whether tests reach this server at plain loopback rather than via the fake public
    /// host name qt-test-server.qt-test-net (see domainName()). True for a memory-safe
    /// (Fil-C) build and for any ordinary unprivileged run; only a privileged run, expected
    /// to resolve the fake name to the server out of band, returns false.
    bool isLocalhost() const { return isLoopbackOnly(); }

    /// The host a client should connect to in order to reach this server: the fake public domain
    /// qt-local-server.test, which server-dummy's shared resolver (see TestEnv::ensureResolver())
    /// maps to loopback, so tests keep their upstream name-based nature rather than degrading to a
    /// raw 127.0.0.1. server-dummy's certificate carries this name plus the legacy
    /// qt-test-server.qt-test-net and localhost/127.0.0.1/::1 in its SANs, so verification succeeds
    /// whether a client reaches the server by this domain, the legacy name, or straight at loopback.
    QString domainName() const { return QtNetworkSettings::domainName(); }

    /// A full URL that reaches this server: <scheme>://domainName():<loopback stand-in port>/<path>.
    /// Prefer this over hand-building "scheme://" + domainName() + ":" + port(...) + path at every
    /// call site. `scheme` selects the canonical service port the fleet stands in for (https->443,
    /// http->80, ftp->21, imap->143, imaps->993); `path` is taken verbatim (give it a leading '/').
    /// A URL that is intentionally malformed for the test (e.g. https:// to the plain-HTTP port to
    /// force a handshake failure) must still be crafted by hand -- this only builds well-formed ones.
    static QUrl url(const QString &path = QString(), const QString &scheme = QLL("https"))
    {
        quint16 canonical = 443;
        if (scheme == QLL("http")) canonical = 80;
        else if (scheme == QLL("ftp")) canonical = 21;
        else if (scheme == QLL("imap")) canonical = 143;
        else if (scheme == QLL("imaps")) canonical = 993;
        QUrl u;
        u.setScheme(scheme);
        u.setHost(QtNetworkSettings::domainName());
        u.setPort(TestServer::port(canonical));
        if (!path.isEmpty()) {
            // The caller may pass "path?query" in one string (e.g. "http-delete.cgi?200-ok").
            // setPath() alone would percent-encode the '?' into the path and lose the query, so
            // split on the first '?' and set the query component separately.
            const int q = path.indexOf(QLatin1Char('?'));
            if (q >= 0) {
                u.setPath(path.left(q));
                u.setQuery(path.mid(q + 1));
            } else {
                u.setPath(path);
            }
        }
        return u;
    }

    /// A freshly generated server certificate + key, created ONCE per test process into an
    /// out-of-source temp dir and handed to server-dummy via --cert/--key. Generating at run time
    /// (rather than committing a cert) means no cert binary lives in the tree at all, and the cert
    /// never carries a stale/expired date -- the two things the checkout must not accumulate.
    struct ServerCert {
        ServerCert() : ok(false) {}
        /// PEM file server-dummy presents (also the trust anchor for tests).
        QString certPath;
        /// Its private key.
        QString keyPath;
        /// The cert bytes, for domainCert().
        QByteArray certPem;
        bool ok;
    };

    /// The server cert the TLS listeners present: a long-lived self-signed cert covering domainName()
    /// + the legacy fake name + localhost/127.0.0.1/::1. Reused across processes via test-env.ini so a
    /// run pays ONE openssl, not one per test process (cached in-process too).
    static const ServerCert &serverCert()
    {
        static ServerCert c = loadOrGenerate(
            QLL("qtLocalServerCert"), QLL("qtLocalServerCertExpiresAt"),
            QLL("server"), QtNetworkSettings::domainName(),
            QLL("subjectAltName=DNS:") + QtNetworkSettings::domainName()
                + QLL(",DNS:") + QtNetworkSettings::serverLocalName()
                + QLatin1Char('.') + QtNetworkSettings::serverDomainName()
                + QLL(",DNS:localhost,IP:127.0.0.1,IP:0:0:0:0:0:0:0:1"));
        return c;
    }

    /// An intentionally-WRONG server certificate, for the middle-man / invalid-cert scenarios the old
    /// committed fake cert (fluke.troll.no) used to exercise: self-signed for fluke.troll.no, which
    /// matches neither domainName() nor serverName(), so a client that verifies it against the real
    /// host gets HostNameMismatch (the MITM case). Cached like serverCert() (invalidCert keys in the
    /// .ini); nothing invalid is committed. Returns the PEM file path (its key is dummyKeyInvalid()
    /// -- so a test can hand server-dummy `--cert=dummyCertInvalid() --key=dummyKeyInvalid()` to make
    /// it present the wrong cert). Not used by any test yet; provided ahead of need.
    static QString dummyCertInvalid() { return dummyInvalid().certPath; }
    static QString dummyKeyInvalid() { return dummyInvalid().keyPath; }

    static const ServerCert &dummyInvalid()
    {
        static ServerCert c = loadOrGenerate(
            QLL("invalidCert"), QLL("invalidCertExpiresAt"),
            QLL("dummy-invalid"), QLL("fluke.troll.no"),
            QLL("subjectAltName=DNS:fluke.troll.no"));
        return c;
    }

    /// Reuse a cert previously recorded in test-env.ini (both files still present and not past its
    /// stored expiry), else generate a fresh one and record its path + expiry for later processes.
    /// The .ini keys are unset by default; the first process to need a cert fills them in.
    static ServerCert loadOrGenerate(const QString &pathKey, const QString &expKey,
                                     const QString &tag, const QString &cn, const QString &sanExt)
    {
        const QString ini = certIniPath();
        const qint64 now = QDateTime::currentMSecsSinceEpoch() / 1000;
        if (!ini.isEmpty()) {
            QSettings s(ini, QSettings::IniFormat);
            const QString certPath = s.value(pathKey).toString();
            const qint64 exp = s.value(expKey).toLongLong();
            if (!certPath.isEmpty() && exp > now + 60) {
                const QString keyPath = keyPathFor(certPath);
                if (QFileInfo::exists(certPath) && QFileInfo::exists(keyPath)) {
                    ServerCert c;
                    c.certPath = certPath;
                    c.keyPath = keyPath;
                    QFile cf(certPath);
                    if (cf.open(QIODevice::ReadOnly)) { c.certPem = cf.readAll(); cf.close(); }
                    c.ok = !c.certPem.isEmpty();
                    if (c.ok)
                        return c;
                }
            }
        }
        ServerCert c = generateCert(tag, cn, sanExt);
        if (c.ok && !ini.isEmpty()) {
            QSettings s(ini, QSettings::IniFormat);
            s.setValue(pathKey, c.certPath);
            s.setValue(expKey, now + qint64(36500) * 24 * 3600); // matches the cert's ~100y validity
            s.sync();
        }
        return c;
    }

    /// The key file that pairs with a cert file generated by generateCert() ("<tag>-cert.pem" -> "-key.pem").
    static QString keyPathFor(const QString &certPath)
    {
        QString k = certPath;
        k.replace(QLL("-cert.pem"), QLL("-key.pem"));
        return k;
    }

    /// Locate the shared test-env.ini (mirrors TestEnv::iniPath(); testserver.h sits below testenv.h so
    /// it cannot call it). Empty when none is found -- the cert is then generated fresh, un-cached.
    static QString certIniPath()
    {
        QDir dir(QCoreApplication::applicationDirPath());
        forever {
            const QString cand = dir.filePath(QLL("test-env.ini"));
            if (QFileInfo::exists(cand))
                return cand;
            if (!dir.cdUp())
                return QString();
        }
    }

    /// Generate a self-signed cert+key (CN=`cn`, extension `sanExt`) via the system openssl into a
    /// shared temp dir under `tag` (shared so the cache above can serve every process). ~100-year
    /// validity so a cached cert never expires mid-run.
    static ServerCert generateCert(const QString &tag, const QString &cn, const QString &sanExt)
    {
        ServerCert c;
        const QString dir = QDir::tempPath() + QLL("/xd-testcerts");
        QDir().mkpath(dir);
        c.certPath = dir + QLatin1Char('/') + tag + QLL("-cert.pem");
        c.keyPath = dir + QLatin1Char('/') + tag + QLL("-key.pem");
        QStringList args;
        args << QLL("req") << QLL("-x509")
             << QLL("-newkey") << QLL("rsa:2048") << QLL("-nodes")
             << QLL("-keyout") << c.keyPath
             << QLL("-out") << c.certPath
             << QLL("-days") << QLL("36500") // ~100y: effectively non-expiring
             << QLL("-subj")
             << (QLL("/C=NO/ST=Oslo/O=Qt Software/CN=") + cn)
             << QLL("-addext") << sanExt
             << QLL("-addext") << QLL("basicConstraints=critical,CA:TRUE");
        QProcess p;
        p.start(QLL("openssl"), args);
        if (!p.waitForStarted(10000) || !p.waitForFinished(30000) || p.exitStatus() != QProcess::NormalExit
            || p.exitCode() != 0)
            return c; // ok stays false
        QFile cf(c.certPath);
        if (cf.open(QIODevice::ReadOnly)) {
            c.certPem = cf.readAll();
            cf.close();
        }
        c.ok = !c.certPem.isEmpty();
        return c;
    }

#ifndef QT_NO_SSL
    /// The certificate a client adds to trust this server (server-dummy presents the matching key,
    /// both from serverCert()). One multi-SAN self-signed certificate covers every host form
    /// domainName()/serverName() yield, so trusting it verifies the server by name or at loopback.
    QSslCertificate domainCert() const
    {
        return QSslCertificate(serverCert().certPem, QSsl::Pem);
    }

    /// The certificate the --second-port listener presents -- deliberately DISTINCT from
    /// domainCert(), so a client reconnecting to secondPort() sees a different chain. Only
    /// meaningful after enableSecondSecurePort() (which makes the child actually serve it).
    QSslCertificate secondDomainCert() const
    {
        return QSslCertificate(secondServerCert().certPem, QSsl::Pem);
    }

    /// Install domainCert() into the OS trust store so QSslSocket::systemCaCertificates()
    /// trusts this server, for the REAL_CA_CERT=true path (see TestEnv::realCaCert()). Needs
    /// an elevated process; returns false when not elevated (or the platform tool fails), so
    /// the caller fails the test rather than pretend the trust was established. Cross-platform
    /// (certutil on Windows, security on macOS, update-ca-certificates elsewhere). Defined in
    /// testserver.cpp so this widely-included header stays free of its process/file machinery.
    bool installTrustSystemWide() const;

    /// The CA set a client should trust to reach this server, ALSO installed as the process
    /// default (QSslSocket::setDefaultCaCertificates). Normally the system roots plus this
    /// server's domainCert() (trusted only in this process, no privilege); when
    /// TestEnv::realCaCert() is set, the system roots after installing domainCert() OS-wide via
    /// installTrustSystemWide() (which needs an elevated process, else the test fails). The
    /// optional `otherCerts` are appended. Returns the full set it made the default, so a caller
    /// can re-apply it after clearing the default. Defined in testserver.cpp (uses QSslSocket).
    QList<QSslCertificate> loadCerts(const QList<QSslCertificate> &otherCerts = QList<QSslCertificate>()) const;
#endif

    /// The port the primary service for this "way" is actually listening on -- FTP for
    /// FileTransfer, HTTP for WebProxy, DNS for NameLookup -- and echoPort() the secondary
    /// (TCP/UDP echo for FileTransfer, the SOCKS5 proxy for WebProxy). These reflect the
    /// REAL bound ports after start()/tryPort(): if the child had to fall back (a
    /// requested port was taken or privileged), these hold where it actually landed.
    quint16 port() const { return m_port; }
    quint16 echoPort() const { return m_echoPort; }

    /// Opt-in second TLS listener with a DISTINCT cert (server-dummy's --second-port).
    /// Off by default -- nothing extra is bound unless a test calls this. Lets a client reconnect
    /// to secondPort() and observe a different certificate chain than the primary port. Relaunches
    /// the child if it is already running so it gets the --second-port/--second-cert/--second-key.
    void enableSecondSecurePort()
    {
        if (m_secondSecure)
            return;
        m_secondSecure = true;
        if (isRunning())
            restart();
    }
    /// Distinct, unprivileged, unmapped by port().
    static quint16 secondPort() { return port(1443); }

    /// A second self-signed cert, distinct from serverCert(), for the --second-port listener.
    /// Generated once via openssl (like serverCert()); returns a plain ServerCert (no SSL types),
    /// so it sits outside the QT_NO_SSL guard.
    static const ServerCert &secondServerCert()
    {
        static ServerCert c = generateCert(QLL("server-second"),
            QLL("second-server.test"),
            QLL("subjectAltName=DNS:localhost,IP:127.0.0.1,IP:0:0:0:0:0:0:0:1"));
        return c;
    }

    /// Expose a real filesystem directory through the FTP "way": server-dummy serves its
    /// contents (LIST/RETR/STOR/...) instead of a tiny in-memory FS. Set before start()
    /// (or before the tryPort() that relaunches the child). Passed as --folder=<dir>.
    void setFolder(const QString &dir) { m_folder = dir; if (isRunning()) restart(); }
    QString folder() const { return m_folder; }

    /// A builder to edit this server's server-config.ini between test-cases (see
    /// TestServerEditor): edit().reset().add("IO.rateLimit", "200").save(). The running child's
    /// IniWatcher picks the saved change up on its next poll -- no restart needed.
    TestServerEditor edit() const
    { return TestServerEditor(QDir(m_folder).filePath(QLL("server-config.ini"))); }

    /// Aim the server at newPort. Checks -- cross-platform -- whether this process may
    /// actually bind it (a privileged port < 1024 fails for a non-root user on Linux/macOS;
    /// an in-use or otherwise reserved port fails on every platform). If it cannot, keeps
    /// the port already in use, or the default when none has been chosen yet. (Re)launches
    /// the child on the resulting port and returns it, so the caller connects to exactly
    /// where the server ended up. The secondary/echo port follows as the chosen port + 1
    /// (the child reports its real value, adopted via parseReady()).
    quint16 tryPort(quint16 newPort)
    {
        const quint16 previous = m_port ? m_port : m_defaultPort;
        const quint16 chosen = canBind(newPort) ? newPort : previous;
        if (!isRunning() || chosen != m_port) {
            m_port = chosen;
            m_echoPort = quint16(chosen + 1);
            restart();
        }
        return m_port;
    }

    /// True if this process can bind port on the loopback right now. A throwaway
    /// QTcpServer::listen reports the OS's permission/availability verdict the same way on
    /// every platform: false on EACCES for a privileged port, or when the port is taken.
    static bool canBind(quint16 port)
    {
        if (port == 0)
            return false;
        QTcpServer probe;
        const bool ok = probe.listen(QHostAddress(QHostAddress::LocalHost), port);
        probe.close();
        return ok;
    }

    /// port(canonical) maps a service's canonical, privileged port (the CORRECT one a real
    /// deployment uses -- 443 for https, 80 for http, 21 for ftp, and so on) onto the port the
    /// tests should actually reach that service on right now, whether the connection is direct
    /// or through the fleet's proxy.
    ///
    /// An elevated process (root on Unix, an elevated token on Windows -- see
    /// QCoreApplication::isElevated()) may bind those below-1024 ports, so it gets the canonical
    /// port verbatim. An unprivileged run -- and every memory-safe (Fil-C) run -- cannot bind
    /// them, so it falls back to a high, unprivileged stand-in. The stand-ins below are exactly
    /// what server-dummy's fleet binds, so a client that asks port(443) and a server that
    /// listens on port(443) meet on the same port whether or not the run is elevated -- no remap
    /// layer in between. It also works through a proxy: the fleet's remapTarget() maps
    /// canonical->stand-in and leaves an already-mapped target unchanged (idempotent), so a test
    /// spells one port and always reaches the server.
    ///
    /// Overloaded with the instance port() above: that reports the real bound port of a running
    /// handle; this static form is the compile-time canonical->stand-in map, usable with no
    /// handle in hand (e.g. when building a QUrl before any server is launched).
    static quint16 port(quint16 canonical)
    {
        if (QCoreApplication::isElevated())
            return canonical; // correct port; an elevated process can bind it

        switch (canonical) {
        case 443: return 4433; // https  (correct: 443)
        case 80:  return 8080; // http   (correct: 80)
        case 21:  return 2100; // ftp    (correct: 21)
        case 143: return 1430; // imap   (correct: 143)
        case 993: return 9993; // imaps  (correct: 993)
        case 7:   return 7000; // echo   (correct: 7)
        case 13:  return 1300; // daytime(correct: 13)
        case 53:  return 5300; // dns    (correct: 53)
        default:  return canonical; // already unprivileged (e.g. 3128, 1080)
        }
    }

    /// Launch server-dummy and block until it reports its listeners are up (it prints
    /// "READY") or timeout elapses. Returns false if it never starts or never readies.
    bool start(int timeout = 10000)
    {
        if (!m_proc) {
            m_proc = new ParentDeathProcess(this);
            m_proc->setProcessChannelMode(QProcess::MergedChannels);
        }
        m_ready.clear();
        QStringList arguments;
        arguments << typeArgument(m_type)
                  << QString::number(m_port) << QString::number(m_echoPort);
        if (!m_folder.isEmpty())
            arguments << (QLL("--folder=") + m_folder);
        // Hand the TLS listeners the runtime-generated cert/key (no cert is committed to the tree).
        // Harmless for the non-TLS ways, which ignore it.
        const ServerCert &cert = serverCert();
        if (cert.ok) {
            arguments << (QLL("--cert=") + cert.certPath)
                      << (QLL("--key=") + cert.keyPath);
        }
        // Opt-in extra TLS listener with its own distinct cert (enableSecondSecurePort()).
        if (m_secondSecure) {
            const ServerCert &second = secondServerCert();
            if (second.ok)
                arguments << (QLL("--second-port=") + QString::number(secondPort()))
                          << (QLL("--second-cert=") + second.certPath)
                          << (QLL("--second-key=") + second.keyPath);
        }
        m_proc->start(m_program, arguments);
        if (!m_proc->waitForStarted(timeout))
            return false;
        QElapsedTimer timer;
        timer.start();
        while (!m_ready.contains("READY") && timer.timeLeft(timeout)) {
            m_proc->waitForReadyRead(200);
            m_ready += m_proc->readAll();
            if (m_proc->state() == QProcess::NotRunning) {
                m_ready += m_proc->readAll();
                break;
            }
        }
        if (!m_ready.contains("READY"))
            return false;
        parseReady(); // adopt the child's actually-bound ports (it may have fallen back)
        // Host-name resolution (domainName() -> loopback) is provided by the shared persistent
        // resolver that TestEnv::getServer() ensures via ensureResolver(); no per-type wiring here.
        return true;
    }

    /// Stop the child (if any) and start it again on the current port. tryPort() uses this
    /// when the target port changes.
    bool restart(int timeout = 10000)
    {
        stop();
        return start(timeout);
    }

    /// Terminate the server and block until it is gone (SIGTERM, then SIGKILL).
    void stop(int timeout = 3000)
    {
        if (!m_proc || m_proc->state() == QProcess::NotRunning)
            return;
        m_proc->terminate();
        if (!m_proc->waitForFinished(timeout)) {
            m_proc->kill();
            m_proc->waitForFinished(timeout);
        }
    }

    /// Ask the server to stop without blocking; it exits on its own shortly after.
    void stopLater()
    {
        if (m_proc && m_proc->state() != QProcess::NotRunning)
            m_proc->terminate();
    }

    /// The QRef deleter TestEnv uses: stop the child promptly, then free the handle,
    /// rather than deferring to a deleteLater that a stopped event loop would never run.
    static void dispose(TestServer *server)
    {
        if (server) {
            server->stop();
            delete server;
        }
    }

    /// A connected pair of loopback TCP sockets, both ready to use -- the analogue of
    /// socketpair(2) for tests that want two ends without speaking a protocol. Modelled
    /// on QtRemote's ServerClientPair: a throwaway listener plus a client, joined with
    /// the async accept + processEvents wait rather than waitForNewConnection. The
    /// caller owns and must delete both returned sockets.
    static QPair<QTcpSocket *, QTcpSocket *> createSocketPair(int timeout = 5000)
    {
        QTcpServer listener;
        listener.listen(QHostAddress::LocalHost, 0);
        QTcpSocket *client = new QTcpSocket;
        client->connectToHost(QHostAddress::LocalHost, listener.serverPort());
        QElapsedTimer timer;
        timer.start();
        while (!listener.hasPendingConnections() && timer.timeLeft(timeout))
            QCoreApplication::processEvents(QEventLoop::AllEvents);
        QTcpSocket *server = listener.nextPendingConnection();
        if (server)
            server->setParent(Q_NULLPTR); // outlive the stack listener
        client->waitForConnected(timeout);
        return qMakePair(client, server);
    }

private:
    static QString typeArgument(Type type)
    {
        switch (type) {
        case WebProxy:   return QLL("web");
        case NameLookup: return QLL("dns");
        case MailProxy:  return QLL("imap");
        case WebSecure:  return QLL("https");
        case Fleet:      return QLL("spdy");
        case FileTransfer: break;
        }
        return QLL("ftp");
    }

    /// The child prints "READY <port> <echoPort>" once its listeners are bound. Adopt the
    /// real ports it reports, so if it had to fall back to an ephemeral port the handle --
    /// and therefore the test, via port()/echoPort() -- learns where it actually is.
    void parseReady()
    {
        const int idx = m_ready.indexOf("READY");
        if (idx < 0)
            return;
        const QList<QByteArray> parts = m_ready.mid(idx).simplified().split(' ');
        if (parts.size() >= 2) {
            bool ok = false;
            const quint16 p = parts.at(1).toUShort(&ok);
            if (ok && p)
                m_port = p;
        }
        if (parts.size() >= 3) {
            bool ok = false;
            const quint16 e = parts.at(2).toUShort(&ok);
            if (ok && e)
                m_echoPort = e;
        }
    }

    QString m_program;
    Type m_type;
    QProcess *m_proc;
    QString m_folder;
    quint16 m_defaultPort;
    quint16 m_port;
    quint16 m_echoPort;
    QByteArray m_ready;
    /// Opt-in: also bind server-dummy's --second-port (distinct cert).
    bool m_secondSecure;
};

#endif // TESTSERVER_H
