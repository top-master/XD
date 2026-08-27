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
#include <QDir>
#include <QFileInfo>
#include <QSettings>
#include <QSharedPointer>
#include <QString>
#include <QtNetwork/QHostAddress>

#include "testserver.h"

// Shared reader for the test-env.ini that auto.pro drops at the build tree root.
// A test finds the file by walking up from its own executable's directory, so any
// test at any depth reads the same settings, and points the network tests at the
// bundled dummy server (localhost:45678 by default) instead of a real host.
class TestEnv
{
public:
    // Absolute path of the nearest test-env.ini at or above the running test's
    // directory, or an empty string if none is found.
    static QString iniPath()
    {
        QDir dir(QCoreApplication::applicationDirPath());
        forever {
            const QString candidate = dir.filePath(QStringLiteral("test-env.ini"));
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

    // Host the dummy test server listens on; localhost when the .ini is absent.
    static QString serverHost()
    {
        return settings().value(QStringLiteral("server/host"),
                                QStringLiteral("localhost")).toString();
    }

    // Base TCP/UDP port the dummy test server listens on; 45678 by default. The
    // server offers FTP on this port and a plain echo on port + 1.
    static quint16 serverPort()
    {
        return quint16(settings().value(QStringLiteral("server/port"), 45678).toUInt());
    }

    static quint16 echoPort() { return quint16(serverPort() + 1); }

    // Resolved listen/connect address for the dummy server ("localhost" -> loopback).
    static QHostAddress serverAddress()
    {
        const QString h = serverHost();
        return QHostAddress(h == QLatin1String("localhost")
                            ? QStringLiteral("127.0.0.1") : h);
    }

    // Absolute path of the bundled server-dummy binary. Taken from the "server/dummy"
    // key if the .ini sets one, otherwise derived from the .ini's own location: it
    // sits beside the tests at helpers/server-dummy/ in the same build tree.
    static QString serverDummyPath()
    {
        const QString custom = settings().value(QStringLiteral("server/dummy")).toString();
        if (!custom.isEmpty())
            return custom;
        const QString ini = iniPath();
        if (ini.isEmpty())
            return QString();
        QString exe = QFileInfo(ini).absoluteDir()
                          .filePath(QStringLiteral("helpers/server-dummy/server-dummy"));
#ifdef Q_OS_WIN
        exe += QStringLiteral(".exe");
#endif
        return exe;
    }

    // Launch the bundled server-dummy for one of the network "ways" and hand back an
    // owning ref. The TestServer runs server-dummy as a QProcess and, when the last
    // QRef drops (or stop()/stopLater() is called), terminates it. QtTest runs test
    // functions serially, so a single server on the shared port is enough and the
    // port never has to change between tests.
    static QRef<TestServer> getServer(TestServer::Type type)
    {
        QRef<TestServer> server(new TestServer(serverDummyPath(), type, serverPort()),
                                &TestServer::dispose);
        server->start();
        return server;
    }
};

#endif // TESTENV_H
