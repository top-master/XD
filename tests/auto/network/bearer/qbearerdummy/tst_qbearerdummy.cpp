// Copyright (C) 2015 The XD Company Ltd.
// SPDX-License-Identifier: Apache-2.0

#include <QtTest/QtTest>
#include <QtCore/QElapsedTimer>
#include <QtCore/QDir>
#include <QtCore/QSettings>
#include <QtCore/QTemporaryDir>
#include <QtNetwork/QNetworkConfigurationManager>
#include <QtNetwork/QNetworkSession>
#include <QtNetwork/QNetworkInterface>

#include "../../../helpers/testbearer.h"
#include "../../../helpers/assertion.h"
#include "../../../../../src/plugins/bearer/dummy/bearerconfig.h"

class tst_QBearerDummy : public QObject
{
    Q_OBJECT
    typedef QObject super;
private slots:
    void testConfigFile_shouldReadUnmodifiedIni();
    void testConfigurations_shouldMatchIni();
    void testDefaultConfiguration_shouldUseConfiguredIdentifier();
    void testOpen_shouldActivateAfterConfiguredDelay();
    void testOpen_shouldReportConfiguredFailure_data();
    void testOpen_shouldReportConfiguredFailure();
    void testStop_shouldAffectOtherSessions();
    void testStop_shouldRejectWhenDisabled();
    void testInterface_shouldMatchConfiguration();
private:
    QNetworkConfigurationManager m_manager;
};

void tst_QBearerDummy::testConfigFile_shouldReadUnmodifiedIni()
{
    const BearerConfig settings(QFINDTESTDATA("bearer-config.ini"));
    qExpect(settings.entries().size())->toEqual(7);
    qExpect(settings.entry(QLL("active")).name)->toEqual(QString(QLL("Configured Ethernet")));
}

void tst_QBearerDummy::testConfigurations_shouldMatchIni()
{
    qExpect(m_manager.allConfigurations().size())->toEqual(7);
    const QNetworkConfiguration active = m_manager
        .configurationFromIdentifier(QLL("active"));
    qExpect(active.name())->toEqual(QString(QLL("Configured Ethernet")));
    QCOMPARE(active.state(), QNetworkConfiguration::StateFlags(QNetworkConfiguration::Active));
    QCOMPARE(active.bearerType(), QNetworkConfiguration::BearerEthernet);
    QCOMPARE(m_manager.configurationFromIdentifier(QLL("delayed")).bearerType(),
             QNetworkConfiguration::BearerWLAN);
}

void tst_QBearerDummy::testDefaultConfiguration_shouldUseConfiguredIdentifier()
{
    qExpect(m_manager.defaultConfiguration().identifier())->toEqual(QString(QLL("active")));
}

void tst_QBearerDummy::testOpen_shouldActivateAfterConfiguredDelay()
{
    QNetworkSession session(m_manager.configurationFromIdentifier(QLL("delayed")));
    QElapsedTimer timer;
    timer.start();
    session.open();
    QCOMPARE(session.state(), QNetworkSession::Connecting);
    QVERIFY( ! session.isOpen());
    QVERIFY(session.waitForOpened(2000));
    QVERIFY(timer.elapsed() >= 25);
    session.close();
    QTRY_COMPARE(session.state(), QNetworkSession::Disconnected);
}

void tst_QBearerDummy::testOpen_shouldReportConfiguredFailure_data()
{
    QTest::addColumn<QString >("id");
    QTest::addColumn<int >("error");
    QTest::newRow("unsupported")
        << QString(QLL("blocked"))
        << int(QNetworkSession::OperationNotSupportedError);
    QTest::newRow("configured-error")
        << QString(QLL("denied"))
        << int(QNetworkSession::RoamingError);
    QTest::newRow("unknown-error")
        << QString(QLL("unknown"))
        << int(QNetworkSession::UnknownSessionError);
    QTest::newRow("unavailable")
        << QString(QLL("unavailable"))
        << int(QNetworkSession::InvalidConfigurationError);
}

void tst_QBearerDummy::testOpen_shouldReportConfiguredFailure()
{
    QFETCH(QString, id);
    QFETCH(int, error);
    QNetworkSession session(m_manager.configurationFromIdentifier(id));
    QSignalSpy errors(&session, SIGNAL(error(QNetworkSession::SessionError)));
    session.open();
    QVERIFY( ! session.waitForOpened(1000));
    QCOMPARE(int(session.error()), error);
    qExpect(errors.count())->toEqual(1);
    QVERIFY( ! session.isOpen());
}

void tst_QBearerDummy::testStop_shouldAffectOtherSessions()
{
    const QNetworkConfiguration config = m_manager
        .configurationFromIdentifier(QLL("delayed"));
    QNetworkSession first(config);
    QNetworkSession second(config);
    first.open();
    QVERIFY(first.waitForOpened(2000));
    second.open();
    QVERIFY(second.waitForOpened(2000));
    first.stop();
    QTRY_VERIFY( ! second.isOpen());
    QCOMPARE(second.error(), QNetworkSession::SessionAbortedError);
    QTRY_COMPARE(first.state(), QNetworkSession::Disconnected);
}

void tst_QBearerDummy::testStop_shouldRejectWhenDisabled()
{
    QNetworkSession session(m_manager.configurationFromIdentifier(QLL("cannot-stop")));
    session.open();
    QVERIFY(session.waitForOpened(2000));
    session.stop();
    QCOMPARE(session.error(), QNetworkSession::OperationNotSupportedError);
    QVERIFY(session.isOpen());
    session.close();
}

void tst_QBearerDummy::testInterface_shouldMatchConfiguration()
{
    const QList<QNetworkInterface > interfaces = QNetworkInterface::allInterfaces();
    if (interfaces.size() < 2) {
        QSKIP("Needs two interfaces to tell the configured one from the default first one");
    }
    QNetworkSession session(m_manager.configurationFromIdentifier(QLL("active")));
    session.open();
    QVERIFY(session.waitForOpened(2000));
    QCOMPARE(session.interface().name(), interfaces.last().name());
    session.close();
}

int main(int argc, char **argv)
{
    QCoreApplication app(argc, argv);
    QTemporaryDir folder;
    const QString source = QFINDTESTDATA("bearer-config.ini");
    const QString path = QDir(folder.path()).filePath(QLL("bearer-config.ini"));
    if ( ! folder.isValid()
        || source.isEmpty()
        || ! QFile::copy(source, path)
    ) {
        qFatal("Could not create bearer fixture");
    }
    {
        QSettings settings(path, QSettings::IniFormat);
        const QList<QNetworkInterface > interfaces = QNetworkInterface::allInterfaces();
        settings.setValue(QLL("Configurations/active/interface"),
                          interfaces.isEmpty() ? QString() : interfaces.last().name());
    }
    TestBearer bearer;
    bearer.setConfigFile(path);
    if ( ! bearer.start()) {
        qFatal("bearer-dummy did not start");
    }
    tst_QBearerDummy test;
    QTEST_SET_MAIN_SOURCE_PATH
    return QTest::qExec(&test, argc, argv);
}

#include "tst_qbearerdummy.moc"
