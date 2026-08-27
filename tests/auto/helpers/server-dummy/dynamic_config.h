/// @file
/// Dynamic (live-reloadable) server-dummy configuration.
///
/// ServiceConfigDynamic is the PARSED state of the --folder's server-config.ini -- distinct
/// from the per-listener static ServiceConfig (service_base.h): that one describes how to BUILD a
/// listener, this one holds run-time knobs the ini carries ([Unreachable] paths, [IO] rateLimit).
///
/// IniWatcher polls the ini for changes and hands out the parsed config. It does NOT parse until
/// load() is first called, and thereafter re-parses (reload()) only after the file actually
/// changes -- so a busy server that edits the ini between test-cases picks the change up on the
/// next load() without a restart. Shared as a QSharedPointer<IniWatcher> so every service reads one
/// watcher and one cache (and so the ref-count keeps it alive across threads to some degree).
#ifndef DYNAMIC_CONFIG_H
#define DYNAMIC_CONFIG_H

#include <QtCore/QObject>
#include <QtCore/QByteArray>
#include <QtCore/QDateTime>
#include <QtCore/QDir>
#include <QtCore/QFile>
#include <QtCore/QFileInfo>
#include <QtCore/QMap>
#include <QtCore/QSharedPointer>
#include <QtCore/QString>
#include <QtCore/QTextStream>
#include <QtCore/QTimer>

class ServiceConfigDynamic
{
public:
    ServiceConfigDynamic()
        : m_rateLimit(0)
        , m_responseDelayMs(0)
        , m_fixedSessionTicketKey(false)
    {}

    /// Parses `iniPath`. [Unreachable] keys are quoted paths RELATIVE to the ini's own directory
    /// (resolved to absolute so they match a service's realPath); the value is the HTTP-style error
    /// the denial stands for. [IO] rateLimit is bytes/sec (0 = unlimited). QSettings treats '/' in a
    /// key as a group separator, so the sections are parsed by hand.
    static ServiceConfigDynamic parse(const QString &iniPath)
    {
        ServiceConfigDynamic c;
        QFile f(iniPath);
        if (!f.open(QIODevice::ReadOnly | QIODevice::Text))
            return c;
        const QDir dir = QFileInfo(iniPath).absoluteDir();
        QTextStream in(&f);
        QString section;
        while (!in.atEnd()) {
            QString line = in.readLine().trimmed();
            if (line.isEmpty() || line.startsWith(QLatin1Char(';')) || line.startsWith(QLatin1Char('#')))
                continue;
            if (line.startsWith(QLatin1Char('['))) {
                const int close = line.indexOf(QLatin1Char(']'));
                section = (close > 1) ? line.mid(1, close - 1).trimmed().toLower() : QString();
                continue;
            }
            const int eq = line.indexOf(QLatin1Char('='));
            if (eq < 0)
                continue;
            QString key = line.left(eq).trimmed();
            const QString val = line.mid(eq + 1).trimmed();
            if (key.size() >= 2
                && ((key.startsWith(QLatin1Char('"')) && key.endsWith(QLatin1Char('"')))
                    || (key.startsWith(QLatin1Char('\'')) && key.endsWith(QLatin1Char('\'')))))
                key = key.mid(1, key.size() - 2);
            if (section == QLL("unreachable")) {
                if (!key.isEmpty())
                    c.m_unreachable.insert(QDir::cleanPath(dir.absoluteFilePath(key)), val.toInt());
            } else if (section == QLL("io")) {
                if (key.compare(QLL("rateLimit"), Qt::CaseInsensitive) == 0)
                    c.m_rateLimit = val.toInt();
                else if (key.compare(QLL("responseDelayMs"), Qt::CaseInsensitive) == 0)
                    c.m_responseDelayMs = val.toInt();
            } else if (section == QLL("tls")) {
                if (key.compare(QLL("fixedSessionTicketKey"), Qt::CaseInsensitive) == 0)
                    c.m_fixedSessionTicketKey = (val.compare(QLL("true"), Qt::CaseInsensitive) == 0
                                                 || val.toInt() != 0);
            }
        }
        f.close();
        return c;
    }

    /// Non-zero when `absPath` is configured unreachable (listed directly or under a listed
    /// directory) -- returns its error code. Such a path stats OK but denies the fetch.
    int unreachableCode(const QString &absPath) const
    {
        const QMap<QString, int>::const_iterator hit = m_unreachable.constFind(absPath);
        if (hit != m_unreachable.constEnd())
            return hit.value();
        for (QMap<QString, int>::const_iterator it = m_unreachable.constBegin();
             it != m_unreachable.constEnd(); ++it)
            if (absPath.startsWith(it.key() + QLatin1Char('/')))
                return it.value();
        return 0;
    }

    /// Rate cap in bytes per second; 0 means unlimited.
    int rateLimit() const { return m_rateLimit; }

    /// Artificial per-response latency in ms (0 = none). A service that honours it holds its reply
    /// this long before writing, to give the client-side state machine RTT to settle (e.g. an auth
    /// retry to update the per-host credential cache before a second parallel challenge arrives). It
    /// is a STABILISER, not a synchroniser: a uniform delay lowers a race's collision probability but
    /// does not serialise genuinely concurrent requests -- see the auth-dedup note in network-report.
    int responseDelayMs() const { return m_responseDelayMs; }

    /// [TLS] fixedSessionTicketKey: when true, the TLS listener installs a FIXED RFC-5077 session-
    /// ticket key (via QSslConfiguration::setSessionTicketKey) so the tickets it issues resume across
    /// separate client sockets. This deliberately defeats ticket-key rotation/forward-secrecy, so it
    /// is OFF by default (each connection keeps OpenSSL's random per-context key) and a test that needs
    /// observable session resumption turns it on for its own duration via the TestServerEditor.
    bool fixedSessionTicketKey() const { return m_fixedSessionTicketKey; }

private:
    /// Maps each unreachable absolute path to the HTTP-style error code its denial returns.
    QMap<QString, int> m_unreachable;
    int m_rateLimit;
    int m_responseDelayMs;
    bool m_fixedSessionTicketKey;
};

class IniWatcher : public QObject
{
    Q_OBJECT
public:
    explicit IniWatcher(const QString &iniPath, QObject *parent = Q_NULLPTR)
        : QObject(parent), m_path(iniPath), m_dirty(true)
    {
        // Poll frequently (<= 100 ms): a TestServerEditor::save() blocks until this watcher
        // acknowledges the change (see writeLastPull), so a short interval keeps that wait brief.
        m_timer.setInterval(50);
        connect(&m_timer, &QTimer::timeout, this, &IniWatcher::poll);
        m_timer.start();
    }

    /// The process-wide shared watcher for `iniPath` (created on first call; the ini path is fixed
    /// for a server-dummy process, so later calls reuse the same instance regardless of argument).
    static QSharedPointer<IniWatcher> shared(const QString &iniPath)
    {
        static QSharedPointer<IniWatcher> inst;
        if (inst.isNull())
            inst = QSharedPointer<IniWatcher>(new IniWatcher(iniPath));
        return inst;
    }

    /// Current config. Parses lazily on the first call and re-parses only after the file changed
    /// since the last load (byIniChange having fired); otherwise returns the cached instance.
    QSharedPointer<ServiceConfigDynamic> load()
    {
        if (m_dirty || m_config.isNull())
            reload();
        return m_config;
    }

Q_SIGNALS:
    void byIniChange();

protected:
    /// Re-reads the ini into a fresh ServiceConfigDynamic and caches it. Called by load() only when
    /// needed (never eagerly -- no config object exists until the first load()).
    void reload()
    {
        m_config = QSharedPointer<ServiceConfigDynamic>(
            new ServiceConfigDynamic(ServiceConfigDynamic::parse(m_path)));
        m_dirty = false;
    }

private:
    void poll()
    {
        // Detect changes by CONTENT, not mtime: two writes within one coarse mtime tick (a save()
        // landing in the same tick as our own lastPull ack) would be missed by a timestamp compare,
        // leaving m_dirty false so a connection right after save() reads stale config. Comparing the
        // file's bytes -- with our own lastPull line excluded -- catches every real edit regardless
        // of mtime granularity, and does not re-trigger on our own ack.
        const QByteArray content = strippedContent();
        if (content == m_lastContent)
            return;
        m_lastContent = content;
        m_dirty = true;      // services re-parse on their next load()
        emit byIniChange();
        // Acknowledge the pull so a waiting TestServerEditor::save() knows the change was picked up:
        // stamp [Watcher] lastPull. That write only touches the (excluded) lastPull line, so it does
        // not change m_lastContent and is not re-detected as a fresh edit.
        writeLastPull(QDateTime::currentMSecsSinceEpoch());
    }

    /// The ini's bytes with the watcher's own "lastPull" line removed, so change detection ignores
    /// our acks (only real edits differ). Empty when the file is unreadable.
    QByteArray strippedContent() const
    {
        QFile f(m_path);
        if (!f.open(QIODevice::ReadOnly | QIODevice::Text))
            return QByteArray();
        const QList<QByteArray> lines = f.readAll().split('\n');
        QByteArray out;
        for (int i = 0; i < lines.size(); ++i)
            if (!lines.at(i).trimmed().startsWith("lastPull"))
                out += lines.at(i) + '\n';
        return out;
    }

    /// Upserts "[Watcher] lastPull = <ms>" into the ini, preserving all other content (including the
    /// edits a save() just wrote). Small file, rewritten whole -- adequate for a test helper.
    void writeLastPull(qint64 ms)
    {
        QFile f(m_path);
        if (!f.open(QIODevice::ReadOnly | QIODevice::Text))
            return;
        QStringList lines = QString::fromUtf8(f.readAll()).split(QLatin1Char('\n'));
        f.close();
        const QString kv = QLL("lastPull = ") + QString::number(ms);
        QString section;
        int watcherHeader = -1, pullLine = -1;
        for (int i = 0; i < lines.size(); ++i) {
            const QString t = lines.at(i).trimmed();
            if (t.startsWith(QLatin1Char('[')) && t.endsWith(QLatin1Char(']'))) {
                section = t.mid(1, t.size() - 2).trimmed().toLower();
                if (section == QLL("watcher"))
                    watcherHeader = i;
            } else if (section == QLL("watcher") && t.startsWith(QLL("lastPull"))) {
                pullLine = i;
            }
        }
        if (pullLine >= 0)
            lines[pullLine] = kv;
        else if (watcherHeader >= 0)
            lines.insert(watcherHeader + 1, kv);
        else
            lines << QLL("[Watcher]") << kv;
        QFile out(m_path);
        if (out.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
            out.write(lines.join(QLatin1Char('\n')).toUtf8());
            out.close();
        }
    }

    QString m_path;
    QTimer m_timer;
    bool m_dirty;
    /// The ini bytes (minus the lastPull line) seen at the last poll; drives change detection so a
    /// real edit re-parses while our own lastPull ack does not.
    QByteArray m_lastContent;
    QSharedPointer<ServiceConfigDynamic> m_config;
};

#endif // DYNAMIC_CONFIG_H
