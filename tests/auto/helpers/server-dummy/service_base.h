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

    QHostAddress m_host;
    quint16 m_wantPort;
    quint16 m_port;
};

#endif // SERVICE_BASE_H
