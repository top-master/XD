#ifndef QNETDEVICE_PAIR_H
#define QNETDEVICE_PAIR_H

#include <QtTest/qtest_global.h>

#include <QtCore/qbuffer.h>
#include <QtCore/qiodevice.h>
#include <QtCore/qobject.h>


QT_BEGIN_NAMESPACE

/// Implementation behind QNetDevicePair,
/// otherwise, may be used to simulate a sequential QBuffer.
class Q_TESTLIB_EXPORT QNetDevice : public QBuffer {
    Q_OBJECT
    typedef QBuffer super;
public:
    QNetDevice();

    inline QIODevice *pair() { return m_pair; }

    inline bool isInstantSync() const { return m_isInstantSync; }
    inline void setInstantSyncEnabled(bool state) { m_isInstantSync = state; }

    bool open(OpenMode mode) Q_DECL_OVERRIDE;

    qint64 writeData(const char *data, qint64 length) Q_DECL_OVERRIDE;

    /// Netowrk Sockets don't allow random-access.
    /// @returns always @c true.
    /// @see seek(qint64)
    bool isSequential() const Q_DECL_OVERRIDE;

    /// Netowrk Socket is not seek-able.
    /// @returns always @c false.
    bool seek(qint64 pos) Q_DECL_OVERRIDE;

    void reset(const QByteArray &);

    inline void clear() { reset(QByteArray()); }

protected:
    inline void setPair(QNetDevice *other) { m_pair = other; }

    inline bool forceSeek(qint64 pos) { return super::seek(pos); }

    void sync(qint64 bytes);

private Q_SLOTS:
    void onBytesWritten(qint64 bytes);

private:
    friend class QNetDevicePair;
    Q_DISABLE_COPY(QNetDevice)

    bool m_isInstantSync;
    QNetDevice *m_pair;
};

/// Helper to simulate a "local" connection's Socket
/// (meaning, zero packet loss).
class QNetDevicePair {
public:
    inline QNetDevicePair()
    {
        m_server.setPair(&m_client);
        m_client.setPair(&m_server);
    }

    inline QNetDevice *server() { return &m_server; }
    inline QNetDevice *client() { return &m_client; }

    inline const QByteArray &serverData() const { return m_server.data(); }
    inline const QByteArray &clientData() const { return m_client.data(); }

    inline void clear() {
        m_server.clear();
        m_client.clear();
    }

private:
    Q_DISABLE_COPY(QNetDevicePair)

    QNetDevice m_server;
    QNetDevice m_client;
};

QT_END_NAMESPACE

#endif // QNETDEVICE_PAIR_H
