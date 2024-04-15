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

#ifndef QT_REMOTE_PACKET_H
#define QT_REMOTE_PACKET_H

#include "qremote-config.h"
// For QString, QByteArray, QRemoteAddress etc.
#include "qremote-type.h"

#include <QtCore/qscopedpointer.h>

#include <memory>


QT_BEGIN_NAMESPACE

namespace QRemote {

/**
 * @class QRemote::Packet
 * @inmodule QtRemote
 * @reentrant
 *
 * Base for any single-and-complete QtRemote request and/or reply.
 *
 * In other words, packet fragmentation is never QtRemote's responsibility.
 *
 * **Note** that this class has nothing to do with internet-protocols (IP), else
 * the "payload" would be a better name, at least in IP context, but
 * in our API's context "payload" would be confusing, because for example
 * any TCP or UDP payload can carry some other un-depending payload, while
 * in QtRemote each and every extension of Packet class depends on base's data.
 */
class PacketPrivate;
class QT_REMOTE_EXPORT Packet {
    Q_DECLARE_PRIVATE(Packet)
    Q_DISABLE_COPY(Packet)
public:

    enum PktType {
        /// Used to mark decode and/or deserialize error.
        InvalidPacketType = 0x0000,

        /// Register/show or unregister/hide user made object(s) to
        /// other users/listeners.
        Status = 0x0001,
        /// Requests Packet::Status from receiver.
        ///
        /// WARNING: if not replyed in time
        /// sender assumes receiver as dead,
        /// where default time-limit is one minute.
        RequestStatus = 0x0002,
        StatusMask = Status|RequestStatus,

        /// If you activate any `signal` of a class written by you/user
        /// that has `Q_REMOTE` macro, we name it "RemoteSignal".
        ///
        /// Because it activates signal of the class generated to match
        /// user-made class, which has `Q_REMOTE_CONTROLLER` macro
        /// if same address as `receiver()`.
        RemoteSignal = 0x0008,
        //note: if you activate 'signal' or 'slot' of generated class we name it
        //      "InvokeSignal" or "InvokeSlot" since it activates user made
        //      'signal' or 'slot' written by you/user having "Q_REMOTE" macro
        InvokeSignal = 0x0010, //emit an user made 'signal' but on remote server
        InvokeSlot = 0x0020, //activate a user made 'slot' but on remote server
        SlotReply = 0x0040, //if "Packet" is result of an user made 'slot'
        MethodMask = RemoteSignal|InvokeSignal|InvokeSlot|SlotReply,

        Error = 0x0080,
        MethodError = 0x0100,
        ErrorMask = Error|MethodError
    };

    enum PktFlag {
        FlagsEmpty = 0x00,
        /// Can be used in combination of PktType::InvokeSlot and PktType::Status.
        NoReply = 0x01,
        /// Can be used by PktType::RequestStatus
        NoObjectStatus = 0x02
    };
    Q_DECLARE_FLAGS(PktFlags, PktFlag)

    enum PktVersion {
        UnknownVersion = 0,
        Version_1_0 = 1,
        CurrentVersion = Version_1_0
    };

    /// Never returns @c nullptr even if failed
    /// (i.e. but isValid() will return `false`).
    static Packet *create(const QByteArray &classType);

    virtual ~Packet();

    QByteArray classType() const;

    static QByteArray save(const Packet &pkt);
    /// WARNING: given @p pkt should start with classType().
    static Packet *load(const QByteArray &pkt, QString *errDescription = 0, int version = 0);

    virtual bool save(QDataStream &stream) const;
    /// WARNING: given @p stream should NOT start with classType().
    virtual bool load(QDataStream &stream);

    bool isValid() const;


    inline PktType type() const;
    inline void setType(PktType val);
    /// **Note** that this is for debugging purposes.
    static const char *typeName(PktType t);

    inline PktFlags flags() const;
    inline void setFlags(PktFlags v);

    inline PktVersion version() const;
    inline void setVersion(PktVersion v);

    inline const QByteArray &id() const;
    inline void setId(const QByteArray &id_);

    inline const QByteArray &timeStamp() const;
    inline void setTimeStamp(const QByteArray &timeStamp_);

    inline const QRemoteUserName &sender() const;
    inline void setSender(const QRemoteUserName &val);

    inline QRemoteUserName &receiver();
    inline const QRemoteUserName &receiver() const;
    inline void setReceiver(const QRemoteUserName &val);


    static QScopedPointer<Packet> fromByteArray(const QByteArray &data);

    Packet(PktType type = InvalidPacketType);

protected:
    inline Packet(PacketPrivate &dd);
    Packet(int subclassId, PktType type = InvalidPacketType);
    QScopedPointer<PacketPrivate> d_ptr;
};

class QT_REMOTE_EXPORT PacketPrivate {
public:
    /// WARNING: given @p subclassId should be already valid, because
    /// it's not checked here.
    PacketPrivate(int subclassId, Packet::PktType type = Packet::InvalidPacketType);

private:
    friend class Packet;
    int subclassId;
    ushort packetType;
    Packet::PktFlags packetFlags;
    Packet::PktVersion packetVersion;

    /// The identifier used to match reply(-ies) with request(s).
    QByteArray packetId;

    /**
    * Unique-name of the QRemoteUser who is sending this Packet.
    *
    * @sa QObject::setName(...)
    */
    QRemoteUserName sourceUser;

    /**
    * Unique-name of the QRemoteUser who should receive this Packet.
    *
    * Can be empty if the Packet is broadcast.
    *
    * @sa QObject::setName(...)
    */
    QRemoteUserName destinationUser;

    /**
     * Optional debug-purpose time at which the Packet was created (not send time).
     */
    QByteArray timeStampFormatted;
};

class StatusPacketPrivate;
class QT_REMOTE_EXPORT StatusPacket : public Packet {
    Q_DECLARE_PRIVATE(StatusPacket)
    Q_DISABLE_COPY(StatusPacket)
public:
    StatusPacket();

    bool save(QDataStream &stream) const Q_DECL_OVERRIDE;
    bool load(QDataStream &stream) Q_DECL_OVERRIDE;

    typedef QList< QRemoteAddress > RemoteInfos;

    inline bool isVisible() const;
    inline void setVisible(bool);

    inline bool isDisconnect() const;

    inline RemoteInfos &remotes();
    inline const RemoteInfos &remotes() const;
    inline void setRemotes(const RemoteInfos &infos);
};

class StatusPacketPrivate : public PacketPrivate {
public:
    inline StatusPacketPrivate(int subclassId)
        : PacketPrivate(subclassId, Packet::Status)
        , isVisible(true)
    {}

    bool isVisible;
    StatusPacket::RemoteInfos infoList;
};

class MethodPacketPrivate;
class QT_REMOTE_EXPORT MethodPacket : public Packet {
public:
    typedef QVector< QRemoteType > Parameters;

    MethodPacket();

    bool save(QDataStream &stream) const Q_DECL_OVERRIDE;
    bool load(QDataStream &stream) Q_DECL_OVERRIDE;

    inline const QByteArray &method() const;
    inline void setMethod(const QByteArray &val);

    inline int methodIndex() const;
    inline void setMethodIndex(int absIndex);

    inline const QRemoteType &returnType() const;
    inline QRemoteType &returnType();
    inline void setReturnType(const QRemoteType &);

    inline const Parameters &params() const;
    inline Parameters &params();
    inline void setParams(const Parameters &val);

    inline const QRemoteAddress &address() const;
    inline QRemoteAddress &address();
    inline void setAddress(const QRemoteAddress &val);

    inline const QRemoteLink &signaler() const;
    inline QRemoteLink &signaler();
    inline void setSignaler(const QRemoteLink &val);

private:
    Q_DECLARE_PRIVATE(MethodPacket)
    Q_DISABLE_COPY(MethodPacket)
};

class MethodPacketPrivate : public PacketPrivate {
public:
    inline MethodPacketPrivate(int subclassId)
        : PacketPrivate(subclassId, Packet::RemoteSignal)
        , methodIndex(-1)
    {}

    static inline const MethodPacketPrivate *get(const MethodPacket *o) { return o->d_func(); }
    static inline MethodPacketPrivate *get(MethodPacket *o) { return o->d_func(); }

    /**
    * Name of the signal or slot or method, which's to be called.
    *
    * @sa MethodPacket::method
    * @sa MethodPacket::setMethod
    */
    QByteArray methodName;

    /// Index in QMetaObject.
    ///
    /// Not really required, is just to speed up the receive.
    int methodIndex;

    /// List of method parameters.
    MethodPacket::Parameters parameters;
    QRemoteType returnType;
    QRemoteAddress target;
    QRemoteLink signaler;
};

// The "NoError" seems to be a popular name (maybe even for Macro).
#ifdef NoError
#  define QT_REMOTE_NO_ERROR NoError
#  undef NoError
#endif

class ErrorPacketPrivate;
class QT_REMOTE_EXPORT ErrorPacket : public Packet {
public:
    /**
     * @brief Different type of errors which may happen in QtRemote communication.
     *
     * @note These are sorted by error-depth, for example the
     * UnknownService can never happen unless there was zero ProtocolError, and
     * getting UnknownService error means everything unitl that point is correct.
     */
    enum ErrorType {
        /// Either received ErrorType is user-made custom-type, but
        /// our Codec didn't support decoding said custom-type, or
        /// binary was damaged somehow and is now invalid.
        InvalidError = -1,

        /// Default of constructor, meaning everything works.
        ///
        /// This is useful when only type is passed around.
        NoError = 0,

        /// Used by PacketCodec to notify decode failure, also
        /// used if MethodPacket is bigger then allowed limit.
        ProtocolError = 1,

        /// Packet type is either not supported by PacketCodec,
        /// or not set, or was received in damaged state.
        UnknownPktType = 2,

        /// Addressed Service is/was not registered on receiving QRemoteUser, hence
        /// is replying with this ErrorType.
        UnknownService = 3,

        /// Addressed Service does not contain given
        /// method-name (same if empty name), or
        /// parameters did not match set method name.
        InvalidMethod = 4,

        ErrorCount,
    };
    ErrorPacket();

    bool save(QDataStream &stream) const Q_DECL_OVERRIDE;
    bool load(QDataStream &stream) Q_DECL_OVERRIDE;

    inline ErrorType errorType() const;
    inline void setErrorType(ErrorType val);
    inline void setErrorTypeInt(int val);

    inline const QString &errorMessage() const;
    inline void setErrorMessage(const QString &val);

    inline const QRemoteAddress &address() const;
    inline QRemoteAddress &address();
    inline void setAddress(const QRemoteAddress &val);

protected:
    inline ErrorPacket(ErrorPacketPrivate &dd);

private:
    Q_DECLARE_PRIVATE(ErrorPacket)
    Q_DISABLE_COPY(ErrorPacket)
};

class ErrorPacketPrivate : public PacketPrivate {
public:
    inline ErrorPacketPrivate(int subclassId, Packet::PktType type = Packet::Error) : PacketPrivate(subclassId, type), errorType(ErrorPacket::NoError) {}

    ErrorPacket::ErrorType errorType;

    QString errorMessage;
    QRemoteAddress address;
};

class MethodErrorPacketPrivate;
class MethodErrorPacket : public ErrorPacket {
public:
    MethodErrorPacket();

    bool save(QDataStream &stream) const Q_DECL_OVERRIDE;
    bool load(QDataStream &stream) Q_DECL_OVERRIDE;

    inline const QByteArray &method() const;
    inline void setMethod(const QByteArray &val);
private:
    Q_DECLARE_PRIVATE(MethodErrorPacket)
    Q_DISABLE_COPY(MethodErrorPacket)
};

class MethodErrorPacketPrivate : public ErrorPacketPrivate {
public:
    inline MethodErrorPacketPrivate(int subclassId) : ErrorPacketPrivate(subclassId, Packet::MethodError) {}
    QByteArray methodName;
};

#ifndef Q_MOC_RUN
    // ------------     Packet     ------------
    inline Packet::Packet(PacketPrivate &dd) : d_ptr(&dd) {}

    inline Packet::PktType Packet::type() const { return static_cast<Packet::PktType>(d_ptr->packetType); }
    inline void Packet::setType(Packet::PktType val) { d_ptr->packetType = val; }

    inline Packet::PktFlags Packet::flags() const { return d_ptr->packetFlags; }
    inline void Packet::setFlags(Packet::PktFlags v) { d_ptr->packetFlags = v; }

    inline Packet::PktVersion Packet::version() const { return d_ptr->packetVersion; }
    inline void Packet::setVersion(Packet::PktVersion v) { d_ptr->packetVersion = v; }

    inline const QByteArray &Packet::id() const      { return d_ptr->packetId; }
    inline void Packet::setId(const QByteArray &id_) { d_ptr->packetId = id_;  }

    inline const QByteArray &Packet::timeStamp() const      { return d_ptr->timeStampFormatted; }
    inline void Packet::setTimeStamp(const QByteArray &timeStamp_) { d_ptr->timeStampFormatted = timeStamp_;  }

    inline const QRemoteUserName &Packet::sender() const      { return d_ptr->sourceUser;      }
    inline void Packet::setSender(const QRemoteUserName &val) { d_ptr->sourceUser = val;       }

    inline QRemoteUserName &Packet::receiver()                  { return d_ptr->destinationUser; }
    inline const QRemoteUserName &Packet::receiver() const      { return d_ptr->destinationUser; }
    inline void Packet::setReceiver(const QRemoteUserName &val) { d_ptr->destinationUser = val;  }

    // ------------  StatusPacket  ------------
    inline bool StatusPacket::isVisible() const { return d_func()->isVisible; }
    inline void StatusPacket::setVisible(bool v) { d_func()->isVisible = v; }
    inline bool StatusPacket::isDisconnect() const
        {
            const StatusPacketPrivate *d = d_func();
            return receiver().isEmpty()
                && flags().includes(Packet::NoReply)
                && ! d->isVisible
                && d->infoList.isEmpty();
        }
    inline StatusPacket::RemoteInfos &StatusPacket::remotes()                    { return d_func()->infoList;  }
    inline const StatusPacket::RemoteInfos &StatusPacket::remotes() const        { return d_func()->infoList;  }
    inline void StatusPacket::setRemotes(const StatusPacket::RemoteInfos &infos) { d_func()->infoList = infos; }

    // ------------  MethodPacket  ------------
    inline const QByteArray &MethodPacket::method() const      { return d_func()->methodName; }
    inline void MethodPacket::setMethod(const QByteArray &val) { d_func()->methodName = val;  }

    inline int MethodPacket::methodIndex() const           { return d_func()->methodIndex;     }
    inline void MethodPacket::setMethodIndex(int absIndex) { d_func()->methodIndex = absIndex; }

    inline const QRemoteType &MethodPacket::returnType() const      { return d_func()->returnType; }
    inline QRemoteType &MethodPacket::returnType()                  { return d_func()->returnType; }
    inline void MethodPacket::setReturnType(const QRemoteType &val) { d_func()->returnType = val; }

    inline const MethodPacket::Parameters &MethodPacket::params() const      { return d_func()->parameters; }
    inline MethodPacket::Parameters &MethodPacket::params()                  { return d_func()->parameters; }
    inline void MethodPacket::setParams(const MethodPacket::Parameters &val) { d_func()->parameters = val;  }

    inline const QRemoteAddress &MethodPacket::address() const { return d_func()->target; }
    inline QRemoteAddress &MethodPacket::address() { return d_func()->target; }
    inline void MethodPacket::setAddress(const QRemoteAddress &val) { d_func()->target = val; }

    inline const QRemoteLink &MethodPacket::signaler() const { return d_func()->signaler; }
    inline QRemoteLink &MethodPacket::signaler() { return d_func()->signaler; }
    inline void MethodPacket::setSignaler(const QRemoteLink &val) { d_func()->signaler = val; }

    // ------------  ErrorPacket  ------------

    inline ErrorPacket::ErrorPacket(ErrorPacketPrivate &dd) : Packet(dd) {}

    inline ErrorPacket::ErrorType ErrorPacket::errorType() const      { return d_func()->errorType; }
    inline void ErrorPacket::setErrorType(ErrorPacket::ErrorType val) { d_func()->errorType = val; }
    inline void ErrorPacket::setErrorTypeInt(int val)
        {
            d_func()->errorType = (val >= ErrorPacket::NoError && val < ErrorPacket::ErrorCount)
                    ? static_cast<ErrorPacket::ErrorType>(val)
                    : ErrorPacket::InvalidError;
        }

    inline const QString &ErrorPacket::errorMessage() const      { return d_func()->errorMessage; }
    inline void ErrorPacket::setErrorMessage(const QString &val) { d_func()->errorMessage = val; }

    inline const QRemoteAddress &ErrorPacket::address() const { return d_func()->address; }
    inline QRemoteAddress &ErrorPacket::address() { return d_func()->address; }
    inline void ErrorPacket::setAddress(const QRemoteAddress &val) { d_func()->address = val; }

    // ------------  MethodErrorPacket  ------------

    inline const QByteArray &MethodErrorPacket::method() const      { return d_func()->methodName; }
    inline void MethodErrorPacket::setMethod(const QByteArray &val) { d_func()->methodName = val;  }
#endif // Q_MOC_RUN

typedef std::unique_ptr<Packet> PacketUniquePtr;
typedef QScopedPointer<MethodPacket> MethodPacketSP;
typedef QScopedPointer<ErrorPacket> ErrorPacketSP;
typedef QScopedPointer<MethodErrorPacket> MethodErrorPacketSP;

} // namespace QRemote

// Intentionally NOT declaring "<<" and ">>" operators, you should
// use PacketCodec or save/load methods instead of:
// ```
// inline QDataStream &operator<<(QDataStream &stream, const QRemote::Packet &pkt) { pkt.save(stream); return stream; }
// ```

QT_REMOTE_EXPORT QDebug &operator <<(QDebug &s, const QRemote::Packet &v);

// MARK: Cleanup.

#ifdef QT_REMOTE_NO_ERROR
#  define NoError QT_REMOTE_NO_ERROR
#  undef QT_REMOTE_NO_ERROR
#endif

QT_END_NAMESPACE

#endif // QT_REMOTE_PACKET_H
