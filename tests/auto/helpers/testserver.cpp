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
/// Out-of-line members for TestServer whose implementation is too heavy for the header:
/// installTrustSystemWide() drives a platform CA-install tool via QProcess, so keeping it
/// here keeps testserver.h (included by every network test) free of that machinery. A test
/// that calls it compiles this file in (see qsslsocket_onDemandCertificates_static).

#include <QtTest/QtTest> // testserver.h -> network-settings.h uses QTest::qFail
#include "testserver.h"

#ifndef QT_NO_SSL

#include <QtCore/QCoreApplication>
#include <QtCore/QDir>
#include <QtCore/QFile>
#include <QtCore/QProcess>
#include <QtCore/QTemporaryFile>
#include <QtNetwork/QSslSocket>

#include "testenv.h" // TestEnv::realCaCert()

bool TestServer::installTrustSystemWide() const
{
    if (!QCoreApplication::isElevated())
        return false; // adding an OS-wide trusted root needs an elevated process

    QTemporaryFile pem(QDir::tempPath() + QLL("/server-dummy-ca-XXXXXX.crt"));
    pem.setAutoRemove(false);
    if (!pem.open())
        return false;
    pem.write(domainCert().toPem());
    pem.close();
    const QString path = pem.fileName();

    QString prog;
    QStringList args;
#if defined(Q_OS_WIN)
    prog = QLL("certutil");
    args << QLL("-addstore") << QLL("-f") << QLL("Root") << path;
#elif defined(Q_OS_MACOS)
    prog = QLL("security");
    args << QLL("add-trusted-cert") << QLL("-d")
         << QLL("-r") << QLL("trustRoot")
         << QLL("-k") << QLL("/Library/Keychains/System.keychain") << path;
#else
    // Debian/Ubuntu convention: drop the PEM in the local CA dir, then refresh the bundle.
    const QString dst = QLL("/usr/local/share/ca-certificates/server-dummy-test.crt");
    QFile::remove(dst);
    if (!QFile::copy(path, dst))
        return false;
    prog = QLL("update-ca-certificates");
#endif
    return QProcess::execute(prog, args) == 0;
}

QList<QSslCertificate> TestServer::loadCerts(const QList<QSslCertificate> &otherCerts) const
{
    QList<QSslCertificate> certs = QSslSocket::systemCaCertificates();
    if (TestEnv::realCaCert()) {
        // REAL_CA_CERT=true: trust the server OS-wide (needs an elevated process), then re-read
        // the system roots so they carry the freshly-installed cert.
        qExpect(installTrustSystemWide())->toBeTruthy()
            ->withContext("REAL_CA_CERT=true, but this process is not elevated to install a system CA");
        certs = QSslSocket::systemCaCertificates();
    } else {
        certs << domainCert(); // trust this server's certificate inside this process only
    }
    const QList<QSslCertificate> all = certs + otherCerts;
    QSslSocket::setDefaultCaCertificates(all);
    return all;
}

#endif // QT_NO_SSL
