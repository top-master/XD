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

#ifndef DUMMY_CLIENT_H
#define DUMMY_CLIENT_H

#include "dummy-server.h"

#include <QtCore/QPointer>


class ClientThread : public QThread {
    Q_OBJECT
public:

    enum {
        TimeoutDefault = 3000
    };

    inline ClientThread(int portArg = ServerThread::PortDefault)
        : m_port(portArg)
        , m_timeout(TimeoutDefault)
        , m_socket(Q_NULLPTR)
        , m_isControlReady(false)
        , m_autoDelete(true)
        , m_signalArg(QLL("Nothing set yet."))
    {
        // First of all, connect things we want to happen in calling-thread
        // (just to be sure, else `moveToThread(...)` takes effect later).
        QObject::connect(this, &QThread::finished, this, &ClientThread::onFinished);
        // Below needs to occur before `start()`.
        this->moveToThread(this);
    }

    inline ~ClientThread() {
        if (ServerThread::VerboseDelete) qDebug("ClientThread: deleting.");
        this->wait(3000);
        if (ServerThread::VerboseDelete) qDebug("ClientThread: deleted.");
    }

    inline void run() Q_DECL_OVERRIDE {
        QT_FINALLY([&] {
            Q_ASSERT_X(qApp, "Sub-Thread", "Keep main-thread alive until sub-threads finish.");
            // Allows destructor to wait for self.
            this->moveToThread(qApp->thread());
        });

        QScopedPointer<QTcpSocket> socket;
        socket.reset(new QTcpSocket());
        QObject::connect(socket.data(), &QTcpSocket::disconnected,
                         this, &ClientThread::onDisconnect);
        socket->connectToHost(QHostAddress::LocalHost, m_port);
        if ( ! socket->waitForConnected(m_timeout)) {
            qWarning("ClientThread: connection timeout.");
            return;
        }

        QRemoteUser *user = new QRemoteUser(this);
        user->setObjectName(QLL("my-unique-client-id"));
        connect(user, &QRemoteUser::newConnection,
                this, &ClientThread::onNewController);
        connect(user, &QRemoteUser::disconnected,
                this, &ClientThread::onLostController);

        // Needs to be after QRemoteUser's parent is set,
        // else QIODevice may get destroyed before QRemoteUser.
        m_socket = socket.take();
        m_socket->setParent(this);
        // Needs to be after signal connections (to prevent signal loss).
        user->addDevice(m_socket);

        // Event-loop till `quit()` called.
        this->exec();
        if (ServerThread::VerboseDelete) qDebug("ClientThread: loop quit.");
    }

    inline bool isReady() const { return m_isControlReady; }

    inline bool isAutoDeletable() const { return m_autoDelete; }
    inline void setAutoDeleteEnabled(bool state) { m_autoDelete = state; }

    inline const QRef<ServerServiceRemote> &controller() const { return m_controller; }
    inline QRef<ServerServiceRemote> &controller() { return m_controller; }

    inline void waitForController(int timeout) {
        QElapsedTimer timer;
        timer.start();
        do {
            QCoreApplication::processEvents(QEventLoop::AllEvents);
        } while ( ! m_isControlReady && timer.timeLeft(timeout));
    }

    inline void waitForController() {
        return waitForController(m_timeout);
    }

private slots:
    inline void onFinished() {
        Q_ASSERT(QThread::currentThread() != this);
        if (m_autoDelete) {
            this->deleteLater();
        }
    }

    inline void onDisconnect() {
        qDebug() << "ClientThread.onDisconnect.";
        // Real client would retry around here, but test does:
        this->requestInterruption();
    }

    inline void onNewController(const QRef<QObjectRemote> &obj)
    {
        QSharedPointer<ServerServiceRemote> remote =
                obj.objectCast<ServerServiceRemote>();
        if (remote) {
            m_controller = remote;
            connect(m_controller.data(), &ServerServiceRemote::myHelloSignal,
                    this, &ClientThread::onServerSignal);

            m_isControlReady = true;
            emit onReady();
        }
    }

    inline void onLostController(const QRef<QObjectRemote> &obj)
    {
        if (obj.data() == m_controller.data()) {
            m_isControlReady = false;
        }
    }

    inline void onServerSignal(const QString &arg) {
        m_signalArg = arg;
    }

    inline const QString &lastSignalArg() { return m_signalArg; }

signals:
    void onReady();

protected:
    int m_port;
    int m_timeout;

    QTcpSocket *m_socket;
    QRef<ServerServiceRemote> m_controller;
    volatile bool m_isControlReady;
    bool m_autoDelete;

    QString m_signalArg;
};

class ServerClientPair {
public:
    inline ServerClientPair()
    {
        server = new ServerThread();
        client = new ClientThread();
    }

    inline void waitForDeleted(int timeout) {
        QElapsedTimer timer;
        timer.start();
        do {
            QCoreApplication::processEvents(QEventLoop::AllEvents);
        } while (  (server || client) && timer.timeLeft(timeout));
    }

    inline void quit(int timeout = 0) {
        this->server->quit();
        this->client->quit();
        if (timeout) {
            this->waitForDeleted(timeout);
        }
    }

public:
    QPointer<ServerThread> server;
    QPointer<ClientThread> client;
};

#endif // DUMMY_CLIENT_H
