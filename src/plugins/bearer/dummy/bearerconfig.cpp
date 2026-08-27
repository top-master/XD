/****************************************************************************
**
** Copyright (C) 2015 The XD Company Ltd.
**
** This file is part of the QtNetwork module of the XD Toolkit.
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

#include "./bearerconfig.h"

#include <QtCore/QCoreApplication>
#include <QtCore/QDir>
#include <QtCore/QFileInfo>
#include <QtCore/QSettings>

/*! Initializes an activatable access point with no injected errors. */
BearerConfig::Entry::Entry()
    : state(QNetworkConfiguration::Discovered), bearer(QNetworkConfiguration::BearerUnknown)
    , canStart(true), canStop(true), openDelayMs(0), openError(-1), stopError(-1)
{
}

/*! Finds the explicit fixture or a bearer-config.ini beside the working
    directory or executable; an explicit `-` names no fixture. */
QString BearerConfig::findFile()
{
    const QString explicitPath = QString::fromLocal8Bit(qgetenv("QT_BEARER_DUMMY_CONFIG"));
    if (explicitPath == QLL("-")) {
        return QString();
    }
    if ( ! explicitPath.isEmpty()) {
        return explicitPath;
    }
    const QString roots[] = {
        QDir::currentPath(),
        QCoreApplication::applicationDirPath()
    };
    for (size_t i = 0; i < sizeof(roots) / sizeof(roots[0]); ++i) {
        QDir dir(roots[i]);
        do {
            const QString path = dir.filePath(QLL("bearer-config.ini"));
            if (QFileInfo::exists(path)) {
                return path;
            }
        } while (dir.cdUp());
    }
    return QString();
}

/*! Reads a numeric or named configuration state. */
static QNetworkConfiguration::StateFlags readState(const QString &value)
{
    const QString name = value.toLower();
    if (name == QLL("active")) {
        return QNetworkConfiguration::Active;
    }
    if (name == QLL("discovered")) {
        return QNetworkConfiguration::Discovered;
    }
    if (name == QLL("defined")) {
        return QNetworkConfiguration::Defined;
    }
    if (name == QLL("undefined")) {
        return QNetworkConfiguration::Undefined;
    }
    bool ok = false;
    const int state = value.toInt(&ok, 0);
    return QNetworkConfiguration::StateFlags(ok ? state : int(QNetworkConfiguration::Discovered));
}

/*! Reads a numeric bearer type or its public enum name without
    the Bearer prefix. */
static QNetworkConfiguration::BearerType readBearer(const QString &value)
{
    QString name = value.toLower();
    if (name.startsWith(QLL("bearer"))) {
        name = name.mid(6);
    }
    static const char *const names[] = {
        "unknown", "ethernet", "wlan", "2g", "cdma2000", "wcdma", "hspa",
        "bluetooth", "wimax", "evdo", "lte", "3g", "4g"
    };
    for (size_t i = 0; i < sizeof(names) / sizeof(names[0]); ++i) {
        if (name == QLatin1String(names[i])) {
            return QNetworkConfiguration::BearerType(i);
        }
    }
    return QNetworkConfiguration::BearerType(value.toInt());
}

/*! Loads the fixture, retaining the standard three access points
    when no file exists. */
BearerConfig::BearerConfig(const QString &path)
    : m_isolate(false), m_serviceNetworks(true)
{
    if (path.isEmpty() || ! QFileInfo::exists(path)) {
        // Fake configurations: one already Discovered-and-Active (so a default session can
        // open immediately) and the others merely Discovered (so open() transitions them to Active).
        // Bearer2G ("2G"): the cellular type that is both in the suitableConfiguration() cellular
        // filter AND in sessionProperties()'s validBearerNames (that list spells the 3G/4G/LTE/EVDO
        // names with a stray "Bearer" prefix, so only 2G/CDMA2000/WCDMA/HSPA actually match there).
        static const char *const ids[] = { "eth", "wlan", "cell" };
        static const char *const names[] = {
            "Dummy Ethernet",
            "Dummy WLAN",
            "Dummy Cellular"
        };
        for (int i = 0; i < 3; ++i) {
            Entry item;
            item.id = QLL("bearer-dummy-") + QLatin1String(ids[i]);
            item.name = QLatin1String(names[i]);
            item.state = i == 0
                ? QNetworkConfiguration::Active
                : QNetworkConfiguration::Discovered;
            item.bearer = QNetworkConfiguration::BearerType(i + 1);
            m_entries.append(item);
        }
        return;
    }
    QSettings settings(path, QSettings::IniFormat);
    // Parses nested INI sections before enumerating their groups.
    settings.allKeys();
    m_isolate = settings.value(QLL("isolate"), false).toBool();
    m_serviceNetworks = settings.value(QLL("serviceNetworks"), true).toBool();
    m_defaultId = settings.value(QLL("defaultConfiguration")).toString();
    settings.beginGroup(QLL("Configurations"));
    foreach (const QString &id, settings.childGroups()) {
        settings.beginGroup(id);
        Entry item;
        item.id = id;
        item.name = settings.value(QLL("name"), id).toString();
        item.interfaceName = settings.value(QLL("interface")).toString();
        item.state = readState(settings.value(
            QLL("state"),
            QString(QLL("Discovered"))
        ).toString());
        item.bearer = readBearer(settings.value(
            QLL("bearer"),
            QString(QLL("Ethernet"))
        ).toString());
        item.canStart = settings.value(QLL("canStart"), true).toBool();
        item.canStop = settings.value(QLL("canStop"), true).toBool();
        item.openDelayMs = qMax(0, settings.value(QLL("openDelayMs"), 0).toInt());
        item.openError = settings.value(QLL("openError"), -1).toInt();
        item.stopError = settings.value(QLL("stopError"), -1).toInt();
        m_entries.append(item);
        settings.endGroup();
    }
}

/*! Finds the settings of the access point @a id; one the fixture does not
    name may start and stop. */
BearerConfig::Entry BearerConfig::entry(const QString &id) const
{
    foreach (const Entry &item, m_entries) {
        if (item.id == id) {
            return item;
        }
    }
    return Entry();
}
