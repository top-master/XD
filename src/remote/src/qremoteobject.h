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

#ifndef QT_QREMOTE_OBJECT_H
#define QT_QREMOTE_OBJECT_H

#include "qremote-config.h"
#include "qremote-exceptions.h"
#include "qremote-type.h"

#include <QtCore/qeventloop.h>


QT_BEGIN_NAMESPACE


class QRemoteUser;

namespace QRemote {
    class Packet;
    class MethodPacket;
    class PacketTypeEntry;

    /// Can be passed as flag to QRemoteEventModer.
    enum EventModeFlag {
        /// The given QObject is NOT usable as a #Q_REMOTE_CONTROLLER.
        InvalidController = 0xdead,
        /// Means to use QRemoteUser::requestWaitMode, without caching.
        InheritMode = 0x00f01100,
        /// This mode blocks all events of the calling-thread until the
        /// remote-slot request's reply arrives.
        /// remote slot request blocks the calling thread until reply
        BlockingMode = 0x00b10c00
    };
} // namespace QRemote

/**
 * @class QObjectRemote
 * @brief Base for any remote service, but not for local services.
 *
 * @warning Normally you never should inherit from this class, and
 * only moc generated classes inherit from this.
 */
class QObjectRemotePrivate;
class QRemoteBound;
class QT_REMOTE_EXPORT QObjectRemote : public QObject {
    Q_OBJECT
    Q_DECLARE_PRIVATE(QObjectRemote)
public:
    QObjectRemote(QObject *parent = 0);
    virtual ~QObjectRemote();

    Q_ALWAYS_INLINE QRemoteBound &remote() const { return *const_cast<QRemoteBound *>(reinterpret_cast<const QRemoteBound *>(this)); }

    inline bool isRemote() const { return d_ptr->isRemote; }

    inline bool remoteSlotsBlocked() const { return d_ptr->blockRemoteSlot; }
    bool blockRemoteSlots(bool block);

    inline bool remoteRepliesBlocked() const { return d_ptr->blockRemoteReply; }
    bool blockRemoteReplies(bool block);

    inline QObject *sender() const { return QObject::sender(); }
    inline int senderSignalIndex() const { return QObject::senderSignalIndex(); }
    inline int receivers(const char *signal) const { return QObject::receivers(signal); }

private:
    Q_DISABLE_COPY(QObjectRemote)
};

/// Data required by both remote and local object(s).
class QT_REMOTE_EXPORT QRemoteData : public QObjectUserData
{
public:
    QRemoteData();
    virtual ~QRemoteData();

    /// The QRemoteUser who owns the QIODevice session, that
    /// created the Servce which this QRemoteData blongs to, or
    /// if local, the QRemoteUser on which this said service was registered.
    QRemoteUser *session;

    /// Reference to any custom-data needed by #Q_REMOTE instance, and,
    /// is unused by QtRemote's source-codes.
    void *instanse;

    int eventMode;
};

template <typename T>
struct QMetaRemoteRegisterHelper {
    enum { Size = sizeof(T) };

    static void *Construct(void *where, QObject *parent)
    {
        return new (where) T(parent);
    }
};

struct QT_REMOTE_EXPORT QMetaRemote
{
    typedef void *(*Constructor)(void *, QObject *);


    static Q_ALWAYS_INLINE bool hasMacro(const QMetaObject *metaObj)
        { return metaObj->isRemote(); }
    static Q_ALWAYS_INLINE bool hasMacro(const QObject *instance)
        { return QMetaRemote::hasMacro(instance->metaObject()); }


    static QRef<QObjectRemote> registerLocal(QRef<QObject> local, QRemoteUser *owner = 0);

    //Q_REMOTE_CONTROLLER objects "*.cpp" files (generated with "moc" executable)
    //  do call below "registerRemote(...)" automatically to allow construct by "typeName"
    static void registerRemote(const char *typeName, Constructor constructor);
    template<typename T>
    static Q_INLINE_TEMPLATE const char *registerRemote(const char *typeName) {
        registerRemote(typeName, QMetaRemoteRegisterHelper<T>::Construct);
        return typeName;
    }
    //returns true if "obj" is an QObjectRemote subclass or registered local
    static inline bool isRegistered(const QObject *obj);
    //constructs Q_REMOTE_CONTROLLER objects from "typeName"
    static QRef<QObjectRemote> remoteFromType(const QByteArray &typeName, QObject *parent = 0);

    //call "registerLocal" first else this returns zero
    static QObjectRemote * QT_FASTCALL remoteObject(const QObject *local);
    static QRemoteData * QT_FASTCALL dataFromObject(const QObject *local);

    static QRemoteUser * QT_FASTCALL session(const QObject *obj);
    static void QT_FASTCALL setSession(QRef<QObject> &obj, QRemoteUser *val);

    static QRef<QObjectRemote> remoteInstance(const char *typeName, const QRemoteId &id = QRemoteId(), const QRemoteUserName &user = QRemoteUserName());

    //send signal and continue (i.e. creates "MethodPacket" of type "RemoteSignal" or "InvokeSignal")
    static void activate(QObject *sender, const QMetaObject *senderMetaObject,
                         int local_signal_index, void **argv);
    //send slot request and wait for reply (i.e. creates "MethodPacket" of type "InvokeSlot")
    static void request(QObjectRemote *sender, const QMetaObject *senderMetaObject,
                        int local_method_index, void **argv);

    template<typename T>
    static inline T request_cast(QObjectRemote *sender, const QMetaObject *senderMetaObject,
                                 int local_method_index, void **argv);

    //receive signals or slots and reply (i.e. creates "MethodPacket" of type "SlotReply")
    static void receive(QObject *receiver, const QRemote::Packet &pkt);


    /// Same as @ref remoteTimeFormat, but in UTF8.
    static const char * const timeStampFormatRaw;

    // Assumes QtRemote's binary uses same compiler as user.
#ifdef QStringLiteralGlobal
    /// Equals "yyyy-MM-dd HH:mm:ss.zzz".
    static const QString timeStampFormat;
#endif

    static QString timeStampFormatted();


    inline QObject *cast(QObject *obj) const { return reinterpret_cast<const QMetaObject *>(this)->cast(obj); }
    inline const QObject *cast(const QObject *obj) const  { return cast(const_cast<QObject *>(obj)); }

#ifndef QT_NO_TRANSLATION
    QString tr(const char *s, const char *c, int n = -1) const;
    QString trUtf8(const char *s, const char *c, int n = -1) const;
#endif // QT_NO_TRANSLATION

    struct { // private data
        const QMetaObject *superdata;
        const char *stringdata;
        const uint *data;
        const void *extradata;
    } d;
};

/// Helper inline-class, to enclose all remote functions in one name-space.
class QRemoteBound {
    Q_DISABLE_COPY(QRemoteBound)
public:
    inline QObjectRemote *object() const;

    /// Registers object as Service, to allow send/reply of signal/slot, where
    /// without this Q_REMOTE object signal will NOT get broadcast
    /// while Q_REMOTE_CONTROLLER object signal/slot will just return `Q_DEFAULT`.
    inline QRef<QObjectRemote> connect(QRemoteUser *sm = 0);
    inline int isConnected() const;
    inline QRemoteUser *session();
    inline void setSession(QRemoteUser *);
    inline QRemoteLink address() const;
    inline QString id() const;

    inline int timeout() const; //in mili-seconds
    inline void setTimeout(int);
    inline bool isTimeoutExpired() const;

    int QT_FASTCALL eventMode() const;
    int QT_FASTCALL setEventMode(int);
};


/**
 * Prevents local-model (i.e. QObject casted into QObjectRemote), from
 * sending and/or writting any "signal" into "QIODevice" of connection.
 */
class QRemoteSignalBlocker
{
public:
    inline explicit QRemoteSignalBlocker(QObject *o) Q_DECL_NOTHROW;
    inline explicit QRemoteSignalBlocker(QObject &o) Q_DECL_NOTHROW;
    inline ~QRemoteSignalBlocker();

#ifdef Q_COMPILER_RVALUE_REFS
    inline QRemoteSignalBlocker(QRemoteSignalBlocker &&other) Q_DECL_NOTHROW;
    inline QRemoteSignalBlocker &operator=(QRemoteSignalBlocker &&other) Q_DECL_NOTHROW;
#endif

    inline void reblock() Q_DECL_NOTHROW;
    inline void unblock() Q_DECL_NOTHROW;
private:
    Q_DISABLE_COPY(QRemoteSignalBlocker)
    QObject * m_o;
    bool m_blocked;
    bool m_inhibited;
};

/**
 * Prevents waiting for any response of Q_REMOTE_CONTROLLER slot,
 * and any remote-slot will return default-constructed value.
 */
class QRemoteReplyBlocker
{
public:
    inline explicit QRemoteReplyBlocker(QObjectRemote *o) Q_DECL_NOTHROW;
    inline explicit QRemoteReplyBlocker(QObjectRemote &o) Q_DECL_NOTHROW;
    inline ~QRemoteReplyBlocker();

#ifdef Q_COMPILER_RVALUE_REFS
    inline QRemoteReplyBlocker(QRemoteReplyBlocker &&other) Q_DECL_NOTHROW;
    inline QRemoteReplyBlocker &operator=(QRemoteReplyBlocker &&other) Q_DECL_NOTHROW;
#endif

    inline void reblock() Q_DECL_NOTHROW;
    inline void unblock() Q_DECL_NOTHROW;

private:
    Q_DISABLE_COPY(QRemoteReplyBlocker)
    QObjectRemote * m_o;
    bool m_blocked;
    bool m_inhibited;
};

/**
 * Limits waiting for any response of #Q_REMOTE_CONTROLLER slot(s), and,
 * once past limit, remaining remote-slot will return default-constructed value.
 *
 * But using zero as timeout means infinite.
 */
class QRemoteTimeLimiter
{
public:
    inline explicit QRemoteTimeLimiter(QObjectRemote *o, int timeout = 30000) Q_DECL_NOTHROW;
    inline explicit QRemoteTimeLimiter(QObjectRemote &o, int timeout = 30000) Q_DECL_NOTHROW;
    inline explicit QRemoteTimeLimiter(const QRef<QObjectRemote> &r, int timeout = 30000) Q_DECL_NOTHROW;
    inline ~QRemoteTimeLimiter();

    inline void reblock(int timeout = 30) Q_DECL_NOTHROW;
    inline void unblock() Q_DECL_NOTHROW;

    inline bool isExpired() const;

    //by default is set to minimum possible timeout which should be 5 seconds
    static inline int defaultLimit() { return QtPrivate::remoteTimeout; }
    static inline void setDefaultLimit(int seconds) { QtPrivate::remoteTimeout = seconds; }

private:
    Q_DISABLE_COPY(QRemoteTimeLimiter)
#ifdef Q_COMPILER_RVALUE_REFS
    QRemoteTimeLimiter(QRemoteTimeLimiter &&other) Q_DECL_NOTHROW;
    QRemoteTimeLimiter &operator=(QRemoteTimeLimiter &&other) Q_DECL_NOTHROW;
#endif
    QObjectRemote * m_o;
    quint8 m_backup;
};

/**
 * Overrides event-handling mode while waiting for any response of
 * #Q_REMOTE_CONTROLLER slot(s).
 *
 * @warning Only applies for current-thread and given %QObjectRemote.
 *
 * However, using zero as flags means %QEventLoop::AllEvents.
 */
class QRemoteEventModer
{
public:
    inline explicit QRemoteEventModer(QObjectRemote *o, int flags = QEventLoop::AllEvents) Q_DECL_NOTHROW;
    inline explicit QRemoteEventModer(QObjectRemote &o, int flags = QEventLoop::AllEvents) Q_DECL_NOTHROW;
    inline explicit QRemoteEventModer(const QRef<QObjectRemote> &r, int flags = QEventLoop::AllEvents) Q_DECL_NOTHROW;
    inline ~QRemoteEventModer();

    inline void redo(int flags = QEventLoop::AllEvents) Q_DECL_NOTHROW;
    inline void undo() Q_DECL_NOTHROW;

    /// Reads current default mode, which's copied on %QObjectRemote creation.
    /// @note The default should be QEventLoop::AllEvents mode, unless changed.
    static inline int defaultMode() { return QtPrivate::remoteEventMode; }
    static inline void setDefaultMode(int flags) { QtPrivate::remoteEventMode = flags; }

private:
    Q_DISABLE_COPY(QRemoteEventModer)
#ifdef Q_COMPILER_RVALUE_REFS
    QRemoteEventModer(QRemoteEventModer &&other) Q_DECL_NOTHROW;
    QRemoteEventModer &operator=(QRemoteEventModer &&other) Q_DECL_NOTHROW;
#endif
    QObjectRemote * m_o;
    qint32 m_backup;
};

#ifndef Q_MOC_RUN
inline bool QMetaRemote::isRegistered(const QObject *obj)
    { QRemoteData *dat = dataFromObject(obj);  return dat && dat->session; }

template<typename T>
Q_INLINE_TEMPLATE T QMetaRemote::request_cast(QObjectRemote *sender, const QMetaObject *senderMetaObject, int local_method_index, void **argv)
{
    //warning: this is optimaized and only works for "request(...)" function results
    const int vid = qMetaTypeId<T>(static_cast<T *>(0));
    QVariant v(vid, (void*)0);
    argv[0] = v.data();
    request(sender, senderMetaObject, local_method_index, argv);
    return v.isValid() ? *reinterpret_cast<const T *>(v.constData()) : T();
}

template<>
Q_INLINE_TEMPLATE QVariant QMetaRemote::request_cast<QVariant>(QObjectRemote *sender, const QMetaObject *senderMetaObject, int local_method_index, void **argv)
{
    const int vid = qMetaTypeId<QVariant>();
    QVariant v(vid, (void*)0);
    argv[0] = v.data();
    request(sender, senderMetaObject, local_method_index, argv);
    if (v.userType() == QMetaType::QVariant)
        return *reinterpret_cast<const QVariant *>(v.constData());
    return v;
}

inline QObjectRemote *QRemoteBound::object() const { return QMetaRemote::remoteObject(reinterpret_cast<const QObject *>(this)); }

inline QRef<QObjectRemote> QRemoteBound::connect(QRemoteUser *sm)
{
    QRef<QObject> ref = qMove(QRef<QObject>::fromStack(reinterpret_cast<QObject *>(this)));
    if (ref.isFake()) {
        qWarning("QRemote: connect needs QRef-wrapped service");
        return QRef<QObjectRemote>();
    }
    return QMetaRemote::registerLocal(ref, sm);
}

inline int QRemoteBound::isConnected() const
{ return QMetaRemote::isRegistered(reinterpret_cast<const QObject *>(this)); }

inline QRemoteUser *QRemoteBound::session()
{ return QMetaRemote::session(object()); }

inline void QRemoteBound::setSession(QRemoteUser *newValue)
{ QRef<QObject> ref(object()); QMetaRemote::setSession(ref, newValue); }

inline QRemoteLink QRemoteBound::address() const
{
    return QRemoteLink::fromObject(reinterpret_cast<const QObject *>(this));
}

inline QString QRemoteBound::id() const
{ return reinterpret_cast<const QObject *>(this)->objectName(); }

inline int QRemoteBound::timeout() const
{
    return QObjectData::get(reinterpret_cast<const QObject *>(this))
            ->remoteTimeout * QObjectData::remoteMiliSecPerTimeout;
}

inline void QRemoteBound::setTimeout(int t) {
    QObjectData::get(reinterpret_cast<QObject *>(this))
            ->remoteTimeout = t / QObjectData::remoteMiliSecPerTimeout;
}

inline bool QRemoteBound::isTimeoutExpired() const
{ return QObjectData::get(reinterpret_cast<const QObject *>(this))->remoteTimeoutExpired; }

inline QRemoteSignalBlocker::QRemoteSignalBlocker(QObject *o) Q_DECL_NOTHROW
    : m_o(o), m_blocked(o && o->blockRemoteSignals(true)), m_inhibited(false)
{}

inline QRemoteSignalBlocker::QRemoteSignalBlocker(QObject &o) Q_DECL_NOTHROW
    : m_o(&o), m_blocked(o.blockRemoteSignals(true)), m_inhibited(false)
{}

#ifdef Q_COMPILER_RVALUE_REFS
inline QRemoteSignalBlocker::QRemoteSignalBlocker(QRemoteSignalBlocker &&other) Q_DECL_NOTHROW
    : m_o(other.m_o),
      m_blocked(other.m_blocked),
      m_inhibited(other.m_inhibited)
{ other.m_o = 0; }

inline QRemoteSignalBlocker &QRemoteSignalBlocker::operator=(QRemoteSignalBlocker &&other) Q_DECL_NOTHROW
{
    if (this != &other) {
        // If both `*this` and `other` block the same object's signals:
        // unblock `*this` if our dtor would unblock, but other's wouldn't
        if (m_o != other.m_o || (!m_inhibited && other.m_inhibited))
            unblock();
        m_o = other.m_o;
        m_blocked = other.m_blocked;
        m_inhibited = other.m_inhibited;
        // disable other:
        other.m_o = 0;
    }
    return *this;
}
#endif

inline QRemoteSignalBlocker::~QRemoteSignalBlocker() {
    if (m_o && !m_inhibited)
        m_o->blockRemoteSignals(m_blocked);
}

inline void QRemoteSignalBlocker::reblock() Q_DECL_NOTHROW {
    if (m_o) m_o->blockRemoteSignals(true);
    m_inhibited = false;
}

inline void QRemoteSignalBlocker::unblock() Q_DECL_NOTHROW {
    if (m_o) m_o->blockRemoteSignals(m_blocked);
    m_inhibited = true;
}


inline QRemoteReplyBlocker::QRemoteReplyBlocker(QObjectRemote *o) Q_DECL_NOTHROW
    : m_o(o), m_blocked(o && o->blockRemoteReplies(true)), m_inhibited(false)
{}

inline QRemoteReplyBlocker::QRemoteReplyBlocker(QObjectRemote &o) Q_DECL_NOTHROW
    : m_o(&o), m_blocked(o.blockRemoteReplies(true)), m_inhibited(false)
{}

#ifdef Q_COMPILER_RVALUE_REFS
inline QRemoteReplyBlocker::QRemoteReplyBlocker(QRemoteReplyBlocker &&other) Q_DECL_NOTHROW
    : m_o(other.m_o),
      m_blocked(other.m_blocked),
      m_inhibited(other.m_inhibited)
{ other.m_o = 0; }

inline QRemoteReplyBlocker &QRemoteReplyBlocker::operator=(QRemoteReplyBlocker &&other) Q_DECL_NOTHROW
{
    if (this != &other) {
        // if both *this and other block the same object's signals:
        // unblock *this iff our dtor would unblock, but other's wouldn't
        if (m_o != other.m_o || (!m_inhibited && other.m_inhibited))
            unblock();
        m_o = other.m_o;
        m_blocked = other.m_blocked;
        m_inhibited = other.m_inhibited;
        // disable other:
        other.m_o = 0;
    }
    return *this;
}
#endif

inline QRemoteReplyBlocker::~QRemoteReplyBlocker() {
    if (m_o && !m_inhibited)
        m_o->blockRemoteReplies(m_blocked);
}

inline void QRemoteReplyBlocker::reblock() Q_DECL_NOTHROW {
    if (m_o) m_o->blockRemoteReplies(true);
    m_inhibited = false;
}

inline void QRemoteReplyBlocker::unblock() Q_DECL_NOTHROW {
    if (m_o) m_o->blockRemoteReplies(m_blocked);
    m_inhibited = true;
}


inline QRemoteTimeLimiter::QRemoteTimeLimiter(QObjectRemote *o, int timeout) Q_DECL_NOTHROW
    : m_o(o)
{
    if(o) {
        m_backup = o->remote().timeout() / QObjectData::remoteMiliSecPerTimeout;
        o->remote().setTimeout(timeout);
    } else {
        m_backup = 0;
    }
}

inline QRemoteTimeLimiter::QRemoteTimeLimiter(QObjectRemote &o, int timeout) Q_DECL_NOTHROW
    : m_o(&o), m_backup(o.remote().timeout() / QObjectData::remoteMiliSecPerTimeout)
{
    o.remote().setTimeout(timeout);
}

inline QRemoteTimeLimiter::QRemoteTimeLimiter(const QRef<QObjectRemote> &r, int timeout) Q_DECL_NOTHROW
    : m_o(r.data())
{
    if(r) {
        m_backup = r->remote().timeout() / QObjectData::remoteMiliSecPerTimeout;
        r->remote().setTimeout(timeout);
    } else {
        m_backup = 0;
    }
}

inline QRemoteTimeLimiter::~QRemoteTimeLimiter() {
    if (m_o)
        m_o->remote().setTimeout(m_backup * QObjectData::remoteMiliSecPerTimeout);
}

inline void QRemoteTimeLimiter::reblock(int timeout) Q_DECL_NOTHROW {
    if (m_o) m_o->remote().setTimeout(timeout);
}

inline void QRemoteTimeLimiter::unblock() Q_DECL_NOTHROW {
    if (m_o) m_o->remote().setTimeout(m_backup * QObjectData::remoteMiliSecPerTimeout);
}

inline bool QRemoteTimeLimiter::isExpired() const
{
    return m_o ? m_o->remote().isTimeoutExpired() : false;
}


inline QRemoteEventModer::QRemoteEventModer(QObjectRemote *o, int flags) Q_DECL_NOTHROW
    : m_o(o)
{
    if (o) {
        m_backup = o->remote().setEventMode(flags);
    } else {
        m_backup = QRemote::InvalidController;
    }
}

inline QRemoteEventModer::QRemoteEventModer(QObjectRemote &o, int flags) Q_DECL_NOTHROW
    : m_o(&o), m_backup(o.remote().setEventMode(flags))
{
}

inline QRemoteEventModer::QRemoteEventModer(const QRef<QObjectRemote> &r, int flags) Q_DECL_NOTHROW
    : m_o(r.data())
{
    if (r) {
        m_backup = r->remote().setEventMode(flags);
    } else {
        m_backup = QRemote::InvalidController;
    }
}

inline QRemoteEventModer::~QRemoteEventModer() {
    if (m_o) {
        m_o->remote().setEventMode(m_backup);
    }
}

inline void QRemoteEventModer::redo(int flags) Q_DECL_NOTHROW {
    if (m_o) m_o->remote().setEventMode(flags);
}

inline void QRemoteEventModer::undo() Q_DECL_NOTHROW {
    if (m_o) m_o->remote().setEventMode(m_backup);
}
#endif //Q_MOC_RUN

QT_END_NAMESPACE

#endif
