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

#ifndef QT_REMOTE_USER_P_H
#define QT_REMOTE_USER_P_H

#include "qremoteuser.h"

#include "qremote-device-handler.h"
#include "qremote-stream-codec.h"
#include "qremoteobject.h"
#include "qremoteobject_p.h"

#include <private/qobject_p.h>
#include <QtCore/QBasicTimer>
#include <QtCore/QPointer>
#include <QtCore/QSharedPointer>
#include <QtCore/QThreadStorage>
#include <QtCore/QMutex>
#include <QtCore/QWriteLocker>


QT_BEGIN_NAMESPACE

namespace QRemote {


class ReplyWaiter {
    Q_DISABLE_COPY(ReplyWaiter)

    class ReplyWaiterPrivate {
    public:
        QRemoteUserPrivate *session;
        MethodPacket *request;
        bool gotReply;
    }  d;
public:
    ReplyWaiter(QRemoteUserPrivate *smp, const MethodPacket *pkt);
    ~ReplyWaiter();

    inline bool waiting() { return !d.gotReply; }

    inline void wake(const QByteArray &result) {
        Q_UNUSED(result)
        d.gotReply = true;
        this->result = result;
    }

    //inline const MethodPacket &request() const { return reinterpret_cast<MethodPacket &>(d->request); }
    inline const QByteArray &id() const { return d.request->id(); }

    QByteArray result;
};


typedef QMap<QRemoteAddress, QRef<QObject> > MapLocal;
typedef QMap<QRemoteAddress, QRef<QObjectRemote> > MapRemote;
typedef QMap<QRemoteUserName, MapRemote > MapRemoteUser;
typedef QMultiHash<QByteArray, QT_PREPEND_NAMESPACE(QRemote)::ReplyWaiter *> WaiterHash;

class QRemoteUserCounter
{
public:
    QReadWriteLock instanceMutex;
    QList<QRemoteUser *> instanceList;
    QRemoteUser *m_instance;
    QSharedPointer<PacketCodec> m_codec;
    enum Flag {
        FlagIsDeleted = 10,
        FlagIsInstanceOwner = 160
    };
    QAtomicFlags<Flag > flags;

    inline QRemoteUserCounter()
        : instanceMutex(QReadWriteLock::Recursive)
    {
    }

    inline ~QRemoteUserCounter()
    {
        QReadLocker locker(&instanceMutex);
        flags.append(FlagIsDeleted);
        if (flags.includes(FlagIsInstanceOwner)) {
            QRemoteUser *user = m_instance;
            m_instance = Q_NULLPTR;
            locker.unlock();
            delete user;
        }
    }

    inline QRemoteUser *instance() {
        QReadLocker readLocker(&instanceMutex);
        if (flags.includes(QRemoteUserCounter::FlagIsDeleted)) {
            return Q_NULLPTR;
        }
        if( ! m_instance) {
            readLocker.unlock();
            QWriteLocker locker(&instanceMutex);
            if ( ! m_instance) {
                new QRemoteUser();
                flags.append(FlagIsInstanceOwner);
            }
        }
        return m_instance;
    }

    inline void append(QRemoteUser *v, bool storeFirst);

    inline void remove(QRemoteUser *v) {
        QWriteLocker locker(&instanceMutex);
        if (flags.includes(FlagIsDeleted)) {
            return;
        }
        instanceList.removeOne(v);
        if (v == m_instance) {
            m_instance = Q_NULLPTR;
        }
    }

    inline QRemoteUser *findInstance(const QRemoteUserName &user) {
        QReadLocker locker(&instanceMutex);
        for (int i = 0; i < instanceList.count(); ++i) {
            QRemoteUser *sm = instanceList.at(i);
            if (sm->objectName() == user) {
                return sm;
            }
        }
        return Q_NULLPTR;
    }

    QSharedPointer<PacketCodec> &defaultCodec() {
        QReadLocker locker(&instanceMutex);
        return m_codec;
    }

    void setDefaultCodec(QSharedPointer<PacketCodec> &v) {
        QWriteLocker locker(&instanceMutex);
        m_codec = v;
    }
};

} //namespace QRemote


class QRemoteUserPrivate : public QObjectPrivate {
    friend class QRemote::QRemoteUserCounter;
    Q_DECLARE_PUBLIC(QRemoteUser)
public:
    QRemoteUserPrivate()
        : packetSizeLimit(0)
        , isDisconnectSendable(true)
        , isDisconnectReceivable(true)
    {}
    static inline QRemoteUserPrivate *get(QRemoteUser *o) { return o->d_func(); }
    static inline const QRemoteUserPrivate *get(const QRemoteUser *o) { return o->d_func(); }
    static inline QRemoteUserPrivate *get(QSharedPointer<QRemoteUser> &o) { return o.data()->d_func(); }
    static inline const QRemoteUserPrivate *get(const QSharedPointer<QRemoteUser> &o) { return o.data()->d_func(); }

    static QRemote::QRemoteUserCounter *instanceManager();
    struct PerThreadStorageGlobal {
        inline PerThreadStorageGlobal()
            : instance(Q_NULLPTR)
        {}
        QRemoteUser *instance;
    };
    static QThreadStorage<PerThreadStorageGlobal> threadStoreGlobal;

    // MARK: Service helpers (register, find and etc).

    /// List of services we provide (i.e. objects with Q_REMOTE macro).
    QRemote::MapLocal locals;
    /// List of remote-services we control
    /// (i.e. generated objects with Q_REMOTE_CONTROLLER macro).
    QRemote::MapRemoteUser remotes;

    QMutex remotesMutex;
    QBasicTimer pollTimer;
    enum {
        Second = 1000,
        Minute = 60 * Second,
        Hour = 60 * Minute,
        Day = 24 * Hour,
        PollDelay = 1 * Minute
    };

    // MARK: Packet creation format and settings.

    QPointer<QRemote::PacketCodec> packetCodec;
    quint32 packetSizeLimit;
    bool isDisconnectSendable;
    bool isDisconnectReceivable;

    // MARK: Packet I.O. device management.

    typedef QVector< QSharedPointer<QRemote::DeviceHandler> > DeviceHandlers;
    /// @note that QSharedPointer has #Q_MOVABLE_TYPE.
    DeviceHandlers deviceHandlers;

    // MARK: Packet upload/send and download/receive helpers.

    QMutex waitersMutex;
    QMultiHash<QByteArray, QRemote::ReplyWaiter *> waiters;
    QReadWriteLock receiveMutex;

    struct PerThreadStorage {
        inline PerThreadStorage()
        {
            requestEventMode = QtPrivate::remoteEventMode;
        }

        int requestEventMode;
    };
    QThreadStorageScoped<PerThreadStorage> threadStore;
};

#ifndef Q_MOC_RUN
namespace QRemote {

inline void QRemoteUserCounter::append(QRemoteUser *v, bool storeFirst) {
    QWriteLocker locker(&instanceMutex);
    if (flags.includes(QRemoteUserCounter::FlagIsDeleted)) {
        return;
    }
    if (storeFirst && m_instance == Q_NULLPTR) {
        m_instance = v;
    }
    instanceList.append(v);

    if (m_codec.data() == Q_NULLPTR) {
        m_codec = QSharedPointer<QRemote::PacketCodec>(
            new QRemote::StreamPacketCodec(v, QDataStream::Qt_5_6)
        );
    }
    QRemoteUserPrivate::get(v)->packetCodec = m_codec.data();
}

} // namespace QRemote
#endif // Q_MOC_RUN

QT_END_NAMESPACE

#endif //QT_REMOTE_USER_P_H
