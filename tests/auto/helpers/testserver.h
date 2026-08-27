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
#include <QtCore/QElapsedTimer>
#include <QtCore/QObject>
#include <QtCore/QPair>
#include <QtCore/QProcess>
#include <QtCore/QString>
#include <QtCore/QStringList>
#include <QtNetwork/QHostAddress>
#include <QtNetwork/QTcpServer>
#include <QtNetwork/QTcpSocket>
#ifndef QT_NO_SSL
#  include <QtNetwork/QSslCertificate>
#endif

#include "../network-settings.h"
#include "test-ports.h"
#ifndef QT_NO_SSL
#  include "server-dummy/spdy_certs.h"
#endif

#if defined(Q_OS_LINUX)
#  include <csignal>
#  include <unistd.h>
#  include <sys/prctl.h>
#endif

// A QProcess that ties the launched child's lifetime to ours. QProcess terminates the
// child on clean teardown (TestServer::stop() below), but if the *test* process itself
// dies abnormally -- a crash, a harness timeout, an outright SIGKILL -- that path never
// runs, and server-dummy would be reparented to init (PPID 1) and linger for as long as
// the box stays up, still holding its listening ports and blocking the next run. Rather
// than bake a raw prctl() into the server binary, we use QProcess's own child-setup hook
// -- setupChildProcess() runs in the forked child, after chdir and just before exec --
// which is exactly where XD's QProcess lets a caller adjust the about-to-be-exec'd child.
// On Linux we ask the kernel to send it SIGTERM the instant we go away, and close the
// race where we had already died before the call (getppid() == 1). setupChildProcess()
// runs post-fork, so only async-signal-safe calls are allowed -- prctl/getppid/_exit all
// qualify. Best-effort and Linux-only; every other platform relies on the stop() path.
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

// A handle to the bundled test server (helpers/server-dummy). It does not implement
// any protocol itself: TestEnv hands it the absolute path of the server-dummy binary,
// and this class runs that binary as a QProcess for one "way" (Type), then terminates
// it when the handle is destroyed, or when stop()/stopLater() is called. TestEnv::
// getServer() owns one through a QRef, so the server lives exactly as long as the ref.
//
// Modelled on the stable/testable server in QtRemote's tests (tests/auto/remote/
// dummy-server.h): start() blocks only until the child says it is listening -- the
// child prints "READY" once its listeners are bound -- so a test can then drive a
// blocking client against it without racing the server's startup.
//
// Being a QObject, stop() is usable as a slot target from the Qt5 connect() syntax,
// e.g. connect(someObject, &Some::signal, server.data(), &TestServer::stop).
class TestServer : public QObject
{
public:
    enum Type {
        FileTransfer, // FTP control/PASV + TCP/UDP echo        (server-dummy "ftp")
        WebProxy,     // HTTP/1.1 responder + SOCKS5 proxy      (server-dummy "web")
        NameLookup,   // DNS A-record responder                (server-dummy "dns")
        MailProxy     // IMAP-ish greeter + SOCKS5 auth proxy   (server-dummy "imap")
    };

    explicit TestServer(const QString &serverDummyPath, Type type,
                        quint16 defaultPort = 45678, QObject *parent = Q_NULLPTR)
        : QObject(parent), m_program(serverDummyPath), m_type(type), m_proc(Q_NULLPTR),
          m_defaultPort(defaultPort), m_port(defaultPort), m_echoPort(quint16(defaultPort + 1))
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

    /// The host a client should connect to in order to reach this server: plain 127.0.0.1
    /// on a loopback-only run, else the fake host name (see QtNetworkSettings::serverName(),
    /// which makes exactly this choice). server-dummy's certificate carries the fake name
    /// plus localhost/127.0.0.1/::1 in its SANs, so verification succeeds either way.
    QString domainName() const { return QtNetworkSettings::serverName(); }

#ifndef QT_NO_SSL
    /// The certificate a client adds to trust this server (server-dummy presents the matching
    /// key). One multi-SAN self-signed certificate covers both host forms domainName() returns,
    /// so trusting it verifies the server whether the run is on loopback or the fake name.
    QSslCertificate domainCert() const
    {
        return QSslCertificate(QByteArray(kServerCertPem), QSsl::Pem);
    }
#endif

    // The port the primary service for this "way" is actually listening on -- FTP for
    // FileTransfer, HTTP for WebProxy, DNS for NameLookup -- and echoPort() the secondary
    // (TCP/UDP echo for FileTransfer, the SOCKS5 proxy for WebProxy). These reflect the
    // REAL bound ports after start()/tryPort(): if the child had to fall back (a
    // requested port was taken or privileged), these hold where it actually landed.
    int port() const { return m_port; }
    int echoPort() const { return m_echoPort; }

    // Expose a real filesystem directory through the FTP "way": server-dummy serves its
    // contents (LIST/RETR/STOR/...) instead of a tiny in-memory FS. Set before start()
    // (or before the tryPort() that relaunches the child). Passed as --folder=<dir>.
    void setFolder(const QString &dir) { m_folder = dir; if (isRunning()) restart(); }
    QString folder() const { return m_folder; }

    // Aim the server at newPort. Checks -- cross-platform -- whether this process may
    // actually bind it (a privileged port < 1024 fails for a non-root user on Linux/macOS;
    // an in-use or otherwise reserved port fails on every platform). If it cannot, keeps
    // the port already in use, or the default when none has been chosen yet. (Re)launches
    // the child on the resulting port and returns it, so the caller connects to exactly
    // where the server ended up. The secondary/echo port follows as the chosen port + 1
    // (the child reports its real value, adopted via parseReady()).
    int tryPort(int newPort)
    {
        const quint16 previous = m_port ? m_port : m_defaultPort;
        const quint16 chosen = canBind(newPort) ? quint16(newPort) : previous;
        if (!isRunning() || chosen != m_port) {
            m_port = chosen;
            m_echoPort = quint16(chosen + 1);
            restart();
        }
        return m_port;
    }

    // True if this process can bind port on the loopback right now. A throwaway
    // QTcpServer::listen reports the OS's permission/availability verdict the same way on
    // every platform: false on EACCES for a privileged port, or when the port is taken.
    static bool canBind(int port)
    {
        if (port <= 0 || port > 65535)
            return false;
        QTcpServer probe;
        const bool ok = probe.listen(QHostAddress(QHostAddress::LocalHost), quint16(port));
        probe.close();
        return ok;
    }

    // Launch server-dummy and block until it reports its listeners are up (it prints
    // "READY") or timeout elapses. Returns false if it never starts or never readies.
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
            arguments << (QStringLiteral("--folder=") + m_folder);
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
        return true;
    }

    // Stop the child (if any) and start it again on the current port. tryPort() uses this
    // when the target port changes.
    bool restart(int timeout = 10000)
    {
        stop();
        return start(timeout);
    }

    // Terminate the server and block until it is gone (SIGTERM, then SIGKILL).
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

    // Ask the server to stop without blocking; it exits on its own shortly after.
    void stopLater()
    {
        if (m_proc && m_proc->state() != QProcess::NotRunning)
            m_proc->terminate();
    }

    // The QRef deleter TestEnv uses: stop the child promptly, then free the handle,
    // rather than deferring to a deleteLater that a stopped event loop would never run.
    static void dispose(TestServer *server)
    {
        if (server) {
            server->stop();
            delete server;
        }
    }

    // A connected pair of loopback TCP sockets, both ready to use -- the analogue of
    // socketpair(2) for tests that want two ends without speaking a protocol. Modelled
    // on QtRemote's ServerClientPair: a throwaway listener plus a client, joined with
    // the async accept + processEvents wait rather than waitForNewConnection. The
    // caller owns and must delete both returned sockets.
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
        case WebProxy:   return QStringLiteral("web");
        case NameLookup: return QStringLiteral("dns");
        case MailProxy:  return QStringLiteral("imap");
        case FileTransfer: break;
        }
        return QStringLiteral("ftp");
    }

    // The child prints "READY <port> <echoPort>" once its listeners are bound. Adopt the
    // real ports it reports, so if it had to fall back to an ephemeral port the handle --
    // and therefore the test, via port()/echoPort() -- learns where it actually is.
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
};

#endif // TESTSERVER_H
