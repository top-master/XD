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

#ifndef TESTENV_H
#define TESTENV_H

#include <QCoreApplication>
#include <QDateTime>
#include <QDir>
#include <QElapsedTimer>
#include <QFile>
#include <QFileInfo>
#include <QProcess>
#include <QSettings>
#include <QSharedPointer>
#include <QString>
#include <QThread>
#include <QtNetwork/QHostAddress>
#include <QtNetwork/QTcpSocket>
#include <QtNetwork/QUdpSocket>
#include <QtNetwork/QHostInfoOverride>
#include <QtNetwork/private/qhostinfo_p.h>

#include "testserver.h"

/// Shared reader for the test-env.ini that auto.pro drops at the build tree root.
/// A test finds the file by walking up from its own executable's directory, so any
/// test at any depth reads the same settings, and points the network tests at the
/// bundled dummy server (localhost:45678 by default) instead of a real host.
class TestEnv
{
public:
    /// Absolute path of the nearest test-env.ini at or above the running test's
    /// directory, or an empty string if none is found.
    static QString iniPath()
    {
        QDir dir(QCoreApplication::applicationDirPath());
        forever {
            const QString candidate = dir.filePath(QLL("test-env.ini"));
            if (QFileInfo::exists(candidate))
                return candidate;
            if (!dir.cdUp())
                return QString();
        }
    }

    static QSettings &settings()
    {
        static QSettings instance(iniPath(), QSettings::IniFormat);
        return instance;
    }

    /// Host the dummy test server listens on; localhost when the .ini is absent.
    static QString serverHost()
    {
        return settings().value(QLL("server/host"),
                                QStringLiteral("localhost")).toString();
    }

    /// Base TCP/UDP port the dummy test server listens on; 45678 by default. The
    /// server offers FTP on this port and a plain echo on port + 1.
    static quint16 serverPort()
    {
        return quint16(settings().value(QLL("server/port"), 45678).toUInt());
    }

    /// Whether server-dummy's certificate should be trusted OS-wide rather than only inside
    /// the running test process. Off by default (test-env.ini "REAL_CA_CERT=true" turns it
    /// on): the process-only path needs no privilege, whereas the OS-wide install needs an
    /// elevated process. Accepts the key at the file top or under [server]. The install
    /// itself is a server action: TestServer::installTrustSystemWide() (in testserver.cpp).
    static bool realCaCert()
    {
        QSettings &s = settings();
        return s.value(QLL("server/REAL_CA_CERT"),
                       s.value(QLL("REAL_CA_CERT"), false)).toBool();
    }

    /// Resolved listen/connect address for the dummy server ("localhost" -> loopback).
    static QHostAddress serverAddress()
    {
        const QString h = serverHost();
        return QHostAddress(h == QLL("localhost")
                            ? QLL("127.0.0.1") : h);
    }

    /// Absolute path of the bundled server-dummy binary. Taken from the "server/dummy"
    /// key if the .ini sets one, otherwise derived from the .ini's own location: it
    /// sits beside the tests at helpers/server-dummy/ in the same build tree.
    static QString serverDummyPath()
    {
        const QString custom = settings().value(QLL("server/dummy")).toString();
        if (!custom.isEmpty())
            return custom;
        const QString ini = iniPath();
        if (ini.isEmpty())
            return QString();
        QString exe = QFileInfo(ini).absoluteDir()
                          .filePath(QLL("helpers/server-dummy/server-dummy"));
#ifdef Q_OS_WIN
        exe += QLL(".exe");
#endif
        return exe;
    }

    /// Path to the bearer-dummy daemon (pairs with TestBearer), analogous to serverDummyPath().
    static QString bearerDummyPath()
    {
        const QString custom = settings().value(QLL("bearer/dummy")).toString();
        if (!custom.isEmpty())
            return custom;
        const QString ini = iniPath();
        if (ini.isEmpty())
            return QString();
        QString exe = QFileInfo(ini).absoluteDir()
                          .filePath(QLL("helpers/bearer-dummy/bearer-dummy"));
#ifdef Q_OS_WIN
        exe += QLL(".exe");
#endif
        return exe;
    }

    /// Directory holding the qbearerdummy engine plugin's "bearer/" subdir, from the [bearer]
    /// pluginpath the .pri recorded at qmake time (the Qt install's plugins dir). Empty if unset.
    static QString bearerPluginPath()
    {
        return settings().value(QLL("bearer/pluginpath")).toString();
    }

    /// Launch the bundled server-dummy for one of the network "ways" and hand back an
    /// owning ref. The TestServer runs server-dummy as a QProcess and, when the last
    /// QRef drops (or stop()/stopLater() is called), terminates it. QtTest runs test
    /// functions serially, so a single server on the shared port is enough and the
    /// port never has to change between tests.
    static QRef<TestServer> getServer(TestServer::Type type, const QString &folder = QString())
    {
        QRef<TestServer> server(new TestServer(serverDummyPath(), type, serverPort()),
                                &TestServer::dispose);
        if (!folder.isEmpty())
            server->setFolder(folder); // expose a real directory (e.g. so /qtest/<file> is served)
        server->start();
        // Every server (except a NameLookup, which is itself a DNS server the qdnslookup test drives
        // directly) reaches its origin by a host NAME -- TestServer::domainName() == qt-local-server.test,
        // or the legacy qt-test-server.qt-test-net -- to preserve the upstream tests' name-based nature.
        // Guarantee the shared resolver that maps those names to loopback is up and point XD at it.
        if (type != TestServer::NameLookup)
            ensureResolver();
        return server;
    }

    /// Whether a UDP DNS resolver is already bound to `port` on loopback. A TCP canBind() would miss
    /// it (DNS is UDP), so probe with a UDP bind: if we can take the port, nothing is there.
    static bool resolverRunning(quint16 port)
    {
        QUdpSocket probe;
        const bool bound = probe.bind(QHostAddress(QHostAddress::LocalHost), port);
        probe.close();
        return !bound;
    }

    /// Make sure the shared persistent DNS resolver (server-dummy "dnsd") is running and point XD's
    /// QHostInfo at it via a QHostInfoOverride, so a server host name resolves to loopback with no
    /// root, /etc/hosts or resolv.conf. The daemon is launched detached so it outlives any one test and
    /// self-terminates 5 min after the last refresh; rewriting the dns/lastLaunch keepalive here keeps a
    /// single daemon alive across a busy run rather than paying a fresh DNS launch per test.
    static void ensureResolver()
    {
        const quint16 dnsPort = TestServer::port(53);
        settings().setValue(QLL("dns/lastLaunch"),
                            qint64(QDateTime::currentMSecsSinceEpoch() / 1000));
        settings().sync();
        if (!resolverRunning(dnsPort)) {
            QProcess::startDetached(serverDummyPath(), QStringList()
                << QLL("dnsd")
                << QString::number(dnsPort) << QString::number(dnsPort + 1));
            QElapsedTimer t;
            t.start();
            while (!resolverRunning(dnsPort) && t.elapsed() < 3000)
                QThread::msleep(25);
        }
        // Point XD's QHostInfo at the loopback dnsd via a QHostInfoOverride (installed once, on the
        // per-application lookup manager), so a server host name resolves to loopback with no root,
        // /etc/hosts or resolv.conf -- and cross-platform, unlike the old env-var hook.
        QHostInfoLookupManager *mgr =
            qobject_cast<QHostInfoLookupManager *>(QAbstractHostInfoLookupManager::globalInstance());
        if (mgr && !mgr->override())
            mgr->setOverride(QSharedPointer<QHostInfoOverride>::create(
                                 QHostAddress(QHostAddress::LocalHost), dnsPort));
    }

    /// Number of logical CPUs (never below 1).
    static int cpuCount()
    {
        const int n = QThread::idealThreadCount();
        return n > 0 ? n : 1;
    }

    /// True if this box has outbound internet. A test that must reach a REAL external host (outside
    /// server-dummy / loopback / the injected test domain) gates on this: internet up but the host
    /// unreachable is a genuine FAILURE (never masked), whereas no internet is a QSKIP. The probe is
    /// a RAW TCP connect to a well-known public IP -- NOT QHostInfo, because the suite points
    /// QHostInfo (via a QHostInfoOverride) at server-dummy's dnsd, which knows only the test hosts
    /// and answers NXDOMAIN for public names, so a resolve-based check would wrongly report "no internet".
    static bool hasInternet()
    {
        static const char *const probes[] = { "1.1.1.1", "8.8.8.8" }; // public DNS anycast, always-up
        for (unsigned i = 0; i < sizeof(probes) / sizeof(probes[0]); ++i) {
            QTcpSocket s;
            s.connectToHost(QString::fromLatin1(probes[i]), 443);
            if (s.waitForConnected(3000)) { s.abort(); return true; }
            s.abort();
        }
        return false;
    }

    /// Approximate per-core clock in GHz. Prefers the nominal top clock from sysfs
    /// (/sys/.../cpufreq/cpuinfo_max_freq, in kHz) over /proc/cpuinfo's "cpu MHz" (which a governor
    /// may have scaled down); falls back to the reference 3.0 GHz where neither is readable (Windows/
    /// macOS, or a container without cpufreq), so timeout() then keys off the core count alone.
    static double cpuGhz()
    {
        QFile maxf(QLL("/sys/devices/system/cpu/cpu0/cpufreq/cpuinfo_max_freq"));
        if (maxf.open(QIODevice::ReadOnly)) {
            const double khz = maxf.readLine().trimmed().toDouble();
            if (khz > 0.0)
                return khz / 1.0e6; // kHz -> GHz
        }
        QFile info(QLL("/proc/cpuinfo"));
        if (info.open(QIODevice::ReadOnly)) {
            while (!info.atEnd()) {
                const QByteArray line = info.readLine();
                if (line.startsWith("cpu MHz")) {
                    const int c = line.indexOf(':');
                    if (c > 0) {
                        const double mhz = line.mid(c + 1).trimmed().toDouble();
                        if (mhz > 0.0)
                            return mhz / 1000.0; // MHz -> GHz
                    }
                    break;
                }
            }
        }
        return 3.0; // reference default when the clock can't be read
    }

    /// A wall-clock timeout in ms, scaled to the current CPU capacity. `ms` is the budget calibrated
    /// for a reference capacity of 8 cores at 3 GHz each (8 * 3 = 24 "GHz-cores"); below that this
    /// returns a proportionally LARGER timeout, so a fixed budget calibrated for the reference does
    /// not spuriously expire at lower capacity (e.g. tst_QTcpSocket::partialRead's IMAP-greeting wait
    /// can exceed the hard-coded 5000 ms). At or above the reference capacity `ms` is returned
    /// unchanged -- the tight budget is kept rather than loosened.
    static int timeout(int ms)
    {
        const double refGhzCores = 8.0 * 3.0;      // reference: 8 cores * 3.0 GHz
        const double cur = cpuCount() * cpuGhz();  // this box: cores * GHz
        if (cur > 0.0 && cur < refGhzCores)
            return int(ms * (refGhzCores / cur));
        return ms;
    }
};

#endif // TESTENV_H
