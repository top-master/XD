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

#include "qremote-device-handler.h"

#include "qremoteuser_p.h"
#include "qremote-packet-codec.h"

#include <QIODevice>
#include <QDebug>
#include <QBuffer>


QT_BEGIN_NAMESPACE
using namespace QRemote;

static Q_CONSTEXPR QString MSG_DEVICE_MISSING = QStringLiteral("Device not set (or destroyed)");
static Q_CONSTEXPR QString MSG_DEVICE_READ_FAILED = QStringLiteral("Device read failed.");


DeviceHandler::DeviceHandler(QRemoteUser *owner, bool isThreadSafe)
    : m_owner(owner)
    , m_device(Q_NULLPTR)
    , m_readSizeMax(0)
    , m_isThreadSafe(isThreadSafe)
{
    QObjectData::get(this)->sendChildEvents = false;
    if (m_isThreadSafe) {
        this->moveToThread(owner->thread());
        this->setParent(owner);
    }
}

DeviceHandler::~DeviceHandler()
{
    // Ensures latest buffered "reply" gets sent to client before disconnection
    // (where QIODevice may log warning if `bytesToWrite()` is not checked).
    if (m_device && m_device->bytesToWrite() > 0) {
        m_device->waitForBytesWritten(3000);
    }
}

void DeviceHandler::dispose(bool removeFromOwner)
{
    const QSharedPointer<DeviceHandler> ref =
            QSharedPointer<DeviceHandler>::fromStack(this);

    this->setDevice(Q_NULLPTR);

    // Unbind from owner.
    QRemoteUser *owner = m_owner.data();
    if (owner) {
        owner->disconnect(this);
        this->disconnect(owner);
        if (removeFromOwner) {
            QRemoteUserPrivate *d = QRemoteUserPrivate::get(owner);
            int i = d->deviceHandlers.indexOf(ref);
            if (i >= 0) {
                d->deviceHandlers.remove(i);
            } else {
                qWarning("QRemote: device-handler was removed from session before IO-device.");
            }
        }
    }

    // Don't reference DeviceHandler after the related QRemoteUser is deleted,
    // otherwise, expect undefined-behaviour or crashes.
    if (m_isThreadSafe) {
        Q_ASSERT(ref.internalData()->strongref.load() > 1);
        this->setParent(Q_NULLPTR);
        Q_ASSERT(ref.internalData()->strongref.load() == 1);
    } else {
        Q_ASSERT(ref.internalData()->strongref.load() == 1);
    }
}

void DeviceHandler::setDevice(QIODevice *newDevice)
{
    QRemoteUser *owner = m_owner.data();

    // Detaches from old device.
    if (m_device != Q_NULLPTR) {
        disconnect(m_device, &QIODevice::destroyed,
                   this, &DeviceHandler::onDeviceDestroyed);
        disconnect(m_device, &QIODevice::readyRead,
                   this, &DeviceHandler::receiveData);
    }

    // Reset state.
    clearLastError();
    m_device = Q_NULLPTR;
    resetReadState();

    // Attach to new device.
    if (newDevice == Q_NULLPTR) {
        setError(MSG_DEVICE_MISSING);
        return;
    }
    if ( ! m_isThreadSafe) {
        this->moveToThread(newDevice->thread());
    }

    // Bind to owner (where Qt will auto use `QueuedConnection` if required).
    connect(owner, &QRemoteUser::sendData,
            this, &DeviceHandler::sendData);
    connect(this, &DeviceHandler::ofReceivedData,
            owner, &QRemoteUser::receiveData);
    connect(this, &DeviceHandler::ofPacketLimitExceed,
            owner, &QRemoteUser::onPacketLimitExceed);

    m_device = newDevice;
    connect(newDevice, &QObject::destroyed,
            this, &DeviceHandler::onDeviceDestroyed);
    connect(newDevice, &QIODevice::readyRead,
            this, &DeviceHandler::receiveData,
            Qt::QueuedConnection);

    // Handles that QIODevice::readyRead may already be sent, and
    // it won't be re-emitted any time soon.
    if (newDevice->bytesAvailable() != 0) {
        receiveData();
    }
}

bool DeviceHandler::flush(int msecs) {
    QIODevice *device = m_device.data();
    if (device) {
        QElapsedTimer timer;
        timer.start();
        forever {
            // Since `waitForBytesWritten` returns `false` if already flushed.
            if (device->waitForBytesWritten(500)
                || device->bytesToWrite() <= Q_INT64_C(0)
            ) {
                return true;
            } else if ( ! timer.timeLeft(msecs)) {
                break;
            }
        }
    }

    return false;
}

bool DeviceHandler::sendData(const QByteArray &pkt)
{
    // Prepare.
#if QREMOTE_DEBUG
    QDebug debug = qDebug();
    debug << "QRemoteUser.send:";
    QT_FINALLY([&] {
        if ( ! this->m_lastError.isEmpty()) {
            debug.noquote() << m_lastError;
        }
    });
#endif
    clearLastError();

    // Intentionally not checking for empty
    // (for example, empty may be used to keep connection alive).
    if (pkt.isNull()) {
        setError(QStringLiteral("Packet-data is null."));
        return false;
    }

    if (m_device == Q_NULLPTR) {
        setError(MSG_DEVICE_MISSING);
        return false;
    }
    if ( ! m_device->isWritable()) {
        setError(QStringLiteral("Device is not Writable"));
        return false;
    }

#if QREMOTE_DEBUG
    debug << pkt.size() << "bytes";
    QElapsedTimer timer; timer.start();
#endif

    // TRACE/remote note: write is optimized with Single-write mode,
    // which ensures `QIODevice::writeData(...)` is called once,
    // where "pkt.size()" and "pkt.data()" are both written first
    // to "buffer", then sent to device.
    QByteArray buffer;
    buffer.reserve(pkt.size() + m_writeExtraReserve);
    QDataStream bStream(&buffer, QIODevice::ReadWrite | QIODevice::Unbuffered);
    // The "BigEndian" is the default, else would do:
    // ```
    // bStream.setByteOrder(QDataStream::BigEndian);
    // ```
    bStream << pkt;
    m_device->write(buffer);

#if QREMOTE_DEBUG
    debug << "in" << timer.elapsed() << "msec";
    if (m_owner) {
        QT_TRY {
            PacketUniquePtr packet = m_owner->codec()->decode(pkt);
            debug << "{\n" << *packet << "\n}";
        } QT_CATCHES(const PacketCodecException &err,
            debug << "but decode failed:" << err.message();
        ) QT_CATCHES(... , )
    }
#endif

    return true;
}

bool DeviceHandler::receiveData()
{
    // Prepare.
#if QREMOTE_DEBUG
    QDebug debug = qDebug();
    debug.setMessageEnabled(false);
    debug << "qRemote::receiveData:";
    QT_FINALLY([&] {
        if ( ! m_lastError.isEmpty()
            && m_lastError.constData() != MSG_DEVICE_READ_FAILED.constData()
        ) {
            debug.setMessageEnabled();
            debug.noquote() << m_lastError;
        }
    });
#endif
    clearLastError();

    //
    // State validation.
    //
    if (m_device == Q_NULLPTR
        || ! m_device->isReadable()
    ) {
        setError(m_device
            ? QStringLiteral("Device not readable.")
            : MSG_DEVICE_MISSING);
        return false;
    }

    // Prepare.
    QDataStream reader(m_device);

    // TRACE/remote: support server disconnecting while receive #1,
    // which needs rechecking `isOpen()` inside the loop.
    while (reader.device()->isOpen()
           && reader.device()->bytesAvailable() > 0) {
        if (m_readBuffer.isEmpty() && m_readSizeMax == 0) {
            Q_ASSERT(m_readSizeDone == 0);

            // Handles having not even 4 bytes yet.
            Q_STATIC_ASSERT(sizeof(quint32) == 4);
            if (reader.device()->bytesAvailable() < sizeof(quint32)) {
                // Assumes being re-called by QIODevice::readyRead later if
                // there's more bytes to read, which's normally the case.
                return true;
            }

            // Reads size and handles magic-number meaning zero-sized QByteArray.
            reader >> m_readSizeMax;
            if (m_readSizeMax == quint32(0xFFFFFFFF)) {
                m_readSizeMax = 0;
                continue;
            }

            // Handles size-limit (if any set).
            const quint32 sizeLimit = QRemoteUserPrivate::get(m_owner)->packetSizeLimit;
            if (sizeLimit > 0 && m_readSizeMax > sizeLimit) {
                emit ofPacketLimitExceed(this, sizeLimit);
                m_readSkip = true;
            } else {
                m_readBuffer.reserve(m_readSizeMax);
            }

#if QREMOTE_DEBUG
            qint64 bytesAvailable = m_device->bytesAvailable();
            if(bytesAvailable < m_readSizeMax) {
                debug << "bytes available:" << sizeof(quint32) + m_device->bytesAvailable();
                debug << "expected size:" << sizeof(quint32) + m_readSizeMax;
            }
#endif
        }

        const int readChunkLength = m_readSkip
                ? reader.skipRawData(m_readSizeMax - m_readSizeDone)
                : reader.readRawData(
                    m_readBuffer.data() + m_readSizeDone,
                    m_readSizeMax - m_readSizeDone);
        if (readChunkLength <= 0) {
            setError(MSG_DEVICE_READ_FAILED);
#if QREMOTE_DEBUG
            debug << "Read error:"
                 << "isOpen" << reader.device()->isOpen()
                 << "bytesAvailable" << reader.device()->bytesAvailable()
                 << "pos" << reader.device()->pos()
                 << "bytesLeft" << m_readSizeMax - m_readSizeDone;
#endif
            return false;
        }

        m_readSizeDone += readChunkLength;
        if (Q_UNLIKELY(m_readSizeDone > m_readSizeMax || m_readSkip)) {
            resetReadState();
        } else if (m_readSizeDone == m_readSizeMax) {
            // Lets buffer know who much of reserved memory is used.
            m_readBuffer.resize(m_readSizeDone);
#if QREMOTE_DEBUG && 0
            MethodPacketSP packet( m_owner->codec()->decode(m_readBuffer) );
            qDebug() << "qRemote::receiveData" << "{\n" << *packet << "\n}";
            debug.setMessageEnabled();
            debug.flush();
#endif
            // TRACE/remote: support server disconnecting while receive #2,
            // where maybe the device gets closed after/while this gets emitted.
            emit ofReceivedData(m_readBuffer);
            resetReadState();
        }
    }

    // TRACE/remote: support server disconnecting while receive #3,
    // where if disconnected, buffer is invalid.
    if (reader.device()->isOpen() == false
        && ! m_readResumable
    ) {
        resetReadState();
    }

    return true;
}

void DeviceHandler::setError(const QString &value)
{
    m_lastError = value;
}

void DeviceHandler::onDeviceDestroyed(QObject *devicePtr) {
#if QREMOTE_DEBUG
    QDebug dbg = qDebug();
    dbg << "QRemoteUser.onDeviceDestroyed";
#endif
    if (devicePtr != m_device.data() && m_device) {
        QREMOTE_DEBUG_SCOPE(dbg << "ignored";)
        return;
    }
    QREMOTE_DEBUG_SCOPE(dbg.flush();)

    this->dispose(true);
}

QT_END_NAMESPACE
