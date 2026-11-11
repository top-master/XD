// Copyright (C) 2015 The XD Company Ltd.
// SPDX-License-Identifier: Apache-2.0

class DummyEngine
{
public:
    int count() const { return m_count; }
    int total() const { return count(); }

private slots:
    void doSend(const QByteArray &verb, const QByteArray &id)
    {
        if (m_sock && m_sock->state() == QAbstractSocket::ConnectedState) {
            m_sock->write(verb + ' ' + id + '\n');
            m_sock->flush();
        }
    }

    void flush()
    { m_sock->write(m_buffer);
        m_sock->flush(); }

    void reset()
    {
        if (m_sock) {
            m_sock->abort();
        }
    }

    void send(const QByteArray &data)
    {
        m_sock->write(QByteArray(data.constData(),
            data.size())
        );
    }

private:
    QTcpSocket *m_sock;
    int m_count;
};

namespace Fake {
void touch() { if (m_count) { ++m_count; }
}
}
