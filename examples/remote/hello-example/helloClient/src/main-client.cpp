// See header usage and styling guide in XD's `include/README.md` file.

#include "client.h"

#include <QtCore/QCoreApplication>
#include <QtCore/QStringList>
#include <QtCore/QDebug>
#include <QtNetwork/QTcpSocket>


// Uses compile-time string join/concat feature.
#define EMPTY_LINE "\r\n"

const int MY_SERVER_PORT = 8081;
const int MY_CONNECTION_TIMEOUT = 30000;

int main(int argc, char** argv) {
    QCoreApplication app(argc, argv);
    qDebug() << "# ------- Client launched" EMPTY_LINE;

    // Validates arguments.
    QString hostAddress;
    QString commandOrName;
    QStringList args = app.arguments();
    if (args.count() == 1) {
        qDebug() << "# ------- No command-line arguments defaults to quitting server." EMPTY_LINE;
        // Sample args.
        hostAddress = QLL("localhost");
        commandOrName = QLL("-q");
    } else if (args.count() != 3) {
        qDebug() << "Usage:\n\t" << args[0] << "server-IP-address some-name";
        return 1;
   } else {
        hostAddress = args[1];
        commandOrName = args[2];
    }

    //
    // First, raw internet-protocol connection.
    //

    QTcpSocket *socket = new QTcpSocket();
    // Not needed yet, but do anyway because *could* later be useless, if
    // the `disconnected` is emitted before we call `QObject::connect`.
    QObject::connect(socket, &QTcpSocket::disconnected,
                     qApp, &QCoreApplication::quit);
    socket->connectToHost(hostAddress, MY_SERVER_PORT);
    if ( ! socket->waitForConnected(MY_CONNECTION_TIMEOUT)) {
        // Keeping logs verbose (is an example).
        QDebug dbg = qDebug();
        dbg << "Failed to connect from client to server for host:";
        dbg.noquote().nospace() << hostAddress << ':' << MY_SERVER_PORT;
        dbg.flush();
        dbg << socket->errorString();
        dbg.flush();
        dbg << "\nNOTE: ensure to run server before running client.";

        delete socket;
        return 1;
   }

    //
    // Finally, see `Client` class's logic for QRemote connection.
    //

    Client client(socket);
    client.waitForController(3000);
    if (commandOrName == "-q") {
        qDebug(EMPTY_LINE "# ------- Client started calling quit:");
        QRemoteTimeLimiter _(client.helloService(), -1);
        client.helloService()->quit();
        qDebug("# ------- Client finished calling quit." EMPTY_LINE);
    } else {
        qDebug(EMPTY_LINE "# ------- Client started calling setName:");
        client.helloService()->setName(commandOrName);
        qDebug("# ------- Client finished calling setName." EMPTY_LINE);
    }

    return app.exec();
}
