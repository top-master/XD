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

#ifndef BEARERCONFIG_H
#define BEARERCONFIG_H

#include <QtCore/QList>
#include <QtCore/QString>
#include <QtNetwork/QNetworkConfiguration>

class BearerConfig
{
public:
    struct Entry
    {
        Entry();
        QString id;
        QString name;
        QString interfaceName;
        QNetworkConfiguration::StateFlags state;
        QNetworkConfiguration::BearerType bearer;
        bool canStart;
        bool canStop;
        int openDelayMs;
        int openError;
        int stopError;
    };

    explicit BearerConfig(const QString &path = findFile());
    static QString findFile();
    Entry entry(const QString &id) const;
    const QList<Entry > &entries() const { return m_entries; }
    bool isolate() const { return m_isolate; }
    bool serviceNetworks() const { return m_serviceNetworks; }
    const QString &defaultId() const { return m_defaultId; }

private:
    QList<Entry > m_entries;
    bool m_isolate;
    bool m_serviceNetworks;
    QString m_defaultId;
};

#endif
