/// @file
/// TestService: the base for one protocol listener, bound in the process's own
/// event loop. Shared by main.cpp's protocol services and https_spdy.h.
///
/// Subclasses implement start() to bind their listener(s) through listenTcp()/
/// bindUdp(), which fall back to an ephemeral port when the wanted one is taken
/// or privileged, and record the port actually bound in port().
#ifndef SERVICE_BASE_H
#define SERVICE_BASE_H

#include <QtCore/QObject>
#include <QtCore/QByteArray>
#include <QtCore/QString>
#include <QtCore/QVariant>
#include <QtCore/QSharedPointer>
#include <QtNetwork/QHostAddress>
#include <QtNetwork/QTcpServer>
#include <QtNetwork/QTcpSocket>
#include <QtNetwork/QUdpSocket>

/// ServiceConfigDynamic + IniWatcher (the live server-config.ini).
#include "dynamic_config.h"

/// Absolute path of the --folder's server-config.ini, set once by main() before any service
/// is built. The shared IniWatcher watches this; empty when no --folder was given.
extern QString g_configIniPath;

/// ServiceConfig describes one listener; it is defined just below TestService, because it names
/// TestService::Kind. Every listener is built from a ServiceConfig via newService() (main.cpp).
struct ServiceConfig;

class TestService : public QObject
{
public:
    /// Which concrete listener to build. This kind-enum lives on the base that every listener
    /// inherits (not on ServiceConfig): a listener IS one of these kinds, and newService() switches
    /// on a config's kind to pick the subclass.
    enum Kind { Ftp, Echo, Http, Https, Socks, Dns, Imap, Imaps,
                PlainHttp, Daytime, ConnectProxy, SocksProxy };

    TestService(const QHostAddress &host, quint16 wantPort, QObject *parent = Q_NULLPTR)
        : QObject(parent), m_host(host), m_wantPort(wantPort), m_port(wantPort) { setupWatcher(); }
    /// Build straight from a ServiceConfig: the base takes host+port from it and each subclass
    /// reads whatever else it needs (folder, cert, auth, ...), so newService() passes the config
    /// as-is rather than splitting it into positional arguments. Defined after ServiceConfig below.
    TestService(const ServiceConfig &c, QObject *parent = Q_NULLPTR);
    virtual ~TestService() {}

    virtual void start() = 0;
    quint16 port() const { return m_port; }

    /// Fired (via the shared IniWatcher) whenever server-config.ini changes. Default: nothing.
    /// A service that reacts to the config overrides this and, if it wants the new values, calls
    /// watcher()->load() -- whether to reload at all is the override's own decision.
    virtual void onIniChange() {}

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
    /// FTP/textual-protocol reply line: "<code> <text>\r\n".
    static void reply(QTcpSocket *c, int code, const char *text)
    {
        c->write(QByteArray::number(code) + ' ' + text + "\r\n");
    }

    /// Generic byte relay, so per-connection handlers can be plain member functions
    /// rather than lambdas (MSVC 2010 cannot compile a lambda nested inside another
    /// lambda). Two sockets are linked with linkPeers(); each recovers its partner from a
    /// "peer" property, and the emitting socket from sender(). Optional "connectReply" /
    /// "errorReply" / "closeOnError" properties on an upstream socket drive the tunnel
    /// handshake replies. All are wired via the pointer-to-member connect() overload,
    /// which needs neither moc nor a lambda.
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

    /// The process-wide config watcher this service is bound to (set in the ctor). An override of
    /// onIniChange() typically refreshes its cached config with `... = watcher()->load()`.
    const QSharedPointer<IniWatcher> &watcher() const { return m_watcher; }

    QHostAddress m_host;
    quint16 m_wantPort;
    quint16 m_port;
    QSharedPointer<IniWatcher> m_watcher;

private:
    /// Bind to the shared IniWatcher and route its change signal to onIniChange(). Called by every
    /// TestService ctor, so the base "always creates the IniWatcher"; overrides decide what to do.
    void setupWatcher()
    {
        m_watcher = IniWatcher::shared(g_configIniPath);
        connect(m_watcher.data(), &IniWatcher::byIniChange, this, &TestService::onIniChange);
    }
};

/// One listener's configuration -- the single description every server-dummy listener is built
/// from. newService() (main.cpp) maps `kind` to the concrete subclass. The cert fields are
/// OPTIONAL: only the TLS kinds read them, and every non-SSL kind ignores them (empty certPem/
/// keyPem also means "use the process-wide g_serverCertPem/g_serverKeyPem from --cert/--key").
/// No default member initialisers: these helpers stay compilable under MSVC 2010.
struct ServiceConfig {
    TestService::Kind kind;
    QHostAddress host;
    quint16 port;
    /// Ftp / Https / PlainHttp; ignored by the rest.
    QString folder;
    /// Optional: TLS kinds only, silently ignored otherwise.
    QByteArray certPem;
    /// Optional: ditto.
    QByteArray keyPem;
    /// ConnectProxy / SocksProxy: demand credentials.
    bool requireAuth;
    /// Ftp: expose the flat --folder as /qtest (spdy fleet).
    bool qtestAtRoot;
    /// Ftp: run as an FTP proxy (USER "user@host" -> strip host, serve locally).
    bool ftpProxy;
};

/// host+port come from the config; subclasses pull their own fields in their initialiser lists.
inline TestService::TestService(const ServiceConfig &c, QObject *parent)
    : QObject(parent), m_host(c.host), m_wantPort(c.port), m_port(c.port) { setupWatcher(); }

#endif // SERVICE_BASE_H
