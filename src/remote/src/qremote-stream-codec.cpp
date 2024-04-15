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

#include "qremote-stream-codec.h"

#include <QtCore/QBuffer>


QT_BEGIN_NAMESPACE
using namespace QRemote;

StreamPacketCodec::~StreamPacketCodec()
{
    // Nothing to do (but required).
}

PacketUniquePtr StreamPacketCodec::decode(const QByteArray &pkt)
{
    QString errorMsg;
    QScopedPointer<Packet> packet(Packet::load(pkt, &errorMsg, version()));
    if (packet.data() == Q_NULLPTR) {
        PacketCodecException error(errorMsg, ErrorPacket::ProtocolError);
        throw error;
    }
    return PacketUniquePtr(packet.take());
}

QByteArray StreamPacketCodec::encode(const Packet &pkt)
{
    QByteArray buffer;
    QDataStream bStream(&buffer, QIODevice::WriteOnly | QIODevice::Unbuffered);
    // The "BigEndian" is the default, else would do:
    // ```
    // bStream.setByteOrder(QDataStream::BigEndian);
    // ```
    const int version = this->version();
    if (version != 0) {
        bStream.setVersion(version);
    }
    pkt.save(bStream);
    return buffer;
}

QT_END_NAMESPACE
