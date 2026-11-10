/****************************************************************************
**
** Copyright (C) 2015 The XD Company Ltd.
**
** This file is part of the test suite of the XD Toolkit.
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

#include "./service_base.h"

/*!
    SOCKS5 BIND: listen on an ephemeral port and tell the client which
    one via the first reply; on the first incoming connection send the
    second reply (its peer) and relay bytes over the same control socket.
    The peer reaches this port through the ordinary CONNECT path,
    so no special-casing of the listen port is needed.
*/
void TestService::startSocksBind(QTcpSocket *control, const QHostAddress &requested)
{
    const QSharedPointer<ServiceConfigDynamic > config = watcher()->load();
    if (config->bindError()) {
        control->write(socksReply(quint8(config->bindError()), control->localAddress(), 0));
        control->disconnectFromHost();
        return;
    }

    // The client asks for an IPv6 or an IPv4 address, never for both
    // families at once (SOCKS5 has no dual-stack bind); hence the
    // listener takes the loopback address of the family asked for,
    // unless the configuration asks for both families.
    QHostAddress address = control->localAddress();
    if (requested.protocol() == QAbstractSocket::IPv6Protocol) {
        address = QHostAddress::LocalHostIPv6;
    }
    if (config->dualStack()) {
        address = QHostAddress::Any;
    }

    // Listens as a child of the control connection, hence the listener goes
    // away with it.
    QTcpServer *server = new QTcpServer(control);
    if ( ! server->listen(address, 0)) {
        control->write(socksReply(0x01, address, 0));
        control->disconnectFromHost();
        return;
    }
    connect(server, &QTcpServer::newConnection,
            this, &TestService::onSocksBindConnection);
    control->write(socksReply(0x00, server->serverAddress(), server->serverPort()));
}

/*! Relays the first connection a SOCKS5 BIND listener accepts. */
void TestService::onSocksBindConnection()
{
    QTcpServer *server = qobject_cast<QTcpServer * >(sender());
    QTcpSocket *control = qobject_cast<QTcpSocket * >(server->parent());
    QTcpSocket *incoming = server->nextPendingConnection();

    // BIND relays a single connection, so stop listening.
    server->close();
    control->write(socksReply(0x00, incoming->peerAddress(), incoming->peerPort()));
    linkPeers(control, incoming);
    connect(incoming, &QTcpSocket::readyRead,
            this, &TestService::relayReadyRead);
    connect(control, &QTcpSocket::readyRead,
            this, &TestService::relayReadyRead);
    connect(incoming, &QTcpSocket::disconnected,
            this, &TestService::relayDisconnected);
    connect(control, &QTcpSocket::disconnected,
            this, &TestService::relayDisconnected);
    control->write(incoming->readAll());
}
