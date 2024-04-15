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

#ifndef QT_REMOTE_EXCEPTION_H
#define QT_REMOTE_EXCEPTION_H

#include "qremote-packet.h"

#include <QtCore/QException>
#include <QtCore/QString>
#include <QtCore/QByteArray>


QT_BEGIN_NAMESPACE

/// Base for all QRemote exceptions,
/// which allows `catch` of all QRemote exceptions at once.
class QT_REMOTE_EXPORT QRemoteException : public QExceptionWithMessage {
    typedef QExceptionWithMessage super;
public:
    explicit QRemoteException(const QString &msg);
    virtual ~QRemoteException() Q_DECL_NOTHROW;
};


namespace QRemote {

class QT_REMOTE_EXPORT InvalidMethodException: public QRemoteException {
public:
    explicit InvalidMethodException(const QString &msg);
};

class QT_REMOTE_EXPORT PacketCodecException: public QRemoteException {
public:
    explicit PacketCodecException(const QString &msg, ErrorPacket::ErrorType val);

    ErrorPacket::ErrorType packetErrorType;
};

} // namespace QRemote

QT_END_NAMESPACE

#endif // QT_REMOTE_EXCEPTION_H
