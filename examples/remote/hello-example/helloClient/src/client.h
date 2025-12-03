
#ifndef EXAMPLE_CLIENT_H
#define EXAMPLE_CLIENT_H

#include <QtCore/QObject>
#include <QtCore/QString>

#include "../helloServer/src/helloservice_remote.h"


class QTcpSocket;

class Client : public QObject {
    Q_OBJECT
public:
    /// Registers new `QRemoteUser` on given @p socket.
    ///
    /// Note that both client and server are just "users" of services,
    /// and server can remote-control any of client's services
    /// (if client allows by manually registering any, but this example doesn't).
    ///
    /// @param socket Connection to server. WARNING:
    /// Client takes ownership of @p socket
    /// (hence caller does not need to `delete`).
    ///
    /// \sa QRemoteUser::registerLocal(...)
    ///
    explicit Client(QTcpSocket *socket, QObject *parent = Q_NULLPTR);

    virtual ~Client();

    void waitForController(int timeout);

    inline my_lib::HelloServiceRemote *helloService() { return m_controller.data(); }

signals:
    void onReady();

private slots:
    void onNewController(const QRef<QObjectRemote> &);
    void onLostController(const QRef<QObjectRemote> &);

    void onHello(const QString &greetings);

private:
    QTcpSocket *m_socket;
    QRef<my_lib::HelloServiceRemote> m_controller;
    volatile bool m_isControlReady;
};


#endif // EXAMPLE_CLIENT_H
