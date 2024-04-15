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

#ifndef QT_QREMOTE_OBJECT_P_H
#define QT_QREMOTE_OBJECT_P_H

#include "qremoteobject.h"

#include <private/qobject_p.h>


QT_BEGIN_NAMESPACE

namespace QRemote {
    void warnType(const char *clazz, const char *func, const char *typeName, const char *address = 0);
} //namespace QRemote


class QT_REMOTE_EXPORT QObjectRemotePrivate : public QObjectPrivate, public QRemoteData
{
public:
    QObjectRemotePrivate();
//    ~QObjectRemotePrivate();

    static inline QObjectRemotePrivate *get(QObjectRemote *o) { return o->d_func(); }
    static inline const QObjectRemotePrivate *get(const QObjectRemote *o) { return o->d_func(); }

    static Q_ALWAYS_INLINE QRemoteUserName findUserName(const QObject *obj) {
        const QObjectRemotePrivate *d =  QObjectRemotePrivate::get(static_cast<const QObjectRemote *>(obj));
        if (d->isRemote)
            return d->user;
        else {
            const QRemoteData *data = QMetaRemote::dataFromObject(obj);
            if(data && data->session) {
                return Q_PTR_CAST(const QObject *, data->session)->objectName();
            }
        }
        return QString();
    }

    static Q_ALWAYS_INLINE QRemoteData *findData(QObjectPrivate *d)
    {
        if (d->isRemote)
            return reinterpret_cast<QObjectRemotePrivate *>(d);
        else if (d->isReinterpretable)
            return reinterpret_cast<QRemoteData *>(d->privateData(QMetaObject::remoteId));
        return Q_NULLPTR; //object is not registered
    }

    static Q_ALWAYS_INLINE QRemoteData *findData(const QObject *local)
    {
        QObjectPrivate * d = QObjectPrivate::get(const_cast<QObject *>(local));
        return QObjectRemotePrivate::findData(d);
    }

    QString user;
};

QT_END_NAMESPACE

#endif //QT_QREMOTE_OBJECT_P_H
