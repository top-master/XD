#ifndef ECHO_SERVICE_H
#define ECHO_SERVICE_H

#include "service_base.h"

// ---------------------------------------------------------------------------
// EchoService: a plain TCP + UDP echo on one port (the FileTransfer secondary).
// ---------------------------------------------------------------------------

class EchoService : public TestService
{
public:
    EchoService(const ServiceConfig &c, QObject *parent = Q_NULLPTR)
        : TestService(c, parent) {}

    void start() Q_DECL_OVERRIDE
    {
        QTcpServer *tcp = new QTcpServer(this);
        listenTcp(tcp); // records m_port
        connect(tcp, &QTcpServer::newConnection, this, &EchoService::onNewConnection);
        QUdpSocket *udp = new QUdpSocket(this);
        udp->bind(m_host, m_port); // match the TCP echo's port
        connect(udp, &QUdpSocket::readyRead, this, &EchoService::onUdpReadyRead);
    }

private:
    void onNewConnection()
    {
        QTcpSocket *s = qobject_cast<QTcpServer *>(sender())->nextPendingConnection();
        connect(s, &QTcpSocket::readyRead, this, &EchoService::onTcpReadyRead);
        connect(s, &QTcpSocket::disconnected, s, &QObject::deleteLater);
    }
    void onTcpReadyRead()
    {
        QTcpSocket *s = qobject_cast<QTcpSocket *>(sender());
        s->write(s->readAll());
    }
    void onUdpReadyRead()
    {
        QUdpSocket *udp = qobject_cast<QUdpSocket *>(sender());
        while (udp->hasPendingDatagrams()) {
            QByteArray data(int(udp->pendingDatagramSize()), Qt::Uninitialized);
            QHostAddress from; quint16 fromPort;
            udp->readDatagram(data.data(), data.size(), &from, &fromPort);
            udp->writeDatagram(data, from, fromPort);
        }
    }
};

#endif // ECHO_SERVICE_H
