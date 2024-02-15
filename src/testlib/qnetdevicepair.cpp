
#include "qnetdevicepair.h"

#include <QtCore/qcoreapplication.h>
#include <QtCore/private/qbuffer_p.h>

QT_BEGIN_NAMESPACE

QNetDevice::QNetDevice()
    : m_pair(Q_NULLPTR)
    , m_isInstantSync(true)
{
    this->open(QIODevice::ReadWrite);

    QObject::connect(this, &QBuffer::bytesWritten,
                     this, &QNetDevice::onBytesWritten,
                     Qt::DirectConnection);
}

bool QNetDevice::open(OpenMode mode)
{
    QBufferPrivate *d = QBufferPrivate::get(this);

    bool result = super::open(mode);
    // Internally allows `RandomAccess`, pulic is `Sequential`.
    d->accessMode = QIODevicePrivate::RandomAccess;

    return result;
}

qint64 QNetDevice::writeData(const char *data, qint64 length)
{
    length = super::writeData(data, length);

    if (m_isInstantSync) {
        this->sync(length);
    }

    return length;
}

bool QNetDevice::isSequential() const
{
    return true;
}

bool QNetDevice::seek(qint64 pos)
{
    Q_UNUSED(pos)
    return false;
}

void QNetDevice::reset(const QByteArray &newValue)
{
    QBufferPrivate *d = QBufferPrivate::get(this);
    *d->buf = newValue;
    forceSeek(0);
    if ( ! newValue.isEmpty()) {
        emit readyRead();
        QCoreApplication::processEvents(QEventLoop::AllEvents);
    }
}

void QNetDevice::sync(qint64 bytes)
{
    QBufferPrivate *d = QBufferPrivate::get(this);

    if (m_pair && bytes > 0) {
        QBufferPrivate *dPair = QBufferPrivate::get(m_pair);
        dPair->buf->append(d->buf->end() - bytes, bytes);
        emit m_pair->readyRead();
        QCoreApplication::processEvents(QEventLoop::AllEvents);
    }
}

void QNetDevice::onBytesWritten(qint64 bytes) {
    if ( ! m_isInstantSync) {
        this->sync(bytes);
    }
}

QT_END_NAMESPACE
