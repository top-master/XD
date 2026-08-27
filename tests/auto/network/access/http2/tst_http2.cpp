/****************************************************************************
** License: Apache 2.0 without attribution need.
**
** tst_http2: drives Qt's backported HTTP/2 client against server-dummy's "h2"
** service, the way tst_spdy drives the SPDY client. Proves the request is
** ALPN-negotiated to "h2" and the response round-trips correctly.
****************************************************************************/

#include <QtTest/QtTest>
#include <QtNetwork/QNetworkAccessManager>
#include <QtNetwork/QNetworkRequest>
#include <QtNetwork/QNetworkReply>
#include <QtNetwork/QSslConfiguration>

#include "../../../network-settings.h"

class tst_Http2 : public QObject
{
    Q_OBJECT
public:
    tst_Http2() : m_rfc3252FilePath(QFINDTESTDATA("../qnetworkreply/rfc3252.txt")) {}

private Q_SLOTS:
    void initTestCase();
    void settingsAndNegotiation_data();
    void settingsAndNegotiation();

private:
    QNetworkAccessManager m_manager;
    const QString m_rfc3252FilePath;
};

void tst_Http2::initTestCase()
{
    if (!QSslSocket::supportsSsl())
        QSKIP("This test requires SSL support");
    QVERIFY(!m_rfc3252FilePath.isEmpty());
    if (!QtNetworkSettings::verifyTestNetworkSettings())
        QSKIP("No Qt test network");
}

void tst_Http2::settingsAndNegotiation_data()
{
    QTest::addColumn<bool>("setAttribute");   // set Http2AllowedAttribute at all
    QTest::addColumn<bool>("enabled");        // ... to true or false
    QTest::addColumn<QByteArray>("expectedProtocol");

    QTest::newRow("h2-enabled")  << true  << true
                                 << QByteArray(QSslConfiguration::NextProtocolHttp2);
    QTest::newRow("h2-disabled") << true  << false << QByteArray();
    QTest::newRow("default")     << false << false << QByteArray();
}

void tst_Http2::settingsAndNegotiation()
{
    QFETCH(bool, setAttribute);
    QFETCH(bool, enabled);
    QFETCH(QByteArray, expectedProtocol);

    QUrl url("https://" + QtNetworkSettings::serverName() + "/qtest/rfc3252.txt");
    QNetworkRequest request(url);
    if (setAttribute)
        request.setAttribute(QNetworkRequest::Http2AllowedAttribute, QVariant(enabled));

    QNetworkReply *reply = m_manager.get(request);
    reply->ignoreSslErrors();
    QSignalSpy finishedSpy(reply, SIGNAL(finished()));
    QObject::connect(reply, SIGNAL(finished()), &QTestEventLoop::instance(), SLOT(exitLoop()));

    QTestEventLoop::instance().enterLoop(15);
    QVERIFY(!QTestEventLoop::instance().timeout());
    QCOMPARE(finishedSpy.count(), 1);
    QCOMPARE(reply->error(), QNetworkReply::NoError);

    const int statusCode = reply->attribute(QNetworkRequest::HttpStatusCodeAttribute).toInt();
    QCOMPARE(statusCode, 200);

    // Whether HTTP/2 was actually used is proven by the ALPN result: "h2" means
    // the request went out over the QHttp2ProtocolHandler.
    if (!expectedProtocol.isEmpty())
        QCOMPARE(reply->sslConfiguration().nextNegotiatedProtocol(), expectedProtocol);
    const bool expectedHttp2 = (expectedProtocol == QByteArray(QSslConfiguration::NextProtocolHttp2));
    QCOMPARE(reply->attribute(QNetworkRequest::Http2WasUsedAttribute).toBool(), expectedHttp2);

    // The body must round-trip byte-for-byte regardless of the protocol used.
    QFile file(m_rfc3252FilePath);
    QVERIFY(file.open(QIODevice::ReadOnly));
    QCOMPARE(reply->readAll(), file.readAll());

    reply->deleteLater();
}

QTEST_MAIN(tst_Http2)
#include "tst_http2.moc"
