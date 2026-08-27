/// @file
/// TestBearer -- the launcher half of the TestBearer / bearer-dummy pair, exactly as TestServer
/// pairs with server-dummy. It starts the bearer-dummy daemon and points the qbearerdummy engine
/// plugin at it via $QT_BEARER_DUMMY_PORT, so a headless Qt process gets a controllable set of
/// activatable network configurations (otherwise there are none and every bearer test skips /
/// QNetworkSession has nothing to open).
///
/// IMPORTANT: construct + start() BEFORE the first QNetworkConfigurationManager in the process --
/// the bearer engine reads $QT_BEARER_DUMMY_PORT once, when the manager first loads it.

#ifndef TESTBEARER_H
#define TESTBEARER_H

#include "testenv.h"     // TestEnv::bearerDummyPath()
#include "testserver.h"  // ParentDeathProcess (child dies with us)

#include <QtCore/QByteArray>
#include <QtCore/QDir>
#include <QtCore/QElapsedTimer>
#include <QtCore/QFile>
#include <QtCore/QLibraryInfo>
#include <QtCore/QObject>
#include <QtCore/QProcess>
#include <QtCore/QStringList>

/// No Q_OBJECT: TestBearer declares no signals/slots of its own (it drives the daemon through
/// ParentDeathProcess's blocking waitFor* calls), so it needs no meta-object -- and being header-only
/// it would otherwise require moc wiring in every including .pro.
class TestBearer : public QObject
{
public:
    explicit TestBearer(QObject *parent = Q_NULLPTR)
        : QObject(parent), m_program(TestEnv::bearerDummyPath()), m_proc(Q_NULLPTR) {}
    ~TestBearer() { stop(); }

    /// Launch the daemon, wait for its "READY <port>" line, and publish the port so the engine
    /// plugin (loaded later by QNetworkConfigurationManager) connects to it. Returns false if the
    /// daemon never starts or never readies.
    bool start(int timeout = 15000)
    {
        if (m_program.isEmpty())
            return false;
        // The qbearerdummy engine plugin is installed with the other Qt plugins (<prefix>/plugins/
        // bearer/), but a headless test's default plugin search does not reach it: this Qt's
        // configured prefix is relative to the app dir, not the plugins dir. QT_PLUGIN_PATH is read
        // only at QCoreApplication construction (too late from here), so add the plugins dir as a
        // runtime library path -- which does refresh the bearer QFactoryLoader -- before the first
        // QNetworkConfigurationManager loads the bearer engines. Prefer the [bearer] pluginpath the
        // .pri recorded at qmake time; fall back to QLibraryInfo.
        QString pluginDir = TestEnv::bearerPluginPath();
        if (pluginDir.isEmpty())
            pluginDir = QLibraryInfo::location(QLibraryInfo::PluginsPath);
        if (!pluginDir.isEmpty() && QDir(pluginDir).exists()) {
            // This process: addLibraryPath is effective immediately (QT_PLUGIN_PATH is read only at
            // QCoreApplication construction, too late from here).
            QCoreApplication::addLibraryPath(pluginDir);
            // Child processes (e.g. qnetworksession's lackey): they construct their own
            // QCoreApplication, so they DO honour QT_PLUGIN_PATH -- publish it so they can load the
            // qbearerdummy engine and reach the same daemon.
            const QByteArray dir = QFile::encodeName(QDir::toNativeSeparators(pluginDir));
            const QByteArray existing = qgetenv("QT_PLUGIN_PATH");
            if (!existing.split(':').contains(dir))
                qputenv("QT_PLUGIN_PATH", existing.isEmpty() ? dir : dir + ':' + existing);
        }
        if (!m_proc) {
            m_proc = new ParentDeathProcess(this);
            m_proc->setProcessChannelMode(QProcess::MergedChannels);
        }
        m_proc->start(m_program, QStringList());
        if (!m_proc->waitForStarted(timeout))
            return false;
        QElapsedTimer timer; timer.start();
        while (!m_ready.contains('\n') && timer.elapsed() < timeout) {
            m_proc->waitForReadyRead(200);
            m_ready += m_proc->readAll();
            if (m_proc->state() == QProcess::NotRunning) { m_ready += m_proc->readAll(); break; }
        }
        const int r = m_ready.indexOf("READY ");
        if (r < 0)
            return false;
        const QByteArray portStr = m_ready.mid(r + 6).split('\n').first().trimmed();
        const quint16 port = quint16(portStr.toUShort());
        if (!port)
            return false;
        m_port = port;
        qputenv("QT_BEARER_DUMMY_PORT", QByteArray::number(port));
        return true;
    }

    quint16 port() const { return m_port; }
    bool isRunning() const { return m_proc && m_proc->state() != QProcess::NotRunning; }

    void stop()
    {
        if (m_proc && m_proc->state() != QProcess::NotRunning) {
            m_proc->kill();
            m_proc->waitForFinished(3000);
        }
    }

    /// For QRef<TestBearer>(new TestBearer, &TestBearer::dispose).
    static void dispose(TestBearer *b) { delete b; }

private:
    QString m_program;
    ParentDeathProcess *m_proc;
    QByteArray m_ready;
    quint16 m_port = 0;
};

#endif // TESTBEARER_H
