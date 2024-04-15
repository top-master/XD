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

#ifndef QT_REMOTE_PACKET_CODEC_H
#define QT_REMOTE_PACKET_CODEC_H

#include "qremote-packet.h"
#include "qremote-exceptions.h"

#include <QtCore/qobject.h>
#include <QtCore/qbytearray.h>

#include <memory>


QT_BEGIN_NAMESPACE

namespace QRemote {

/// Base interface for any Codec which converts between %Packet and QByteArray.
///
/// Said QByteArray is then later written to QIODevice by %DeviceHandler, which
/// uses @ref QDataStream &operator<<(QDataStream &, const QByteArray &)
/// meaning, the QByteArray's binary-header (4 bytes for size) will prefix actual bytes.
///
/// @warning Codec is not forced to include version in %Packet, and
/// the available version() is only for local-compare purposes,
///
class QT_REMOTE_EXPORT PacketCodec : public QObject {
    Q_OBJECT
    typedef QObject super;
public:
    /// @param parent Can be #Q_NULLPTR.
    /// @param version Supported encode/decode format version.
    ///
    /// @warning Both arguments are required to force setting version, and
    /// to prevent mistaking @p version and @p parent with each other.
    inline explicit PacketCodec(QObject *parent, int version)
        : super(parent)
        , m_version(version)
    {}

    virtual ~PacketCodec();

    /// Supported protocol version.
    inline int version() const { return m_version; }

    /// @throw PacketCodecException
    virtual QByteArray encode(const Packet &pld) = 0;

    /// @throw PacketCodecException
    virtual PacketUniquePtr decode(const QByteArray &pld) = 0;

private:
    const int m_version;
};


} // namespace QRemote

QT_END_NAMESPACE

#endif // QT_REMOTE_PACKET_CODEC_H
