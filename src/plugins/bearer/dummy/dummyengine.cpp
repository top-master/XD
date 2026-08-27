/// @file
/// qbearerdummy: the engine-plugin half of the TestBearer / bearer-dummy pair. Loaded by
/// QNetworkConfigurationManager (QBearerEngineFactoryInterface, key "dummy"), it connects to the
/// bearer-dummy daemon named by $QT_BEARER_DUMMY_PORT, exposes the daemon's fake configurations as
/// QNetworkConfigurations, and drives QNetworkSession open/close through the daemon so activation is
/// real and shared across processes. With no daemon ($QT_BEARER_DUMMY_PORT unset/unreachable) the
/// engine simply publishes nothing, so ordinary runs are unaffected.

#include <QtNetwork/private/qbearerengine_p.h>
#include <QtNetwork/private/qbearerplugin_p.h>
#include <QtNetwork/private/qnetworksession_p.h>
#include <QtNetwork/private/qnetworkconfiguration_p.h>
#include <QtNetwork/QNetworkSession>
#include <QtNetwork/QNetworkInterface>
#include <QtNetwork/QTcpSocket>
#include <QtCore/QByteArray>
#include <QtCore/QList>
#include <QtCore/QTimer>
#include <QtCore/QVariantMap>

QT_BEGIN_NAMESPACE

class DummyEngine;

// MARK: session backend.
class DummySession : public QNetworkSessionPrivate
{
    Q_OBJECT
public:
    DummySession(DummyEngine *engine) : m_engine(engine), m_policies(0)
    {
        // Present the AutoCloseSessionTimeout property (default -1 = off) so the auto-close test has
        // an applicable configuration; setting it >= 0 arms a one-shot that releases this session.
        m_props.insert(QLL("AutoCloseSessionTimeout"), -1);
    }

    void syncStateWithInterface() Q_DECL_OVERRIDE;
#ifndef QT_NO_NETWORKINTERFACE
    /// QNetworkSession::interface() must be valid exactly when the session is Connected (a live
    /// route exists) and invalid otherwise. We have no real interface of our own, so hand back the
    /// host's first interface as a stand-in -- the tests only check isValid().
    QNetworkInterface currentInterface() const Q_DECL_OVERRIDE
    {
        if (!isOpen || state != QNetworkSession::Connected)
            return QNetworkInterface();
        const QList<QNetworkInterface> all = QNetworkInterface::allInterfaces();
        return all.isEmpty() ? QNetworkInterface() : all.first();
    }
#endif
    QVariant sessionProperty(const QString &key) const Q_DECL_OVERRIDE { return m_props.value(key); }
    void setSessionProperty(const QString &key, const QVariant &value) Q_DECL_OVERRIDE
    {
        m_props.insert(key, value);
        // Arm auto-close when the client requests it (0 = at the next poll, i.e. as soon as
        // possible). autoClose() then releases the session but leaves the interface up.
        if (key == QLL("AutoCloseSessionTimeout") && value.toInt() >= 0)
            QTimer::singleShot(value.toInt(), this, SLOT(autoClose()));
    }

    void open() Q_DECL_OVERRIDE;
    void close() Q_DECL_OVERRIDE;
    void stop() Q_DECL_OVERRIDE;

private slots:
    /// Release this session at the armed AutoCloseSessionTimeout: emit closed() and drop isOpen, but
    /// keep the interface Connected (auto-close ends the session's hold, not the interface), and
    /// reset the timeout to -1 (off).
    void autoClose()
    {
        if (!isOpen)
            return;
        isOpen = false;
        m_props.insert(QLL("AutoCloseSessionTimeout"), -1);
        emit closed();
    }

public:
    void migrate() Q_DECL_OVERRIDE {}
    void accept() Q_DECL_OVERRIDE {}
    void ignore() Q_DECL_OVERRIDE {}
    void reject() Q_DECL_OVERRIDE {}

    QString errorString() const Q_DECL_OVERRIDE { return QString(); }
    QNetworkSession::SessionError error() const Q_DECL_OVERRIDE { return QNetworkSession::UnknownSessionError; }

    quint64 bytesWritten() const Q_DECL_OVERRIDE { return 0; }
    quint64 bytesReceived() const Q_DECL_OVERRIDE { return 0; }
    quint64 activeTime() const Q_DECL_OVERRIDE { return 0; }

    QNetworkSession::UsagePolicies usagePolicies() const Q_DECL_OVERRIDE { return m_policies; }
    void setUsagePolicies(QNetworkSession::UsagePolicies p) Q_DECL_OVERRIDE
    {
        if (m_policies == p)
            return;
        m_policies = p;
        emit usagePoliciesChanged(m_policies);
    }

private:
    QByteArray configId() const { return publicConfig.identifier().toLatin1(); }
    DummyEngine *m_engine;
    QVariantMap m_props;
    QNetworkSession::UsagePolicies m_policies;
};

// MARK: engine.
class DummyEngine : public QBearerEngine
{
    Q_OBJECT
public:
    DummyEngine(QObject *parent = 0);

    bool hasIdentifier(const QString &id) Q_DECL_OVERRIDE
    { QMutexLocker locker(&mutex); return accessPointConfigurations.contains(id); }

    QNetworkConfigurationManager::Capabilities capabilities() const Q_DECL_OVERRIDE
    {
        // We can activate/deactivate our fake interfaces, and each config is a direct route.
        return QNetworkConfigurationManager::CanStartAndStopInterfaces
             | QNetworkConfigurationManager::DirectConnectionRouting;
    }

    QNetworkSessionPrivate *createSessionBackend() Q_DECL_OVERRIDE { return new DummySession(this); }

    QNetworkConfigurationPrivatePointer defaultConfiguration() Q_DECL_OVERRIDE
    {
        QMutexLocker locker(&mutex);
        // Prefer an Active configuration (a real default is a connected interface), so adding more
        // seed configs never changes which one is the default just because of QMap key ordering.
        QNetworkConfigurationPrivatePointer first;
        QHash<QString, QNetworkConfigurationPrivatePointer>::ConstIterator it;
        for (it = accessPointConfigurations.constBegin(); it != accessPointConfigurations.constEnd(); ++it) {
            if (!first)
                first = it.value();
            if ((it.value()->state & QNetworkConfiguration::Active) == QNetworkConfiguration::Active)
                return it.value();
        }
        return first;
    }

    bool requiresPolling() const Q_DECL_OVERRIDE { return false; }

    /// The manager drives these two by name (QMetaObject::invokeMethod) on the engine's own thread:
    /// initialize() once, with BlockingQueuedConnection, before it reads our configurations, and
    /// requestUpdate() whenever it wants a rescan. Both must exist as invokable methods or the
    /// manager blocks forever waiting for the updateCompleted() that a missing requestUpdate() can
    /// never emit. We connect to the daemon here (in the bearer thread) rather than in the ctor.
    Q_INVOKABLE void initialize();
    Q_INVOKABLE void requestUpdate() { emit updateCompleted(); }

    /// Relay an OPEN/CLOSE for a config id to the daemon; the resulting STATE push updates the config.
    /// Called from a QNetworkSession (the main thread), but m_sock lives in the engine's bearer
    /// thread -- so marshal the actual write onto that thread (a direct cross-thread write trips
    /// "QSocketNotifier: cannot be enabled/disabled from another thread" and is unsafe).
    void sendToDaemon(const QByteArray &verb, const QByteArray &id)
    { QMetaObject::invokeMethod(this, "doSend", Qt::QueuedConnection,
                                Q_ARG(QByteArray, verb), Q_ARG(QByteArray, id)); }

private slots:
    void onReadyRead();
    void doSend(const QByteArray &verb, const QByteArray &id)
    { if (m_sock && m_sock->state() == QAbstractSocket::ConnectedState) { m_sock->write(verb + ' ' + id + '\n'); m_sock->flush(); } }

private:
    void applyConfig(const QByteArray &id, int state, int bearer, const QByteArray &name);
    void applyState(const QByteArray &id, int state);

    QTcpSocket *m_sock;
    QByteArray m_buf;
};

// MARK: session impl.
void DummySession::syncStateWithInterface()
{
    // QNetworkConfiguration::StateFlag values are CUMULATIVE masks, not single bits
    // (Defined=0x2, Discovered=0x6, Active=0xe), so test with "== flag", never a bare "& flag"
    // (which would treat every Discovered config as Active).
    const QNetworkConfiguration::StateFlags cfgState = publicConfig.state();
    if ((cfgState & QNetworkConfiguration::Active) == QNetworkConfiguration::Active) {
        state = QNetworkSession::Connected;
        isOpen = true;
    } else if ((cfgState & QNetworkConfiguration::Discovered) == QNetworkConfiguration::Discovered) {
        state = QNetworkSession::Disconnected;
        isOpen = false;
    } else {
        state = QNetworkSession::NotAvailable;
        isOpen = false;
    }
}

void DummySession::open()
{
    if (isOpen)
        return;
    m_engine->sendToDaemon("OPEN", configId());
    // The daemon's STATE broadcast will flip the config to Active; reflect the session now so a
    // waitForOpened() returns promptly.
    isOpen = true;
    state = QNetworkSession::Connected;
    emit quitPendingWaitsForOpened();
    emit stateChanged(QNetworkSession::Connected);
    emit newConfigurationActivated();
}

void DummySession::close()
{
    if (!isOpen)
        return;
    m_engine->sendToDaemon("CLOSE", configId());
    isOpen = false;
    state = QNetworkSession::Disconnected;
    emit stateChanged(QNetworkSession::Disconnected);
    emit closed();
}

void DummySession::stop()
{
    close();
}

// MARK: engine impl.
DummyEngine::DummyEngine(QObject *parent)
    : QBearerEngine(parent), m_sock(0)
{
    // Deliberately light: the daemon connection is made in initialize(), which the manager calls on
    // this engine's own (bearer) thread, so the socket and its notifications live there.
}

void DummyEngine::initialize()
{
    const QByteArray portStr = qgetenv("QT_BEARER_DUMMY_PORT");
    if (portStr.isEmpty()) {
        emit updateCompleted(); // no daemon -> publish nothing (ordinary runs are unaffected)
        return;
    }
    m_sock = new QTcpSocket(this);
    connect(m_sock, &QTcpSocket::readyRead, this, &DummyEngine::onReadyRead);
    m_sock->connectToHost(QHostAddress::LocalHost, quint16(portStr.toUShort()));
    if (m_sock->waitForConnected(3000)) {
        // Read the initial CONFIG.../READY snapshot synchronously so the manager sees our
        // configurations as soon as initialize() returns.
        while (m_sock->state() == QAbstractSocket::ConnectedState) {
            if (m_buf.contains("READY\n"))
                break;
            if (!m_sock->waitForReadyRead(3000))
                break;
            m_buf += m_sock->readAll();
        }
        onReadyRead();
    }
    emit updateCompleted();
}

void DummyEngine::onReadyRead()
{
    if (m_sock)
        m_buf += m_sock->readAll();
    int nl;
    while ((nl = m_buf.indexOf('\n')) >= 0) {
        const QByteArray line = m_buf.left(nl);
        m_buf.remove(0, nl + 1);
        if (line == "READY")
            continue;
        const QList<QByteArray> p = line.split(' ');
        if (p.value(0) == "CONFIG" && p.size() >= 5) {
            const QByteArray name = line.mid(line.indexOf(' ', line.indexOf(' ', line.indexOf(' ', line.indexOf(' ') + 1) + 1) + 1) + 1);
            applyConfig(p.at(1), p.at(2).toInt(), p.at(3).toInt(), name);
        } else if (p.value(0) == "STATE" && p.size() >= 3) {
            applyState(p.at(1), p.at(2).toInt());
        }
    }
}

void DummyEngine::applyConfig(const QByteArray &id, int st, int bearer, const QByteArray &name)
{
    const QString sid = QString::fromLatin1(id);
    QMutexLocker locker(&mutex);
    QNetworkConfigurationPrivatePointer ptr = accessPointConfigurations.value(sid);
    const bool isNew = !ptr;
    if (isNew) {
        ptr = QNetworkConfigurationPrivatePointer(new QNetworkConfigurationPrivate);
        ptr->id = sid;
    }
    {
        QMutexLocker cfgLock(&ptr->mutex);
        ptr->name = QString::fromUtf8(name);
        ptr->isValid = true;
        ptr->state = QNetworkConfiguration::StateFlags(st);
        ptr->type = QNetworkConfiguration::InternetAccessPoint;
        ptr->purpose = QNetworkConfiguration::PublicPurpose;
        ptr->bearerType = QNetworkConfiguration::BearerType(bearer);
    }
    if (isNew) {
        accessPointConfigurations.insert(sid, ptr);
        locker.unlock();
        emit configurationAdded(ptr);
    } else {
        locker.unlock();
        emit configurationChanged(ptr);
    }
}

void DummyEngine::applyState(const QByteArray &id, int st)
{
    const QString sid = QString::fromLatin1(id);
    QMutexLocker locker(&mutex);
    QNetworkConfigurationPrivatePointer ptr = accessPointConfigurations.value(sid);
    if (!ptr)
        return;
    {
        QMutexLocker cfgLock(&ptr->mutex);
        ptr->state = QNetworkConfiguration::StateFlags(st);
    }
    locker.unlock();
    emit configurationChanged(ptr);
}

// MARK: plugin.
class DummyBearerPlugin : public QBearerEnginePlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "org.qt-project.Qt.QBearerEngineFactoryInterface" FILE "dummy.json")
public:
    QBearerEngine *create(const QString &key) const Q_DECL_OVERRIDE
    { return key == QLL("dummy") ? new DummyEngine : 0; }
};

QT_END_NAMESPACE

#include "dummyengine.moc"
