#ifndef SOCKS_SERVICE_H
#define SOCKS_SERVICE_H

#include "service_base.h"

#include <QtCore/QList>
#include <QtCore/QtEndian>

// ---------------------------------------------------------------------------
// SocksService: a minimal SOCKS5 CONNECT proxy (no-auth and user/pass).
// ---------------------------------------------------------------------------

class SocksService : public TestService
{
public:
    SocksService(const ServiceConfig &c, QObject *parent = Q_NULLPTR)
        : TestService(c, parent) {}

    void start() Q_DECL_OVERRIDE
    {
        QTcpServer *server = new QTcpServer(this);
        listenTcp(server);
        connect(server, &QTcpServer::newConnection, this, &SocksService::onNewConnection);
    }

private:
    void onNewConnection()
    {
        QTcpSocket *client = qobject_cast<QTcpServer *>(sender())->nextPendingConnection();
        connect(client, &QTcpSocket::readyRead, this, &SocksService::onReadyRead);
        connect(client, &QTcpSocket::disconnected, client, &QObject::deleteLater);
    }
    void onReadyRead() { onSocks(qobject_cast<QTcpSocket *>(sender())); }

    void onSocks(QTcpSocket *client)
    {
        // Stage 0: greeting "05 nmethods methods..." -> pick no-auth (00) if offered,
        // else user/pass (02); reply "05 <method>".
        if (!client->property("greeted").toBool()) {
            if (client->bytesAvailable() < 2)
                return;
            QByteArray g = client->read(2);
            if (quint8(g.at(0)) != 0x05)
                return;
            const QByteArray methods = client->read(quint8(g.at(1)));
            char method = char(0x00);
            if (!methods.contains(char(0x00)) && methods.contains(char(0x02)))
                method = char(0x02);
            client->write(QByteArray(1, char(0x05)) + QByteArray(1, method));
            client->setProperty("greeted", true);
            client->setProperty("needauth", method == char(0x02));
            return;
        }
        // Stage 0.5: username/password auth "01 ulen user plen pass" -> "01 00" (ok).
        if (client->property("needauth").toBool() && !client->property("authed").toBool()) {
            if (client->bytesAvailable() < 2)
                return;
            QByteArray v = client->read(1);
            const quint8 ulen = quint8(client->read(1).at(0));
            client->read(ulen);
            if (client->bytesAvailable() < 1)
                return;
            const quint8 plen = quint8(client->read(1).at(0));
            client->read(plen);
            Q_UNUSED(v);
            client->write(QByteArray::fromRawData("\x01\x00", 2)); // accept any credentials
            client->setProperty("authed", true);
            return;
        }
        // Stage 1: request "05 CMD 00 ATYP addr port" -> CONNECT (relay to upstream)
        // or BIND (listen, then relay the first incoming connection to the client).
        if (client->property("target").isNull()) {
            if (client->bytesAvailable() < 4)
                return;
            QByteArray hdr = client->read(4);
            const quint8 cmd = quint8(hdr.at(1));
            const quint8 atyp = quint8(hdr.at(3));
            QString hostStr;
            if (atyp == 0x01) {
                QByteArray a = client->read(4);
                hostStr = QHostAddress(qFromBigEndian<quint32>((const uchar *)a.constData())).toString();
            } else if (atyp == 0x03) {
                quint8 len = quint8(client->read(1).at(0));
                hostStr = QString::fromLatin1(client->read(len));
            }
            const quint16 dport = qFromBigEndian<quint16>((const uchar *)client->read(2).constData());
            client->setProperty("target", true);
            if (cmd == 0x02) { // BIND: the DST fields are advisory only, so they are ignored here.
                startBind(client);
                return;
            }
            if (cmd == 0x03) { // UDP ASSOCIATE: likewise the DST is the client's own bind, ignored.
                startUdpAssociate(client);
                return;
            }
            QTcpSocket *upstream = new QTcpSocket(client);
            linkPeers(client, upstream);
            upstream->setProperty("connectReply", QByteArray("\x05\x00\x00\x01\x00\x00\x00\x00\x00\x00", 10));
            upstream->setProperty("errorReply", QByteArray("\x05\x05\x00\x01\x00\x00\x00\x00\x00\x00", 10));
            connect(upstream, &QTcpSocket::connected, this, &SocksService::onTunnelConnected);
            connect(upstream, static_cast<void (QTcpSocket::*)(QAbstractSocket::SocketError)>(&QTcpSocket::error),
                    this, &SocksService::onTunnelError);
            connect(upstream, &QTcpSocket::readyRead, this, &SocksService::relayReadyRead);
            connect(client, &QTcpSocket::readyRead, this, &SocksService::relayReadyRead);
            connect(upstream, &QTcpSocket::disconnected, this, &SocksService::relayDisconnected);
            upstream->connectToHost(hostStr, dport);
        }
    }

    // Encode "ATYP(01) <IPv4> <port>" shared by the TCP reply and the UDP header.
    static QByteArray encodeAddrPort(const QHostAddress &addr, quint16 port)
    {
        const quint32 v4 = addr.toIPv4Address();
        QByteArray b;
        b.append(char(0x01));
        b.append(char((v4 >> 24) & 0xff)).append(char((v4 >> 16) & 0xff));
        b.append(char((v4 >> 8) & 0xff)).append(char(v4 & 0xff));
        b.append(char((port >> 8) & 0xff)).append(char(port & 0xff));
        return b;
    }

    // Build a SOCKS5 reply "05 REP 00 01 <IPv4> <port>".
    static QByteArray socksReply(quint8 rep, const QHostAddress &addr, quint16 port)
    {
        QByteArray b;
        b.append(char(0x05)).append(char(rep)).append(char(0x00));
        return b + encodeAddrPort(addr, port);
    }

    // Encode "ATYP(04) <v4-mapped IPv6> <port>". The client's UDP socket learns its own
    // address through a dual-stack probe, so it sees v4-mapped IPv6; the sender field
    // must match that form for the datagram-sender comparison to hold.
    static QByteArray encodeAddrPortV6(const QHostAddress &addr, quint16 port)
    {
        const Q_IPV6ADDR a6 = addr.toIPv6Address();
        QByteArray b;
        b.append(char(0x04));
        for (int i = 0; i < 16; ++i)
            b.append(char(a6[i]));
        b.append(char((port >> 8) & 0xff)).append(char(port & 0xff));
        return b;
    }

    // SOCKS5 BIND: listen on an ephemeral port and tell the client which one via the
    // first reply; on the first incoming connection send the second reply (its peer)
    // and relay bytes over the same control socket. The peer reaches this port through
    // the ordinary CONNECT path, so no special-casing of the listen port is needed.
    void startBind(QTcpSocket *control)
    {
        QTcpServer *bindServer = new QTcpServer(control);
        bindServer->listen(control->localAddress(), 0);
        bindServer->setProperty("bindControl", QVariant::fromValue<QObject *>(control));
        control->write(socksReply(0x00, bindServer->serverAddress(), bindServer->serverPort()));
        connect(bindServer, &QTcpServer::newConnection, this, &SocksService::onBindConnection);
    }

    void onBindConnection()
    {
        QTcpServer *bindServer = qobject_cast<QTcpServer *>(sender());
        QTcpSocket *control = qobject_cast<QTcpSocket *>(bindServer->property("bindControl").value<QObject *>());
        QTcpSocket *incoming = bindServer->nextPendingConnection();
        // BIND relays a single connection, so stop listening.
        bindServer->close();
        control->write(socksReply(0x00, incoming->peerAddress(), incoming->peerPort()));
        linkPeers(control, incoming);
        connect(incoming, &QTcpSocket::readyRead, this, &SocksService::relayReadyRead);
        connect(control, &QTcpSocket::readyRead, this, &SocksService::relayReadyRead);
        connect(incoming, &QTcpSocket::disconnected, this, &SocksService::relayDisconnected);
        connect(control, &QTcpSocket::disconnected, this, &SocksService::relayDisconnected);
    }

    // A live SOCKS5 UDP association. Its relay socket doubles as the endpoint peers
    // target: QSocks5SocketEngine reports the relay endpoint as the client's own
    // localAddress (via the bootstrap probe), so peers address each other by it.
    struct UdpAssoc
    {
        UdpAssoc() : control(0), relay(0), clientPort(0) {}
        QTcpSocket *control;
        QUdpSocket *relay;
        QHostAddress clientAddr;
        quint16 clientPort; // the client's real UDP source; nonzero once its first datagram lands.
    };
    QList<UdpAssoc *> m_udpAssocs;

    // SOCKS5 UDP ASSOCIATE: hand the client a relay endpoint, then forward datagrams
    // between associations, rewriting the SOCKS5 header's sender to the source relay
    // endpoint (which is what the receiving peer knows the sender as).
    void startUdpAssociate(QTcpSocket *control)
    {
        UdpAssoc *assoc = new UdpAssoc;
        assoc->control = control;
        assoc->relay = new QUdpSocket(control);
        assoc->relay->bind(control->localAddress(), 0);
        m_udpAssocs.append(assoc);
        control->write(socksReply(0x00, assoc->relay->localAddress(), assoc->relay->localPort()));
        connect(assoc->relay, &QUdpSocket::readyRead, this, &SocksService::onUdpRelayReady);
        connect(control, &QTcpSocket::disconnected, this, &SocksService::onUdpControlGone);
    }

    void onUdpRelayReady()
    {
        QObject *relay = sender();
        for (int i = 0; i < m_udpAssocs.size(); ++i)
            if (m_udpAssocs.at(i)->relay == relay) { onUdpRelay(m_udpAssocs.at(i)); return; }
    }

    void onUdpControlGone()
    {
        QObject *control = sender();
        for (int i = 0; i < m_udpAssocs.size(); ++i)
            if (m_udpAssocs.at(i)->control == control) {
                UdpAssoc *a = m_udpAssocs.takeAt(i);
                delete a->relay;
                delete a;
                return;
            }
    }

    void onUdpRelay(UdpAssoc *assoc)
    {
        while (assoc->relay->hasPendingDatagrams()) {
            QByteArray dg(int(assoc->relay->pendingDatagramSize()), 0);
            QHostAddress from;
            quint16 fromPort = 0;
            assoc->relay->readDatagram(dg.data(), dg.size(), &from, &fromPort);
            assoc->clientAddr = from; // the client's real UDP source, for delivery back.
            assoc->clientPort = fromPort;
            if (dg.size() < 4 || dg.at(0) != 0 || dg.at(1) != 0 || dg.at(2) != 0)
                continue; // RSV(2)+FRAG(1) must be zero; fragmentation is unsupported.
            const quint8 atyp = quint8(dg.at(3));
            int pos = 4;
            QHostAddress dst;
            if (atyp == 0x01) {
                dst = QHostAddress(qFromBigEndian<quint32>((const uchar *)dg.constData() + pos));
                pos += 4;
            } else if (atyp == 0x04) {
                Q_IPV6ADDR a6;
                for (int i = 0; i < 16; ++i)
                    a6[i] = quint8(dg.at(pos + i));
                dst = QHostAddress(a6);
                pos += 16;
            } else if (atyp == 0x03) {
                pos += 1 + quint8(dg.at(pos)); // domain: length byte then name.
            } else {
                continue;
            }
            if (dg.size() < pos + 2)
                continue;
            const quint16 dport = qFromBigEndian<quint16>((const uchar *)dg.constData() + pos);
            pos += 2;
            const QByteArray payload = dg.mid(pos);
            if (payload.isEmpty()) {
                // Bootstrap probe: replying to the DST lets the client read this relay's
                // endpoint (this datagram's sender) as its own local address.
                assoc->relay->writeDatagram(QByteArray(), dst, dport);
                continue;
            }
            Q_UNUSED(dst); // routing is by relay port, which is unique per association.
            for (int i = 0; i < m_udpAssocs.size(); ++i) {
                UdpAssoc *target = m_udpAssocs.at(i);
                if (target->relay->localPort() == dport && target->clientPort != 0) {
                    // Sender = this association's relay endpoint, which is what the peer
                    // knows the sender as; v4-mapped IPv6 to match its own dual-stack view.
                    const QByteArray out = QByteArray(3, char(0))
                        + encodeAddrPortV6(assoc->relay->localAddress(), assoc->relay->localPort())
                        + payload;
                    target->relay->writeDatagram(out, target->clientAddr, target->clientPort);
                    break;
                }
            }
        }
    }
};

#endif // SOCKS_SERVICE_H
