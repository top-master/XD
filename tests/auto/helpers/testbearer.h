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

#include "testenv.h"
#include "testserver.h"

#include <QtCore/QByteArray>
#include <QtCore/QDir>
#include <QtCore/QElapsedTimer>
#include <QtCore/QFile>
#include <QtCore/QObject>
#include <QtCore/QProcess>
#include <QtCore/QScopedPointer>
#include <QtCore/QStringList>
#include <QtCore/QTemporaryDir>

class TestBearer : public QObject
{
public:
    explicit TestBearer(QObject *parent = Q_NULLPTR)
        : QObject(parent), m_program(TestEnv::bearerDummyPath()), m_proc(Q_NULLPTR), m_port(0) {}
    ~TestBearer() { stop(); }

    bool start(int timeout = 15000);

    void setConfigFile(const QString &path) { m_configFile = path; }

    quint16 port() const { return m_port; }
    bool isRunning() const { return m_proc && m_proc->state() != QProcess::NotRunning; }

    void stop();

    /// For QRef<TestBearer>(new TestBearer, &TestBearer::dispose).
    static void dispose(TestBearer *b) { delete b; }

private:
    bool isolatePlugins();

    QString m_program;
    QString m_configFile;
    /// Gives the process a plugin folder whose `bearer` holds only the dummy
    /// engine, while a fixture asks for `isolate`; null otherwise.
    QScopedPointer<QTemporaryDir > m_isolatedPlugins;
    QStringList m_savedLibraryPaths;
    ParentDeathProcess *m_proc;
    QByteArray m_ready;
    quint16 m_port;
};

#endif // TESTBEARER_H
