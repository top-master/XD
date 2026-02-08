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

#ifndef QT_REMOTE_USER_H
#define QT_REMOTE_USER_H

#include "qremote-packet.h"

#include <QtCore/QObject>
#include <QtCore/QSharedPointer>

#include <limits.h>


QT_BEGIN_NAMESPACE

class QObjectRemote;
class QIODevice;

namespace QRemote {

class PacketCodec;
class DeviceHandler;

} //namespace QRemote



/**
 * @class QRemoteUser
 * 
 * @note Header methods are sorted by common call-order, for example
 * it's faster if local services get registered first, then later
 * you add device, because else QRemote::StatusPacket is sent twice
 * (once empty to report new User's existence, then later to report services).
 */
class QRemoteUserPrivate;
class QT_REMOTE_EXPORT QRemoteUser : public QObject {
    Q_OBJECT
public:

    // MARK: Instance creation and finder.

    explicit QRemoteUser(QObject *parent = Q_NULLPTR, bool storeFirst = true);
    virtual ~QRemoteUser();

    static QRemoteUser *instance();
    static QRemoteUser *findInstance(const QRemoteUserName &user);

    inline QByteArray userId() const { return objectName().toUtf8(); }
    inline void setUserId(const QByteArray &v) { setObjectName(QString::fromUtf8(v.constData(), v.size())); }

    // MARK: Service helpers (register, find and etc).

    bool registerLocal(const QRef<QObject> &service);
    QRef<QObject> unregisterLocal(const QRemoteAddress &address, QRemoteData *data = Q_NULLPTR);

    QRef<QObjectRemote> createRemote(const QRemoteLink &address);

    /// @internal
    /// Remote objects are autmatically created and registered
    /// hence "registerRemote(...)" is not meant to be called, that is
    /// unless you're sure what you're doing.
    bool registerRemote(const QRef<QObjectRemote> &obj);
    QRef<QObjectRemote> unregisterRemote(const QRemoteLink &address);

    /// @note Accepts both local-services (@ref QObject) and
    /// remote-controllers (@ref QObjectRemote).
    /// @sa registerRemote(...)
    bool registerOnce(const QRef<QObject> &obj);

    /// Unregisters given @p obj instance, even if renamed, and that
    /// no matter if it's a local or remote service.
    bool unregister(const QRef<QObject> &obj);

    /// Whether given @p object is registered as service, and that
    /// no matter if it's a local or remote service.
    bool contains(const QObject *object) const;

    /// Unregisters all remote services, or
    /// if @p user is specified, only those regestered for given @p user.
    void clearRemotes(const QRemoteUserName &user = QRemoteUserName());

    /// Finds local-service if any registered by given @p name.
    QRef<QObject> findLocal(const QRemoteAddress &name);
    /// Finds remote-service, if any registered for given @p link.
    QRef<QObjectRemote> findRemote(const QRemoteLink &link) const;
    /// Similar to findRemote(const QRemoteLink &), but
    /// ignores owning @ref QRemoteUser, and first Service found is returned, where
    /// if failed returns @c nullptr.
    QRef<QObjectRemote> findRemoteId(const QRemoteAddress &name) const;

    bool setObjectVisible(const QObject *local, bool visible = true, const QString &user = QString());
    inline bool showObject(const QObject *local, const QString &user) { return setObjectVisible(local, true, user); }
    inline bool hideObject(const QObject *local, const QString &user) { return setObjectVisible(local, false, user); }

    // MARK: Packet communication settings: encoder/decoder, limit and etc.

    const QRemote::PacketCodec *codec() const;
    QRemote::PacketCodec *codec();
    void setCodec(QRemote::PacketCodec *codec);

    static QSharedPointer<QRemote::PacketCodec > defaultCodec();
    static void setDefaultCodec(QSharedPointer<QRemote::PacketCodec > &codec);

    /// Getter for packet size limitation (if ever set, default is zero).
    /// @se setPacketSizeLimit
    Q_REQUIRED_RESULT quint32 packetSizeLimit() const;

    /// Enables packet size limitation, where default zero means disabled.
    ///
    /// @note This limit's only for @ref QIODevice, and
    /// both @ref receiveData and @ref receive slots ignore this.
    ///
    /// @sa packetSizeLimit()
    /// @sa onPacketLimitExceed
    void setPacketSizeLimit(quint32 sizeLimit);

    /// Mode of event-handling while waiting for slot replies.
    /// @warning This only reads current-thread's configuration, since we
    /// intentionally use thread-local storage to behave that way.
    int requestEventMode() const;

    /// Sets how to handle events while waiting for slot replies.
    /// @warning This only changes current-thread's configuration, since we
    /// intentionally use thread-local storage to behave that way.
    ///
    /// @note You could use @c ExcludeUserInputEvents, to prevent conflicts, but
    /// the App should itself disable GUI that may conflict.
    ///
    /// @param flags Can be either QRemote::BlockEvents, or, any combination of
    /// the QEventLoop::ProcessEventsFlag
    void setRequestEventMode(int flags);

    /// Getter for @ref setDisconnectSendable.
    /// @returns @c true by default.
    Q_REQUIRED_RESULT bool isDisconnectSendable() const;
    /// Sets whether `delete` of "this user" should send a packet, which
    /// on receiving side triggers @ref clearRemotes (with this user's name).
    /// @returns Old state of this setting.
    bool setDisconnectSendable(bool enabled);

    /// Getter for @ref setDisconnectReceivable.
    /// @returns @c true by default.
    Q_REQUIRED_RESULT bool isDisconnectReceivable() const;
    /// Sets whether `delete` of "remote users" should unregister related services.
    /// @returns Old enable-state of this setting.
    bool setDisconnectReceivable(bool enabled);

    // MARK: Packet I.O. device management.

    /// Enlists given @p device for read/ write of QRemote communication packets.
    ///
    /// WARNING: device(s) are never deleted by QRemote directly, but
    /// you could use @ref QIODevice::setParent, which we support.
    ///
    /// @param isThreadSafe Set this to @c true if the device is thread-safe.
    void addDevice(QIODevice *device, bool isThreadSafe = false);
    Q_REQUIRED_RESULT QIODevice *deviceAt(int i);
    Q_REQUIRED_RESULT int deviceCount() const;

    Q_REQUIRED_RESULT QIODevice *takeDeviceAt(int i);
    inline void removeDeviceAt(int i) { (void) this->takeDeviceAt(i); }
    void removeAllDevices();

    // MARK: Packet upload/send and download/receive helpers.

    /// WARNING: this will set "sender" to @ref QRemoteUser's id
    /// (the QObject::objectName()).
    ///
    /// @note Used for emitting remote signals, Status-packets and Error-packets,
    /// but NOT for remote "slots".
    void send(QRemote::Packet &pkt);
    /// WARNING: caller should take ownership of returned QRemote::MethodPacket
    /// (i.e. `delete` it).
    ///
    /// @note Used for triggering remote "slots" only.
    QRemote::MethodPacket *request(QRemote::MethodPacket &pkt, long waitTime = -1, int eventMode = 0xdead);
    /// @note Used for "slot" result receivement.
    void receiveRequestReply(const QRemote::MethodPacket &pkt);

    static QByteArray nextRequestId();
    static QByteArray currentRequestId();

public slots:
    /// Invoked when a packet should get received directly from device.
    void receiveData(const QByteArray &pkt);

    /// Invoked by receiveData().
    virtual void receive(QRemote::PacketUniquePtr &pkt);

public:
    void receiveMethod(const QRemote::MethodPacket &pkt);
    void receiveRemoteSignal(const QRemote::MethodPacket &pkt, QDebug *dbg);

    void receiveStatus(const QRemote::StatusPacket &pkt);
    void sendStatus( QRemote::DeviceHandler *device = Q_NULLPTR
                   , const QString &requester = QString()
                   , QRemote::Packet::PktFlags flags = QRemote::Packet::FlagsEmpty
                   , bool sendServiceList = true);
    /// Short-hand for @ref sendStatus with disconnection-request-status.
    bool sendDisconnect(QRemote::DeviceHandler *device = Q_NULLPTR);

    // MARK: Notifications.
signals:
    /// This signal is emitted when a packet encoded to
    /// bytes (by codec) should be sent directly to device.
    void sendData(const QByteArray &pkt);

    /// Emitted when @ref registerRemote registers new remote object.
    void newConnection(const QRef<QObjectRemote> &remote);
    void disconnected(const QRef<QObjectRemote> &remote);

    /// Signals that we're about to send a QRemote::ErrorPacket, which's
    /// probably caused by @p sender's packet being rejected by @ref receiveData.
    void ofSendingError(QRemoteUser *sender,
        QRemote::ErrorPacket::ErrorType error, const QString &message);

    /// Signals that we received a QRemote::ErrorPacket, which's
    /// probably caused by @p sender's invalid packet(s) sent.
    void ofReceivedError(QRemoteUser *sender,
        QRemote::ErrorPacket::ErrorType error, const QString &message);

    void ofPacketLimitExceed(QIODevice *device);

    // MARK: Internal even if public.
public:

    /// @internal
    /// Required by "QRef<" meta-type.
    ///
    /// QVariant does not provide any context, hence we use Thread-Local-Storage.
    static QRemoteUser *fromThreadStorage();
    /// @internal
    static QRemoteUser *toThreadStorage(QRemoteUser *);
    /// @internal
    inline QRemoteUser *toThreadStorage() { return toThreadStorage(this); }

    // MARK: Private or protected internals.

protected:
    QRemoteUser( QRemoteUserPrivate &dd
               , QObject *parent = Q_NULLPTR
               , bool storeFirst = true);

    void timerEvent(QTimerEvent *event) Q_DECL_OVERRIDE;
    void onPolling();

private slots:
    friend class QRemote::DeviceHandler;
    void onPacketLimitExceed(QRemote::DeviceHandler *causer, quint32 limit);

private:
    void sendAddressError(const QRemote::MethodPacket &packet, QDebug * = Q_NULLPTR);

private:
    Q_DISABLE_COPY(QRemoteUser)
    Q_DECLARE_PRIVATE(QRemoteUser)
};

/// Same as `QRef<QRemoteUser>`, but has simpler forward declaration.
///
/// WARNING: only `QSharedPointer<QRemoteUser>` is supported by MOC's #Q_REMOTE handler.
class QT_REMOTE_EXPORT QRemoteUserShared : public QSharedPointer<QRemoteUser> {
    typedef QSharedPointer<QRemoteUser> super;
protected:
    inline explicit QRemoteUserShared(Qt::Initialization i) : super(i) {}
public:
    Q_DECL_CONSTEXPR inline QRemoteUserShared()
    {
    }

    inline explicit QRemoteUserShared(QRemoteUser *ptr)
        : super(ptr, QtSharedPointer::ObjectDeleter()) // throws
    {
    }

    /// Same as @ref QSharedPointer::fromStack, but with compatible return-type.
    Q_SKIP_UNUSED
    Q_INLINE_TEMPLATE static QRemoteUserShared fromStack(QRemoteUser *ptr)
        { QRemoteUserShared r(Qt::Uninitialized); r.internalConstructFake(ptr); return qMove(r); }
};

/// @internal
///
/// Required by QRemoteRef meta-type.
/// @sa QRemoteUser::fromThreadStorage
class QRemoteUserSaver {
public:
    inline QRemoteUserSaver(QRemoteUser *session)
        : m_saved(QRemoteUser::toThreadStorage(session))
    {}
    inline ~QRemoteUserSaver() { QRemoteUser::toThreadStorage(m_saved); }
private:
    QRemoteUser *m_saved;
};

QT_END_NAMESPACE

#endif // QT_REMOTE_USER_H
