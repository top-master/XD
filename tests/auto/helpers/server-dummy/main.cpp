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


/// @file
/// The bundled test server, run as its own process. A test does not link this; it
/// launches it through TestServer (helpers/testserver.h), which is handed this binary's
/// absolute path by TestEnv and runs it as a QProcess. It implements each "way" the
/// network tests need, selected by the first argument, as a set of protocol services --
/// one per header here, each a TestService subclass (see service_base.h):
///
///   ftp   -> FtpService + EchoService        (ftp_service.h, echo_service.h)
///   web   -> HttpService + SocksService      (http_service.h, socks_service.h)
///   dns   -> DnsService                      (dns_service.h)
///   imap  -> ImapService (+ SocksService)    (imap_service.h)
///   https -> HttpsService                    (https_spdy.h)
///   spdy  -> the full TLS/SPDY/HTTP2 fleet    (https_spdy.h)
///
/// TestService owns each listener's host and wanted port, binds with an ephemeral
/// fallback (a privileged or taken port degrades gracefully), and remembers the port it
/// actually got; the primary/secondary ports arrive as argv[2]/argv[3] and are echoed in
/// the READY line so the handle learns where the listeners are. Connections use the async
/// newConnection/nextPendingConnection pair, never the synchronous
/// QTcpServer::waitForNewConnection, which does not accept reliably under Fil-C. Handlers
/// are plain member functions wired through the pointer-to-member connect() overload --
/// no lambdas (MSVC 2010 rejects a lambda nested in a lambda) and no moc.

#include <cstdio>
#include <csignal>
#include <cstdlib>

#include <QtCore/QCoreApplication>
#include <QtCore/QDateTime>
#include <QtCore/QDir>
#include <QtCore/QFile>
#include <QtCore/QMap>
#include <QtCore/QSettings>
#include <QtCore/QString>
#include <QtCore/QStringList>
#include <QtCore/QTextStream>
#include <QtCore/QTimer>
#include <QtNetwork/QHostAddress>

#include "../testenv.h"
#include "../test-ports.h"
#include "ftp_service.h"
#include "echo_service.h"
#include "http_service.h"
#include "socks_service.h"
#include "dns_service.h"
#include "imap_service.h"
#include "https_spdy.h"

// MARK: main: build the services for the requested "way" and report the real ports.

/// Remove the FTP upload scratch dir on shutdown, so a test leaves nothing behind (TestServer stops us
/// with SIGTERM; the kernel PR_SET_PDEATHSIG is also SIGTERM). Path is captured at startup so the
/// handler needn't call into Qt. removeRecursively() isn't strictly async-signal-safe, but the process
/// is exiting and this is a test helper. A hard SIGKILL fallback can still leave the dir, but it is
/// under the system temp dir (never the source tree) and gets reaped by the OS.
/// The TLS listeners' cert+key (declared extern in https_spdy.h), read at startup from the
/// --cert/--key paths TestServer passes. Empty until then; no cert is compiled in.
QByteArray g_serverCertPem;
QByteArray g_serverKeyPem;

/// Last DNS query answered (ms since epoch); updated by DnsService, read by dnsdKeepaliveTick.
qint64 g_dnsLastActivityMs = 0;

/// Absolute path of the --folder's server-config.ini (empty when no --folder). The shared
/// IniWatcher (dynamic_config.h) watches this path; each service reads the live [Unreachable] /
/// [IO] state through watcher()->load(), and re-reads on change via onIniChange().
QString g_configIniPath;

static char g_uploadScratch[4096];
extern "C" void serverDummyCleanup(int)
{
    if (g_uploadScratch[0])
        QDir(QString::fromLocal8Bit(g_uploadScratch)).removeRecursively();
    ::_exit(0);
}

/// The persistent DNS resolver (dnsd mode) self-terminates once no test has refreshed its
/// keepalive deadline for 5 minutes. TestServer::ensureResolver() rewrites dns/lastLaunch in
/// test-env.ini on every start(), so a busy run keeps this one shared daemon alive while an idle
/// box lets it exit on its own. A plain function (no lambda/moc) so it can be wired to a QTimer
/// under the same MSVC2010 rules the services follow.
static void dnsdKeepaliveTick()
{
    QSettings &s = TestEnv::settings();
    s.sync(); // pick up dns/lastLaunch writes made by the test processes
    const qint64 launched = s.value(QLL("dns/lastLaunch")).toLongLong();
    const qint64 nowMs = QDateTime::currentMSecsSinceEpoch();
    const qint64 now = nowMs / 1000;
    // Stay alive while there is recent activity: either a test process refreshed dns/lastLaunch
    // (a new TestServer start), OR this resolver answered a query recently. A single long test
    // binary (tst_QNetworkReply runs ~15-20 min) may not touch lastLaunch again after initTestCase,
    // but it keeps resolving qt-local-server.test as QHostInfo's short cache expires, so query
    // activity is what actually keeps the shared daemon up. Exit only after a genuine idle gap.
    const qint64 lastActivity = qMax(launched, g_dnsLastActivityMs / 1000);
    if (lastActivity == 0 || now - lastActivity > 600)
        QCoreApplication::quit();
}

/// The single factory: build the concrete listener for a ServiceConfig. Every "way" and the opt-in
/// --second-port create their listeners through this, so one place maps Kind -> TestService
/// subclass. Non-SSL kinds ignore the cert fields; the TLS kinds fall back to the process-wide
/// g_serverCertPem/g_serverKeyPem when the config's are empty (see HttpsService).
static TestService *newService(const ServiceConfig &c, QObject *parent)
{
    switch (c.kind) {
    case TestService::Ftp:          return new FtpService(c, parent);
    case TestService::Echo:         return new EchoService(c, parent);
    case TestService::Http:         return new HttpService(c, parent);
    case TestService::Https:        return new HttpsService(c, parent);
    case TestService::Socks:        return new SocksService(c, parent);
    case TestService::Dns:          return new DnsService(c, parent);
    case TestService::Imap:         return new ImapService(c, parent);
    case TestService::Imaps:        return new ImapsService(c, parent);
    case TestService::PlainHttp:    return new PlainHttpService(c, parent);
    case TestService::Daytime:      return new DaytimeService(c, parent);
    case TestService::ConnectProxy: return new ConnectProxyService(c, parent);
    case TestService::SocksProxy:   return new SocksProxyService(c, parent);
    }
    return Q_NULLPTR;
}

/// A ServiceConfig with the optional fields defaulted (ServiceConfig itself carries no NSDMI, to
/// stay MSVC-2010-clean). Callers override cert / requireAuth / qtestAtRoot as needed.
static ServiceConfig cfg(TestService::Kind kind, const QHostAddress &host, quint16 port,
                        const QString &folder = QString())
{
    ServiceConfig c;
    c.kind = kind;
    c.host = host;
    c.port = port;
    c.folder = folder;
    c.requireAuth = false;
    c.qtestAtRoot = false;
    c.ftpProxy = false;
    return c;
}

/// Bind the opt-in second TLS listener (distinct-cert twin of the primary secure port) through
/// newService. No-op when --second-port was not given, so nothing extra is ever "always up".
static void startSecond(const QHostAddress &host, quint16 secondPort, const QString &folder,
                        const QByteArray &certPem, const QByteArray &keyPem, QObject *parent)
{
    if (!secondPort)
        return;
    ServiceConfig c = cfg(TestService::Https, host, secondPort, folder);
    c.certPem = certPem;
    c.keyPem = keyPem;
    newService(c, parent)->start();
}

int main(int argc, char **argv)
{
    QCoreApplication app(argc, argv);

    qstrncpy(g_uploadScratch, FtpService::uploadScratchDir().toLocal8Bit().constData(), sizeof(g_uploadScratch));
    std::signal(SIGTERM, serverDummyCleanup);
    std::signal(SIGINT, serverDummyCleanup);

    // Positional args are: <way> <port> <echoPort>; options (e.g. --folder=<dir>, which
    // makes the FTP server expose a real directory) may appear anywhere.
    // --second-port/--second-cert/--second-key add ONE extra TLS listener alongside whatever
    // the "way" set up, serving a DISTINCT certificate -- so a client can observe a different
    // certificate chain than the primary port (tst_QSslSocket::peerCertificateChain). It is
    // opt-in: absent these options, no second listener is bound (nothing is "always up").
    QStringList positional;
    QString folder;
    quint16 secondPort = 0;
    QByteArray secondCertPem;
    QByteArray secondKeyPem;
    for (int i = 1; i < argc; ++i) {
        const QString a = QString::fromLocal8Bit(argv[i]);
        if (a.startsWith(QLL("--folder=")))
            folder = a.mid(9);
        else if (a.startsWith(QLL("--cert="))) {
            QFile f(a.mid(7));
            if (f.open(QIODevice::ReadOnly)) { g_serverCertPem = f.readAll(); f.close(); }
        } else if (a.startsWith(QLL("--key="))) {
            QFile f(a.mid(6));
            if (f.open(QIODevice::ReadOnly)) { g_serverKeyPem = f.readAll(); f.close(); }
        } else if (a.startsWith(QLL("--second-port="))) {
            secondPort = quint16(a.mid(14).toUShort());
        } else if (a.startsWith(QLL("--second-cert="))) {
            QFile f(a.mid(14));
            if (f.open(QIODevice::ReadOnly)) { secondCertPem = f.readAll(); f.close(); }
        } else if (a.startsWith(QLL("--second-key="))) {
            QFile f(a.mid(13));
            if (f.open(QIODevice::ReadOnly)) { secondKeyPem = f.readAll(); f.close(); }
        } else
            positional << a;
    }

    // Point the shared IniWatcher at this folder's server-config.ini (if any). Nothing is
    // parsed until a service calls watcher()->load(); changes are then picked up live.
    if (!folder.isEmpty())
        g_configIniPath = QDir(folder).filePath(QLL("server-config.ini"));

    const QString which = positional.value(0).toLower();

    // A persistent, shared DNS resolver for host-name lookups -- qt-local-server.test
    // (TestServer::domainName()) and the legacy qt-test-server.qt-test-net -- which XD's QHostInfo is
    // pointed at through a QHostInfoOverride. TestServer::ensureResolver() launches it detached so it
    // outlives any single test; it exits ~5 min after the last TestServer refreshed the keepalive.
    if (which == QLL("dnsd")) {
        const QHostAddress host = QHostAddress::LocalHost;
        DnsService *dns = new DnsService(cfg(TestService::Dns, host, TestServer::port(53)), &app);
        dns->start();
        if (dns->port() != TestServer::port(53)) {
            // Another resolver already owns the port (bindUdp fell back to an ephemeral one):
            // don't run a duplicate; the existing daemon serves everyone.
            return 0;
        }
        QTimer *watch = new QTimer(&app);
        watch->setInterval(20000);
        QObject::connect(watch, &QTimer::timeout, &dnsdKeepaliveTick);
        watch->start();
        std::fputs("READY dnsd\n", stdout);
        std::fflush(stdout);
        return app.exec();
    }

    if (which == QLL("spdy")) {
        // The tst_spdy harness needs a whole fleet of services. Each binds its
        // CORRECT (privileged) port when this process is elevated -- https on 443,
        // http on 80, ftp on 21, imap(s) on 143/993, echo on 7, daytime on 13 --
        // and an unprivileged stand-in otherwise (TestServer::port(): 4433/8080/2100/...).
        // A client that asks TestServer::port() for the same service meets it on the same
        // port, so an unelevated run needs no port-remap layer at all. The HTTP
        // CONNECT proxies (3128/3129 auth) and SOCKS5 proxies (1080/1081 auth) are
        // already unprivileged, so they keep their fixed ports.
        const QHostAddress host = QHostAddress::LocalHost;
        newService(cfg(TestService::Https, host, TestServer::port(443), folder), &app)->start(); // primary HTTPS
        startSecond(host, secondPort, folder, secondCertPem, secondKeyPem, &app);
        newService(cfg(TestService::Imaps, host, TestServer::port(993)), &app)->start(); // imaps
        newService(cfg(TestService::Imap, host, TestServer::port(143)), &app)->start();  // plain imap
        newService(cfg(TestService::Echo, host, TestServer::port(7)), &app)->start();    // tcp+udp echo
        { ServiceConfig c = cfg(TestService::Ftp, host, TestServer::port(21), folder);
          c.qtestAtRoot = true; newService(c, &app)->start(); }  // ftp control (flat folder as /qtest)
        newService(cfg(TestService::Daytime, host, TestServer::port(13)), &app)->start();
        newService(cfg(TestService::PlainHttp, host, TestServer::port(80), folder), &app)->start();
        // Host-name resolution (qt-local-server.test / qt-test-server.qt-test-net -> loopback) is
        // served by the shared persistent resolver (dnsd mode), which TestServer::ensureResolver()
        // guarantees is up and points XD's QHostInfo at -- so the fleet needs no in-process DNS.
        newService(cfg(TestService::ConnectProxy, host, 3128), &app)->start();
        { ServiceConfig c = cfg(TestService::ConnectProxy, host, 3129); c.requireAuth = true; newService(c, &app)->start(); }
        newService(cfg(TestService::SocksProxy, host, 1080), &app)->start();
        { ServiceConfig c = cfg(TestService::SocksProxy, host, 1081); c.requireAuth = true; newService(c, &app)->start(); }
        // Terminating FTP proxy on the fixed port the ftp-proxy rows use (serves the same fixture
        // locally after stripping the "user@host" Qt's FTP backend sends through a proxy).
        { ServiceConfig c = cfg(TestService::Ftp, host, 2121, folder); c.qtestAtRoot = true; c.ftpProxy = true; newService(c, &app)->start(); }
        std::fputs("READY spdy\n", stdout);
        std::fflush(stdout);
        return app.exec();
    }

    TestServer::Type type = TestServer::FileTransfer;
    if (which == QLL("web"))
        type = TestServer::WebProxy;
    else if (which == QLL("dns"))
        type = TestServer::NameLookup;
    else if (which == QLL("imap"))
        type = TestServer::MailProxy;
    else if (which == QLL("https"))
        type = TestServer::WebSecure;

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
        primary = newService(cfg(TestService::Ftp, host, wantPort, folder), &app);
        secondary = newService(cfg(TestService::Echo, host, wantEcho), &app);
        // Terminating FTP proxy on 2121 for tst_QFtp::proxy() -- same fixture, served locally.
        { ServiceConfig c = cfg(TestService::Ftp, host, 2121, folder); c.ftpProxy = true; newService(c, &app)->start(); }
        break;
    case TestServer::WebProxy:
        primary = newService(cfg(TestService::Http, host, wantPort), &app);
        secondary = newService(cfg(TestService::Socks, host, wantEcho), &app);
        // The proxy rows of the access tests reach the origin above THROUGH a proxy on a
        // fixed, unprivileged port: HTTP CONNECT/forward proxies (3128, and 3129 with Basic
        // auth) and SOCKS5 proxies (1080, and 1081 with user/pass), the same set the spdy
        // fleet exposes. remapTarget() forwards an already-unprivileged target port (the
        // origin's) unchanged, so these relay straight back to primary above.
        newService(cfg(TestService::ConnectProxy, host, 3128), &app)->start();
        { ServiceConfig c = cfg(TestService::ConnectProxy, host, 3129); c.requireAuth = true; newService(c, &app)->start(); }
        newService(cfg(TestService::SocksProxy, host, 1080), &app)->start();
        { ServiceConfig c = cfg(TestService::SocksProxy, host, 1081); c.requireAuth = true; newService(c, &app)->start(); }
        // Terminating FTP proxy on 2121 for the ftp-through-proxy rows (see the spdy fleet above).
        { ServiceConfig c = cfg(TestService::Ftp, host, 2121, folder); c.qtestAtRoot = true; c.ftpProxy = true; newService(c, &app)->start(); }
        break;
    case TestServer::NameLookup:
        primary = newService(cfg(TestService::Dns, host, wantPort), &app);
        break;
    case TestServer::MailProxy:
        primary = newService(cfg(TestService::Imap, host, wantPort), &app);
        secondary = newService(cfg(TestService::Socks, host, wantEcho), &app);
        break;
    case TestServer::WebSecure:
        primary = newService(cfg(TestService::Https, host, wantPort, folder), &app);
        break;
    }
    if (primary)
        primary->start();
    if (secondary)
        secondary->start();
    // Opt-in extra TLS listener with its own cert (--second-port), through newService.
    startSecond(host, secondPort, folder, secondCertPem, secondKeyPem, &app);

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
