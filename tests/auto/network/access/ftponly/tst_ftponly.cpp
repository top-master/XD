/****************************************************************************
** License: Apache 2.0 without attribution need.
**
** tst_ftponly: an SSL-free replica of tst_QNetworkReply's FTP paths
** (getFromFtp, getFromFtpAfterError). The full tst_qnetworkreply pulls in SSL and
** so does not build under Fil-C; this focused copy keeps the exact same shared FTP
** logic minus any SSL, so Fil-C's memory safety can turn the flaky FTP
** connection-cache use-after-free -- a timing race under a native build -- into a
** deterministic, located panic.
**
** It launches its own server-dummy in "ftp" mode through the TestServer helper (the
** same way tst_qftp does): the whole toolchain is Fil-C now, so there is no longer a
** native server to keep separate. The FTP port is whatever that server bound.
****************************************************************************/

#include <QtTest/QtTest>
#include <QtCore/QCoreApplication>
#include <QtCore/QDir>
#include <QtCore/QFile>
#include <QtNetwork/QNetworkAccessManager>
#include <QtNetwork/QNetworkRequest>
#include <QtNetwork/QNetworkReply>

#include "../../../helpers/testenv.h"

// Same alias tst_QNetworkReply uses; the QSharedPointer coverage on the reply is
// part of the lifetime under test.
typedef QSharedPointer<QNetworkReply> QNetworkReplyPtr;

class tst_FtpOnly : public QObject
{
    Q_OBJECT

    QNetworkAccessManager manager; // shared, so the FTP connection cache is exercised
    QRef<TestServer> server;       // owns the server-dummy (ftp) process for this test
    QString fixtureDir;            // out-of-source folder the FTP server exposes
    int port;

    QString ftpUrl(const QString &path) const
    {
        return QStringLiteral("ftp://127.0.0.1:%1/%2").arg(port).arg(path);
    }
    bool waitFinished(QNetworkReply *reply, int ms = 15000)
    {
        if (reply->isFinished())
            return true;
        QSignalSpy spy(reply, SIGNAL(finished()));
        return spy.wait(ms);
    }

private Q_SLOTS:
    void initTestCase();
    void cleanupTestCase();
    void getFromFtp();
    void getFromFtpAfterError();
    void reuseChurn();
};

void tst_FtpOnly::initTestCase()
{
    // Build an out-of-source fixture the FTP server will serve. The tests only check
    // that a valid transfer yields NoError and non-empty data, so any content does;
    // no rfc3252.txt fixture file needs to live in the source tree.
    fixtureDir = QDir::tempPath() + QStringLiteral("/ftponly-fixture-")
                 + QString::number(QCoreApplication::applicationPid());
    QDir().mkpath(fixtureDir);
    QFile f(fixtureDir + QStringLiteral("/rfc3252.txt"));
    QVERIFY2(f.open(QIODevice::WriteOnly), "could not create FTP fixture file");
    f.write("Network Working Group\nrfc3252 body used only for a non-empty FTP GET.\n");
    f.close();

    server = TestEnv::getServer(TestServer::FileTransfer, fixtureDir);
    QVERIFY2(server && server->isRunning(), "server-dummy (ftp) did not start");
    port = server->port();
    QVERIFY(port > 0);
}

void tst_FtpOnly::cleanupTestCase()
{
    // The server-dummy process is torn down when the QRef drops; drop it explicitly so
    // its own SIGTERM upload-scratch cleanup runs before we remove our fixture.
    server.reset();
    if (!fixtureDir.isEmpty())
        QDir(fixtureDir).removeRecursively();
}

void tst_FtpOnly::getFromFtp()
{
    // Prime the FTP connection cache with a successful transfer.
    QNetworkRequest request(QUrl(ftpUrl(QStringLiteral("rfc3252.txt"))));
    QNetworkReplyPtr reply(manager.get(request));
    QVERIFY(waitFinished(reply.data()));
    QCOMPARE(reply->error(), QNetworkReply::NoError);
    QVERIFY(!reply->readAll().isEmpty());
}

void tst_FtpOnly::getFromFtpAfterError()
{
    // Exactly tst_QNetworkReply::getFromFtpAfterError's shape: an erroring request,
    // then a reuse of the cached connection through the same manager.
    QNetworkRequest invalidRequest(QUrl(ftpUrl(QStringLiteral("invalid.txt"))));
    QNetworkReplyPtr invalidReply;
    invalidReply.reset(manager.get(invalidRequest));
    QSignalSpy spy(invalidReply.data(), SIGNAL(error(QNetworkReply::NetworkError)));
    QVERIFY(spy.wait(15000));

    QNetworkRequest validRequest(QUrl(ftpUrl(QStringLiteral("rfc3252.txt"))));
    QNetworkReplyPtr validReply(manager.get(validRequest));
    QVERIFY(waitFinished(validReply.data()));
    QCOMPARE(validReply->error(), QNetworkReply::NoError);
    QVERIFY(!validReply->readAll().isEmpty());
}

void tst_FtpOnly::reuseChurn()
{
    // Drive many error/success cycles through the shared manager so the FTP
    // connection cache churns -- the state the native crash needs. Under Fil-C any
    // access to a freed cache entry panics here rather than racing past it.
    for (int i = 0; i < 12; ++i) {
        QNetworkReplyPtr bad(manager.get(QNetworkRequest(QUrl(ftpUrl(QStringLiteral("nope%1.txt").arg(i))))));
        QSignalSpy s(bad.data(), SIGNAL(error(QNetworkReply::NetworkError)));
        QVERIFY(s.wait(15000));
        QNetworkReplyPtr good(manager.get(QNetworkRequest(QUrl(ftpUrl(QStringLiteral("rfc3252.txt"))))));
        QVERIFY(waitFinished(good.data()));
        QCOMPARE(good->error(), QNetworkReply::NoError);
    }
}

QTEST_MAIN(tst_FtpOnly)
#include "tst_ftponly.moc"
