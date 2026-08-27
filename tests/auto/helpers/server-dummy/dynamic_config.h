/// @file
/// Dynamic (live-reloadable) server-dummy configuration.
///
/// ServiceConfigDynamic is the PARSED state of the --folder's
/// server-config.ini -- distinct from the per-listener static ServiceConfig
/// (service_base.h): that one describes how to BUILD a listener, this one
/// holds run-time knobs the ini carries ([Unreachable] paths, [IO] rateLimit),
/// and the HTTP route table.
///
/// IniWatcher polls the ini for changes and hands out the parsed config.
/// It does NOT parse until load() is first called, and thereafter re-parses
/// (reload()) only after the file actually changes -- so a busy server that
/// edits the ini between test-cases picks the change up on the next load()
/// without a restart. Shared as a QSharedPointer<IniWatcher> so every service
/// reads one watcher and one cache (and so the ref-count keeps it alive
/// across threads to some degree).
#ifndef DYNAMIC_CONFIG_H
#define DYNAMIC_CONFIG_H

#include <QtCore/QObject>
#include <QtCore/QByteArray>
#include <QtCore/QDateTime>
#include <QtCore/QDir>
#include <QtCore/QFile>
#include <QtCore/QFileInfo>
#include <QtCore/QSaveFile>
#include <QtCore/QGlobalStatic>
#include <QtCore/QList>
#include <QtCore/QMap>
#include <QtCore/QSharedPointer>
#include <QtCore/QString>
#include <QtCore/QTextStream>
#include <QtCore/QTimer>

/// Describes one HTTP route of the route table: the request paths it answers,
/// the listeners it answers on, and the reply type with that type's settings.
///
/// A route comes from a `[Routes/<name>]` section: `path` (an exact path) and
/// `prefix` (a path start) may each repeat, `listener` names the listeners it
/// is limited to (`plain`, `secure`, `http2`; all when absent), `type` names
/// the reply, and every other key is a setting of that type. A value may carry
/// C escapes (`\n`, `\t`, `\0`, `\xNN`, `\\`).
struct HttpRoute
{
    QByteArray name;
    QList<QByteArray > paths;
    QList<QByteArray > prefixes;
    QList<QByteArray > listeners;
    QByteArray type;
    /// Holds the type's settings by lower-case key.
    QMap<QByteArray, QByteArray > params;

    inline bool matches(const QByteArray &listener,
                        const QByteArray &path) const
    {
        if (!listeners.isEmpty() && !listeners.contains(listener)) {
            return false;
        }
        if (paths.contains(path)) {
            return true;
        }
        for (int i = 0; i < prefixes.size(); ++i) {
            if (path.startsWith(prefixes.at(i))) {
                return true;
            }
        }
        return false;
    }

    inline bool has(const QByteArray &key) const { return params.contains(key); }
    inline QByteArray param(const QByteArray &key,
                            const QByteArray &fallback = QByteArray()) const
    {
        return params.value(key, fallback);
    }

    /// Reads a size setting such as `10 MB`, `512 KiB` or `25962` (bytes).
    inline qint64 size(const QByteArray &key, qint64 fallback = 0) const
    {
        const QByteArray text = param(key).trimmed();
        if (text.isEmpty()) {
            return fallback;
        }
        int unit = 0;
        while (unit < text.size() && (isdigit(uchar(text.at(unit))) || text.at(unit) == '.')) {
            ++unit;
        }
        const double number = text.left(unit).toDouble();
        const QByteArray suffix = text.mid(unit).trimmed().toLower();
        qint64 scale = 1;
        if (suffix == "kb") {
            scale = 1000;
        } else if (suffix == "kib") {
            scale = 1024;
        } else if (suffix == "mb") {
            scale = 1000 * 1000;
        } else if (suffix == "mib") {
            scale = 1024 * 1024;
        } else if (suffix == "gb") {
            scale = 1000 * 1000 * 1000;
        } else if (suffix == "gib") {
            scale = 1024 * 1024 * 1024;
        }
        return qint64(number * scale);
    }
};

/// Decodes the C escapes of an ini value: `\n`, `\r`, `\t`, `\0`, `\xNN`, and
/// `\\`; a value in double quotes keeps its outer spaces.
static inline QByteArray unescapeIniValue(const QByteArray &raw)
{
    QByteArray in = raw;
    if (in.size() >= 2 && in.startsWith('"') && in.endsWith('"')) {
        in = in.mid(1, in.size() - 2);
    }
    QByteArray out;
    out.reserve(in.size());
    for (int i = 0; i < in.size(); ++i) {
        const char c = in.at(i);
        if (c != '\\' || i + 1 >= in.size()) {
            out += c;
            continue;
        }
        const char e = in.at(++i);
        if (e == 'n') {
            out += '\n';
        } else if (e == 'r') {
            out += '\r';
        } else if (e == 't') {
            out += '\t';
        } else if (e == '0') {
            out += '\0';
        } else if (e == 'x' && i + 2 < in.size()) {
            out += char(in.mid(i + 1, 2).toInt(Q_NULLPTR, 16));
            i += 2;
        } else {
            out += e;
        }
    }
    return out;
}

/// Keeps the route table of routes.ini once it is read.
Q_GLOBAL_STATIC(QList<HttpRoute >, defaultRouteTable)

class ServiceConfigDynamic
{
public:
    ServiceConfigDynamic()
        : m_rateLimit(0)
        , m_responseDelayMs(0)
        , m_closeOnError(false)
        , m_fixedSessionTicketKey(false)
    {}

    /// Parses `iniPath`. [Unreachable] keys are quoted paths RELATIVE to
    /// the ini's own directory (resolved to absolute so they match a
    /// service's realPath); the value is the HTTP-style error the denial
    /// stands for. [IO] rateLimit is bytes/sec (0 = unlimited).
    /// QSettings treats '/' in a key as a group separator, so the sections
    /// are parsed by hand.
    ///
    /// The route table starts from server-dummy's own routes.ini; a
    /// `[Routes/<name>]` section of `iniPath` replaces the default route of
    /// that name, and one of a new name goes in front of the defaults, in the
    /// order the file lists them.
    static ServiceConfigDynamic parse(const QString &iniPath)
    {
        ServiceConfigDynamic c;
        c.m_routes = defaultRoutes();
        if (iniPath.isEmpty()) {
            return c;
        }
        QFile f(iniPath);
        if (!f.open(QIODevice::ReadOnly | QIODevice::Text)) {
            return c;
        }
        QList<HttpRoute > routes;
        parseInto(&f, QFileInfo(iniPath).absoluteDir(), &c, &routes);
        f.close();
        QList<HttpRoute > added;
        for (int i = 0; i < routes.size(); ++i) {
            int known = 0;
            while (known < c.m_routes.size() && c.m_routes.at(known).name != routes.at(i).name) {
                ++known;
            }
            if (known < c.m_routes.size()) {
                c.m_routes[known] = routes.at(i);
            } else {
                added.append(routes.at(i));
            }
        }
        c.m_routes = added + c.m_routes;
        return c;
    }

    /// Reads the route table compiled into server-dummy (its routes.ini), once.
    static QList<HttpRoute > defaultRoutes()
    {
        QRef<QList<HttpRoute > > cached = defaultRouteTable();
        if (cached->isEmpty()) {
            QFile f(QLL(":/server-dummy/routes.ini"));
            if (f.open(QIODevice::ReadOnly | QIODevice::Text)) {
                ServiceConfigDynamic unused;
                parseInto(&f, QDir(), &unused, cached.data());
            }
        }
        return *cached;
    }

    /// Lists the HTTP routes in the order a request is matched against them.
    const QList<HttpRoute > &routes() const { return m_routes; }

    /// Non-zero when `absPath` is configured unreachable
    /// (listed directly or under a listed directory) -- returns its error code.
    /// Such a path stats OK but denies the fetch.
    int unreachableCode(const QString &absPath) const
    {
        const QMap<QString, int >::const_iterator hit = m_unreachable.constFind(absPath);
        if (hit != m_unreachable.constEnd()) {
            return hit.value();
        }
        for (QMap<QString, int >::const_iterator it = m_unreachable.constBegin();
             it != m_unreachable.constEnd();
             ++it)
            if (absPath.startsWith(it.key() + QLatin1Char('/'))) {
                return it.value();
            }
        return 0;
    }

    /// Rate cap in bytes per second; 0 means unlimited.
    int rateLimit() const { return m_rateLimit; }

    /// Artificial per-response latency in ms (0 = none).
    /// A service that honours it holds its reply this long before writing,
    /// to give the client-side state machine RTT to settle (e.g.
    /// an auth retry to update the per-host credential cache before a
    /// second parallel challenge arrives). It is a STABILISER, not a
    /// synchroniser: a uniform delay lowers a race's collision probability
    /// but does not serialize genuinely concurrent requests -- see the
    /// auth-dedup note in network-report.
    int responseDelayMs() const { return m_responseDelayMs; }

    /// [IO] closeOnError: when true, an HTTP/1.1 connection aborts its socket
    /// on the socket's first error, the way a server that gives up on a broken
    /// peer does; when false (the default), the connection stays as the error
    /// left it. A test whose edge case needs one behavior turns it on or off
    /// for its own rows through the TestServerEditor.
    bool closeOnError() const { return m_closeOnError; }

    /// [TLS] fixedSessionTicketKey: when true, the TLS listener installs a
    /// FIXED RFC-5077 session-ticket key (via QSslConfiguration::setSessionTicketKey)
    /// so the tickets it issues resume across separate client sockets.
    /// This deliberately defeats ticket-key rotation/forward-secrecy,
    /// so it is OFF by default (each connection keeps OpenSSL's random
    /// per-context key) and a test that needs observable session resumption
    /// turns it on for its own duration via the TestServerEditor.
    bool fixedSessionTicketKey() const { return m_fixedSessionTicketKey; }

private:
    /// Reads the sections of `device` into `c`, and its `[Routes/<name>]`
    /// sections into `routes`; `dir` resolves the relative paths of
    /// [Unreachable].
    static void parseInto(QIODevice *device, const QDir &dir, ServiceConfigDynamic *c,
                          QList<HttpRoute > *routes)
    {
        QTextStream in(device);
        QString section;
        while (!in.atEnd()) {
            QString line = in.readLine().trimmed();
            if (line.isEmpty() || line.startsWith(QLatin1Char(';')) || line.startsWith(QLatin1Char('#'))) {
                continue;
            }
            if (line.startsWith(QLatin1Char('['))) {
                const int close = line.indexOf(QLatin1Char(']'));
                section = (close > 1) ? line.mid(1, close - 1).trimmed().toLower() : QString();
                if (section.startsWith(QLL("routes/"))) {
                    HttpRoute route;
                    route.name = section.mid(7).toUtf8();
                    routes->append(route);
                }
                continue;
            }
            const int eq = line.indexOf(QLatin1Char('='));
            if (eq < 0) {
                continue;
            }
            QString key = line.left(eq).trimmed();
            const QString val = line.mid(eq + 1).trimmed();
            if (key.size() >= 2
                && ((key.startsWith(QLatin1Char('"')) && key.endsWith(QLatin1Char('"')))
                    || (key.startsWith(QLatin1Char('\'')) && key.endsWith(QLatin1Char('\''))))
            ) {
                key = key.mid(1, key.size() - 2);
            }
            if (section.startsWith(QLL("routes/")) && !routes->isEmpty()) {
                addRouteSetting(&routes->last(), key.toLower().toUtf8(), val.toUtf8());
            } else if (section == QLL("unreachable")) {
                if (!key.isEmpty()) {
                    c->m_unreachable.insert(QDir::cleanPath(dir.absoluteFilePath(key)), val.toInt());
                }
            } else if (section == QLL("io")) {
                if (key.compare(QLL("rateLimit"), Qt::CaseInsensitive) == 0) {
                    c->m_rateLimit = val.toInt();
                } else if (key.compare(QLL("responseDelayMs"), Qt::CaseInsensitive) == 0) {
                    c->m_responseDelayMs = val.toInt();
                } else if (key.compare(QLL("closeOnError"), Qt::CaseInsensitive) == 0) {
                    c->m_closeOnError = val.compare(QLL("true"), Qt::CaseInsensitive) == 0
                        || val.toInt() != 0;
                }
            } else if (section == QLL("tls")) {
                if (key.compare(QLL("fixedSessionTicketKey"), Qt::CaseInsensitive) == 0) {
                    c->m_fixedSessionTicketKey = (val.compare(QLL("true"), Qt::CaseInsensitive) == 0
                        || val.toInt() != 0);
                }
            }
        }
    }

    /// Files one `key = value` line of a route section into `route`.
    static void addRouteSetting(HttpRoute *route, const QByteArray &key,
                                const QByteArray &value)
    {
        if (key == "path") {
            route->paths.append(unescapeIniValue(value));
        } else if (key == "prefix") {
            route->prefixes.append(unescapeIniValue(value));
        } else if (key == "listener") {
            const QList<QByteArray > names = value.split(',');
            for (int i = 0; i < names.size(); ++i) {
                route->listeners.append(names.at(i).trimmed().toLower());
            }
        } else if (key == "type") {
            route->type = value.trimmed().toLower();
        } else {
            route->params.insert(key, unescapeIniValue(value));
        }
    }

    /// Maps each unreachable absolute path to the HTTP-style error code
    /// its denial returns.
    QMap<QString, int > m_unreachable;
    int m_rateLimit;
    int m_responseDelayMs;
    bool m_closeOnError;
    bool m_fixedSessionTicketKey;
    /// Matches each HTTP request to the route that answers it, in order.
    QList<HttpRoute > m_routes;
};

class IniWatcher : public QObject
{
    Q_OBJECT
    typedef QObject super;
public:
    explicit IniWatcher(const QString &iniPath, QObject *parent = Q_NULLPTR)
        : QObject(parent), m_path(iniPath), m_dirty(true)
    {
        // Poll frequently (<= 100 ms): a `TestServerEditor::save()` blocks
        // until this watcher acknowledges the change (see `writeLastPull`),
        // so a short interval keeps that wait brief.
        m_timer.setInterval(50);
        connect(&m_timer, &QTimer::timeout, this, &IniWatcher::poll);
        // Without a path (a process given no `--folder`, such as `dnsd`) there
        // is no ini to watch; hence no poll runs, else each one warns that
        // `QFile` has no file name.
        if (!m_path.isEmpty()) {
            m_timer.start();
        }
    }

    /// The process-wide shared watcher for `iniPath` (created on first call;
    /// the ini path is fixed for a server-dummy process, so later calls
    /// reuse the same instance regardless of argument).
    static QSharedPointer<IniWatcher > shared(const QString &iniPath)
    {
        static QSharedPointer<IniWatcher > inst;
        if (inst.isNull()) {
            inst = QSharedPointer<IniWatcher >(new IniWatcher(iniPath));
        }
        return inst;
    }

    /// Current config. Parses lazily on the first call and re-parses only
    /// after the file changed since the last load (byIniChange having fired);
    /// otherwise returns the cached instance.
    QSharedPointer<ServiceConfigDynamic > load()
    {
        if (m_dirty || m_config.isNull()) {
            reload();
        }
        return m_config;
    }

Q_SIGNALS:
    void byIniChange();

protected:
    /// Re-reads the ini into a fresh ServiceConfigDynamic and caches it.
    /// Called by load() only when needed (never eagerly -- no config object
    /// exists until the first load()).
    void reload()
    {
        m_config = QSharedPointer<ServiceConfigDynamic >(
            new ServiceConfigDynamic(ServiceConfigDynamic::parse(m_path)));
        m_dirty = false;
    }

private:
    void poll()
    {
        // Detect changes by CONTENT, not mtime: two writes within one
        // coarse mtime tick would be missed by a timestamp compare,
        // leaving `m_dirty` false so a connection right after `save()`
        // reads stale config. Comparing the file's bytes catches every
        // real edit regardless of mtime granularity.
        const QByteArray content = fileContent();
        if (content == m_lastContent) {
            return;
        }
        m_lastContent = content;
        // Services re-parse on their next `load()`.
        m_dirty = true;
        emit byIniChange();
        // Acknowledge the pull so a waiting `TestServerEditor::save()` knows
        // the change was picked up. The ack goes to its own file beside the
        // ini, never into the ini: the test may write the ini again at any
        // moment, and a rewrite of it here would put back the content read
        // before that write, which erases the test's edits.
        writeLastPull(QDateTime::currentMSecsSinceEpoch());
    }

    /// The ini's bytes; empty when the file is unreadable.
    QByteArray fileContent() const
    {
        QFile f(m_path);
        if (!f.open(QIODevice::ReadOnly)) {
            return QByteArray();
        }
        return f.readAll();
    }

    /// Writes @p ms, the moment of the latest pull, into the ack file
    /// `<ini>.pull`, which TestServerEditor::save() polls;
    /// written through QSaveFile so that a reader never
    /// sees it half written.
    void writeLastPull(qint64 ms)
    {
        QSaveFile out(m_path + QLL(".pull"));
        if (out.open(QIODevice::WriteOnly)) {
            out.write(QByteArray::number(ms));
            out.commit();
        }
    }

    QString m_path;
    QTimer m_timer;
    bool m_dirty;
    /// The ini bytes seen at the last poll; drives change detection,
    /// hence a real edit re-parses.
    QByteArray m_lastContent;
    QSharedPointer<ServiceConfigDynamic > m_config;
};

#endif // DYNAMIC_CONFIG_H
