/// @file
/// bearer-dummy: a tiny controllable "network bearer" backend, the daemon half of the
/// TestBearer / bearer-dummy pair (mirroring TestServer / server-dummy). Qt's bearer
/// configurations normally come from an OS-specific plugin (NetworkManager, generic, ...);
/// none of those load into the Fil-C libQt5Network here, so a headless run has NO network
/// configurations and every bearer test skips or a session test aborts. This daemon owns a
/// small set of fake QNetworkConfigurations and their active/inactive state, shared across
/// processes over a loopback TCP line protocol, so the qbearerdummy engine plugin (loaded in
/// each Qt process) exposes them and QNetworkSession open/close actually activates them --
/// including tst_QNetworkSession::outOfProcessSession, where a lackey subprocess activates a
/// config and the parent process must observe it go Active.
///
/// Line protocol (one message per line, '\n'-terminated):
///   server->client on connect, one per config then a READY sentinel:
///       CONFIG <id> <stateInt> <bearerTypeInt> <name...>\n
///       READY\n
///   client->server:  OPEN <id>\n    CLOSE <id>\n
///   server->all clients on any state change:  STATE <id> <stateInt>\n
/// State ints are QNetworkConfiguration::StateFlags: Discovered=6, Active=14.

#include <QtCore/QCoreApplication>
#include <QtCore/QByteArray>
#include <QtCore/QList>
#include <QtCore/QMap>
#include <QtCore/QStringList>
#include <QtNetwork/QTcpServer>
#include <QtNetwork/QTcpSocket>
#include <cstdio>

static const int kDiscovered = 6;
static const int kActive = 14;

struct Cfg {
    QByteArray id;
    QByteArray name;
    int state;
    /// QNetworkConfiguration::BearerType.
    int bearer;
};

class BearerDummy : public QObject
{
    Q_OBJECT
public:
    BearerDummy(quint16 port, QObject *parent = 0) : QObject(parent)
    {
        // Two fake configurations: one already Discovered-and-Active (so a default session can
        // open immediately) and one merely Discovered (so open() transitions it to Active).
        seed("bearer-dummy-eth", "Dummy Ethernet", kActive, 1 /*BearerEthernet*/);
        seed("bearer-dummy-wlan", "Dummy WLAN", kDiscovered, 2 /*BearerWLAN*/);
        // Bearer2G ("2G"): the cellular type that is both in the suitableConfiguration() cellular
        // filter AND in sessionProperties()'s validBearerNames (that list spells the 3G/4G/LTE/EVDO
        // names with a stray "Bearer" prefix, so only 2G/CDMA2000/WCDMA/HSPA actually match there).
        seed("bearer-dummy-cell", "Dummy Cellular", kDiscovered, 3 /*Bearer2G*/);

        connect(&m_server, &QTcpServer::newConnection, this, &BearerDummy::onNewConnection);
        m_server.listen(QHostAddress::LocalHost, port);
        std::printf("READY %u\n", unsigned(m_server.serverPort()));
        std::fflush(stdout);
    }

private slots:
    void onNewConnection()
    {
        while (m_server.hasPendingConnections()) {
            QTcpSocket *c = m_server.nextPendingConnection();
            m_clients.append(c);
            connect(c, &QTcpSocket::readyRead, this, &BearerDummy::onReadyRead);
            connect(c, &QTcpSocket::disconnected, this, &BearerDummy::onDisconnected);
            foreach (const Cfg &cfg, m_configs)
                c->write("CONFIG " + cfg.id + ' ' + QByteArray::number(cfg.state) + ' '
                         + QByteArray::number(cfg.bearer) + ' ' + cfg.name + '\n');
            c->write("READY\n");
        }
    }

    void onReadyRead()
    {
        QTcpSocket *c = qobject_cast<QTcpSocket *>(sender());
        m_buf[c] += c->readAll();
        int nl;
        while ((nl = m_buf[c].indexOf('\n')) >= 0) {
            const QByteArray line = m_buf[c].left(nl).trimmed();
            m_buf[c].remove(0, nl + 1);
            const QList<QByteArray> parts = line.split(' ');
            if (parts.size() < 2)
                continue;
            const QByteArray verb = parts.at(0).toUpper();
            const QByteArray id = parts.at(1);
            if (!m_configs.contains(id))
                continue;
            // StateFlags are cumulative masks (Discovered=0x6, Active=0xe), not OR-able bits, so set
            // the whole level: OPEN -> Active, CLOSE -> back to Discovered (never bit-clear, which
            // would yield 0x0 and drop the config out of "discovered" entirely).
            if (verb == "OPEN")
                setState(id, kActive);
            else if (verb == "CLOSE")
                setState(id, kDiscovered);
        }
    }

    void onDisconnected()
    {
        QTcpSocket *c = qobject_cast<QTcpSocket *>(sender());
        m_clients.removeAll(c);
        m_buf.remove(c);
        c->deleteLater();
    }

private:
    void seed(const char *id, const char *name, int state, int bearer)
    {
        Cfg cfg; cfg.id = id; cfg.name = name; cfg.state = state; cfg.bearer = bearer;
        m_configs.insert(cfg.id, cfg);
    }

    void setState(const QByteArray &id, int state)
    {
        if (m_configs[id].state == state)
            return;
        m_configs[id].state = state;
        const QByteArray msg = "STATE " + id + ' ' + QByteArray::number(state) + '\n';
        foreach (QTcpSocket *c, m_clients)
            c->write(msg);
    }

    QTcpServer m_server;
    QList<QTcpSocket *> m_clients;
    QMap<QTcpSocket *, QByteArray> m_buf;
    QMap<QByteArray, Cfg> m_configs;
};

int main(int argc, char **argv)
{
    QCoreApplication app(argc, argv);
    // Optional positional port (0 = pick a free one, reported on the READY line).
    quint16 port = 0;
    if (argc > 1)
        port = quint16(QByteArray(argv[1]).toUShort());
    BearerDummy dummy(port);
    return app.exec();
}

#include "main.moc"
