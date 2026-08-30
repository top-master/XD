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

// QDnsOverride and its minimal, cross-platform DNS-over-UDP engine (QDnsOverride::query), built on
// QUdpSocket. It replaces the platform resolvers only for the "override" path: a chosen nameserver
// and port, resolved without libresolv / dn_expand (name compression is decoded here). The wire
// format mirrors, inverted, the server side in tests/auto/helpers/server-dummy/dns_service.h.

#include "qdnsoverride.h"
#include "qdnslookup_p.h"

#include <QtNetwork/qudpsocket.h>
#include <QtNetwork/qnetworkproxy.h>
#include <QtCore/qcoreapplication.h>
#include <QtCore/qatomic.h>
#include <QtCore/qurl.h>

#include <string.h> // memcpy

QT_BEGIN_NAMESPACE

static inline QString qdo_tr(const char *text)
{
    return QCoreApplication::translate("QDnsLookup", text);
}

// dotted ACE name -> length-prefixed labels + zero terminator.
static QByteArray qdo_encodeName(const QByteArray &aceName)
{
    QByteArray out;
    const QList<QByteArray> labels = aceName.split('.');
    for (int i = 0; i < labels.size(); ++i) {
        const QByteArray &label = labels.at(i);
        if (label.isEmpty())
            continue; // tolerate a trailing dot or doubled dots
        const int len = qMin(label.size(), 63);
        out.append(char(len));
        out.append(label.constData(), len);
    }
    out.append(char(0)); // root label
    return out;
}

// Build a standard recursive query datagram (12-byte header + one question).
static QByteArray qdo_buildQuery(quint16 id, const QByteArray &aceName, quint16 qtype)
{
    QByteArray pkt;
    pkt.reserve(18 + aceName.size());
    const unsigned char hdr[12] = {
        (unsigned char)(id >> 8), (unsigned char)(id & 0xff),
        0x01, 0x00,   // flags: RD (recursion desired)
        0x00, 0x01,   // QDCOUNT = 1
        0x00, 0x00,   // ANCOUNT
        0x00, 0x00,   // NSCOUNT
        0x00, 0x00    // ARCOUNT
    };
    pkt.append(reinterpret_cast<const char *>(hdr), 12);
    pkt.append(qdo_encodeName(aceName));
    pkt.append(char(qtype >> 8)); pkt.append(char(qtype & 0xff)); // QTYPE
    pkt.append(char(0x00));       pkt.append(char(0x01));         // QCLASS = IN
    return pkt;
}

// Decode a (possibly compressed) name at *offset. Advances *offset past the name in the record
// stream (i.e. past the 2-byte pointer when compression is used). Caps redirections so a malformed
// or looping packet cannot hang. Returns false on malformed input.
static bool qdo_decodeName(const QByteArray &pkt, int *offset, QByteArray *out)
{
    const int size = pkt.size();
    const unsigned char *d = reinterpret_cast<const unsigned char *>(pkt.constData());
    int pos = *offset;
    int hops = 0;
    bool jumped = false;
    int resume = -1; // where to continue in the record stream after the first pointer
    out->clear();
    while (pos >= 0 && pos < size) {
        const unsigned char len = d[pos];
        if ((len & 0xc0) == 0xc0) {          // compression pointer
            if (pos + 1 >= size)
                return false;
            const int target = ((len & 0x3f) << 8) | d[pos + 1];
            if (!jumped)
                resume = pos + 2;
            jumped = true;
            pos = target;
            if (++hops > 128)                // guard against pointer loops
                return false;
            continue;
        }
        if ((len & 0xc0) != 0)               // reserved label type
            return false;
        if (len == 0) {                      // end of name
            pos += 1;
            break;
        }
        if (pos + 1 + len > size)
            return false;
        if (!out->isEmpty())
            out->append('.');
        out->append(reinterpret_cast<const char *>(d + pos + 1), len);
        pos += 1 + len;
    }
    *offset = jumped ? resume : pos;
    return true;
}

// Performs one synchronous DNS query over UDP against nameserver:port, using a locally-scoped
// QUdpSocket (never stored, so it is safe to call from any thread). On a transport failure
// (timeout / socket error) returns false with *error = ServerFailureError. On a received reply,
// returns true, sets *error from the DNS rcode (NoError on success), and fills *records with the
// answer section. `aceName` must already be in ACE form; `qtype` is a QDnsLookup::Type value.
// This is the shared engine QDnsOverride (and QHostInfoOverride) resolve through.
bool QDnsOverride::query(const QHostAddress &nameserver, quint16 port, int timeoutMs,
                         const QByteArray &aceName, quint16 qtype,
                         QDnsLookup::Error *error, QString *errorString,
                         QList<QDnsWireRecord> *records)
{
    static QBasicAtomicInt counter = Q_BASIC_ATOMIC_INITIALIZER(0);
    const quint16 id = quint16(counter.fetchAndAddRelaxed(1) & 0xffff);
    const QByteArray request = qdo_buildQuery(id, aceName, qtype);

    // Transact over a fresh, locally-scoped UDP socket (never stored -> safe on any thread).
    QByteArray response;
    {
        QUdpSocket socket;
        // Reach the nameserver directly. A datagram to it must never be tunnelled through an
        // application-wide proxy -- that proxy may be the very host we are resolving, and a SOCKS5
        // proxy rejects the datagram outright -- so this infrastructure socket bypasses any proxy.
        socket.setProxy(QNetworkProxy::NoProxy);
        if (socket.writeDatagram(request, nameserver, port) < 0
            || !socket.waitForReadyRead(timeoutMs)) {
            *error = QDnsLookup::ServerFailureError;
            *errorString = qdo_tr("DNS server did not respond");
            return false;
        }
        // Accept the datagram whose id matches our request; ignore stray ones.
        while (socket.hasPendingDatagrams()) {
            QByteArray dg;
            dg.resize(int(socket.pendingDatagramSize()));
            socket.readDatagram(dg.data(), dg.size());
            if (dg.size() >= 2
                && (unsigned char)dg.at(0) == (unsigned char)(id >> 8)
                && (unsigned char)dg.at(1) == (unsigned char)(id & 0xff)) {
                response = dg;
                break;
            }
        }
        if (response.isEmpty()) {
            *error = QDnsLookup::ServerFailureError;
            *errorString = qdo_tr("DNS server did not respond");
            return false;
        }
    }

    // Parse the response header.
    if (response.size() < 12) {
        *error = QDnsLookup::InvalidReplyError;
        *errorString = qdo_tr("Invalid reply received");
        return true; // a reply WAS received, just malformed
    }
    const unsigned char *h = reinterpret_cast<const unsigned char *>(response.constData());
    switch (h[3] & 0x0f) { // rcode
    case 0: break; // NOERROR
    case 1: *error = QDnsLookup::InvalidRequestError; *errorString = qdo_tr("Server could not process query"); return true;
    case 2: *error = QDnsLookup::ServerFailureError;  *errorString = qdo_tr("Server failure"); return true;
    case 3: *error = QDnsLookup::NotFoundError;       *errorString = qdo_tr("Non existent domain"); return true;
    case 5: *error = QDnsLookup::ServerRefusedError;  *errorString = qdo_tr("Server refused to answer"); return true;
    default: *error = QDnsLookup::InvalidReplyError;  *errorString = qdo_tr("Invalid reply received"); return true;
    }
    const int qdcount = (h[4] << 8) | h[5];
    const int ancount = (h[6] << 8) | h[7];
    const int size = response.size();

    int p = 12;
    for (int q = 0; q < qdcount; ++q) { // skip the question section
        QByteArray dummy;
        if (!qdo_decodeName(response, &p, &dummy) || p + 4 > size) {
            *error = QDnsLookup::InvalidReplyError;
            *errorString = qdo_tr("Invalid reply received");
            return true;
        }
        p += 4; // QTYPE + QCLASS
    }

    for (int a = 0; a < ancount; ++a) {
        QByteArray nameAce;
        if (!qdo_decodeName(response, &p, &nameAce)) {
            *error = QDnsLookup::InvalidReplyError;
            *errorString = qdo_tr("Could not expand domain name");
            return true;
        }
        if (p + 10 > size) {
            *error = QDnsLookup::InvalidReplyError;
            *errorString = qdo_tr("Invalid reply received");
            return true;
        }
        const unsigned char *r = reinterpret_cast<const unsigned char *>(response.constData());
        const quint16 type  = (r[p] << 8) | r[p + 1];
        const quint32 ttl   = (quint32(r[p + 4]) << 24) | (r[p + 5] << 16) | (r[p + 6] << 8) | r[p + 7];
        const quint16 rdlen = (r[p + 8] << 8) | r[p + 9];
        const int rd = p + 10;
        if (rd + rdlen > size) {
            *error = QDnsLookup::InvalidReplyError;
            *errorString = qdo_tr("Invalid reply received");
            return true;
        }

        QDnsWireRecord rec;
        rec.type = type;
        rec.name = QUrl::fromAce(nameAce);
        rec.ttl = ttl;

        if (type == QDnsLookup::A && rdlen == 4) {
            rec.address = QHostAddress((quint32(r[rd]) << 24) | (r[rd + 1] << 16) | (r[rd + 2] << 8) | r[rd + 3]);
            records->append(rec);
        } else if (type == QDnsLookup::AAAA && rdlen == 16) {
            rec.address = QHostAddress(r + rd);
            records->append(rec);
        } else if (type == QDnsLookup::CNAME || type == QDnsLookup::NS || type == QDnsLookup::PTR) {
            int off = rd; QByteArray target;
            if (!qdo_decodeName(response, &off, &target)) {
                *error = QDnsLookup::InvalidReplyError;
                *errorString = qdo_tr("Could not expand domain name");
                return true;
            }
            rec.value = QUrl::fromAce(target);
            records->append(rec);
        } else if (type == QDnsLookup::MX && rdlen >= 3) {
            rec.preference = (r[rd] << 8) | r[rd + 1];
            int off = rd + 2; QByteArray exchange;
            if (!qdo_decodeName(response, &off, &exchange)) {
                *error = QDnsLookup::InvalidReplyError;
                *errorString = qdo_tr("Could not expand domain name");
                return true;
            }
            rec.value = QUrl::fromAce(exchange);
            records->append(rec);
        } else if (type == QDnsLookup::SRV && rdlen >= 7) {
            rec.priority = (r[rd] << 8) | r[rd + 1];
            rec.weight   = (r[rd + 2] << 8) | r[rd + 3];
            rec.port     = (r[rd + 4] << 8) | r[rd + 5];
            int off = rd + 6; QByteArray target;
            if (!qdo_decodeName(response, &off, &target)) {
                *error = QDnsLookup::InvalidReplyError;
                *errorString = qdo_tr("Could not expand domain name");
                return true;
            }
            rec.value = QUrl::fromAce(target);
            records->append(rec);
        } else if (type == QDnsLookup::TXT) {
            int t = rd;
            while (t < rd + rdlen) {
                const int len = r[t];
                ++t;
                if (t + len > rd + rdlen) {
                    *error = QDnsLookup::InvalidReplyError;
                    *errorString = qdo_tr("Invalid text record");
                    return true;
                }
                rec.txt << QByteArray(reinterpret_cast<const char *>(r + t), len);
                t += len;
            }
            records->append(rec);
        }
        // Any other type is skipped.

        p = rd + rdlen;
    }

    *error = QDnsLookup::NoError;
    errorString->clear();
    return true;
}

// The runnable that carries out an override lookup on the thread pool. It copies the nameserver,
// port and timeout at construction, so it is fully self-contained and never touches the QDnsOverride
// object from the pool thread.
class QDnsOverrideRunnable : public QDnsLookupRunnable
{
public:
    QDnsOverrideRunnable(QDnsLookup::Type type, const QByteArray &name,
                         const QHostAddress &nameserver, quint16 port, int timeout)
        : QDnsLookupRunnable(type, name, nameserver)
        , m_port(port)
        , m_timeout(timeout)
    { }

protected:
    void query(const int requestType, const QByteArray &requestName,
               const QHostAddress &nameserver, QDnsLookupReply *reply) Q_DECL_OVERRIDE
    {
        QList<QDnsWireRecord> records;
        QDnsOverride::query(nameserver, m_port, m_timeout, requestName, quint16(requestType),
                            &reply->error, &reply->errorString, &records);
        if (reply->error == QDnsLookup::NoError)
            reply->add(records);
    }

private:
    quint16 m_port;
    int m_timeout;
};

QDnsOverride::QDnsOverride(const QHostAddress &nameserver, quint16 port, QObject *parent)
    : QDnsLookup(parent)
    , m_port(port)
    , m_timeout(3000)
{
    setNameserver(nameserver);
}

QDnsOverride::QDnsOverride(Type type, const QString &name, const QHostAddress &nameserver,
                           quint16 port, QObject *parent)
    : QDnsLookup(type, name, nameserver, parent)
    , m_port(port)
    , m_timeout(3000)
{
}

QDnsOverride::~QDnsOverride()
{
}

QDnsLookupRunnable *QDnsOverride::newRunnable()
{
    // logRequest runs here, on the caller's thread, where the object is plainly alive.
    logRequest(name(), int(type()));
    return new QDnsOverrideRunnable(type(), QUrl::toAce(name()), nameserver(), m_port, m_timeout);
}

// Called (on the thread that started the lookup) with the pre-DNS name and query type before the
// request is dispatched. Default: does nothing. Override to log or trace every request.
void QDnsOverride::logRequest(const QString &, int)
{
}

QT_END_NAMESPACE
