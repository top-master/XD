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

#ifndef QREMOTEREF_H
#define QREMOTEREF_H

#include "qremote-config.h"

#include <QtCore/QObject>
#include <QtCore/QString>
#include <QtCore/QVariant>
#include <QtCore/QVector>
#include <QtCore/qsharedpointer.h>


QT_BEGIN_NAMESPACE

class QDebug;
class QRemoteUser;
class QObjectRemote;
class QRemoteData;

class QRemoteType {
public:
    inline QRemoteType() {}
    inline explicit QRemoteType(const QByteArray &type)
        : typeName(type) {}
    inline QRemoteType(const QByteArray &type, const QByteArray &value)
        : typeName(type), data(value) {}

    QByteArray typeName;
    QByteArray data;

    QVariant toVariant() const;
    QString toString() const;
    int toTypeId() const { return find(typeName); }

    static inline QRemoteType fromRawType(const QByteArray &name, void *ptr, QRemoteUser *registery = 0);
    /// @returns Pointer to constructed type, but `nullptr` for
    /// the `void` type, or if any error happens.
    ///
    /// @warning Manually delete returned "ptr" using both
    /// `QMetaType::destroy(typeId, ptr)` and `qFreeAligned(ptr)` (in this order).
    void *toRawType(QRemoteUser *registery) const;
    /// Same as toRawType(QRemoteUser *registery), but allows custom type-id,
    /// instead of finding said id based on @ref typeName.
    void *toRawType(int id, QRemoteUser *registery) const;

    //setting "registery" ensures any "QRef<...>" type is registered
    bool load(const void *sourcePtr, QRemoteUser *registery);
    bool load(int id, const void *sourcePtr, QRemoteUser *registery);

    //returns false if "targetPtr" is unchanged.
    //  note: use "targetType" to force type check
    bool save(void *targetPtr, QRemoteUser *registery) const;
    bool save(int id, void *targetPtr, QRemoteUser *registery) const;

    static int find(const QLatin1String &name);
    static inline int find(const QByteArray &name) { return find(QLatin1String(name)); }
};

QDebug &operator <<(QDebug &s, const QRemoteType &v);

QDataStream &operator<<(QDataStream &stream, const QRemoteType &param);
QDataStream &operator>>(QDataStream &stream, QRemoteType &param);

Q_DECLARE_METATYPE(QRemoteType)
Q_DECLARE_METATYPE(QList<QRemoteType>)
Q_DECLARE_METATYPE(QVector<QRemoteType>)

class QT_REMOTE_EXPORT QRemoteAddress {
public:
    inline QRemoteAddress() {}
    inline explicit QRemoteAddress(const QObject *obj)
        : typeName(typeFromObject(obj)), id(obj->objectName())
    {}

    inline bool isNull() const { return typeName.isEmpty() && id.isEmpty(); }
    inline bool isEmpty() const { return typeName.isEmpty(); } //we allow/expect "id" to be empty or null

    QByteArray typeName;
    QString id;

    /// @returns same path for each local and remote object pair.
    static QByteArray typeFromObject(const QObject *obj);
};

typedef QString QRemoteId;

/**
* Unique-name given to a QRemoteUser, to address it in QtRemote communications.
*
* Note that this is only for QRemoteUser, while QRemoteAddress is for services.
*
* @sa QObject::setObjectName(...)
*/
typedef QString QRemoteUserName;

class QT_REMOTE_EXPORT QRemoteLink : public QRemoteAddress {
//    Q_DISABLE_COPY(QRemoteAddress)
public:
    inline QRemoteLink() {}
//    inline QRemoteLink(const QObject *obj)
//        : QRemoteAddress(obj) {}
    inline QRemoteLink(const QRemoteUserName &user, const QRemoteAddress &address)
        : QRemoteAddress(address), userName(user) {}

    QRemoteUserName userName;

    inline bool isNull() const { return userName.isEmpty() && typeName.isEmpty() && id.isEmpty(); }
    inline bool isEmpty() const { return typeName.isEmpty(); }

    inline void clear() { typeName.clear(); id.clear(); }

    QString toString() const;
    static QRemoteLink fromString(const QString &s);

    QRef<QObject> toObject(QRemoteUser *session);
    static QRemoteLink fromObject(const QObject *obj);
};

#ifndef Q_MOC_RUN

inline QRemoteType QRemoteType::fromRawType(const QByteArray &name, void *ptr, QRemoteUser *registery)
{
    QRemoteType r(name);
    r.load(ptr, registery);
    return r;
}
inline bool operator==(const QRemoteLink &left, const QRemoteLink &right)
    { return left.userName == right.userName && left.typeName == right.typeName && left.id == right.id; }

inline bool operator!=(const QRemoteLink &left, const QRemoteLink &right)
    { return left.userName != right.userName || left.typeName != right.typeName || left.id != right.id; }

inline bool operator<(const QRemoteLink &c1, const QRemoteLink &c2)
    {
        return c1.userName < c2.userName
            || ( ! (c2.userName < c1.userName) && c1.typeName < c2.typeName)
            || ( ! (c2.typeName < c1.typeName) && c1.id < c2.id);
    }

inline bool operator>(const QRemoteLink &c1, const QRemoteLink &c2)
    { return c2 < c1; }

inline bool operator<=(const QRemoteLink &c1, const QRemoteLink &c2)
    { return !(c2 < c1); }

inline bool operator>=(const QRemoteLink &c1, const QRemoteLink &c2)
    { return !(c1 < c2); }

inline QDataStream &operator <<(QDataStream &s, const QRemoteLink &v)
    { return s << v.userName << v.typeName << v.id; }
inline QDataStream &operator >>(QDataStream &s, QRemoteLink &v)
    { return s >> v.userName >> v.typeName >> v.id; }
QT_REMOTE_EXPORT QDebug &operator <<(QDebug &dbg, const QRemoteLink &v);

inline bool operator==(const QRemoteAddress &left, const QRemoteAddress &right)
    { return left.typeName == right.typeName && left.id == right.id; }

inline bool operator!=(const QRemoteAddress &left, const QRemoteAddress &right)
    { return left.typeName != right.typeName || left.id != right.id; }

inline bool operator<(const QRemoteAddress &c1, const QRemoteAddress &c2)
    { return c1.typeName < c2.typeName || (!(c2.typeName < c1.typeName) && c1.id < c2.id); }

inline bool operator>(const QRemoteAddress &c1, const QRemoteAddress &c2)
    { return c2 < c1; }

inline bool operator<=(const QRemoteAddress &c1, const QRemoteAddress &c2)
    { return !(c2 < c1); }

inline bool operator>=(const QRemoteAddress &c1, const QRemoteAddress &c2)
    { return !(c1 < c2); }

inline QDataStream &operator <<(QDataStream &s, const QRemoteAddress &v)
    { return s << v.id << v.typeName; }
inline QDataStream &operator >>(QDataStream &s, QRemoteAddress &v)
    { return s >> v.id >> v.typeName; }
QT_REMOTE_EXPORT QDebug &operator <<(QDebug &dbg, const QRemoteAddress &v);

#endif // Q_MOC_RUN

QT_END_NAMESPACE

#endif // QREMOTEREF_H
