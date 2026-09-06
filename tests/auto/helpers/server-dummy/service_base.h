// ---------------------------------------------------------------------------
// TestService: the base for one protocol listener, bound in the process's own
// event loop. Shared by main.cpp's protocol services and https_spdy.h.
//
// Subclasses implement start() to bind their listener(s) through listenTcp()/
// bindUdp(), which fall back to an ephemeral port when the wanted one is taken
// or privileged, and record the port actually bound in port().
// ---------------------------------------------------------------------------
#ifndef SERVICE_BASE_H
#define SERVICE_BASE_H

#include <QtCore/QObject>
#include <QtCore/QVariant>
#include <QtNetwork/QHostAddress>
#include <QtNetwork/QTcpServer>
#include <QtNetwork/QTcpSocket>
#include <QtNetwork/QUdpSocket>

class TestService : public QObject
{
public:
    TestService(const QHostAddress &host, quint16 wantPort, QObject *parent = Q_NULLPTR)
        : QObject(parent), m_host(host), m_wantPort(wantPort), m_port(wantPort) {}
    virtual ~TestService() {}

    virtual void start() = 0;
    quint16 port() const { return m_port; }

protected:
    quint16 listenTcp(QTcpServer *s)
    {
        if (!s->listen(m_host, m_wantPort))
            s->listen(m_host, 0);
        m_port = s->serverPort();
        return m_port;
    }
    quint16 bindUdp(QUdpSocket *u)
    {
        if (!u->bind(m_host, m_wantPort))
            u->bind(m_host, 0);
        m_port = u->localPort();
        return m_port;
    }
    // FTP/textual-protocol reply line: "<code> <text>\r\n".
    static void reply(QTcpSocket *c, int code, const char *text)
    {
        c->write(QByteArray::number(code) + ' ' + text + "\r\n");
    }

    // --- Generic byte relay, so per-connection handlers can be plain member functions
    // rather than lambdas (MSVC 2010 cannot compile a lambda nested inside another
    // lambda). Two sockets are linked with linkPeers(); each recovers its partner from a
    // "peer" property, and the emitting socket from sender(). Optional "connectReply" /
    // "errorReply" / "closeOnError" properties on an upstream socket drive the tunnel
    // handshake replies. All are wired via the pointer-to-member connect() overload,
    // which needs neither moc nor a lambda.
    static void linkPeers(QObject *a, QObject *b)
    {
        a->setProperty("peer", QVariant::fromValue<QObject *>(b));
        b->setProperty("peer", QVariant::fromValue<QObject *>(a));
    }
    static QTcpSocket *peerOf(const QObject *s)
    {
        return s ? qobject_cast<QTcpSocket *>(s->property("peer").value<QObject *>()) : Q_NULLPTR;
    }

    void relayReadyRead()
    {
        QTcpSocket *s = qobject_cast<QTcpSocket *>(sender());
        QTcpSocket *peer = peerOf(s);
        if (s && peer)
            peer->write(s->readAll());
    }
    void relayDisconnected()
    {
        QTcpSocket *s = qobject_cast<QTcpSocket *>(sender());
        QTcpSocket *peer = peerOf(s);
        if (s && peer) {
            // Flush any final bytes, then close so the peer sees EOF.
            peer->write(s->readAll());
            peer->disconnectFromHost();
        }
    }
    void onTunnelConnected()
    {
        QTcpSocket *up = qobject_cast<QTcpSocket *>(sender());
        if (!up)
            return;
        up->setProperty("up", true);
        if (QTcpSocket *client = peerOf(up))
            client->write(up->property("connectReply").toByteArray());
    }
    void onTunnelError()
    {
        QTcpSocket *up = qobject_cast<QTcpSocket *>(sender());
        // A close once relaying has begun is normal, not a handshake failure.
        if (!up || up->property("up").toBool())
            return;
        if (QTcpSocket *client = peerOf(up)) {
            client->write(up->property("errorReply").toByteArray());
            if (up->property("closeOnError").toBool())
                client->disconnectFromHost();
        }
    }

    QHostAddress m_host;
    quint16 m_wantPort;
    quint16 m_port;
};

#endif // SERVICE_BASE_H
