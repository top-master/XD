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

#include <QtTest/QTest>
#include <QtCore/QCoreApplication>
#include <QtCore/QFileInfo>
#include <QtCore/QLibraryInfo>

#include "./testbearer.h"
#include "../../../src/plugins/bearer/dummy/bearerconfig.h"

/// Launch the daemon, wait for its "READY <port>" line, and publish the port so the engine
/// plugin (loaded later by QNetworkConfigurationManager) connects to it. Returns false if the
/// daemon never starts or never readies. The daemon and the plugin read the same INI fixture.
bool TestBearer::start(int timeout)
{
    if (m_program.isEmpty()) {
        return false;
    }
    // The engine plugin is one of the bearer plugins of XD's own plugins dir.
    TestEnv::addBearerPluginPath();
    if ( ! m_proc) {
        m_proc = new ParentDeathProcess(this);
        m_proc->setProcessChannelMode(QProcess::MergedChannels);
    }

    // Hands the daemon and the engine plugin the same fixture, or none.
    QString path = m_configFile.isEmpty()
        ? BearerConfig::findFile() : m_configFile;
    if ( ! path.isEmpty()) {
        path = QFileInfo(path).absoluteFilePath();
    }
    qputenv("QT_BEARER_DUMMY_CONFIG", path.isEmpty() ? QByteArray("-") : path.toLocal8Bit());
    if (BearerConfig(path).isolate() && ! isolatePlugins()) {
        return false;
    }
    m_ready.clear();
    m_proc->start(m_program, QStringList());
    if ( ! m_proc->waitForStarted(timeout)) {
        return false;
    }
    QElapsedTimer timer;
    timer.start();
    while ( ! m_ready.contains('\n') && timer.elapsed() < timeout) {
        m_proc->waitForReadyRead(200);
        m_ready += m_proc->readAll();
        if (m_proc->state() == QProcess::NotRunning) {
            m_ready += m_proc->readAll();
            break;
        }
    }
    const int r = m_ready.indexOf("READY ");
    if (r < 0) {
        return false;
    }
    const QByteArray portStr = m_ready.mid(r + 6).split('\n').first().trimmed();
    const quint16 port = quint16(portStr.toUShort());
    if ( ! port) {
        return false;
    }
    m_port = port;
    qputenv("QT_BEARER_DUMMY_PORT", QByteArray::number(port));
    return true;
}

/// Ends the daemon and undoes what start() changed in this process: the
/// environment the engine plugin reads, and the plugin folders.
void TestBearer::stop()
{
    if (m_proc && m_proc->state() != QProcess::NotRunning) {
        m_proc->kill();
        m_proc->waitForFinished(3000);
    }
    qunsetenv("QT_BEARER_DUMMY_PORT");
    qunsetenv("QT_BEARER_DUMMY_CONFIG");
    if (m_isolatedPlugins) {
        QCoreApplication::setLibraryPaths(m_savedLibraryPaths);
        m_isolatedPlugins.reset();
    }
}

/// Makes the dummy engine the only bearer plugin that this process loads:
/// the library paths become a folder whose `bearer` holds a link to it.
bool TestBearer::isolatePlugins()
{
    QString pluginDir = TestEnv::bearerPluginPath();
    if (pluginDir.isEmpty()) {
        pluginDir = QLibraryInfo::location(QLibraryInfo::PluginsPath);
    }
    const QDir bearerDir(pluginDir + QLL("/bearer"));
    const QStringList found = bearerDir.entryList(
        QStringList() << QLL("*qbearerdummy*"),
        QDir::Files
    );
    if (found.isEmpty()) {
        return false;
    }
    m_isolatedPlugins.reset(new QTemporaryDir);
    const QString isolated = m_isolatedPlugins->path() + QLL("/bearer");
    if ( ! m_isolatedPlugins->isValid()
        || ! QDir().mkpath(isolated)
        || ! QFile::link(bearerDir.filePath(found.first()), isolated + QLatin1Char('/') + found.first())
    ) {
        m_isolatedPlugins.reset();
        return false;
    }
    m_savedLibraryPaths = QCoreApplication::libraryPaths();
    QCoreApplication::setLibraryPaths(QStringList() << m_isolatedPlugins->path());
    return true;
}
