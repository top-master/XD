#ifndef DNS_SERVICE_H
#define DNS_SERVICE_H

#include "service_base.h"

#include <QtCore/QByteArray>
#include <QtCore/QDateTime>
#include <QtCore/QList>
#include <QtCore/QtEndian>

/// Wall-clock (ms since epoch) of the last DNS query this resolver answered. The persistent dnsd
/// keepalive (main.cpp) uses it so an actively-queried resolver stays alive through a long single
/// test binary (e.g. tst_QNetworkReply), and only self-exits once queries stop. Defined in main.cpp.
extern qint64 g_dnsLastActivityMs;

/// DnsService: a DNS responder for tst_QDnsLookup's zone. It answers the exact records the
/// test expects (A/AAAA/MX/NS/SRV/TXT), so the test can run offline by pointing QDnsLookup at
/// this server with setNameserver(host, port). Any name whose base label is not in the zone
/// answers NXDOMAIN. Only the base label (and, for SRV, the label after _service._proto)
/// selects the record; the trailing zone labels of the query are echoed onto relative targets,
/// so the same table serves both the plain and the IDN zone the test uses.
class DnsService : public TestService
{
public:
    DnsService(const ServiceConfig &c, QObject *parent = Q_NULLPTR)
        : TestService(c, parent) {}

    void start() Q_DECL_OVERRIDE
    {
        QUdpSocket *dns = new QUdpSocket(this);
        bindUdp(dns);
        connect(dns, &QUdpSocket::readyRead, this, &DnsService::onReadyRead);
    }

    void onReadyRead()
    {
        g_dnsLastActivityMs = QDateTime::currentMSecsSinceEpoch(); // keep the shared dnsd alive while queried
        QUdpSocket *dns = qobject_cast<QUdpSocket *>(sender());
        while (dns->hasPendingDatagrams()) {
            QByteArray query(int(dns->pendingDatagramSize()), Qt::Uninitialized);
            QHostAddress from; quint16 fromPort;
            dns->readDatagram(query.data(), query.size(), &from, &fromPort);
            const QByteArray reply = buildReply(query);
            if (!reply.isEmpty())
                dns->writeDatagram(reply, from, fromPort);
        }
    }

private:
    enum { T_A = 1, T_NS = 2, T_PTR = 12, T_MX = 15, T_TXT = 16, T_AAAA = 28, T_SRV = 33, T_ANY = 255 };

    static QByteArray u16(quint16 v) { QByteArray b; b.append(char((v >> 8) & 0xff)); b.append(char(v & 0xff)); return b; }
    static QByteArray u32(quint32 v)
    {
        QByteArray b;
        b.append(char((v >> 24) & 0xff)); b.append(char((v >> 16) & 0xff));
        b.append(char((v >> 8) & 0xff)); b.append(char(v & 0xff));
        return b;
    }

    static QByteArray encodeName(const QList<QByteArray> &labels)
    {
        QByteArray out;
        for (int i = 0; i < labels.size(); ++i) {
            out.append(char(labels.at(i).size() & 0x3f));
            out.append(labels.at(i));
        }
        out.append(char(0));
        return out;
    }

    /// One answer resource record whose owner name is a compression pointer to the question
    /// (offset 12) -- every record the test asks for is owned by the queried name.
    static QByteArray rr(quint16 type, const QByteArray &rdata)
    {
        QByteArray out;
        out.append(char(0xc0)); out.append(char(0x0c));
        out += u16(type); out += u16(1); out += u32(60);
        out += u16(quint16(rdata.size())); out += rdata;
        return out;
    }

    static QByteArray aRdata(const char *ip) { return u32(QHostAddress(QString::fromLatin1(ip)).toIPv4Address()); }
    static QByteArray aaaaRdata(const char *ip)
    {
        const Q_IPV6ADDR addr = QHostAddress(QString::fromLatin1(ip)).toIPv6Address();
        return QByteArray(reinterpret_cast<const char *>(addr.c), 16);
    }
    static QByteArray txtChunk(const QByteArray &s) { QByteArray out; out.append(char(s.size() & 0xff)); out += s; return out; }

    QByteArray buildReply(const QByteArray &query)
    {
        if (query.size() < 12)
            return QByteArray();

        // Read the question name into labels, then qtype.
        int off = 12;
        QList<QByteArray> labels;
        while (off < query.size()) {
            const int len = quint8(query.at(off));
            if (len == 0) { ++off; break; }
            if ((len & 0xc0) == 0xc0) { off += 2; break; }
            ++off;
            labels << query.mid(off, len);
            off += len;
        }
        if (off + 4 > query.size())
            return QByteArray();
        const quint16 qtype = (quint16(quint8(query.at(off))) << 8) | quint8(query.at(off + 1));
        const int questionEnd = off + 4;

        // Pick the base label and the trailing zone labels (echoed onto relative targets).
        QByteArray base;
        QList<QByteArray> zone;
        if (labels.size() >= 3 && labels.at(0).startsWith('_')) {
            base = labels.at(2); zone = labels.mid(3);   // _service._proto.<base>.<zone>
        } else if (!labels.isEmpty()) {
            base = labels.at(0); zone = labels.mid(1);
        }

        const bool known = appendRecords(base, qtype, zone);

        QByteArray reply = query.left(2);                 // copy the query id
        reply += u16(known ? 0x8580 : 0x8583);            // QR + AA + RD + RA; rcode 3 when unknown
        reply += u16(1);                                  // one question
        reply += u16(quint16(m_rrCount));
        reply += u16(0); reply += u16(0);                 // no authority / additional
        reply += query.mid(12, questionEnd - 12);         // echo the question
        reply += m_rrOut;
        return reply;
    }

    /// The add*/relName/absName emit helpers below append to these scratch fields; one
    /// appendRecords() call runs synchronously at a time, so plain members are enough (and
    /// keep the dispatch table free of a lambda per record, which the XD tree forbids).
    QByteArray m_rrOut;
    int m_rrCount;
    /// Trailing zone labels appended to relative targets.
    QList<QByteArray> m_rrZone;

    /// A relative target ("multi") becomes "multi.<zone>"; an absolute one keeps its labels.
    QByteArray relName(const char *n) const
    {
        QList<QByteArray> ls = QByteArray(n).split('.');
        ls += m_rrZone;
        return encodeName(ls);
    }
    static QByteArray absName(const char *n) { return encodeName(QByteArray(n).split('.')); }
    void addA(const char *ip) { m_rrOut += rr(T_A, aRdata(ip)); ++m_rrCount; }
    void addAAAA(const char *ip) { m_rrOut += rr(T_AAAA, aaaaRdata(ip)); ++m_rrCount; }
    void addMX(quint16 pref, const char *tgt) { m_rrOut += rr(T_MX, u16(pref) + relName(tgt)); ++m_rrCount; }
    void addNS(const char *tgt) { m_rrOut += rr(T_NS, absName(tgt)); ++m_rrCount; }
    void addPTR(const char *tgt) { m_rrOut += rr(T_PTR, relName(tgt)); ++m_rrCount; }
    void addSRV(quint16 p, quint16 w, quint16 port, const char *tgt)
    {
        m_rrOut += rr(T_SRV, u16(p) + u16(w) + u16(port) + relName(tgt)); ++m_rrCount;
    }
    void addTXT(const QByteArray &s) { m_rrOut += rr(T_TXT, txtChunk(s)); ++m_rrCount; }

    /// Appends the resource records for base+qtype into m_rrOut/m_rrCount; returns false for an
    /// unknown base so the caller answers NXDOMAIN. zone is appended to relative targets.
    bool appendRecords(const QByteArray &base, quint16 qtype, const QList<QByteArray> &zone)
    {
        const bool any = (qtype == T_ANY);
        m_rrOut.clear();
        m_rrCount = 0;
        m_rrZone = zone;

        // The test server's own fake hosts -> loopback, so XD's QHostInfo (pointed here via a
        // QHostInfoOverride) resolves the server host name to the running server-dummy.
        // qt-local-server.test is TestServer::domainName() (the domain adapted tests connect to);
        // qt-test-server[.<zone>] is the legacy QtNetworkSettings::serverName() form, kept so any
        // test still using it resolves too. The one multi-SAN cert (TestServer::serverCert(), runtime-generated) covers both.
        if (base == "qt-local-server" || base == "qt-test-server") {
            if (any || qtype == T_A) addA("127.0.0.1");
            if (any || qtype == T_AAAA) addAAAA("::1");
        }
        else if (base == "a-single") { if (any || qtype == T_A) addA("192.0.2.1"); }
        else if (base == "a-multi") { if (any || qtype == T_A) { addA("192.0.2.1"); addA("192.0.2.2"); addA("192.0.2.3"); } }
        else if (base == "aaaa-single") { if (any || qtype == T_AAAA) addAAAA("2001:db8::1"); }
        else if (base == "aaaa-multi") { if (any || qtype == T_AAAA) { addAAAA("2001:db8::1"); addAAAA("2001:db8::2"); addAAAA("2001:db8::3"); } }
        else if (base == "a-plus-aaaa") { if (any || qtype == T_A) addA("198.51.100.1"); if (any || qtype == T_AAAA) addAAAA("2001:db8::1:1"); }
        else if (base == "multi") {
            if (any || qtype == T_A) { addA("198.51.100.1"); addA("198.51.100.2"); addA("198.51.100.3"); }
            if (any || qtype == T_AAAA) { addAAAA("2001:db8::1:1"); addAAAA("2001:db8::1:2"); }
        }
        else if (base == "mx-single") { if (any || qtype == T_MX) addMX(10, "multi"); }
        else if (base == "mx-single-cname") { if (any || qtype == T_MX) addMX(10, "cname"); }
        else if (base == "mx-multi") { if (any || qtype == T_MX) { addMX(10, "multi"); addMX(20, "a-single"); } }
        else if (base == "mx-multi-sameprio") { if (any || qtype == T_MX) { addMX(10, "multi"); addMX(10, "a-single"); } }
        else if (base == "ptr-single") { if (any || qtype == T_PTR) addPTR("a-single"); }
        else if (base == "ns-single") { if (any || qtype == T_NS) addNS("ns11.cloudns.net"); }
        else if (base == "ns-multi") { if (any || qtype == T_NS) { addNS("ns11.cloudns.net"); addNS("ns12.cloudns.net"); } }
        else if (base == "srv-single") { if (any || qtype == T_SRV) addSRV(5, 0, 7, "multi"); }
        else if (base == "srv-prio") { if (any || qtype == T_SRV) { addSRV(1, 0, 7, "multi"); addSRV(2, 0, 7, "a-plus-aaaa"); } }
        else if (base == "srv-weighted") { if (any || qtype == T_SRV) { addSRV(5, 75, 7, "multi"); addSRV(5, 25, 7, "a-plus-aaaa"); } }
        else if (base == "srv-multi") {
            if (any || qtype == T_SRV) {
                addSRV(1, 50, 7, "multi"); addSRV(2, 50, 7, "a-single");
                addSRV(2, 50, 7, "aaaa-single"); addSRV(3, 50, 7, "a-multi");
            }
        }
        else if (base == "txt-single") { if (any || qtype == T_TXT) addTXT("Hello"); }
        else if (base == "txt-multi-onerr") { if (any || qtype == T_TXT) { m_rrOut += rr(T_TXT, txtChunk("Hello") + txtChunk("World")); ++m_rrCount; } }
        else if (base == "txt-multi-multirr") { if (any || qtype == T_TXT) { addTXT("Hello"); addTXT("World"); } }
        else { return false; } // unknown base -> NXDOMAIN

        return true;
    }
};

#endif // DNS_SERVICE_H
