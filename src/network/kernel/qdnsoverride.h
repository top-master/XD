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

#ifndef QDNSOVERRIDE_H
#define QDNSOVERRIDE_H

#include <QtNetwork/qdnslookup.h>
#include <QtNetwork/qhostaddress.h>
#include <QtCore/qbytearray.h>
#include <QtCore/qlist.h>
#include <QtCore/qstring.h>

QT_BEGIN_NAMESPACE

/**
 * One parsed resource record in a neutral form -- it deliberately holds no QDns*Record (whose
 * d-pointers are private to QDnsLookupRunnable), so both QDnsLookup's runnable and the standalone
 * QHostInfoOverride can consume what @ref QDnsOverride::query returns. Only the fields relevant to
 * `type` are populated.
 */
struct QDnsWireRecord
{
    QDnsWireRecord()
        : type(0), ttl(0), preference(0), priority(0), weight(0), port(0) {}

    quint16 type;            // QDnsLookup::Type numeric value
    QString name;            // owner name (decoded from ACE)
    quint32 ttl;
    QHostAddress address;    // A / AAAA
    QString value;           // CNAME / NS / PTR target, or MX exchange, or SRV target
    quint16 preference;      // MX
    quint16 priority;        // SRV
    quint16 weight;          // SRV
    quint16 port;            // SRV
    QList<QByteArray> txt;   // TXT
};

/**
 * A @ref QDnsLookup that resolves over UDP against a chosen nameserver:port, cross-platform, using
 * an existing QUdpSocket instead of the operating system's resolver. Construct it in place of a
 * QDnsLookup -- no separate installation step -- e.g.:
 * ```
 * QDnsOverride dns(QHostAddress::LocalHost, 5300);
 * dns.setType(QDnsLookup::A);
 * dns.setName("example.test");
 * dns.lookup();
 * ```
 * The base nameserver (@ref QDnsLookup::nameserver) holds the address; this class adds the UDP
 * @ref port. It stays fully asynchronous: the query runs on the shared lookup thread pool.
 *
 * Subclass and override @ref logRequest to trace every request (the pre-DNS name).
 */
class Q_NETWORK_EXPORT QDnsOverride : public QDnsLookup
{
    Q_OBJECT
public:
    explicit QDnsOverride(const QHostAddress &nameserver, quint16 port = 53, QObject *parent = Q_NULLPTR);
    QDnsOverride(Type type, const QString &name, const QHostAddress &nameserver, quint16 port = 53, QObject *parent = Q_NULLPTR);
    ~QDnsOverride();

    quint16 port() const { return m_port; }
    void setPort(quint16 port) { m_port = port; }

    int timeout() const { return m_timeout; }
    void setTimeout(int msecs) { m_timeout = msecs; }

    static bool query(const QHostAddress &nameserver, quint16 port, int timeoutMs,
                      const QByteArray &aceName, quint16 qtype,
                      QDnsLookup::Error *error, QString *errorString,
                      QList<QDnsWireRecord> *records);

protected:
    QDnsLookupRunnable *newRunnable() Q_DECL_OVERRIDE;

    virtual void logRequest(const QString &name, int type);

private:
    quint16 m_port;
    int m_timeout;
};

QT_END_NAMESPACE

#endif // QDNSOVERRIDE_H
