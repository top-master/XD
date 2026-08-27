/// @file
/// qbearerdummy: the engine-plugin half of the TestBearer / bearer-dummy pair. Loaded by
/// QNetworkConfigurationManager (QBearerEngineFactoryInterface, key "dummy"), it connects to the
/// bearer-dummy daemon named by $QT_BEARER_DUMMY_PORT, exposes the daemon's fake configurations as
/// QNetworkConfigurations, and drives QNetworkSession open/close through the daemon so activation is
/// real and shared across processes. With no daemon ($QT_BEARER_DUMMY_PORT unset/unreachable) the
/// engine simply publishes nothing, so ordinary runs are unaffected.
/// A `bearer-config.ini` fixture sets the access points and how their sessions behave.

#include <QtNetwork/private/qbearerengine_p.h>
#include <QtNetwork/private/qbearerplugin_p.h>
#include <QtNetwork/private/qnetworksession_p.h>
#include <QtNetwork/private/qnetworkconfiguration_p.h>
#include <QtNetwork/QNetworkConfigurationManager>
#include <QtNetwork/QNetworkSession>
#include <QtNetwork/QNetworkInterface>
#include <QtNetwork/QTcpSocket>
#include <QtCore/QByteArray>
#include <QtCore/QList>
#include <QtCore/QMap>
#include <QtCore/QStringList>
#include <QtCore/QTimer>
#include <QtCore/QVariantMap>

#include "./bearerconfig.h"

QT_BEGIN_NAMESPACE

class DummyEngine;

// MARK: session backend.
class DummySession : public QNetworkSessionPrivate
{
    Q_OBJECT
public:
    DummySession(DummyEngine *engine)
        : m_engine(engine)
        , m_policies(0)
        , m_opened(false)
        , m_awaitingActive(false)
        , m_error(QNetworkSession::UnknownSessionError)
    {
        // Present the AutoCloseSessionTimeout property (default -1 = off) so the auto-close test has
        // an applicable configuration; setting it >= 0 arms a one-shot that releases this session.
        m_props.insert(QLL("AutoCloseSessionTimeout"), -1);
    }

    void syncStateWithInterface() Q_DECL_OVERRIDE;
#ifndef QT_NO_NETWORKINTERFACE
    QNetworkInterface currentInterface() const Q_DECL_OVERRIDE;
#endif
    QVariant sessionProperty(const QString &key) const Q_DECL_OVERRIDE { return m_props.value(key); }
    void setSessionProperty(const QString &key,
                            const QVariant &value) Q_DECL_OVERRIDE
    {
        m_props.insert(key, value);
        // Arm auto-close when the client requests it (0 = at the next poll, i.e. as soon as
        // possible). autoClose() then releases the session but leaves the interface up.
        if (key == QLL("AutoCloseSessionTimeout") && value.toInt() >= 0) {
            QTimer::singleShot(value.toInt(), this, SLOT(autoClose()));
        }
    }

    void open() Q_DECL_OVERRIDE;
    void close() Q_DECL_OVERRIDE;
    void stop() Q_DECL_OVERRIDE;

private slots:
    void onConfigurationChanged(QNetworkConfigurationPrivatePointer config);
    void activate();

    /// Release this session at the armed AutoCloseSessionTimeout: emit closed() and drop isOpen, but
    /// keep the interface Connected (auto-close ends the session's hold, not the interface), and
    /// reset the timeout to -1 (off).
    void autoClose()
    {
        if ( ! isOpen) {
            return;
        }
        isOpen = false;
        m_opened = false;
        m_props.insert(QLL("AutoCloseSessionTimeout"), -1);
        emit closed();
    }

public:
    void migrate() Q_DECL_OVERRIDE {}
    void accept() Q_DECL_OVERRIDE {}
    void ignore() Q_DECL_OVERRIDE {}
    void reject() Q_DECL_OVERRIDE {}

    QString errorString() const Q_DECL_OVERRIDE { return m_errorString; }
    QNetworkSession::SessionError error() const Q_DECL_OVERRIDE { return m_error; }

    quint64 bytesWritten() const Q_DECL_OVERRIDE { return 0; }
    quint64 bytesReceived() const Q_DECL_OVERRIDE { return 0; }
    quint64 activeTime() const Q_DECL_OVERRIDE { return 0; }

    QNetworkSession::UsagePolicies usagePolicies() const Q_DECL_OVERRIDE { return m_policies; }
    void setUsagePolicies(QNetworkSession::UsagePolicies p) Q_DECL_OVERRIDE
    {
        if (m_policies == p) {
            return;
        }
        m_policies = p;
        emit usagePoliciesChanged(m_policies);
    }

private:
    QByteArray configId() const { return activeConfig.identifier().toLatin1(); }
    void chooseActiveConfig();
    QNetworkSession::State stateFromConfig() const;
    void updateState();
    void fail(QNetworkSession::SessionError error);

    DummyEngine *m_engine;
    QVariantMap m_props;
    QNetworkSession::UsagePolicies m_policies;
    /// Marks that this session's own open() ran (and no close() since); hence
    /// isOpen holds only while it is set and the interface is connected.
    bool m_opened;
    /// Marks an OPEN of this session that the daemon has not turned Active
    /// yet; hence an update sent before that OPEN cannot close the session.
    bool m_awaitingActive;
    /// Reports why the session last ended without its own close() or stop().
    QNetworkSession::SessionError m_error;
    QString m_errorString;
};

// MARK: engine.
class DummyEngine : public QBearerEngine
{
    Q_OBJECT
public:
    DummyEngine(QObject *parent = 0);

    bool hasIdentifier(const QString &id) Q_DECL_OVERRIDE
    {
        QMutexLocker locker(&mutex);
        return accessPointConfigurations.contains(id)
            || snapConfigurations.contains(id)
            || userChoiceConfigurations.contains(id);
    }

    QNetworkConfigurationManager::Capabilities capabilities() const Q_DECL_OVERRIDE
    {
        // We can activate/deactivate our fake interfaces, and each config is a direct route.
        return QNetworkConfigurationManager::CanStartAndStopInterfaces
            | QNetworkConfigurationManager::DirectConnectionRouting;
    }

    BearerConfig::Entry settings(const QString &id) const { return m_fixture.entry(id); }

    QNetworkSessionPrivate *createSessionBackend() Q_DECL_OVERRIDE { return new DummySession(this); }

    QNetworkConfigurationPrivatePointer defaultConfiguration() Q_DECL_OVERRIDE;

    bool requiresPolling() const Q_DECL_OVERRIDE { return false; }

    /// The manager drives these two by name (QMetaObject::invokeMethod) on the engine's own thread:
    /// initialize() once, with BlockingQueuedConnection, before it reads our configurations, and
    /// requestUpdate() whenever it wants a rescan. Both must exist as invokable methods or the
    /// manager blocks forever waiting for the updateCompleted() that a missing requestUpdate() can
    /// never emit. We connect to the daemon here (in the bearer thread) rather than in the ctor.
    Q_INVOKABLE void initialize();
    Q_INVOKABLE void requestUpdate() { emit updateCompleted(); }

    /// Relay an OPEN, CLOSE, or STOP for a config id to the daemon; the
    /// resulting STATE push updates the config.
    /// Called from a QNetworkSession (the main thread), but m_sock lives in the engine's bearer
    /// thread -- so marshal the actual write onto that thread (a direct cross-thread write trips
    /// "QSocketNotifier: cannot be enabled/disabled from another thread" and is unsafe).
    void sendToDaemon(const QByteArray &verb, const QByteArray &id)
    { QMetaObject::invokeMethod(this, "doSend", Qt::QueuedConnection,
                                Q_ARG(QByteArray, verb), Q_ARG(QByteArray, id)); }

private slots:
    void onReadyRead();
    void doSend(const QByteArray &verb, const QByteArray &id)
    {
        if (m_sock && m_sock->state() == QAbstractSocket::ConnectedState) {
            m_sock->write(verb + ' ' + id + '\n');
            m_sock->flush();
        }
    }

private:
    void applyConfig(
            const QByteArray &id,
            int state,
            int bearer,
            const QByteArray &name);
    void applyState(const QByteArray &id, int state);
    void updateServiceNetwork();
    void updateGroup(const QString &id, const QString &name,
                     QNetworkConfiguration::Type type, bool wirelessOnly,
                     QList<QNetworkConfigurationPrivatePointer > *added,
                     QList<QNetworkConfigurationPrivatePointer > *changed);

    BearerConfig m_fixture;
    QTcpSocket *m_sock;
    QByteArray m_buf;
};

// MARK: session impl.
#ifndef QT_NO_NETWORKINTERFACE
/// QNetworkSession::interface() must be valid exactly when the session is
/// Connected (a live route exists, whichever session opened it) and invalid
/// otherwise. We have no real interface of our own; hence the fixture's
/// interface, else the host's first one, stands in for one.
QNetworkInterface DummySession::currentInterface() const
{
    if (state != QNetworkSession::Connected) {
        return QNetworkInterface();
    }
    const QString name = m_engine->settings(activeConfig.identifier()).interfaceName;
    if ( ! name.isEmpty()) {
        return QNetworkInterface::interfaceFromName(name);
    }
    const QList<QNetworkInterface> all = QNetworkInterface::allInterfaces();
    return all.isEmpty() ? QNetworkInterface() : all.first();
}
#endif

/*! Reports a configured operation failure without changing the interface state. */
void DummySession::fail(QNetworkSession::SessionError error)
{
    m_error = error;
    m_errorString = QString(QLL("Configured bearer operation failed (%1).")).arg(int(error));
    emit QNetworkSessionPrivate::error(error);
    emit quitPendingWaitsForOpened();
}

/*! Activates an open session after its configured delay. */
void DummySession::activate()
{
    if ( ! m_opened) {
        return;
    }
    m_engine->sendToDaemon("OPEN", configId());
    if (stateFromConfig() == QNetworkSession::Connected) {
        updateState();
    }
}

/*!
    Follows the configuration's state from here on: the engine reports each
    change the daemon broadcasts, and the session's state tracks the interface
    whichever session opened it.
*/
void DummySession::syncStateWithInterface()
{
    QObject::connect(m_engine, SIGNAL(configurationChanged(QNetworkConfigurationPrivatePointer)),
                     this, SLOT(onConfigurationChanged(QNetworkConfigurationPrivatePointer)),
                     Qt::UniqueConnection);
    chooseActiveConfig();
    state = stateFromConfig();
    // An Active configuration may be open through another session; this one
    // still counts as open only once its own `open()` ran.
    isOpen = state == QNetworkSession::Connected && m_opened;
}

/*!
    Picks the access point the session runs on. A service network runs on one
    of its members: an Active one when there is one, else a Discovered one,
    else its first. A user choice runs as the engine's service network would;
    every other configuration runs on itself.
*/
void DummySession::chooseActiveConfig()
{
    if (publicConfig.type() == QNetworkConfiguration::UserChoice) {
        serviceConfig = QNetworkConfigurationManager()
            .configurationFromIdentifier(QLL("dummy-service-network"));
    } else if (publicConfig.type() == QNetworkConfiguration::ServiceNetwork) {
        serviceConfig = publicConfig;
    } else {
        activeConfig = publicConfig;
        return;
    }
    const QList<QNetworkConfiguration > members = serviceConfig.children();
    activeConfig = members.value(0);
    const QNetworkConfiguration::StateFlag preferred[] = {
        QNetworkConfiguration::Active,
        QNetworkConfiguration::Discovered
    };
    for (size_t i = 0; i < sizeof(preferred) / sizeof(preferred[0]); ++i) {
        foreach (const QNetworkConfiguration &member, members) {
            if ((member.state() & preferred[i]) == preferred[i]) {
                activeConfig = member;
                return;
            }
        }
    }
}

/*!
    Maps the configuration's state onto a session state.

    QNetworkConfiguration::StateFlag values are CUMULATIVE masks, not
    single bits (Defined=0x2, Discovered=0x6, Active=0xe);
    hence test with "== flag", never a bare "& flag"
    (which would treat every Discovered config as Active).
*/
QNetworkSession::State DummySession::stateFromConfig() const
{
    const QNetworkConfiguration::StateFlags cfgState = activeConfig.state();
    if ((cfgState & QNetworkConfiguration::Active)
            == QNetworkConfiguration::Active
    ) {
        return QNetworkSession::Connected;
    }
    if ((cfgState & QNetworkConfiguration::Discovered)
            == QNetworkConfiguration::Discovered
    ) {
        return QNetworkSession::Disconnected;
    }
    return QNetworkSession::NotAvailable;
}

void DummySession::onConfigurationChanged(
        QNetworkConfigurationPrivatePointer config)
{
    QString id;
    {
        QMutexLocker locker(&config->mutex);
        id = config->id;
    }
    if (id == activeConfig.identifier()) {
        updateState();
    }
}

/*!
    Takes the session's state from its configuration, and opens or closes the
    session to match: open only while connected and opened by this session.
*/
void DummySession::updateState()
{
    const QNetworkSession::State newState = stateFromConfig();
    if (m_awaitingActive) {
        if (newState != QNetworkSession::Connected) {
            return;
        }
        m_awaitingActive = false;
    }
    const QNetworkSession::State oldState = state;
    const bool wasOpen = isOpen;
    state = newState;
    isOpen = state == QNetworkSession::Connected && m_opened;
    if ( ! wasOpen && isOpen) {
        emit quitPendingWaitsForOpened();
    }
    if (wasOpen && ! isOpen) {
        // Another session stopped the interface under this open one.
        m_opened = false;
        m_error = QNetworkSession::SessionAbortedError;
        m_errorString = QLL("The interface was stopped.");
        emit QNetworkSessionPrivate::error(m_error);
        emit closed();
    }
    if (oldState != state) {
        emit stateChanged(state);
    }
}

/*!
    Adds this session's hold on the interface. A connected interface opens the
    session at once; else the session connects, and opens once the daemon
    reports the configuration Active.
*/
void DummySession::open()
{
    // An open that is still waiting for its interface sends no second OPEN.
    if (isOpen || (m_opened && m_awaitingActive)) {
        return;
    }
    // A service network may have another best member by now.
    if ( ! m_awaitingActive) {
        chooseActiveConfig();
        state = stateFromConfig();
    }
    const BearerConfig::Entry config = m_engine->settings(activeConfig.identifier());
    if ( ! config.canStart || config.openError >= 0) {
        fail(config.openError >= 0 ? QNetworkSession::SessionError(config.openError)
            : QNetworkSession::OperationNotSupportedError);
        return;
    }
    if (state == QNetworkSession::NotAvailable || state == QNetworkSession::Invalid) {
        fail(QNetworkSession::InvalidConfigurationError);
        return;
    }
    m_opened = true;
    if (state == QNetworkSession::Connected && config.openDelayMs == 0) {
        m_engine->sendToDaemon("OPEN", configId());
        isOpen = true;
        emit quitPendingWaitsForOpened();
        return;
    }
    m_awaitingActive = true;
    state = QNetworkSession::Connecting;
    emit stateChanged(state);
    if (config.openDelayMs > 0) {
        QTimer::singleShot(config.openDelayMs, this, SLOT(activate()));
    } else {
        activate();
    }
}

/*!
    Ends this session's hold on the interface; the state keeps following the
    interface, which other sessions may still hold.
*/
void DummySession::close()
{
    if ( ! isOpen && ! m_awaitingActive) {
        return;
    }
    m_engine->sendToDaemon("CLOSE", configId());
    m_opened = false;
    m_awaitingActive = false;
    if (isOpen) {
        isOpen = false;
        emit closed();
    }
}

void DummySession::stop()
{
    const BearerConfig::Entry config = m_engine->settings(activeConfig.identifier());
    if ( ! config.canStop || config.stopError >= 0) {
        fail(config.stopError >= 0 ? QNetworkSession::SessionError(config.stopError)
            : QNetworkSession::OperationNotSupportedError);
        return;
    }
    // Brings the interface down for every session; `close()` only ends this
    // session's own hold on it. The session closes until the daemon reports
    // the interface down.
    if (state == QNetworkSession::Connected) {
        state = QNetworkSession::Closing;
        emit stateChanged(state);
    }
    m_engine->sendToDaemon("STOP", configId());
    close();
}

// MARK: engine impl.
DummyEngine::DummyEngine(QObject *parent)
    : QBearerEngine(parent), m_fixture(QString()), m_sock(0)
{
    // Deliberately light: the daemon connection is made in initialize(), which the manager calls on
    // this engine's own (bearer) thread, so the socket and its notifications live there.
}

QNetworkConfigurationPrivatePointer DummyEngine::defaultConfiguration()
{
    QMutexLocker locker(&mutex);
    // A test that needs a default the user picks at `open()` asks for it
    // through the environment, for as long as it needs it.
    if (qgetenv("QT_BEARER_DUMMY_DEFAULT") == "userchoice"
        && ! userChoiceConfigurations.isEmpty()
    ) {
        return userChoiceConfigurations.constBegin().value();
    }
    if (accessPointConfigurations.contains(m_fixture.defaultId())) {
        return accessPointConfigurations.value(m_fixture.defaultId());
    }
    // Prefer an Active configuration (a real default is a connected interface), so adding more
    // seed configs never changes which one is the default just because of QMap key ordering.
    QNetworkConfigurationPrivatePointer first;
    QHash<QString, QNetworkConfigurationPrivatePointer >::ConstIterator it;
    for (it = accessPointConfigurations.constBegin(); it != accessPointConfigurations.constEnd(); ++it) {
        if ( ! first) {
            first = it.value();
        }
        if ((it.value()->state & QNetworkConfiguration::Active)
                == QNetworkConfiguration::Active
        ) {
            return it.value();
        }
    }
    return first;
}

void DummyEngine::initialize()
{
    const QByteArray portStr = qgetenv("QT_BEARER_DUMMY_PORT");
    if (portStr.isEmpty()) {
        // No daemon -> publish nothing (ordinary runs are unaffected).
        emit updateCompleted();
        return;
    }
    m_fixture = BearerConfig(BearerConfig::findFile());
    m_sock = new QTcpSocket(this);
    connect(m_sock, &QTcpSocket::readyRead, this, &DummyEngine::onReadyRead);
    m_sock->connectToHost(QHostAddress::LocalHost, quint16(portStr.toUShort()));
    if (m_sock->waitForConnected(3000)) {
        // Read the initial CONFIG.../READY snapshot synchronously so the manager sees our
        // configurations as soon as initialize() returns.
        while (m_sock->state() == QAbstractSocket::ConnectedState) {
            if (m_buf.contains("READY\n")) {
                break;
            }
            if ( ! m_sock->waitForReadyRead(3000)) {
                break;
            }
            m_buf += m_sock->readAll();
        }
        onReadyRead();
    }
    emit updateCompleted();
}

void DummyEngine::onReadyRead()
{
    if (m_sock) {
        m_buf += m_sock->readAll();
    }
    int nl;
    while ((nl = m_buf.indexOf('\n')) >= 0) {
        const QByteArray line = m_buf.left(nl);
        m_buf.remove(0, nl + 1);
        if (line == "READY") {
            continue;
        }
        const QList<QByteArray > p = line.split(' ');
        if (p.value(0) == "CONFIG" && p.size() >= 5) {
            const QByteArray name = line.mid(
                line.indexOf(
                    ' ',
                    line.indexOf(' ',
                        line.indexOf(' ', line.indexOf(' ') + 1) + 1
                    )
                    + 1
                )
                + 1
            );
            applyConfig(p.at(1), p.at(2).toInt(), p.at(3).toInt(), name);
        } else if (p.value(0) == "STATE" && p.size() >= 3) {
            applyState(p.at(1), p.at(2).toInt());
        }
    }
}

void DummyEngine::applyConfig(
        const QByteArray &id,
        int st,
        int bearer,
        const QByteArray &name)
{
    const QString sid = QString::fromLatin1(id);
    QMutexLocker locker(&mutex);
    QNetworkConfigurationPrivatePointer ptr = accessPointConfigurations.value(sid);
    const bool isNew = ! ptr;
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
    updateServiceNetwork();
}

void DummyEngine::applyState(const QByteArray &id, int st)
{
    const QString sid = QString::fromLatin1(id);
    QMutexLocker locker(&mutex);
    QNetworkConfigurationPrivatePointer ptr = accessPointConfigurations.value(sid);
    if ( ! ptr) {
        return;
    }
    {
        QMutexLocker cfgLock(&ptr->mutex);
        ptr->state = QNetworkConfiguration::StateFlags(st);
    }
    locker.unlock();
    emit configurationChanged(ptr);
    updateServiceNetwork();
}

/*!
    Publishes the configurations that group the daemon's access points: one
    service network of every access point, one of the wireless ones only (WLAN
    and cellular), and one user choice that resolves as the first does.
*/
void DummyEngine::updateServiceNetwork()
{
    if ( ! m_fixture.serviceNetworks()) {
        return;
    }
    QList<QNetworkConfigurationPrivatePointer > added;
    QList<QNetworkConfigurationPrivatePointer > changed;
    {
        QMutexLocker locker(&mutex);
        if (accessPointConfigurations.isEmpty()) {
            return;
        }
        updateGroup(
            QLL("dummy-service-network"), QLL("Dummy service network"),
            QNetworkConfiguration::ServiceNetwork, false, &added, &changed
        );
        updateGroup(
            QLL("dummy-wireless-service-network"),
            QLL("Dummy wireless service network"),
            QNetworkConfiguration::ServiceNetwork, true, &added, &changed
        );
        updateGroup(
            QLL("dummy-user-choice"), QLL("Dummy user choice"),
            QNetworkConfiguration::UserChoice, false, &added, &changed
        );
    }
    foreach (const QNetworkConfigurationPrivatePointer &config, added) {
        emit configurationAdded(config);
    }
    foreach (const QNetworkConfigurationPrivatePointer &config, changed) {
        emit configurationChanged(config);
    }
}

/*!
    Builds or refreshes the group configuration @p id of the access points
    (only the wireless ones when @p wirelessOnly is set, every one otherwise),
    in id order; its state is the best state of its members: Active while one
    is Active, else Discovered while one is Discovered. Files the
    configuration into @p added or @p changed when it is new or differs; the
    caller holds the engine's mutex and emits them after releasing it.
*/
void DummyEngine::updateGroup(
        const QString &id,
        const QString &name,
        QNetworkConfiguration::Type type,
        bool wirelessOnly,
        QList<QNetworkConfigurationPrivatePointer > *added,
        QList<QNetworkConfigurationPrivatePointer > *changed)
{
    QMap<unsigned int, QNetworkConfigurationPrivatePointer > members;
    QNetworkConfiguration::StateFlags best = QNetworkConfiguration::Defined;
    QStringList ids = accessPointConfigurations.keys();
    ids.sort();
    for (int i = 0; i < ids.size(); ++i) {
        QNetworkConfigurationPrivatePointer member = accessPointConfigurations
            .value(ids.at(i));
        QMutexLocker memberLock(&member->mutex);
        if (wirelessOnly
            && member->bearerType == QNetworkConfiguration::BearerEthernet
        ) {
            continue;
        }
        members.insert(unsigned(members.size()), member);
        if ((member->state & QNetworkConfiguration::Active)
                == QNetworkConfiguration::Active
        ) {
            best = QNetworkConfiguration::Active;
        } else if ((member->state & QNetworkConfiguration::Discovered) == QNetworkConfiguration::Discovered
            && best != QNetworkConfiguration::Active
        ) {
            best = QNetworkConfiguration::Discovered;
        }
    }
    if (members.isEmpty()) {
        return;
    }
    // A user choice lists no members; it resolves when a session opens it.
    if (type == QNetworkConfiguration::UserChoice) {
        members.clear();
    }
    QHash<QString, QNetworkConfigurationPrivatePointer > &table = type == QNetworkConfiguration::UserChoice
        ? userChoiceConfigurations : snapConfigurations;
    QNetworkConfigurationPrivatePointer group = table.value(id);
    const bool isNew = ! group;
    if (isNew) {
        group = QNetworkConfigurationPrivatePointer(new QNetworkConfigurationPrivate);
        group->id = id;
    }
    {
        QMutexLocker groupLock(&group->mutex);
        if ( ! isNew
            && group->state == best
            && group->serviceNetworkMembers == members
        ) {
            return;
        }
        group->name = name;
        group->isValid = true;
        group->state = best;
        group->type = type;
        group->purpose = QNetworkConfiguration::PublicPurpose;
        group->serviceNetworkMembers = members;
    }
    if (isNew) {
        table.insert(id, group);
        added->append(group);
    } else {
        changed->append(group);
    }
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
