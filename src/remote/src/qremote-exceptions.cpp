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

#include "qremote-exceptions.h"

#include <QtCore/qdebug.h>


QT_BEGIN_NAMESPACE
using namespace QRemote;

QRemoteException::QRemoteException(const QString &msg)
    : super(msg)
{
    qDebug() << "QRemoteException::created:" << msg;
}

QRemoteException::~QRemoteException() Q_DECL_NOTHROW
{
    // Nothing to do (but required).
}

InvalidMethodException::InvalidMethodException(const QString &msg)
    : QRemoteException(msg)
{
    // Nothing to do (but required).
}

PacketCodecException::PacketCodecException(const QString &msg, ErrorPacket::ErrorType val)
    : QRemoteException(msg)
{
    packetErrorType = val;
}

QT_END_NAMESPACE
