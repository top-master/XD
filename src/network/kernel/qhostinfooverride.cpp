/****************************************************************************
**
** Copyright (C) 2026 The XD Company Ltd.
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

#include "qhostinfooverride.h"
#include "qdnsoverride.h"

#include <QtCore/qurl.h>

QT_BEGIN_NAMESPACE

QHostInfoOverride::QHostInfoOverride(const QHostAddress &nameserver, quint16 port)
    : m_nameserver(nameserver)
    , m_port(port)
    , m_timeout(3000)
{
}

QHostInfoOverride::~QHostInfoOverride()
{
}

// Resolves `name` to addresses by querying the configured nameserver over UDP (A then AAAA).
// Returns true and fills *addresses on a hit; returns false (leaving *addresses untouched) when the
// name is unknown or the query fails, so QHostInfo falls through to the operating-system resolver.
bool QHostInfoOverride::lookupHost(const QString &name, QList<QHostAddress> *addresses)
{
    const QByteArray ace = QUrl::toAce(name);
    if (ace.isEmpty())
        return false;

    bool found = false;
    const int types[2] = { QDnsLookup::A, QDnsLookup::AAAA };
    for (int i = 0; i < 2; ++i) {
        logRequest(name, types[i]);

        QDnsLookup::Error error = QDnsLookup::NoError;
        QString errorString;
        QList<QDnsWireRecord> records;
        if (!QDnsOverride::query(m_nameserver, m_port, m_timeout, ace, quint16(types[i]),
                                 &error, &errorString, &records))
            continue; // transport failure: try the other type, then fall through
        if (error != QDnsLookup::NoError)
            continue; // NXDOMAIN etc.: let QHostInfo fall through to the OS resolver

        for (int r = 0; r < records.size(); ++r) {
            const QDnsWireRecord &rec = records.at(r);
            if (rec.type == QDnsLookup::A || rec.type == QDnsLookup::AAAA) {
                addresses->append(rec.address);
                found = true;
            }
        }
    }
    return found;
}

// Called with the (pre-DNS, dotted) name and the query type before each wire request. Default: does
// nothing. Override to log or trace every resolution.
void QHostInfoOverride::logRequest(const QString &, int)
{
}

QT_END_NAMESPACE
