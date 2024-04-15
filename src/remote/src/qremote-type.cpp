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

#include "qremote-type.h"
#include "qremoteobject_p.h"
#include "qremoteuser.h"

#include <QtCore/qvariant.h>
#include <QtCore/qdatastream.h>
#include <QtCore/QDebug>


QT_BEGIN_NAMESPACE

static int qMetaTypeRemoteRef = QMetaType::UnknownType;

inline QDataStream &operator <<(QDataStream &s, const QRef<QObject> &v) {
    QRemoteUser *session = QRemoteUser::fromThreadStorage();
    if (session)
        session->registerOnce(v);
    const QRemoteLink &link = QRemoteLink::fromObject(v.data());
    s << link;
    return s;
}
inline QDataStream &operator >>(QDataStream &s, QRef<QObject> &v) {
    QRemoteLink link;
    s >> link;
    QRemoteUser *session = QRemoteUser::fromThreadStorage();
    v = link.toObject(session);
    return s;
}

int qRegisterRemoteVariant()
{
    qMetaTypeRemoteRef = qRegisterMetaTypeStreamOperators<QRef<QObject> >("QRef<");
    qRegisterMetaTypeStreamOperators<QRemoteLink>("QRemoteLink");
    return 1;
}
Q_CONSTRUCTOR_FUNCTION(qRegisterRemoteVariant)


QVariant QRemoteType::toVariant() const
{
    int typeId = QRemoteType::find(typeName);
    if(typeId == QMetaType::Void) {
        return QVariant(); //special case if return type is void
    } else if (typeId <= QMetaType::UnknownType) {
        QRemote::warnType("QRemoteType", "toVariant", typeName.constData());
        return QVariant();
    }
    //construct type
    QVariant result(typeId, static_cast<void*>(0));
    QDataStream stream(data);
    if(QMetaType::load(stream, typeId, result.data()) == false)
        return QVariant(); //ensure invalid result if load failed
    return result;
}

static const char STR_REMOTE_REF[] = "QRef<";

QString QRemoteType::toString() const
{
    if(this->typeName.startsWith(STR_REMOTE_REF)) {
        if(this->data.isEmpty())
            return QString(); //is null pointer

        QDataStream stream(const_cast<QByteArray *>(&this->data), QIODevice::ReadOnly);
        QRemoteLink link;
        stream >> link;
        return link.toString();
    }
    return this->toVariant().toString();
}

void *QRemoteType::toRawType(int id, QRemoteUser *registery) const
{
    if(id == QMetaType::Void) {
        return Q_NULLPTR;
    } else if(id > QMetaType::UnknownType) {
        QDataStream stream(this->data);
        if(id == qMetaTypeRemoteRef) {
            QRef<QObject> *v = new QRef<QObject>();
            if(v) {
                QRemoteLink link;
                stream >> link;
                *v = link.toObject(registery);
            }
            return v;
        }

        // Any other meta-type.

#if QT_HAS_XD(5, 6, 5)
        // TRACE/remote compat: support memory-allocation for Meta-type constructor #1,
        // since Qt5 removed Qt4's auto-memory-allocation, and that with the
        // excuse that this method was internal, hence no need to be compatible.
        const int size = QMetaType::sizeOf(id);
        if (size <= 0) {
            return Q_NULLPTR;
        }
#if QREMOTE_ALIGNED_ALLOC
        void *where = qMallocAlignedT<void *>(size);
#else
        void *where = new char[size / sizeof(char) + 1];
#endif
        Q_CHECK_PTR(where);
        void *ptr = QMetaType::construct(id, where, Q_NULLPTR);
#else
        void *ptr = QMetaType::construct(id, Q_NULLPTR, Q_NULLPTR);
#endif

        if(ptr) {
            if(QMetaType::load(stream, id, ptr)) {
                return ptr;
            }
            QMetaType::destroy(id, ptr);
#if QREMOTE_ALIGNED_ALLOC
            // TRACE/remote compat: support memory-allocation for Meta-type constructor #2.
            qFreeAligned(ptr);
#endif
        }
    }
    return Q_NULLPTR;
}

void *QRemoteType::toRawType(QRemoteUser *registery) const
{
    return toRawType(find(typeName), registery);
}

bool QRemoteType::load(int id, const void *sourcePtr, QRemoteUser *registery)
{
    this->data = QByteArray();
    if(id == QMetaType::Void)
        return true;
    else if(id <= QMetaType::UnknownType)
        return false;

    QByteArray buffer;
    QDataStream stream(&buffer, QIODevice::ReadWrite | QIODevice::Unbuffered);
    if(id == qMetaTypeRemoteRef) {
        const QRef<QObject> &v = *reinterpret_cast<const QRef<QObject> *>(sourcePtr);
        //ensures service or controller is registered
        if(registery)
            registery->registerOnce(v);
        //save link
        const QRemoteLink &link = QRemoteLink::fromObject(v.data());
        stream << link;
        if(stream.status() == QDataStream::Ok) {
            this->data = buffer;
            return true;
        }
    } else if(QMetaType::save(stream, id, sourcePtr)) {
        this->data = buffer;
        return true;
    }
    return false;
}

bool QRemoteType::load(const void *sourcePtr, QRemoteUser *registery)
{
    return load(find(typeName), sourcePtr, registery);
}

bool QRemoteType::save(int id, void *targetPtr, QRemoteUser *registery) const
{
    if(id == QMetaType::Void)
        return true;
    else if(id <= QMetaType::UnknownType) {
        return false;
    }

    QDataStream stream(this->data);
    if(id == qMetaTypeRemoteRef) {
        QRemoteLink link;
        stream >> link;
        if(stream.status() == QDataStream::Ok) {
            QRef<QObject> &v = *reinterpret_cast<QRef<QObject> *>(targetPtr);
            v = link.toObject(registery);
            return true;
        }
        return false;
    }
    return QMetaType::load(stream, id, targetPtr);
}

bool QRemoteType::save(void *targetPtr, QRemoteUser *registery) const
{
    return save(find(typeName), targetPtr, registery);
}

static const char STR_REF_PREFIX[] = "QRef<";

int QRemoteType::find(const QLatin1String &name)
{
    //QREMOTE_VOID_SUPPORT: supporting "void" is required for request/reply of slots without return-value
    if (name.isEmpty() || name == QLL("void")) //QREMOTE_VOID_SUPPORT
        return QMetaType::Void; //special case if return type is void
    else if (name.endsWith('*'))
        return QMetaType::UnknownType; //just to prevent pointers
    else if (name.size() >= int(sizeof(STR_REF_PREFIX) - 1)
            && memcmp(name.data(), STR_REF_PREFIX, sizeof(STR_REF_PREFIX)-1) == 0)
    {
        return qMetaTypeRemoteRef;
    }
    int id = QMetaType::type(name.data());
    return id;
}

QDebug &operator <<(QDebug &s, const QRemoteType &v)
{
    QDebugStateSaver _(s);
    s.noQuotes().nospace(); //ensures output is the same by any "QDebug &"
    s << QString::fromUtf8(v.typeName, v.typeName.length());
    s << '=';
    const QString &str = v.toString();
    if(str.length())
        s.quotes() << str;
    else if(v.data.size())
        s << v.data.size() << QLL("-bytes");
    else
        s << '0';
    return s;
}

QDataStream &operator<<(QDataStream &stream, const QRemoteType &param)
{
    return stream << param.typeName << param.data;
}
QDataStream &operator>>(QDataStream &stream, QRemoteType &param)
{
    stream >> param.typeName;
    if (stream.status() != QDataStream::Ok) return stream;
    stream >> param.data;
    return stream;
}

QString QRemoteLink::toString() const
{
    QString str;
    str.reserve(this->userName.size() + 1
            + this->typeName.size() + 1
            + this->id.size()
        );
    str += this->userName;
    str += QLatin1Char('/');
    str += QString::fromUtf8(this->typeName);
    str += QLatin1Char('/');
    str += this->id;
    return str;
}

QRemoteLink QRemoteLink::fromString(const QString &s)
{
    QRemoteLink result;
    int pos = s.indexOf(QLatin1Char('/'));
    if(pos >= 0) {
        result.userName = s.left(pos);
        int idPos = s.lastIndexOf(QLatin1Char('/'));
        if(idPos > pos) {
            result.typeName = s.mid(pos, idPos - pos - 1).toUtf8();
            result.id = s.mid(idPos);
        } else {
            result.typeName = s.mid(pos).toUtf8();
        }
    }
    return result;
}

QRef<QObject> QRemoteLink::toObject(QRemoteUser *session)
{
    QRef<QObject> ref;
    if (session) {
        if(this->userName != session->objectName()) {
            ref = session->createRemote(*this);
        } else {
            ref = session->findLocal(*this);
        }
    }
    return ref;
}

QRemoteLink QRemoteLink::fromObject(const QObject *obj)
{
    QRemoteLink result;
    if(obj) {
        result.userName = QObjectRemotePrivate::findUserName(obj);
        result.typeName = QRemoteAddress::typeFromObject(obj);
        result.id = obj->objectName();
    }
    return result;
}

QByteArray QRemoteAddress::typeFromObject(const QObject *obj)
{
#if 1
    const QMetaObject *m = obj->metaObject();
    if (m) {
        const bool isRemoteObj = QObjectPrivate::get(obj)->isRemote;
        if (m->indexOfClassInfo(QMetaObject::remoteIdRaw) >= 0) {
            const char *className = m->className();
            uint len = qstrlen(className);
            if (isRemoteObj)
                len -= 6;
            return QByteArray::fromRawData(className, len);
        }
    }
    return QByteArray();
#else
    //includes full namespace (e.g. returns "My::Service" from "My::ServiceRemote")
    QByteArray result;
    QList<const char *> nemes;
    nemes.reserve(20 * sizeof(qintptr));
    const QMetaObject *m = obj->metaObject();
    while (m) {
        if(m->indexOfClassInfo(QMetaObject::remoteIdRaw) >= 0)
            nemes.prepend(m->className());
        m = m->d.superdata;
    }

    int i = 0;
    if(nemes.count() > 0) {
        if(qstrcmp(nemes.at(0), "QObject") == 0)
            i++;
        if(nemes.count() > 1 && qstrcmp(nemes.at(1), "QObjectRemote") == 0)
            i++;
    }

    QObjectPrivate *p = QObjectPrivate::get(const_cast<QObject *>(obj));
    result.reserve((35 + p->isRemote ? 2 : 8) * nemes.count());
    for(; i < nemes.count(); i++) {
        const char *name = nemes.at(i);
        result.append(name);
        if(p->isRemote)
            result.chop(6); //remove "Remote" suffix
        result.append(':');
        result.append(':');
    }
    result.chop(2);
    result.squeeze();
    return result;
#endif
}

QDebug &operator <<(QDebug &dbg, const QRemoteLink &v) {
    QDebugStateSaver last(dbg);
    dbg.noQuotes().nospace();
    if (last.hadQuotes()) dbg << '\"';

    if(v.isNull())
        dbg << "<Null>";
    else {
        if(!v.userName.isEmpty())
            dbg << v.userName;
        else
            dbg << "?";
        dbg << '/';

        if(v.id.isEmpty()) {
            if(!v.typeName.isEmpty()) {
                dbg << QString::fromUtf8(v.typeName);
            } else
                dbg << "<Unknown>";
        } else {
            dbg << v.id;
        }
    }

    if(last.hadQuotes()) dbg << '\"';
    return dbg;
}

QDebug &operator <<(QDebug &dbg, const QRemoteAddress &v)
{
    QDebugStateSaver last(dbg);
    dbg.noQuotes().nospace();
    if (last.hadQuotes())
        dbg << '"';

    if(v.id.isEmpty()) {
        if(!v.typeName.isEmpty()) {
            dbg << QString::fromUtf8(v.typeName);
        } else
            dbg << "[Unknown]";
    } else {
        dbg << QString::fromUtf8(v.typeName);
        dbg << '<' << v.id << '>';
    }

    if (last.hadQuotes())
        dbg << '"';
    return dbg;
}

QT_END_NAMESPACE
