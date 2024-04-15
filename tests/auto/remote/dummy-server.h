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

#ifndef DUMMY_SERVER_H
#define DUMMY_SERVER_H

#include <QtCore/QCoreApplication>
#include <QtCore/QElapsedTimer>
#include <QtCore/QThread>
#include <QtNetwork/QTcpServer>
#include <QtNetwork/QTcpSocket>
#include <QtRemote/QObjectRemote>
#include <QtRemote/QRemoteUser>

class ServerService : public QObject {
    Q_REMOTE
public:
    inline ServerService()
        : m_isSecretCalled(false)
    {}

    inline bool isSecretCalled() const { return m_isSecretCalled; }

signals:
    void myHelloSignal(const QString &message);

private slots:
    inline void onSecretCallable() {
        m_isSecretCalled = true;
        myHelloSignal("Secret revealed");
    }

private:
    bool m_isSecretCalled;
};

// Should be included after Service's header, but
// in single-header context, simply after declaration:
#include "dummy-server_remote.h"

class ServerConnection : public QObject {
    Q_OBJECT
public:
    inline ServerConnection(QTcpSocket *socket, QObject *parent)
        : QObject(parent)
    {
        connect(socket, &QTcpSocket::disconnected,
                this, &ServerConnection::onDisconnect);

        QRemoteUser *session = new QRemoteUser(this);
        session->setObjectName(QLL("my-unique-server-id"));

        service = QRef<ServerService>(new ServerService());
        service->remote().connect(session);

        session->addDevice(socket);
        socket->setParent(session);
        connect(socket, &QTcpSocket::disconnected,
                this, &ServerConnection::onDisconnect);
    }

    QRef<ServerService> service;

private slots:
    inline void onDisconnect() {
        QTcpSocket *socket = qobject_cast<QTcpSocket *>(this->sender());
        if (socket) {
            qDebug("ServerConnection: socket disconnected.");
            this->deleteLater();
        }
    }
};

class ServerThread : public QThread {
    Q_OBJECT

public:
    enum {
        PortDefault = 8081,
        VerboseDelete = 0
    };

    inline ServerThread(QObject *parent = Q_NULLPTR, int portArg = PortDefault)
        : m_port(portArg)
        , m_tcpServer(Q_NULLPTR)
        , m_isStarted(false)
        , m_autoDelete(true)
    {
        // First of all, connect things we want to happen in calling-thread
        // (just to be sure, else `moveToThread(...)` takes effect later).
        QObject::connect(this, &QThread::finished, this, &ServerThread::onFinished);
        // Below needs to occur before `start()`,
        // else `onNewConnection(...)` gets called from calling-thread.
        this->moveToThread(this);
        // Needs to happen after `moveToThread(...)`.
        this->setParent(parent);
    }

    inline ~ServerThread() {
        if (VerboseDelete) qDebug("ServerThread: deleting.");
        this->wait(3000);
        if (VerboseDelete) qDebug("ServerThread: deleted.");
    }

    inline void run() Q_DECL_OVERRIDE {
        QT_FINALLY([&] {
            if (VerboseDelete) qDebug("ServerThread: loop quit.");
            disconnect(m_tcpServer, &QTcpServer::newConnection,
                       this, &ServerThread::onNewConnection);
            Q_ASSERT_X(qApp, "Sub-Thread", "Keep main-thread alive until sub-threads finish.");
            // Allows destructor to wait for self.
            this->moveToThread(qApp->thread());
            // Since `moveToThread(...)` happens later,
            // the `deleteLater()` would be queued to happen in this same thread,
            // hence `m_autoDelete` is handled in `onFinished()`.
        });

        m_tcpServer = new QTcpServer(this);
        connect(m_tcpServer, &QTcpServer::newConnection,
                this, &ServerThread::onNewConnection);
        m_tcpServer->listen(QHostAddress::LocalHost, m_port);
        m_isStarted = true;

        // Event-loop till `quit()` called.
        this->exec();
    }

    inline bool isStarted() const { return static_cast<bool>(m_isStarted); }
    inline void waitForStarted(int timeout) {
        QElapsedTimer timer;
        timer.start();
        do {
            QCoreApplication::processEvents(QEventLoop::AllEvents);
        } while ( ! m_isStarted && timer.timeLeft(timeout));
    }

    inline QTcpServer *raw() { return m_tcpServer; }

    inline bool isAutoDeletable() const { return m_autoDelete; }
    inline void setAutoDeleteEnabled(bool state) { m_autoDelete = state; }

private slots:
    inline void onFinished() {
        Q_ASSERT(QThread::currentThread() != this);
        if (m_autoDelete) {
            this->deleteLater();
        }
    }

    inline void onNewConnection() {
        QTcpSocket *socket = m_tcpServer->nextPendingConnection();
        if (socket == Q_NULLPTR) {
            return;
        }

        ServerConnection *manager = new ServerConnection(socket, this);
        clients.append(manager);
    }

private:
    int m_port;
    QTcpServer *m_tcpServer;
    volatile bool m_isStarted;
    bool m_autoDelete;

public:
    QList<ServerConnection *> clients;
};

#endif // DUMMY_SERVER_H
