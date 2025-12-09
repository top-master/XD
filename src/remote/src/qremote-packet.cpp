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

#include "qremote-packet.h"
#include "qremote-object.h"

#include <QDebug>
#include <QReadWriteLock>
#include <QDataStream>

#include <extras/customtypes.h>


QT_BEGIN_NAMESPACE
using namespace QRemote;

QDebug &operator <<(QDebug &dbg, const Packet &v)
{
    const Packet *vPtr = &v;
    if( ! vPtr) {
        dbg << "error:null-packet";
        return dbg;
    }
    QDebugStateSaver saver(dbg); Q_UNUSED(saver)
    QString *dbgRaw = dbg.data();
    dbgRaw->reserve(dbgRaw->size() + 1024);

    if((v.type() & Packet::MethodMask) != 0) {
        const MethodPacket &m = reinterpret_cast<const MethodPacket & >(v);
        // Prints begin of "ClassName.methodName(param1=value1)" format
        // (i.e. method address, name and arguments).
        QByteArray method = m.method();
        if( ! method.isEmpty())
            method.chop(method.length() - method.indexOf('('));
        dbg.noQuotes().nospace()
                << m.address() << QLatin1Char('.') << method
                << QLatin1Char('(');
        // Prints arguments.
        MethodPacket::Parameters::const_iterator it = m.params().constBegin();
        MethodPacket::Parameters::const_iterator iEnd = m.params().constEnd();
        if (it != iEnd) {
            do {
                const QRemoteType &currentType = *it;
                dbg << currentType;
                dbg << ',';
                ++it;
            } while(it != iEnd);
            dbg.data()->chop(1);
        }
        dbg << QLatin1Char(')');
    } else if((v.type() & Packet::StatusMask) != 0) {
        const StatusPacket &m = reinterpret_cast<const StatusPacket & >(v);
        dbg.nospace() << "Status.";
        if((v.type() & Packet::RequestStatus) != 0)
            dbg << "Request:";
        else
            dbg << "List:";
        StatusPacket::RemoteInfos::const_iterator i = m.remotes().constBegin();
        StatusPacket::RemoteInfos::const_iterator iEnd = m.remotes().constEnd();
        if(i == iEnd)
            dbg << "<empty>";
        for (; i != iEnd; i++) {
            const StatusPacket::RemoteInfos::value_type &te = *i;
            dbg << te;
        }
    } else if((v.type() & Packet::ErrorMask) != 0) {
        const ErrorPacket &m = reinterpret_cast<const ErrorPacket & >(v);
        dbg << "Error:";
        dbg << m.errorMessage();
    } else {
        dbg << "Unknown Packet:" << v.type();
    }

    if (saver.hadSpace()) {
#ifdef Q_OS_WIN
        dbg << "\r\n";
#else
        dbg << "\n";
#endif
    }

    dbg.quotes().space()
        << "sender:" << v.sender()
        << "receiver:";
    if (v.receiver().length()) {
        dbg << v.receiver();
    } else {
        dbg.noQuotes() << QLL("[ALL]");
        dbg.quote();
    }

    dbg.nospace() << "(id: " << v.id() << " time: " << v.timeStamp() << ')';

    return dbg;
}

Q_GLOBAL_STATIC(CustomTypes, customTypes)

#define Q_REGISTER_PACKET(TYPE) \
    Q_DECLARE_CUSTOM_TYPE_FLAGS(TYPE, customTypes(), CustomTypes::flag_NoCopy) \
    static qint8 QT_JOIN(q_static_type_id, __LINE__) = (qint8)CustomTypeInfo<TYPE>::id();

Q_REGISTER_PACKET(Packet)
Q_REGISTER_PACKET(StatusPacket)
Q_REGISTER_PACKET(MethodPacket)
Q_REGISTER_PACKET(ErrorPacket)
Q_REGISTER_PACKET(MethodErrorPacket)

Packet::Packet(Packet::PktType type)
    : d_ptr(new PacketPrivate(CustomTypeInfo<Packet>::id(), type))
{}
Packet::Packet(int subclassId, Packet::PktType type)
    : d_ptr(new PacketPrivate(subclassId, type))
{}
Packet::~Packet() {}

QByteArray Packet::classType() const
{
    return customTypes()->typeName(d_ptr->subclassId);
}

Packet *Packet::create(const QByteArray &classType)
{
    //qDebug("Packet::create: %s", classType.constData());
    Packet *pkt = 0;
    const TypeInfo &info = customTypes()->typeInfo(classType.constData());
    if( ! info.isEmpty()) {
        pkt = reinterpret_cast<Packet *>(info.construct());
    }
    if( ! pkt) {
        qWarning("Packet::create: failed by type: %s", classType.constData());
        // Return invalid Packet.
        pkt = new Packet;
    }

    // TODO: maybe throw on out-of-memory.

    return pkt;
}

bool Packet::save(QDataStream &stream) const
{
    QByteArray typeId(classType());
    stream << typeId;

    qint16 type; quint8 flags; quint8 version;
    type = (qint16)this->type();
    flags = (quint8)this->flags();
    version = (quint8)this->version();
    stream << type << flags << version;
    stream << d_ptr->packetId << d_ptr->sourceUser << d_ptr->destinationUser;
    stream << d_ptr->timeStampFormatted;

    return stream.status() == QDataStream::Ok;
}

bool Packet::load(QDataStream &stream)
{
    qint16 type; quint8 flags; quint8 version;
    QByteArray id;
    QRemoteUserName sender, receiver;
    QByteArray timeStamp;
    stream >> type;
    if (stream.status() != QDataStream::Ok) return false;
    stream >> flags;
    if (stream.status() != QDataStream::Ok) return false;
    stream >> version;
    if (stream.status() != QDataStream::Ok) return false;
    stream >> id;
    if (stream.status() != QDataStream::Ok) return false;
    stream >> sender;
    if (stream.status() != QDataStream::Ok) return false;
    stream >> receiver;
    if (stream.status() != QDataStream::Ok) return false;
    stream >> timeStamp;
    if (stream.status() != QDataStream::Ok) return false;
    d_ptr->packetType = PktType(type);
//    if(!isValid()) return false;
    d_ptr->packetFlags = PktFlags(flags);
    d_ptr->packetVersion = PktVersion(version);
    d_ptr->packetId = id;
    d_ptr->sourceUser = sender;
    d_ptr->destinationUser = receiver;
    d_ptr->timeStampFormatted = timeStamp;
    return true;
}

QByteArray QRemote::Packet::save(const QRemote::Packet &pkt)
{
    QByteArray buffer;
    QDataStream stream(&buffer, QIODevice::ReadWrite);
    pkt.save(stream);
    return buffer;
}

QRemote::Packet *QRemote::Packet::load(const QByteArray &pkt, QString *errDescription, int version)
{
#if QT_VERSION_MAJOR >= 5
    // No QBuffer need, since Qt now supports QByteArray directly.
    QDataStream stream(pkt);
#else
    QBuffer buffer;
    buffer.setData(pkt);
    buffer.open(QIODevice::ReadOnly);
    QDataStream stream(&buffer);
#endif

    if (version) {
        stream.setVersion(version);
    }

    QByteArray classType;
    stream >> classType;
    if (stream.status() != QDataStream::Ok)
        goto posError;
    else {
        QScopedPointer<Packet> result(Packet::create(classType));
        if (result) {
            result->load(stream);
            if (stream.status() != QDataStream::Ok)
                goto posError;
        }
        return result.take();
    }

posError:
    if (errDescription != Q_NULLPTR) {
        switch (stream.status()) {
            case QDataStream::ReadPastEnd :
                *errDescription = QLatin1Literal("MethodPacket incompleate");
                break;
            case QDataStream::ReadCorruptData :
                *errDescription = QLatin1Literal("MethodPacket corrupted");
                break;
        case QDataStream::Ok:
            Q_FALLTHROUGH();
        case QDataStream::WriteFailed:
            *errDescription = QLatin1Literal("Internal error");
            break;
        }
    }
    return Q_NULLPTR;
}

const char *Packet::typeName(Packet::PktType t)
{
    switch (t) {
        case RemoteSignal: return "RemoteSignal";
        case InvokeSignal: return "InvokeSignal";
        case InvokeSlot: return "InvokeSlot";
        case SlotReply: return "SlotReply";
        case Error: return "Error";
        case MethodError: return "Error";
        default: return "Unknown";
    }
}

bool QRemote::Packet::isValid() const {
    switch (d_ptr->packetType) {
    case InvalidPacketType:
        return false;
    case Status:
    case RequestStatus:
        return d_ptr->subclassId == CustomTypeInfo<StatusPacket>::id();
    case RemoteSignal:
    case InvokeSignal:
    case InvokeSlot:
    case SlotReply:
        return d_ptr->subclassId == CustomTypeInfo<MethodPacket>::id();
    case Error:
        return d_ptr->subclassId == CustomTypeInfo<ErrorPacket>::id();
    case MethodError:
        return d_ptr->subclassId == CustomTypeInfo<MethodErrorPacket>::id();
    }
    return false; //any invalid type caused by QDataStream read error
}

PacketPrivate::PacketPrivate(int subclassId, Packet::PktType type)
    : subclassId(subclassId), packetType(type)
{
    this->timeStampFormatted = QMetaRemote::timeStampFormatted().toUtf8();
    // The version QtRemote was compiled with, not version of user headers.
    this->packetVersion = Packet::CurrentVersion;
}

StatusPacket::StatusPacket()
    : Packet(*new StatusPacketPrivate(CustomTypeInfo<StatusPacket>::id()))
{
}

bool StatusPacket::save(QDataStream &stream) const
{
    if( ! Packet::save(stream)) return false;
    stream << d_func()->isVisible;
    stream << d_func()->infoList;
    return (stream.status() == QDataStream::Ok);
}

bool StatusPacket::load(QDataStream &stream)
{
    if ( ! Packet::load(stream)) return false;
    stream >> d_func()->isVisible;
    stream >> d_func()->infoList;
    if (stream.status() != QDataStream::Ok) return false;
    return true;
}

MethodPacket::MethodPacket()
    : Packet(*new MethodPacketPrivate(CustomTypeInfo<MethodPacket>::id()))
{}

bool MethodPacket::save(QDataStream &stream) const
{
    if( ! Packet::save(stream)) return false;
    stream << d_func()->methodName << d_func()->methodIndex;
    stream << d_func()->returnType;
    stream << d_func()->parameters;
    stream << d_func()->target;
    stream << d_func()->signaler;
    return (stream.status() == QDataStream::Ok);
}

bool MethodPacket::load(QDataStream &stream)
{
    if(! Packet::load(stream)) return false;

    QByteArray method;
    int methodIndex;
    QRemoteType returnType;
    MethodPacket::Parameters params;
    QRemoteAddress target;
    QRemoteLink signaler;

    stream >> method;
    if (stream.status() != QDataStream::Ok) return false;
    stream >> methodIndex;
    if (stream.status() != QDataStream::Ok) return false;
    stream >> returnType;
    if (stream.status() != QDataStream::Ok) return false;
    stream >> params;
    if (stream.status() != QDataStream::Ok) return false;
    stream >> target;
    if (stream.status() != QDataStream::Ok) return false;
    stream >> signaler;
    if (stream.status() != QDataStream::Ok) return false;

    d_func()->methodName = method;
    d_func()->methodIndex = methodIndex;
    d_func()->returnType = returnType;
    d_func()->parameters = params;
    d_func()->target = target;
    d_func()->signaler = signaler;
    return true;
}

ErrorPacket::ErrorPacket()
    : Packet(*new ErrorPacketPrivate(CustomTypeInfo<ErrorPacket>::id()))
{}

bool ErrorPacket::save(QDataStream &stream) const
{
    if( ! Packet::save(stream)) return false;
    qint8 errorType;
    errorType = (quint8)d_func()->errorType;
    stream << errorType;
    stream << d_func()->errorMessage;
    stream << d_func()->address;
    return (stream.status() == QDataStream::Ok);
}

bool ErrorPacket::load(QDataStream &stream)
{
    if ( ! Packet::load(stream)) return false;

    quint8 errorType;
    QString errorMessage;
    QRemoteAddress address;

    stream >> errorType;
    if (stream.status() != QDataStream::Ok) return false;
    stream >> errorMessage;
    if (stream.status() != QDataStream::Ok) return false;
    stream >> address;
    if (stream.status() != QDataStream::Ok) return false;

    d_func()->errorType = (ErrorPacket::ErrorType)errorType;
    d_func()->errorMessage = errorMessage;
    d_func()->address = address;
    return true;
}

MethodErrorPacket::MethodErrorPacket()
    : ErrorPacket(*new MethodErrorPacketPrivate(CustomTypeInfo<MethodErrorPacket>::id()))
{}

bool MethodErrorPacket::save(QDataStream &stream) const
{
    if(!ErrorPacket::save(stream)) return false;
    stream << d_func()->methodName;
    return (stream.status() == QDataStream::Ok);
}

bool MethodErrorPacket::load(QDataStream &stream)
{
    if(! ErrorPacket::load(stream)) return false;

    QByteArray method;
    stream >> method;
    if (stream.status() != QDataStream::Ok) return false;
    d_func()->methodName = method;
    return true;
}

QT_END_NAMESPACE
