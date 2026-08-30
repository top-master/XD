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

#ifndef QHOSTINFOOVERRIDE_H
#define QHOSTINFOOVERRIDE_H

#include <QtNetwork/qhostaddress.h>
#include <QtCore/qlist.h>
#include <QtCore/qstring.h>

QT_BEGIN_NAMESPACE

/**
 * A per-application resolution override for @ref QHostInfo: while one is installed, name lookups
 * are first offered to it, so a host can be resolved through a chosen nameserver over UDP without
 * touching /etc/hosts or the system resolver. Install it with
 * @c QHostInfoLookupManager::setOverride() (held via a QSharedPointer).
 *
 * The virtual methods make it a real extension point: the default @ref lookupHost queries the
 * configured nameserver:port, while a subclass may override @ref logRequest to trace every
 * resolved name, or @ref lookupHost to answer names itself.
 */
class Q_NETWORK_EXPORT QHostInfoOverride
{
public:
    explicit QHostInfoOverride(const QHostAddress &nameserver, quint16 port = 53);
    virtual ~QHostInfoOverride();

    QHostAddress nameserver() const { return m_nameserver; }
    quint16 port() const { return m_port; }

    int timeout() const { return m_timeout; }
    void setTimeout(int msecs) { m_timeout = msecs; }

    virtual bool lookupHost(const QString &name, QList<QHostAddress> *addresses);

protected:
    virtual void logRequest(const QString &name, int type);

private:
    Q_DISABLE_COPY(QHostInfoOverride)
    QHostAddress m_nameserver;
    quint16 m_port;
    int m_timeout;
};

QT_END_NAMESPACE

#endif // QHOSTINFOOVERRIDE_H
