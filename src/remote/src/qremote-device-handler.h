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

#ifndef QT_REMOTE_DEVICE_HANDLER_H
#define QT_REMOTE_DEVICE_HANDLER_H

#include "qremote-config.h"

#include <QtCore/qbytearray.h>
#include <QtCore/qdatastream.h>
#include <QtCore/qelapsedtimer.h>
#include <QtCore/qpointer.h>


QT_BEGIN_NAMESPACE

class QRemoteUser;

namespace QRemote {

/**
 * @internal
 *
 * Use QRemoteUser::addDevice(QIODevice *) instead, which
 * creates one %DeviceHandler per QIODevice
 *
 * @warning Signal(s) may be used instead of directly calling QRemoteUser which
 * allows Qt to process events, preventing connection timeout(s) and
 * if App's connection gets timeout even with these, then
 * search for the cause outside of QtRemote module.
 *
 * @warning Don't use this after related QRemoteUser A.K.A. owner is deleted,
 * otherwise, expect undefined-behaviour or crashes.
 */
class DeviceHandler : public QObject {
    Q_OBJECT
public:
    /// @warning Needs @ref setDevice to be called separately.
    explicit DeviceHandler(QRemoteUser *owner, bool isThreadSafe = false);
    ~DeviceHandler() Q_DECL_OVERRIDE_DEL;

    void dispose(bool removeFromOwner = false);


    /// @warning Does not handle @ref QIODevice::readChannelFinished, which
    /// may mean connection lost.
    void setDevice(QIODevice *);
    inline const QPointer<QIODevice> &device() const { return m_device; }
    inline QPointer<QIODevice> &device() { return m_device; }

    inline bool isDeviceReady() const {
        return m_device && m_device->isOpen() && m_device->isReadable() && m_device->isWritable();
    }

    bool flush(int msecs = 3000);


    inline const QString &lastError() const { return m_lastError; }
    inline void clearLastError() { setError(QString()); }

public slots:
    bool sendData(const QByteArray &pkt);

    /// Reads all/any %Packet from QIODevice's current position, and that
    /// without ever trying to "seek" or "open"
    /// (those are not even QRemoteUser class's responsibility).
    ///
    /// @returns @c true if QIDevice was valid, even if had nothing to read yet.
    ///
    /// @sa QIODevice::seek
    /// @sa QIODevice::open
    /// @sa lastError
    bool receiveData();

signals:
    void ofReceivedData(const QByteArray &pkt);

    /// Notifies @ref QRemoteUser::setPacketSizeLimit "setPacketSizeLimit(...)" exceeds.
    void ofPacketLimitExceed(DeviceHandler *, quint32 sizeLimit);

protected:
    virtual void setError(const QString &);

private slots:
    void onDeviceDestroyed(QObject *devicePtr);

private:
    inline void resetReadState() {
        m_readBuffer.clear();
        m_readSizeMax = 0;
        m_readSizeDone = 0;
        m_readSkip = false;
    }

private:
    Q_DISABLE_COPY(DeviceHandler)

    QPointer<QRemoteUser> m_owner;

    /// Maximum size reported (by binary-header of QByteArray) for %Packet.
    quint32 m_readSizeMax;
    quint32 m_readSizeDone;
    bool m_readSkip;
    /// @var m_readResumable Whether QIODevice will provide same data and that
    /// without seek need, if re-openned after previous close/disconnect.
    enum {
        m_readResumable = 0
    };
    /// Whether the device is thread-safe by its own or not.
    bool m_isThreadSafe;

    QString m_lastError;
    QPointer<QIODevice> m_device;
    /// For performance reasons, reserves memory for raw-data packet receive/read.
    QByteArray m_readBuffer;

    /// @var m_writeExtraReserve
    /// Assumend QByteArray binary-header size.
    ///
    /// @warning Intentionally more than needed, to
    /// support different Qt versions.
    enum {
        m_writeExtraReserve = 16
    };
};

} // namespace QRemote

QT_END_NAMESPACE

#endif // QT_REMOTE_DEVICE_HANDLER_H
