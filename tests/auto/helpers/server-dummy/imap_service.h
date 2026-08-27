#ifndef IMAP_SERVICE_H
#define IMAP_SERVICE_H

#include "service_base.h"

#include <QtCore/QByteArray>

// ---------------------------------------------------------------------------
// ImapService: an IMAP-ish greeter. Enough for the socket-engine tests, which only
// drive the greeting + NOOP + LOGOUT and echo the client's command tag.
// ---------------------------------------------------------------------------

class ImapService : public TestService
{
public:
    ImapService(const ServiceConfig &c, QObject *parent = Q_NULLPTR)
        : TestService(c, parent) {}

    void start() Q_DECL_OVERRIDE
    {
        QTcpServer *server = new QTcpServer(this);
        listenTcp(server);
        connect(server, &QTcpServer::newConnection, this, &ImapService::onNewConnection);
    }

    void onNewConnection()
    {
        QTcpSocket *c = qobject_cast<QTcpServer *>(sender())->nextPendingConnection();
        connect(c, &QTcpSocket::readyRead, this, &ImapService::onReadyRead);
        connect(c, &QTcpSocket::disconnected, c, &QObject::deleteLater);
        // Greeting: must start with "* OK " and end with "server ready\r\n".
        c->write("* OK [CAPABILITY IMAP4rev1] server-dummy IMAP server ready\r\n");
    }
    void onReadyRead() { onImap(qobject_cast<QTcpSocket *>(sender())); }

private:
    static void onImap(QTcpSocket *c)
    {
        while (c->canReadLine()) {
            const QByteArray line = c->readLine().trimmed();
            const int sp = line.indexOf(' ');
            const QByteArray tag = sp < 0 ? line : line.left(sp);
            const QByteArray cmd = (sp < 0 ? QByteArray() : line.mid(sp + 1)).toUpper();
            if (cmd == "LOGOUT") {
                // "* BYE LOGOUT received\r\n<tag> OK Completed\r\n", then close, so the
                // client observes RemoteHostClosedError.
                c->write("* BYE LOGOUT received\r\n" + tag + " OK Completed\r\n");
                c->flush();
                c->disconnectFromHost();
            } else {
                // NOOP and anything else: "<tag> OK Completed\r\n".
                c->write(tag + " OK Completed\r\n");
            }
        }
    }
};

#endif // IMAP_SERVICE_H
