/****************************************************************************
**
** Copyright (C) 2015 The XD Company Ltd.
**
** This file is part of the QtRemote module of the XD Toolkit.
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

#include "qremoteuser_p.h"


#include <QtCore/QCoreApplication>
#include <QtCore/QBuffer>
#include <QtCore/QDebug>
#include <QtCore/QElapsedTimer>
#include <QtCore/QFunction>
#include <QtCore/QList>
#include <QtCore/QMap>
#include <QtCore/QMutex>
#include <QtCore/QReadLocker>
#include <QtCore/QReadWriteLock>
#include <QtCore/QThread>
#include <QtCore/QTimer>
#include <QtCore/QWaitCondition>


QT_BEGIN_NAMESPACE

/**
 * @namespace QRemote
 * @brief QtRemote module's namespace for non-common API.
 */
using namespace QRemote;


ReplyWaiter::ReplyWaiter(QRemoteUserPrivate *sessionArg, const MethodPacket *pkt)
{
    d.session = sessionArg;
    d.request = const_cast<MethodPacket *>(pkt);
    d.gotReply = false;

    QMutexLocker _(&sessionArg->waitersMutex);
    sessionArg->waiters.insert(d.request->id(), this);
}

ReplyWaiter::~ReplyWaiter()
{
    QMutexLocker _(&d.session->waitersMutex);
    d.session->waiters.remove(d.request->id(), this);
}


// Some old-compilers would fail without the space " )".
Q_GLOBAL_STATIC(QRemoteUserCounter, globalInstanceManager )

QRemoteUserCounter *QRemoteUserPrivate::instanceManager()
{
    return globalInstanceManager();
}

QThreadStorage<QRemoteUserPrivate::PerThreadStorage> QRemoteUserPrivate::threadStore;


QRemoteUser::QRemoteUser(QObject *parent, bool storeFirst):
        QObject(*new QRemoteUserPrivate, parent)
{
    QRemoteUserPrivate::instanceManager()->append(this, storeFirst);
}
QRemoteUser::QRemoteUser(QRemoteUserPrivate &dd, QObject *parent, bool storeFirst)
    : QObject(dd, parent)
{
    QRemoteUserPrivate::instanceManager()->append(this, storeFirst);
}

QRemoteUser::~QRemoteUser()
{
    QRemoteUserPrivate::instanceManager()->remove(this);

    Q_D(QRemoteUser);

    // Ensures no one tries to use this any longer,
    // and that the `destroyed` signal happens before QPointer(s) get `nullptr`.
    d->emitDestroyed();

    // TRACE/remote status: broadcast shutdown #1,
    // where any local-services registered by `objectName()` will be
    // unregistered from remote sides (if enabled).
    sendDisconnect(Q_NULLPTR);

    // Device-Handler's destructor ensures any buffered "reply" gets sent
    // but we need to enusre said Handler gets deleted before other child-objects
    // because child-objects could be of type "QIODevice", with pending data to send.
    //
    // note that device(s) are never deleted by QRemote directly.
    d->deviceHandlers.clear(); // QSharedPointer should delete all `deviceHandlers`.

    // Detach all local-services.
    MapLocal::const_iterator it = d->locals.begin();
    MapLocal::const_iterator end = d->locals.end();
    for (; it != end; ++it) {
        const QRef<QObject> obj = it.value();
        QRemoteData *data = QMetaRemote::dataFromObject(obj.data());
        Q_ASSERT(data->session == this);
        data->session = Q_NULLPTR;
    }
    d->locals.clear();
    // Detach all remote-controllers.
    MapRemoteUser::iterator user = d->remotes.begin();
    MapRemoteUser::const_iterator endUser = d->remotes.constEnd();
    for(; user != endUser; ++user) {
        MapRemote &remotes = *user;
        MapRemote::iterator it = remotes.begin();
        MapRemote::const_iterator end = remotes.constEnd();
        for(; it != end; ++it) {
            QRef<QObjectRemote> &obj = it.value();
            QObjectRemotePrivate *d = QObjectRemotePrivate::get(obj.data());
            Q_ASSERT(d->session == this);
            d->session = Q_NULLPTR;
            emit disconnected(obj);
        }
    }
    d->remotes.clear();
}

QRemoteUser *QRemoteUser::instance()
{
    return QRemoteUserPrivate::instanceManager()->instance();
}

QRemoteUser *QRemoteUser::findInstance(const QRemoteUserName &user)
{
    return QRemoteUserPrivate::instanceManager()->findInstance(user);
}

QRemoteUser *QRemoteUser::fromThreadStorage()
{
    return QRemoteUserPrivate::threadStore.localData().instance;
}

QRemoteUser *QRemoteUser::toThreadStorage(QRemoteUser *v)
{
    QRemoteUserPrivate::PerThreadStorage &store = QRemoteUserPrivate::threadStore.localData();
    QRemoteUser *last = store.instance;
    store.instance = v;
    return last;
}

void QRemoteUser::timerEvent(QTimerEvent *event)
{
    Q_D(QRemoteUser);
    if (event->timerId() == d->pollTimer.timerId()) {
        d->pollTimer.stop();
        this->onPolling();
    }
}

void QRemoteUser::onPolling()
{
    Q_D(QRemoteUser);
    QMutexLocker _(&d->remotesMutex);
    MapRemoteUser::iterator user = d->remotes.begin();
    MapRemoteUser::const_iterator endUser = d->remotes.constEnd();
    while (user != endUser) {
        MapRemote &remotes = *user;
        if (remotes.isEmpty()) {
            user = d->remotes.erase(user);
        } else {
            ++user;
        }
    }
}

// MARK: Service helpers (register, find and etc).

bool QRemoteUser::registerLocal(const QRef<QObject> &local)
{
    if(local.isNull())
        return false;
    QObjectPrivate *d = QObjectPrivate::get(local.data());
    if(d->isRemote) {
        qWarning("QRemoteUser.registerLocal: Can not cast to service from controller: %s(%p)",
                 local->metaObject()->className(), &local);
        return false;
    }
    //fetch or create internal-data
    QRemoteData *data = QMetaRemote::dataFromObject(local.data());
    if(data) {
        // Detaches from old owner.
        QRemoteUser *owner = data->session;
        if (owner) {
            if(owner == this)
                return true;
            //qWarning("QRemoteUser.registerLocal: object %s(%p) is registered before",
            //         local->metaObject()->className(), local);
            owner->unregister(local);
        }
    } else {
        d->isReinterpretable = true;
        data = new QRemoteData;
        d->setPrivateData(QMetaObject::remoteId, data);
    }

    //at last attach to object
    const QRemoteAddress &address = QRemoteAddress(local.data());
    d_func()->locals[address] = local;
    data->session = this;

    QScopedPointer<StatusPacket> pkt(new StatusPacket);
    pkt->remotes().append(address);
    send(*pkt);
    return true;
}

QRef<QObject> QRemoteUser::unregisterLocal(const QRemoteAddress &name, QRemoteData *data)
{
    Q_D(QRemoteUser);
    MapLocal::iterator it = d->locals.find(name);
    if (it != d->locals.end()) {
        QRef<QObject> result = it.value();
        d->locals.erase(it);
        if( ! data) {
            data = QMetaRemote::dataFromObject(result.data());
            if ( ! data)
                return result;
        }
        data->session = 0;
        return result;
    }
    return QRef<QObject>();
}

QRef<QObjectRemote> QRemoteUser::createRemote(const QRemoteLink &address)
{
    QRef<QObjectRemote> obj = findRemote(address);
    if (obj)
        return obj; //already registered

    obj = QMetaRemote::remoteFromType(address.typeName);
    if (obj) {
        //if(QRemoteAddress::typeFromObject(obj) != address.typeName)
        //    qWarning("QRemoteUser::createRemote: unexpected type mismatch");

        QObjectRemotePrivate *d = QObjectRemotePrivate::get(obj.data());
        d->session = this;
        // Set address and store.
        d->user = address.userName;
        obj->setObjectName(address.id);
        d_func()->remotes[address.userName][address] = obj;
        // Notify about created service-controller.
        emit newConnection(obj);
    } else {
        qWarning("QRemoteUser.createRemote: there is no controller registered for type(%s):\n"
                "ensure any required *_remote.cpp is linked and loaded in runtime"
                , address.typeName.constData()
            );
    }
    return obj;
}

bool QRemoteUser::registerRemote(const QRef<QObjectRemote> &obj)
{
    Q_ASSERT(obj);
    QObjectRemotePrivate *d = QObjectRemotePrivate::get(obj.data());
    if(d->isRemote) {
        if(d->session) {
            d->session->unregister(obj);
        }
        //since we already unregistered address will not contain name of old QRemoteUser
        const QRemoteLink &address = QRemoteLink::fromObject(obj.data());
        d_func()->remotes[address.userName][address] = obj;
        emit newConnection(obj);
        return true;
    }
    return false;
}

QRef<QObjectRemote> QRemoteUser::unregisterRemote(const QRemoteLink &name)
{
    Q_D(QRemoteUser);

    MapRemoteUser::iterator user = d->remotes.find(name.userName);
    if (user != d->remotes.end()) {
        MapRemote &remotes = *user;
        MapRemote::iterator it = remotes.find(name);
        if (it != remotes.end()) {
            QRef<QObjectRemote> &res = it.value();
            remotes.erase(it);
            QRemoteData *data = QMetaRemote::dataFromObject(res.data());
            Q_ASSERT_X(data, "QRemoteUser::unregisterRemote", "missing registration data");
            data->session = 0;
            emit disconnected(res);
            return res;
        }
    }
    return QRef<QObjectRemote>();
}

bool QRemoteUser::registerOnce(const QRef<QObject> &v)
{
    QObject *obj = v.data();
    if(obj == Q_NULLPTR)
        return false;
    QRemoteData *data = QMetaRemote::dataFromObject(obj);
    if(data == Q_NULLPTR || data->session == Q_NULLPTR) {
        if(QObjectPrivate::get(obj)->isRemote) {
            const QRef<QObjectRemote> &remoteRef = v.staticCast<QObjectRemote>();
            return this->registerRemote(remoteRef);
        } else
            return this->registerLocal(v);
    }
    return true; //both data and session was set
}

bool QRemoteUser::unregister(const QRef<QObject> &obj)
{
    Q_D(QRemoteUser);
    if(obj.isNull())
        return false;
    if(QObjectPrivate::get(obj.data())->isRemote) {
        // TRACE/remote: Caching QHash::end() #1
        // which works since it's not changed by `QHash::erase(...)`,
        // also note that a for-loop's "++it" would go past end if erased last.
        QMutexLocker _(&d->remotesMutex);
        MapRemoteUser::iterator iUser = d->remotes.begin();
        MapRemoteUser::const_iterator endUser = d->remotes.constEnd();
        for(; iUser != endUser; ++iUser) {
            MapRemote &remotes = *iUser;
            MapRemote::iterator it = remotes.begin();
            MapRemote::const_iterator end = remotes.constEnd();
            while(it != end) {
                if (it.value().data() == static_cast<QObjectRemote *>(obj.data())) {
                    it = remotes.erase(it);
                    //found and break but should continue once we support register with another id
                    goto posCleanData;
                } else {
                    ++it;
                }
            }
        }
    } else {
        // TRACE/remote: Caching QHash::end() #x
        QMutexLocker _(&d->remotesMutex);
        MapLocal::iterator it = d->locals.begin();
        MapLocal::const_iterator end = d->locals.constEnd();
        while (it != end) {
            if (it.value() == obj) {
                it = d->locals.erase(it);
                //found and break but should continue once we support register with another id
                break;
            } else {
                ++it;
            }
        }
    }

posCleanData:
    QRemoteData *data = QMetaRemote::dataFromObject(obj.data());
    if(data && data->session) {
        if(data->session == this) {
            data->session = Q_NULLPTR;
            emit disconnected(QRef<QObjectRemote>(static_cast<QObjectRemote *>(obj.data())));
        } else {
            return data->session->unregister(obj);
        }
    }
    return true;
}

bool QRemoteUser::contains(const QObject *obj) const
{
    Q_D(const QRemoteUser);
    if(QObjectPrivate::get(obj)->isRemote) {
        MapRemoteUser::const_iterator user = d->remotes.begin();
        for (; user != d->remotes.end(); ++user) {
            const MapRemote &remotes = *user;
            MapRemote::const_iterator it = remotes.begin();
            for (; it != remotes.end(); ++it) {
                if (it.value() == obj)
                    return true;
            }
        }
    } else {
        MapLocal::const_iterator it = d->locals.begin();
        for (; it != d->locals.end(); ++it) {
            if (it.value() == obj)
                return true;
        }
    }
    return false;
}

void QRemoteUser::clearRemotes(const QRemoteUserName &user)
{
    Q_D(QRemoteUser);
    MapRemoteUser::iterator iUser = d->remotes.find(user);
    if (iUser != d->remotes.end()) {
        QFunction<bool (const QRef<QObjectRemote> &)> maybeSkip;
        const bool isNotBroadcast = ! user.isEmpty();
        // TRACE/remote: Caching QHash::end() #x
        QMutexLocker _(&d->remotesMutex);
        MapRemote &remotes = *iUser;
        MapRemote::iterator it = remotes.begin();
        MapRemote::const_iterator end = remotes.constEnd();
        while (it != end) {
            // Intentionally does copy (to prevent destructor's auto remove).
            QRef<QObjectRemote> service = *it;
            if ( ! service) {
                Q_ASSERT(false);
                goto posRemove;
            }
            // Skips if wrong user.
            if (isNotBroadcast) {
                const QRemoteUserName &serviceUser = QObjectRemotePrivate::findUserName(service.data());
                if (serviceUser != user || serviceUser.isEmpty()) {
                    ++it;
                    continue;
                }
            }

            // Need braces (since `goto`).
            {
                QRemoteData *data = QObjectRemotePrivate::findData(service.data());
                // If still no `data`, we prefer to crash, but let's assert.
                Q_IF(data != Q_NULLPTR) {
                    data->session = Q_NULLPTR;
                    emit disconnected(service);
                }
            }

            posRemove:
            it = remotes.erase(it);
        }

        // Keeps empty user until `PollDelay`, since may re-register soon.
        if (remotes.isEmpty()) {
            d->pollTimer.start(QRemoteUserPrivate::PollDelay, this);
        }
    }
}

QRef<QObject> QRemoteUser::findLocal(const QRemoteAddress &name)
{
    return d_func()->locals.value(name);
}

QRef<QObjectRemote> QRemoteUser::findRemote(const QRemoteLink &link) const
{
    MapRemoteUser::const_iterator user = d_func()->remotes.find(link.userName);
    if(user != d_func()->remotes.end()) {
        return (*user).value(link);
    }
    return QRef<QObjectRemote>();
}

QRef<QObjectRemote> QRemoteUser::findRemoteId(const QRemoteAddress &name) const
{
    Q_D(const QRemoteUser);
    MapRemoteUser::const_iterator user = d->remotes.begin();
    for (; user != d->remotes.end(); ++user) {
        const MapRemote &remotes = *user;
        MapRemote::const_iterator it = remotes.begin();
        for (; it != remotes.end(); ++it) {
            const MapRemote::key_type &key = it.key();
            if (key.typeName == name.typeName && key.id == name.id)
                return it.value();
        }
    }
    return QRef<QObjectRemote>();
}

bool QRemoteUser::setObjectVisible(const QObject *local, bool visible, const QString &user)
{
    Q_D(QRemoteUser);
    QObjectPrivate *pr = QObjectPrivate::get(const_cast<QObject *>(local));
    if (pr->isRemote) {
        // Can not change visibilty of services provided by others.
        return false;
    }
    QRemoteData *dat = QMetaRemote::dataFromObject(local);
    if (dat) {
        const QRemoteAddress &address = QRemoteAddress(local);
        MapLocal::const_iterator it = d->locals.find(address);
        if(it == d->locals.end()) {
            return false; //object is not registered in this QRemoteUser
        }
        QScopedPointer<StatusPacket> pkt(new StatusPacket);
        pkt->setType(StatusPacket::Status);
        pkt->setRemotes(StatusPacket::RemoteInfos() << address);
        pkt->setReceiver(user);
        pkt->setVisible(visible);
        send(*pkt);
        return true;
    }
    return false; //object is not yet registered on any QRemoteUser
}

// MARK: Packet settings (creation format, limit and etc).

const PacketCodec *QRemoteUser::codec() const
{
    const Q_D(QRemoteUser);
    return d->packetCodec;
}

PacketCodec *QRemoteUser::codec()
{
    Q_D(QRemoteUser);
    return d->packetCodec;
}


void QRemoteUser::setCodec(PacketCodec *codec)
{
    Q_D(QRemoteUser);
    d->packetCodec = codec;
}

QSharedPointer<PacketCodec> QRemoteUser::defaultCodec() {
    return QRemoteUserPrivate::instanceManager()->defaultCodec();
}

void QRemoteUser::setDefaultCodec(QSharedPointer<PacketCodec> &codec) {
    QRemoteUserPrivate::instanceManager()->setDefaultCodec(codec);
}

quint32 QRemoteUser::packetSizeLimit() const
{
    Q_D(const QRemoteUser);
    return d->packetSizeLimit;
}

void QRemoteUser::setPacketSizeLimit(quint32 sizeLimit)
{
    Q_D(QRemoteUser);
    d->packetSizeLimit = sizeLimit;
}

/**
 * @internal
 *
 * Gets called by per-device signal of DeviceHandler, and
 * it first notifies the device about limit error, then
 * closes device, finally triggers one-for-all-devices signal.
 *
 * @see ofPacketLimitExceed
 */
void QRemoteUser::onPacketLimitExceed(DeviceHandler *causer, quint32 sizeLimit)
{
    // First notifies the device about limit error.
    ErrorPacketSP err(new ErrorPacket);
    err->setType(ErrorPacket::Error);
    err->setErrorType(ErrorPacket::ProtocolError);
    err->setErrorMessage(QString(QLatin1Literal("MethodPacket exceeded '%1' bytes limit."))
            .arg(sizeLimit));
    send(*err);

    // Then closes device.
    causer->device()->close();

    // Finally triggers one-for-all-devices signal.
    emit ofPacketLimitExceed(causer->device());
}

bool QRemoteUser::isDisconnectSendable() const
{
    Q_D(const QRemoteUser);
    return d->isDisconnectSendable;
}

bool QRemoteUser::setDisconnectSendable(bool enabled)
{
    Q_D(QRemoteUser);
    const bool old = d->isDisconnectSendable;
    d->isDisconnectSendable = enabled;
    return old;
}

bool QRemoteUser::isDisconnectReceivable() const
{
    Q_D(const QRemoteUser);
    return d->isDisconnectReceivable;
}

bool QRemoteUser::setDisconnectReceivable(bool enabled)
{
    Q_D(QRemoteUser);
    const bool old = d->isDisconnectReceivable;
    d->isDisconnectReceivable = enabled;
    return old;
}

// MARK: Packet I.O. device management.

void QRemoteUser::addDevice(QIODevice *newDevice)
{
    Q_D(QRemoteUser);

    // TODO: maybe use qFatal instead, to force even in release.
    Q_ASSERT_X( ! objectName().isEmpty(), "QRemoteUser", "unique-name is missing.");

    if ( ! newDevice) {
        qWarning("QRemoteUser: IO-device should be non-null.");
        return;
    }

    // Skips if already added.
    const int handlerCount = d->deviceHandlers.count();

    for (int i = 0; i < handlerCount; ++i) {
        const QSharedPointer<DeviceHandler> &handler = *d->deviceHandlers.ptr(i);
        if (handler->device() == newDevice) {
            return;
        }
    }

    // Should set parent to "this" for debugging support.
    QSharedPointer<DeviceHandler> handler(new DeviceHandler(this));
    d->deviceHandlers.append(handler);

    // Needs to be after append to `deviceHandlers`, since
    // device's destroy causes remove from `deviceHandlers`.
    handler->setDevice(newDevice);

    sendStatus(handler.data(), QString(), Packet::FlagsEmpty, true);
}

QIODevice *QRemoteUser::deviceAt(int i)
{
    return d_func()->deviceHandlers[i]->device();
}

int QRemoteUser::deviceCount() const
{
    return d_func()->deviceHandlers.count();
}

QIODevice *QRemoteUser::takeDeviceAt(int i)
{
    QSharedPointer<DeviceHandler> handler = qMove(d_func()->deviceHandlers.takeAt(i));
    sendDisconnect(handler.data());
    QIODevice *device = handler->device();
    handler->dispose(false);
    return device;
}

// MARK: Packet upload/send and download/receive helpers.

void QRemoteUser::send(Packet &pkt)
{
    Q_D(QRemoteUser);
    pkt.setSender(this->objectName()); //to ensure not getting send again
    if (d->packetCodec /*&& d->deviceHandlers.count()*/ ) {
        emit sendData(d->packetCodec->encode(pkt));
    }

    // Notify App itself only at end, to speed up communication.
    if ((pkt.type() & Packet::ErrorMask) != 0) {
        ErrorPacket *err = reinterpret_cast<ErrorPacket *>(&pkt);
        emit ofSendingError(this, err->errorType(), err->errorMessage());
    }
}

MethodPacket *QRemoteUser::request(MethodPacket &pkt, long waitTime)
{
    Q_D(QRemoteUser);
    if (d->packetCodec) {
        QScopedPointer<ReplyWaiter> rw(
                    new ReplyWaiter(d, &pkt)
                );
        send(pkt);

        //ensure data is sent before waiting for response
        bool hasDevice = false;
        QRemoteUserPrivate::DeviceHandlers::const_iterator it;
        QRemoteUserPrivate::DeviceHandlers::const_iterator end = d->deviceHandlers.cend();
        for (it = d->deviceHandlers.cbegin(); it != end; ++it) {
            const QSharedPointer<DeviceHandler> &h = *it;
            if(h && h->flush()) {
                hasDevice = true; //waitForBytesWritten
            }
        }

        if(hasDevice) {
            QElapsedTimer stopwach; stopwach.start();
            int remaining = 0;
            while (rw->waiting() && (remaining = stopwach.timeLeft(waitTime))) {
#if QREMOTE_SLOT_BLOCK_EVENTS
                // Blocks QApplication's main-loop if called in main-thread.
                foreach (QSharedPointer<DeviceHandler> handler, d->deviceHandlers) {
                    if (handler->device()
                        && handler->device()->bytesAvailable() > 0
                    ) {
                        handler->receiveData();
                    }
                }
                QThread::msleep(20);
#else
                // Could use `ExcludeUserInputEvents`, to prevent conflicts, but
                // the App should itself disable GUI that may conflict.
                QCoreApplication::processEvents(QEventLoop::AllEvents, 100);
                QCoreApplication::sendPostedEvents(Q_NULLPTR, QEvent::DeferredDelete);
#endif
            }

            Packet *reply = Packet::load(rw->result);
            if(reply) {
                if(reply->type() == Packet::SlotReply)
                    return reinterpret_cast<MethodPacket *>(reply);
                delete reply;
            }
        }
    }
    return Q_NULLPTR;
}

void QRemoteUser::receiveRequestReply(const MethodPacket &packet)
{
    Q_D(QRemoteUser);
    QMutexLocker locker(&d->waitersMutex);
    const QByteArray &buffer = Packet::save(packet);
    QRemoteUserPrivate::WaiterHash::iterator it = d->waiters.find(packet.id());
    // TRACE/remote: Caching QHash::end() #x
    QRemoteUserPrivate::WaiterHash::const_iterator end = d->waiters.constEnd();
    while (it != end && it.key() == packet.id()) {
        ReplyWaiter *w = it.value();
        w->wake(buffer);
        it = d->waiters.erase(it);
    }
}

static QBasicAtomicInt qRemoteRequestId = Q_BASIC_ATOMIC_INITIALIZER(786);

/// Requests are separated by sender (QRemoteUser::objectName) anyway, hence
/// an atomic number should be enough.
///
/// However, uses QByteArray to be future proof.
QByteArray QRemoteUser::nextRequestId()
{
    return QByteArray::number(qRemoteRequestId.fetchAndAddRelaxed(1));
}

QByteArray QRemoteUser::currentRequestId()
{
    return QByteArray::number(qRemoteRequestId);
}

void QRemoteUser::receiveData(const QByteArray &pkt)
{
    Q_D(QRemoteUser);
#if QREMOTE_DEBUG
    QDebug debug = qDebug();
    debug << "QRemoteUser.receiveData:";
    debug << pkt.size() << "bytes";
#endif

    // Ensures Codec's set.
    if (d->packetCodec == Q_NULLPTR) {
        QREMOTE_DEBUG_SCOPE(debug << "Codec missing.";)
        return;
    }

    // Decodes packet, or handles related error.
    PacketUniquePtr packet;
    try {
        packet = d->packetCodec->decode(pkt);
#if QREMOTE_DEBUG
        debug << "{\n" << *packet << "\n}";
#endif
    } catch (const PacketCodecException &e) {
        ErrorPacketSP err(new ErrorPacket);
        err->setErrorType(e.packetErrorType);
        err->setErrorMessage(e.message());
#if QREMOTE_DEBUG
        debug << "decode failed, sending error:" << e.message();
        debug.flush();
#endif
        send(*err);
        return;
    }

#if QREMOTE_DEBUG
    debug.flush();
#endif

    receive(packet);
}

void QRemoteUser::receive(PacketUniquePtr &packet)
{
    // Skips if this instance was not the target.
    if( ! packet->receiver().isEmpty()
       && packet->receiver() != objectName()) {
        return;
    }

    if((packet->type() & Packet::MethodMask) != 0) {
        receiveMethod(*reinterpret_cast<MethodPacket *>(packet.get()));
    } else if((packet->type() & Packet::ErrorMask) != 0) {
        ErrorPacketSP err(reinterpret_cast<ErrorPacket *>(packet.release()));
#if QREMOTE_DEBUG
        QDebug debug = qDebug();
        debug << "QRemoteUser:receive.Error:" << err->errorMessage();
        debug.flush();
#endif
        emit ofReceivedError(this, err->errorType(), err->errorMessage());
        return;
    } else if (packet->type() == StatusPacket::RequestStatus) {
        DeviceHandler *handler = qobject_cast<DeviceHandler *>(sender());
        sendStatus( handler, packet->sender(), Packet::NoReply
                  , packet->flags().excludes(MethodPacket::NoObjectStatus));
        return;
    } else if (packet->type() == StatusPacket::Status) {
        receiveStatus(*reinterpret_cast<StatusPacket *>(packet.get()));
        return;
    }
}

void QRemoteUser::receiveMethod(const MethodPacket &packet)
{
    Q_D(QRemoteUser);

    QDebug *dbg;
#if QREMOTE_DEBUG
    QDebug debug = qDebug();
    debug << "QRemoteUser.receiveMethod:";
    debug.setMessageEnabled(false);
    dbg = &debug;
#else
    dbg = 0;
#endif
    if(packet.type() == Packet::SlotReply) {
        return receiveRequestReply(packet);
    }

#if 1
    //FastMode: anything should be registered using status packet
#else
    //Ensure: registered "remote" if signal
    //  but if "InvokeSlot" we ensure signaler registered
    QObjectRemote *remote = 0;
    if(packet.type() == MethodPacket::RemoteSignal) {
        remote = registerRemote(QRemoteLink(packet.sender(), packet.address()));
    } else if(packet.type() == MethodPacket::InvokeSlot) {
        if(packet.signaler().user != objectName())
            remote = registerRemote(packet.signaler());
    }
#endif

    if(packet.type() == Packet::RemoteSignal) {
        //emit using objects with Q_REMOTE_CONTROLLER macro
        return receiveRemoteSignal(packet, dbg);
    }

    //search in objects with Q_REMOTE macro
    MapLocal::iterator it;
    MapLocal::iterator end = d->locals.end();

    const bool singleTarget = (packet.address().isEmpty() == false);
    if (singleTarget) {
        it = d->locals.find(packet.address());
        if (it == end) {
            sendAddressError(packet, dbg);
            return;
        }
        end = it + 1; //skip all others
    } else {
        it = d->locals.begin();
#if QREMOTE_DEBUG
        if(it == end)
            debug << "no service registered!";
#endif
    }

#if QREMOTE_DEBUG
    debug.flush();
#endif

    for(; it != end; ++it) {
        QRef<QObject> receiver = (*it);
        try {
//            if(singleTarget) {
//                if (packet.receiver() != (*res)->qt_address())
//                    throw( ::QRemote::InvalidMethodException(QObjectRemote::tr("Invalid receiver: %1 expected: %2").arg(packet.receiver(), (*res)->qt_address())) );
//                if (singleTarget && packet.sender() == (*res)->qt_address())
//                    throw( ::QRemote::InvalidMethodException(QObjectRemote::tr("Invalid sender equals receiver: %1").arg(packet.sender())) );
//            }

            const MethodPacketPrivate *d = MethodPacketPrivate::get(&packet);
            Q_UNUSED(d) // Not always used.
            Q_ASSERT_X(d && !d->methodName.isEmpty(),
                       "QRemoteUser", "Method info missing.");

            QMetaRemote::receive(receiver.data(), packet);
        } catch (InvalidMethodException &e) {
#if QREMOTE_DEBUG
            debug << "QObjectRemote.receiveMethod: packet process failed->";
#endif
            if(singleTarget) {
                if(!d->packetCodec)
                    return;
                MethodErrorPacketSP err(new MethodErrorPacket);
                err->setErrorType(MethodErrorPacket::InvalidMethod);
                err->setErrorMessage(e.message());
                err->setReceiver(packet.sender());
                err->setMethod(packet.method());
#if QREMOTE_DEBUG
                debug << "sending error: Incorrect method";
                debug.flush();
#endif
                send(*err);
                return;
            } else {
#if QREMOTE_DEBUG
                debug << "ignoring error";
                debug.flush();
#endif
            }
        }
    }
}

void QRemoteUser::receiveRemoteSignal(const MethodPacket &packet, QDebug *dbg)
{
    //Q_D(QRemoteUser);

    if( ! packet.address().isEmpty()) {
        //search in objects with Q_REMOTE_CONTROLLER macro for "packet.address()"
        QRef<QObjectRemote> receiver = findRemote(QRemoteLink(packet.sender(), packet.address()));
        if(receiver) {
            QMetaRemote::receive(receiver.data(), packet);
        } else {
            sendAddressError(packet, dbg);
        }
    } else {
        //search in all objects with Q_REMOTE_CONTROLLER macro
        MapRemoteUser::iterator user = d_func()->remotes.find(packet.sender());
        if(user != d_func()->remotes.end()) {
            MapRemote &list = user.value();
            MapRemote::iterator it = list.begin();
            MapRemote::iterator end = list.end();
            for(; it != end; ++it) {
                QRef<QObjectRemote> &receiver = it.value();
                QMetaRemote::receive(receiver.data(), packet);
            }
        }
    }
}

void QRemoteUser::receiveStatus(const QRemote::StatusPacket &pkt)
{
#if QREMOTE_DEBUG && 0
    QDebug debug;
    debug << "QRemoteUser.receiveStatus:";
    debug.flush();
#endif
//    if(pkt.type() != Packet::Status || !pkt.isValid())
//        return;

    Q_D(QRemoteUser);

    // TRACE/remote status: broadcast shutdown #2,
    // which's received here.
    if (pkt.isDisconnect()) {
        if (d->isDisconnectReceivable && ! pkt.sender().isEmpty()) {
            clearRemotes(pkt.sender());
        }
        return;
    }

    const bool newUser = pkt.receiver().isEmpty();
    if (newUser) {
        if ( ! d->remotes.contains(pkt.sender())) {
            d->remotes.insert(pkt.sender(), MapRemote());
        }
    }

    const bool visible = pkt.isVisible();
    QRemoteLink address;
    address.userName = pkt.sender();

    const StatusPacket::RemoteInfos &pktRemotes = pkt.remotes();
    const int count = pktRemotes.count();
    for (int i = 0; i < count; ++i) {
        address.typeName = pktRemotes[i].typeName;
        address.id = pktRemotes[i].id;
        if (visible)
            createRemote(address);
        else
            unregisterRemote(address);
    }

    // TRACE/remote TODO: limit number of times we reply, for security-reasons.
    if (newUser && pkt.flags().excludes(Packet::NoReply)) {
        sendStatus(Q_NULLPTR, pkt.sender(), Packet::NoReply, true);
    }
}

void QRemoteUser::sendStatus(DeviceHandler *device, const QString &requester
                             , Packet::PktFlags flags
                             , bool sendServiceList)
{
    Q_D(QRemoteUser);

    // Prepare.
    QScopedPointer<StatusPacket> pkt(new StatusPacket);
    pkt->setReceiver(requester);
    pkt->setFlags(flags);

    // Maybe list local-services.
    if (sendServiceList) {
        StatusPacket::RemoteInfos &serviceList = pkt->remotes();
        serviceList.reserve(d->locals.count());
        MapLocal::const_iterator it = d->locals.cbegin();
        MapLocal::const_iterator end = d->locals.cend();
        for (; it != end; ++it) {
            const QRef<QObject> obj = it.value();
            serviceList.append(QRemoteAddress(obj.data()));
        }
    }

    // Finally, sends.
    if (device) {
        // Direct send.
        pkt->setSender(this->objectName());
        if (d->packetCodec) {
            device->sendData(d->packetCodec->encode(*pkt));
        }
    } else {
        // Broadcast.
        send(*pkt);
    }
}

bool QRemoteUser::sendDisconnect(QRemote::DeviceHandler *device)
{
    Q_D(QRemoteUser);
    if (d->isDisconnectSendable) {
        // Ensures all divice-handlers receive our final packet through signal.
        bool wasBlockSig = d->blockSig;
        d->blockSig = 0;
        QT_FINALLY([&] { d->blockSig = wasBlockSig; });
        // Actual packet.
        QScopedPointer<StatusPacket> pkt(new StatusPacket);
        pkt->setType(Packet::Status);
        pkt->setVisible(false);
        pkt->setFlags(Packet::NoReply);

        // Finally, sends.
        if (device) {
            // Direct send.
            pkt->setSender(this->objectName());
            if (d->packetCodec) {
                device->sendData(d->packetCodec->encode(*pkt));
            }
        } else {
            // Broadcast.
            send(*pkt);
        }
        return true;
    }

    return false;
}

void QRemoteUser::sendAddressError(const MethodPacket &packet, QDebug *dbg)
{
    Q_D(QRemoteUser); Q_UNUSED(dbg)
    if( ! d->packetCodec) {
        return;
    }
    ErrorPacketSP err(new ErrorPacket);
    err->setErrorType(ErrorPacket::UnknownService);
    err->setErrorMessage(QStringLiteral("Unknown address: \"%1\"")
                            .arg(packet.receiver()));
    err->setReceiver(packet.sender());
    err->setAddress(packet.address());
#if QREMOTE_DEBUG
    if(dbg) {
        *dbg << "sending error: cant find receiver";
#  if QREMOTE_DEBUG_VERBOSE
        const QString &user = objectName();
        dbg->noQuotes();
        if(packet.type() == Packet::RemoteSignal) {
            MapRemoteUser::const_iterator iUser = d->remotes.constFind(packet.sender());
            if(iUser != d->remotes.constEnd()) {
                const MapRemote &list = iUser.value();
                *dbg << "in" << list.count() << "controllers {\n";
                MapRemote::const_iterator it = list.constBegin();
                MapRemote::const_iterator end = list.constEnd();
                for(; it != end; ++it) {
                    *dbg << '\"' << user << '/' << it.key() << '\"' << "\n";
                }
            }
        } else {
            *dbg << "in" << d->locals.count() << "locals {\n";
            const MapLocal::const_iterator end = d->locals.constEnd();
            for(MapLocal::const_iterator it = d->locals.constBegin(); it != end; ++it) {
                *dbg << '\"' << user << '/' << it.key() << '\"' << "\n";
            }
        }
        dbg->nospace() << "}";
#  endif
        dbg->setMessageEnabled();
        dbg->flush();
    }
#endif
    send(*err);
}

QT_END_NAMESPACE
