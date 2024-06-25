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

#include "dummy-server.h"
#include "dummy-client.h"

#include "remote-spy.h"
#include "message-storage.h"

#include <QtTest/QtTest>
#include <QtTest/QNetDevicePair>
#include <QtCore/qendian.h>
#include <QtCore/QThreadSlotable>
#include <QtCore/QTimestamp>
#include <QtRemote/QRemoteUser>
#include <QtRemote/qremote-stream-codec.h>
#include <QtRemote/qremote-device-handler.h>
#include <QtTest/private/qtestlog_p.h>


#ifdef Q_OS_WIN
#  define EXE_EXT ".exe"
#else
#  define EXE_EXT ""
#endif


using namespace QRemote;

/// Packets encoded to QByteArray are wrapped into another QByteArray.
static inline QByteArray expandByteArray(const char *data) {
    return QByteArray::fromRawData( Q_PTR_CAST(const char *, Q_PTR_ADD_OFFSET(data, 4))
                                  , qFromBigEndian<qint32>(Q_PTR_CAST(const uchar *, data)));
}

class tst_remote : public QObject
{
    Q_OBJECT

public:
    inline tst_remote()
        : ignoreThreadingTests(false)
        , ignoreBlockingModeTests(false)
    {
    }

    inline virtual ~tst_remote()
    {
    }

public slots:
    inline void initTestCase()
    {
        QString binPath = QLibraryInfo::location(QLibraryInfo::BinariesPath);
        moc_path = binPath + QLL("/moc" EXE_EXT);
    }

    inline void cleanupTestCase()
    {
    }

private slots:
    void types_shouldRegisterQRef();
    void streamCodec_shouldEncodeStatusPacket();
    void streamCodec_shouldEncodeMethodPacket();
    void deviceHandler_shouldKeepPacketUnmodified();
    void userDefaults_shouldUseStreamCodec();

    void server_shouldSendStatusPacket();
    void client_shouldRegisterServiceByStatusPacket();
    void userDestructor_shouldReportDisconnect_data();
    QList<QByteArray> userDestructor_shouldReportDisconnect();
    void user_shouldHandleDisconnect_data();
    void user_shouldHandleDisconnect();
    void serverAndClient_connectToEachOther();
    void serverAndClient_shouldWaitForSlotResultInMainThread();
    void serverAndClient_shouldWaitForSlotResultThreadSafely();
    void clientAndServerThread_connectToEachOther();

    void eventMode_shouldSupportAllTestCasesInBlockingMode();

    void threading_shouldSupportAllTestCasesInAsyncThreads();

public:
    bool runTestCases();
    void runTestCasesAsync(int threadCount);

private:
    QString moc_path;
    /// Helper to stop on first failure (to keep logs clean).
    QAtomicInt m_isSubThreadAborted;
    bool ignoreThreadingTests;
    bool ignoreBlockingModeTests;
};

class MyClass {};
QDebug &operator <<(QDebug &dbg, const MyClass &)
{
    dbg << "MyClass";
    return dbg;
}

/// Preset for a status packet.
///
/// Contatins string-values: "StatusPacket my-server 2024-06-25 12:02:12.527 ServerService"
///
/// @note To re-generate, search for `logging PACKET_STATUS hex` comment, and
/// enable related code, then rebuild and run, finally copy result from logs.
static const char PACKET_STATUS[] =
"000000670000000c5374617475735061636b657400010001ffffffff00000012006d0079002d0073"
"00650072007600650072ffffffff00000017323032342d30362d32352031323a30323a31322e3532"
"370100000001ffffffff0000000d53657276657253657276696365";

enum {
    PACKET_STATUS_SIZE = sizeof(PACKET_STATUS) / sizeof(PACKET_STATUS[0]) - 1
};


void tst_remote::types_shouldRegisterQRef()
{
    // With being already registered by QRemote.
    int eistingId = qMetaTypeId<QRef<QObject>>();
    qExpect(eistingId)->toBeGreaterOrEqual(QMetaType::User);
    qExpect(QMetaType::hasRegisteredStreamOperators(eistingId))->toBeTruthy();
    // Without being different.
    int newId = qRegisterMetaType<QRef<QObject> >();
    qExpect(newId)->toEqual(eistingId);
    // Without supporting stream-operators for sub-classes (even QObjectRemote).
    newId = qRegisterMetaType<QRef<QObjectRemote> >();
    qExpect(newId)->Not->toEqual(eistingId);
    qExpect(QMetaType::hasRegisteredStreamOperators(newId))->toBeFalsy();
}

void tst_remote::streamCodec_shouldEncodeStatusPacket()
{
    // Dummy.
    QScopedPointer<StreamPacketCodec> codec;
    codec.reset(new StreamPacketCodec());
    // With packet.
    StatusPacket pktIn;
    pktIn.setVisible(false);
    QRemoteAddress address;
    address.id = "my-object-name";
    address.typeName = "ServerService";
    pktIn.remotes().append(address);

    // Actual test.
    try {
        QByteArray data = codec->encode(pktIn);
        qExpect(data.size())->toBeGreaterThan(address.id.size() + address.typeName.size());
        // With decoding to same value.
        PacketUniquePtr pktBase = codec->decode(data);
        qExpect(pktBase->type())->toEqual(Packet::Status);
        StatusPacket *statusPkt = reinterpret_cast<StatusPacket *>(pktBase.get());
        qExpect(statusPkt->isVisible())->toBeFalsy();
        qExpect(address)->toBeInArray(statusPkt->remotes());
    } catch (QRemoteException &err) {
        qThrowTestFailure(err.message());
    }
}

void tst_remote::streamCodec_shouldEncodeMethodPacket()
{
    // Dummy.
    QScopedPointer<StreamPacketCodec> codec;
    codec.reset(new StreamPacketCodec());
    // With packet.
    MethodPacket pkt;
    QByteArray methodName = "my-method-name";
    pkt.setMethod(methodName);

    // Actual test.
    try {
        QByteArray data = codec->encode(pkt);
        qExpect(data.size())->toBeGreaterThan(methodName.size());
        // With decoding to same value.
        PacketUniquePtr pktBase = codec->decode(data);
        qExpect(pktBase->type())->toEqual(Packet::RemoteSignal);
        MethodPacket *methodPkt = reinterpret_cast<MethodPacket *>(pktBase.get());
        qExpect(methodPkt->method())->toEqual(methodName);
        auto timeStamp = methodPkt->timeStamp();
        qExpect(timeStamp.length())->toBeGreaterOrEqual(QMetaRemote::timeStampFormat.length())
                ->withContext(timeStamp.constData());
    } catch (QRemoteException &err) {
        qThrowTestFailure(err.message());
    }
}

void tst_remote::deviceHandler_shouldKeepPacketUnmodified()
{
}

void tst_remote::userDefaults_shouldUseStreamCodec()
{
    QScopedPointer<QRemoteUser> userServer(new QRemoteUser());
    StreamPacketCodec *codec = qobject_cast<StreamPacketCodec *>(userServer->codec());
    qExpect(codec)->Not->toBeNull();
}

void tst_remote::server_shouldSendStatusPacket()
{
    // Dummy.
    QScopedPointer<QRemoteUser> userServer(new QRemoteUser());
    userServer->setObjectName("my-server");
    QRef<ServerService> service = QRef<ServerService>(new ServerService());
    userServer->registerLocal(service);
    // With server device/connection.
    QNetDevice device;
    userServer->addDevice(&device);
    // With keeping global-codec.
    PacketCodec *codec = userServer->codec();
    qExpect(codec)->Not->toBeNull();

    // Actual test.
    const QByteArray deviceData = device.data();
    qExpect(deviceData)->Not->toBeEmpty();
    // With packet decode.
    PacketUniquePtr pktBase;
    try {
        pktBase = codec->decode(expandByteArray(deviceData.constData()));
    } catch (QRemoteException &err) {
        qThrowTestFailure(err.message());
    }
    // With right type.
    qExpect(pktBase->type())->toEqual(Packet::Status);
    StatusPacket *statusPkt = reinterpret_cast<StatusPacket *>(pktBase.get());
    // With right content.
    qExpect(statusPkt->isVisible())->toBeTruthy();
    qExpect(statusPkt->remotes())->Not->toBeEmpty();
    QRemoteAddress address(service.data());
    qExpect(address)->toBeInArray(statusPkt->remotes());
    // Without any packet sent after disconnect.
    qExpect(device.pos())->toEqual(device.data().size());
    // Without disconnect issues.
    delete userServer.take();

    // With optionally logging PACKET_STATUS hex.
#if 0
    qDebug().nospace().noquote() << "PACKET_STATUS hex: " << deviceData.toHex();
#endif
}

void tst_remote::client_shouldRegisterServiceByStatusPacket()
{
    // Dummy.
    QScopedPointer<QRemoteUser> userClient( new QRemoteUser() );
    userClient->setObjectName("my-client");
    QRef<ServerService> service = QRef<ServerService>(new ServerService());
    userClient->registerLocal(service);
    // With connection spy.
    UserSpy userSpy(*userClient);
    // With device/connection containing StatusPacket.
    QNetDevice device;
    device.reset(QByteArray::fromHex(
            QByteArray::fromRawData(PACKET_STATUS, PACKET_STATUS_SIZE)));
    userClient->addDevice(&device);
    qExpect(device.data())->Not->toBeEmpty();

    // Actual test.
    userSpy.assertNewConnection(0);
    // Without disconnect isssues.
    delete userClient.take();
}

void tst_remote::userDestructor_shouldReportDisconnect_data()
{
    QTest::addColumn<bool>("isEnabled");
    QTest::addColumn<int>("serviceCount");

    QTest::newRow("If enabled.")
        << true << 0;
    QTest::newRow("Unless disabled.")
        << false << 0;
    QTest::newRow("If enabled; with service.")
        << true << 3;
    QTest::newRow("Unless disabled; with service.")
        << false << 3;
}

QList<QByteArray> tst_remote::userDestructor_shouldReportDisconnect()
{
    QList<QByteArray> states;

    const QFETCH(bool, isEnabled);
    const QFETCH(int, serviceCount);

    // Dummy.
    QScopedPointer<QRemoteUser> user;
    user.reset(new QRemoteUser());
    user->setObjectName("my-server");
    // With local-services.
    for (int i = 0; i < serviceCount; ++i) {
        QRef<ServerService> service = QRef<ServerService>(new ServerService());
        service->setObjectName(QStringLiteral("ID-%1").arg(i));
        user->registerLocal(service);
    }
    // With server device/connection.
    QNetDevice device;
    user->addDevice(&device);
    // Without previous packets.
    states << device.data();
    device.clear();
    // With keeping global-codec.
    PacketCodec *codec = user->codec();

    // Actual test.
    user->setDisconnectSendable(isEnabled);
    delete user.take();
    if (isEnabled) {
        states << device.data();
        qExpect(device.data())->Not->toBeEmpty();
        // With right content.
        PacketUniquePtr pkt;
        try {
            QByteArray b = expandByteArray(device.data().constData());
            pkt = codec->decode(b);
        } catch (QRemoteException &err) {
            qThrowTestFailure(err.message());
        }
        qExpect(pkt->type())->toEqual(Packet::Status);
        StatusPacket *status = reinterpret_cast<StatusPacket *>(pkt.get());
        qExpect(status->isDisconnect())->toBeTruthy();
        // Without any packet sent after disconnect.
        qExpect(device.pos())->toEqual(device.data().size());
    } else {
        qExpect(device.data())->toBeEmpty();
    }

    return states;
}

void tst_remote::user_shouldHandleDisconnect_data()
{
    userDestructor_shouldReportDisconnect_data();
}

void tst_remote::user_shouldHandleDisconnect()
{
    const QFETCH(bool, isEnabled);
    const QFETCH(int, serviceCount);
    if ( ! isEnabled) {
        // Disabled is handled by QRemoteUser with doing nothing.
        return;
    }

    QList<QByteArray> states = userDestructor_shouldReportDisconnect();
    qExpect(states.count())->toBeGreaterOrEqual(2);

    // Dummy.
    QScopedPointer<QRemoteUser> user(new QRemoteUser());
    user->setObjectName(QLL("my-client"));
    UserSpy userSpy(*user);
    // With services created.
    QNetDevice device;
    device.reset(states.first());
    user->addDevice(&device);
    // With service.
    QList<QRef<QObjectRemote>> serviceList;
    for (int i = 0; i < serviceCount; ++i) {
        serviceList << userSpy.assertNewConnection(i);
    }

    // Actual test.
    device.reset(states[1]);
    for (int i = 0; i < serviceCount; ++i) {
        qExpect(user->contains(serviceList[i].data()))->toBeFalsy();
    }
    // Without logging "Device not set (or destroyed)".
    user->setDisconnectSendable(false);
}

void tst_remote::serverAndClient_connectToEachOther()
{
    // Dummy.
    QScopedPointer<QRemoteUser> userServer( new QRemoteUser() );
    userServer->setObjectName("my-server");
    QRef<ServerService> service = QRef<ServerService>(new ServerService());
    userServer->registerLocal(service);
    // With device/connection.
    QNetDevicePair network;
    userServer->addDevice(network.server());
    qExpect(network.serverData())->Not->toBeEmpty();
    // Without needing server if service's already registered.
    QByteArray oldData = network.serverData();
    userServer->setDisconnectSendable(false);
    delete userServer.take();
    qExpect(network.serverData())->toEqual(oldData)
            ->withContext([&] { return QLL("Don't send any disconnect packets."); });
    // With client.
    QScopedPointer<QRemoteUser> userClient( new QRemoteUser() );
    userClient->setObjectName("my-client");
    // With connection spy.
    UserSpy spy(*userClient);
    // With connection to server.
    userClient->addDevice(network.client());

    // Actual test.
    QSharedPointer<QObjectRemote> controller = spy.assertNewConnection(0);
    // With controller being a separate object.
    qExpect( qptrdiff(controller.data()) )
            ->Not->toEqual( qptrdiff(service.data()) );
    // With right type.
    QSharedPointer<ServerServiceRemote> casted =
            qMove(controller.objectCast<ServerServiceRemote>());
    qExpect(casted)->Not->toBeNull();
    // Without disconnect isssues.
    delete userClient.take();
}

void tst_remote::serverAndClient_shouldWaitForSlotResultInMainThread()
{
    // Dummy.
    QScopedPointer<QRemoteUser> userServer( new QRemoteUser() );
    userServer->setObjectName("my-server");
    QRef<ServerService> service = QRef<ServerService>(new ServerService());
    userServer->registerLocal(service);
    // With device/connection.
    QNetDevicePair network;
    userServer->addDevice(network.server());
    qExpect(network.serverData())->Not->toBeEmpty();
    // With client.
    QScopedPointer<QRemoteUser> userClient( new QRemoteUser() );
    userClient->setObjectName("my-client");
    // With connection spy.
    UserSpy spy(*userClient);
    // With connection to server.
    userClient->addDevice(network.client());
    // With connection success.
    QSharedPointer<QObjectRemote> rawController = spy.assertNewConnection(0);
    // With right type.
    QRef<ServerServiceRemote> controller = rawController.objectCast<ServerServiceRemote>();
    qExpect(controller)->Not->toBeNull();

    // Actual test.
    qExpect(service->isSecretCalled())->toBeFalsy();
    QString reslut = controller->onSecretCallable();
    qExpect(reslut)->toEqualString(QLL("My secret's result."));
    qExpect(service->isSecretCalled())->toBeTruthy();

    // Without disconnect isssues.
    QByteArray oldData = network.serverData();
    userServer->setDisconnectSendable(false);
    delete userServer.take();
    qExpect(network.serverData())->toEqual(oldData)
            ->withContext([&] { return QLL("Don't send any disconnect packets."); });
    delete userClient.take();
}


class SecretCallerThread : public QThread {
public:
    QRef<ServerServiceRemote> controller;

    void run() Q_DECL_OVERRIDE {
        qExpect(controller)->Not->toBeNull();
        QString reslut = controller->onSecretCallable();
        qExpect(reslut)->toEqualString(QLL("My secret's result."));
    }
};

void tst_remote::serverAndClient_shouldWaitForSlotResultThreadSafely()
{
    // Dummy.
    QScopedPointer<QRemoteUser> userServer( new QRemoteUser() );
    userServer->setObjectName("my-server");
    QRef<ServerService> service = QRef<ServerService>(new ServerService());
    userServer->registerLocal(service);
    // With device/connection.
    QNetDevicePair network;
    userServer->addDevice(network.server());
    qExpect(network.serverData())->Not->toBeEmpty();
    // With client.
    QScopedPointer<QRemoteUser> userClient( new QRemoteUser() );
    userClient->setObjectName("my-client");
    // With connection spy.
    UserSpy spy(*userClient);
    // With connection to server.
    userClient->addDevice(network.client());
    // With connection success.
    QSharedPointer<QObjectRemote> rawController = spy.assertNewConnection(0);
    // With right type.
    QRef<ServerServiceRemote> controller = rawController.objectCast<ServerServiceRemote>();
    qExpect(controller)->Not->toBeNull();

    // Actual test.
    qExpect(service->isSecretCalled())->toBeFalsy();
    SecretCallerThread *thread = new SecretCallerThread();
    thread->controller = controller;
    thread->start();
    QTest::qWaitForThread(thread, 15000);
    qExpect(service->isSecretCalled())->toBeTruthy();

    // Without disconnect isssues.
    QByteArray oldData = network.serverData();
    userServer->setDisconnectSendable(false);
    delete userServer.take();
    qExpect(network.serverData())->toEqual(oldData)
            ->withContext([&] { return QLL("Don't send any disconnect packets."); });
    delete userClient.take();
}

void tst_remote::clientAndServerThread_connectToEachOther()
{
    // Dummy.
    ServerClientPair network;
    network.setPort(ServerClientPair::registerPort());
    QPointer<ServerThread> server = network.server;
    QPointer<ClientThread> client = network.client;
    // With server execution.
    server->start();
    server->waitForStarted(3000);
    qExpect(server->isStarted())->toBeTruthy();
    qExpect(server->raw()->isListening())->toBeTruthy();
    // With waiting for connection to server.
    client->start();
    client->waitForController(7000);
    // With server yet listening.
    qExpect(server->raw()->isListening())->toBeTruthy();

    // Actual test.
    qExpect(client->isReady())->toBeTruthy();
    qExpect(server->clients.count())->toEqual(1);
    // With new QObjectRemote (even if client is in the same process as server).
    const QRef<ServerService> &service = server->clients[0]->service;
    const QRef<ServerServiceRemote> &controller = client->controller();
    qExpect( qptrdiff(controller.data()) )
            ->Not->toEqual( qptrdiff(service.data()) );
    // Without exit issues.
    network.quit(3000);
    qExpect(server)->toBeNull();
    qExpect(client)->toBeNull();
}

void tst_remote::eventMode_shouldSupportAllTestCasesInBlockingMode()
{
    if (ignoreBlockingModeTests) {
        return;
    } else if (QTestLog::failCount() > 0) {
        QSKIP("Only tested if all other tests pass.");
    }
    ignoreBlockingModeTests = true;
    bool oldThreadingTests = ignoreThreadingTests;
    ignoreThreadingTests = true;
    QtPrivate::remoteEventMode = QRemote::BlockingMode;

    defer {
        ignoreBlockingModeTests = false;
        ignoreThreadingTests = oldThreadingTests;
        QtPrivate::remoteEventMode = QEventLoop::AllEvents;
    };

    runTestCases();
}

void tst_remote::threading_shouldSupportAllTestCasesInAsyncThreads()
{
    if (ignoreThreadingTests) {
        return;
    } else if (QTestLog::failCount() > 0) {
        QSKIP("Only tested if all other tests pass.");
    }
    ignoreThreadingTests = true;

    // Configures.
    int oldLevel = QTestLog::setVerboseLevel(-1);
    defer {
        QTestLog::setVerboseLevel(oldLevel);
    };
#if 0
    QtPrivate::debugDeleteEvents = true;
#endif

    // Firstly, checks with a single background-thread to ensure that
    // non-main threads work to begin with.
    runTestCasesAsync(1);
    // Then we increase overhead.
    runTestCasesAsync(15);
}

bool tst_remote::runTestCases()
{
    QObject *obj = this;
    int oldFailures = QTestLog::failCount();
    const QMetaObject *meta = obj->metaObject();
    const int count = meta->methodCount();
    int i = 0;
    for (; i < count; ++i) {
        QMetaMethod m = meta->method(i);
        if ( ! QTest::isTestCase(m)) {
            continue;
        }
        try {
            if (m_isSubThreadAborted.load()) {
                return false;
            }

            // Skips tests that need data-providers.
            QByteArray name = m.methodSignature();
            name.chop(2);
            name.append(QLL("_data"));
            if (meta->indexOfMethod(name.constData())) {
                continue;
            }

            m.invoke(obj);
        } catch (...) {
            goto posAbort;
        }

        if (oldFailures < QTestLog::failCount()) {
posAbort:
            if (m_isSubThreadAborted.fetchAndStoreRelaxed(1)) {
                // Already aborted.
                return false;
            }
            break;
        }
    }

    if (i < count) {
        MessageStorage::get().flush();
    }

    return true;
}

class SubThread : public QThreadSlotable {
    typedef QThreadSlotable super;
public:
    SubThread(tst_remote *ownerArg)
        : owner(ownerArg)
    {
    }
    ~SubThread()
    {
        super::requireDeleteSafe();
        owner.clear();
    }

    bool preRun() Q_DECL_OVERRIDE {
        owner->runTestCases();

        // Continues to execute events.
        return true;
    }

public:
    QPointer<tst_remote> owner;
};

QSHAREDPOINTER_DELETER(SubThread, ptr->deleteSafe())

void tst_remote::runTestCasesAsync(int threadCount)
{
    qDebug("runTestCasesAsync: started for thread-count: %d", threadCount);
    QtMessageHandler oldHandler = MessageStorage::get().registerHandler();
    defer {
        qInstallMessageHandler(oldHandler);
    };

    const int timeout = 60 * QTimestamp::Second;
    QElapsedTimer timer;
    timer.start();

    QVector<QRef<SubThread>> threadList;
    for (int i = 0; i < threadCount; ++i) {
        threadList.append(QRef<SubThread>(new SubThread(this)));
    }

    for (int i = 0; i < threadCount; ++i) {
        threadList.ptr(i)->data()->start();
    }

    for (int i = 0; i < threadCount; ++i) {
        QThread *thread = threadList.ptr(i)->data();
        QTest::qWaitForThread(thread, timer.timeLeft(timeout));
    }

    qExpect(m_isSubThreadAborted.load())->toEqual(0)
            ->withContext([&]() -> QString {
                return QString(QLL("Test repeats failed, elapsed-time: %s"))
                        .arg(timer.toString());
            });
}

QTEST_MAIN(tst_remote)
#include "tst_remote.moc"
