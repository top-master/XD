// License: Apache 2.0 without attribution need.

#include "qhpack_p.h"

QT_BEGIN_NAMESPACE

namespace QHttp2
{

// --- RFC 7541, Appendix A: the static table (indices 1..61). ---------------
static const HeaderField *staticTable()
{
    static const struct { const char *name; const char *value; } table[] = {
        {":authority", ""}, {":method", "GET"}, {":method", "POST"},
        {":path", "/"}, {":path", "/index.html"}, {":scheme", "http"},
        {":scheme", "https"}, {":status", "200"}, {":status", "204"},
        {":status", "206"}, {":status", "304"}, {":status", "400"},
        {":status", "404"}, {":status", "500"}, {"accept-charset", ""},
        {"accept-encoding", "gzip, deflate"}, {"accept-language", ""},
        {"accept-ranges", ""}, {"accept", ""}, {"access-control-allow-origin", ""},
        {"age", ""}, {"allow", ""}, {"authorization", ""}, {"cache-control", ""},
        {"content-disposition", ""}, {"content-encoding", ""}, {"content-language", ""},
        {"content-length", ""}, {"content-location", ""}, {"content-range", ""},
        {"content-type", ""}, {"cookie", ""}, {"date", ""}, {"etag", ""},
        {"expect", ""}, {"expires", ""}, {"from", ""}, {"host", ""},
        {"if-match", ""}, {"if-modified-since", ""}, {"if-none-match", ""},
        {"if-range", ""}, {"if-unmodified-since", ""}, {"last-modified", ""},
        {"link", ""}, {"location", ""}, {"max-forwards", ""},
        {"proxy-authenticate", ""}, {"proxy-authorization", ""}, {"range", ""},
        {"referer", ""}, {"refresh", ""}, {"retry-after", ""}, {"server", ""},
        {"set-cookie", ""}, {"strict-transport-security", ""},
        {"transfer-encoding", ""}, {"user-agent", ""}, {"vary", ""}, {"via", ""},
        {"www-authenticate", ""}
    };
    static HeaderField fields[sizeof(table) / sizeof(table[0])];
    static bool init = false;
    if (!init) {
        for (size_t i = 0; i < sizeof(table) / sizeof(table[0]); ++i)
            fields[i] = HeaderField(table[i].name, table[i].value);
        init = true;
    }
    return fields;
}
enum { StaticTableCount = 61 };

// --- RFC 7541, Appendix B: the Huffman code, symbol -> (code, bit length). --
struct HuffCode { quint32 code; quint8 bits; };
static const HuffCode huffTable[257] = {
#include "qhpack_huffman.inc"
};

// A binary decode tree for the Huffman code, built once from huffTable.
struct HuffNode {
    Q_DECL_CONSTEXPR inline HuffNode()
        : sym(-1)
    {
        child[0] = -1;
        child[1] = -1;
    }

    int child[2];
    int sym;
};
static const QVector<HuffNode> &huffTree()
{
    static QVector<HuffNode> tree;
    if (tree.isEmpty()) {
        tree.append(HuffNode()); // root
        for (int s = 0; s < 256; ++s) {       // EOS (256) is only padding, not a symbol
            quint32 code = huffTable[s].code;
            int bits = huffTable[s].bits;
            int node = 0;
            for (int b = bits - 1; b >= 0; --b) {
                const int bit = (code >> b) & 1;
                if (tree[node].child[bit] == -1) {
                    tree.append(HuffNode());
                    tree[node].child[bit] = tree.size() - 1;
                }
                node = tree[node].child[bit];
            }
            tree[node].sym = s;
        }
    }
    return tree;
}

static bool huffmanDecode(const QByteArray &in, QByteArray *out)
{
    const QVector<HuffNode> &tree = huffTree();
    int node = 0;
    for (int i = 0; i < in.size(); ++i) {
        const quint8 byte = quint8(in.at(i));
        for (int b = 7; b >= 0; --b) {
            const int bit = (byte >> b) & 1;
            node = tree[node].child[bit];
            if (node == -1)
                return false; // no such code path
            if (tree[node].sym != -1) {
                out->append(char(tree[node].sym));
                node = 0;
            }
        }
    }
    // Any leftover bits must be a proper prefix of EOS (all ones) and < 8 bits.
    // Reaching a non-root node with only 1-bits left is valid padding.
    return true;
}

// --------------------------------------------------------------------------

QByteArray HpackEncoder::encode(const HttpHeaders &headers) const
{
    QByteArray out;
    const HeaderField *st = staticTable();

    for (int h = 0; h < headers.size(); ++h) {
        const HeaderField &f = headers.at(h);
        int nameIndex = 0; // 1-based static index whose NAME matches, or 0

        for (int i = 0; i < StaticTableCount; ++i) {
            if (st[i].name == f.name) {
                if (st[i].value == f.value) {
                    // Exact match -> "Indexed Header Field" (6.1): 1xxxxxxx.
                    const int idx = i + 1;
                    if (idx < 0x7f) {
                        out.append(char(0x80 | idx));
                    } else {
                        out.append(char(0xff));
                        int rem = idx - 0x7f;
                        while (rem >= 0x80) { out.append(char((rem & 0x7f) | 0x80)); rem >>= 7; }
                        out.append(char(rem));
                    }
                    nameIndex = -1; // signals "already emitted"
                    break;
                }
                if (!nameIndex)
                    nameIndex = i + 1; // remember first name-only match
            }
        }
        if (nameIndex == -1)
            continue;

        // "Literal Header Field without Indexing" (6.2.2): 0000xxxx name-index.
        if (nameIndex < 0xf) {
            out.append(char(nameIndex)); // 0000 | nameIndex (0 => name as literal)
        } else {
            out.append(char(0x0f));
            int rem = nameIndex - 0xf;
            while (rem >= 0x80) { out.append(char((rem & 0x7f) | 0x80)); rem >>= 7; }
            out.append(char(rem));
        }
        if (nameIndex == 0)
            appendString(out, f.name);
        appendString(out, f.value);
    }
    return out;
}

void HpackEncoder::appendString(QByteArray &out, const QByteArray &s) const
{
    // Raw (non-Huffman) literal string: H=0, 7-bit length prefix, then the bytes.
    int len = s.size();
    if (len < 0x7f) {
        out.append(char(len));
    } else {
        out.append(char(0x7f));
        int rem = len - 0x7f;
        while (rem >= 0x80) { out.append(char((rem & 0x7f) | 0x80)); rem >>= 7; }
        out.append(char(rem));
    }
    out.append(s);
}

// --------------------------------------------------------------------------

HpackDecoder::HpackDecoder(quint32 maxDynamicTableSize)
    : m_maxTableSize(maxDynamicTableSize)
    , m_tableSize(0)
{
}

bool HpackDecoder::decodeInteger(const uchar *&p, const uchar *end, int prefixBits, quint64 *value)
{
    if (p >= end)
        return false;
    const quint64 mask = (quint64(1) << prefixBits) - 1;
    quint64 i = *p & mask;
    ++p;
    if (i < mask) {
        *value = i;
        return true;
    }
    quint64 m = 0;
    quint8 b;
    do {
        if (p >= end || m > 63)
            return false;
        b = *p++;
        i += quint64(b & 0x7f) << m;
        m += 7;
    } while (b & 0x80);
    *value = i;
    return true;
}

bool HpackDecoder::decodeString(const uchar *&p, const uchar *end, QByteArray *out)
{
    if (p >= end)
        return false;
    const bool huffman = (*p & 0x80) != 0;
    quint64 len = 0;
    if (!decodeInteger(p, end, 7, &len))
        return false;
    if (quint64(end - p) < len)
        return false;
    QByteArray raw(reinterpret_cast<const char *>(p), int(len));
    p += len;
    if (huffman)
        return huffmanDecode(raw, out);
    *out = raw;
    return true;
}

bool HpackDecoder::lookup(quint64 index, HeaderField *out) const
{
    if (index == 0)
        return false;
    if (index <= StaticTableCount) {
        *out = staticTable()[index - 1];
        return true;
    }
    const quint64 dyn = index - StaticTableCount - 1;
    if (dyn >= quint64(m_dynamicTable.size()))
        return false;
    *out = m_dynamicTable.at(int(dyn));
    return true;
}

void HpackDecoder::setMaxTableSize(quint32 size)
{
    m_maxTableSize = size;
    evictToFit();
}

void HpackDecoder::evictToFit()
{
    while (m_tableSize > m_maxTableSize && !m_dynamicTable.isEmpty()) {
        const HeaderField &last = m_dynamicTable.last();
        m_tableSize -= quint32(last.name.size() + last.value.size() + 32);
        m_dynamicTable.removeLast();
    }
}

void HpackDecoder::insert(const HeaderField &field)
{
    const quint32 entrySize = quint32(field.name.size() + field.value.size() + 32);
    m_dynamicTable.prepend(field);
    m_tableSize += entrySize;
    evictToFit();
}

bool HpackDecoder::decode(const QByteArray &block, HttpHeaders *out)
{
    const uchar *p = reinterpret_cast<const uchar *>(block.constData());
    const uchar *end = p + block.size();

    while (p < end) {
        const quint8 first = *p;
        if (first & 0x80) {
            // 6.1 Indexed Header Field.
            quint64 index = 0;
            if (!decodeInteger(p, end, 7, &index))
                return false;
            HeaderField f;
            if (!lookup(index, &f))
                return false;
            out->append(f);
        } else if (first & 0x40) {
            // 6.2.1 Literal Header Field with Incremental Indexing.
            quint64 nameIndex = 0;
            if (!decodeInteger(p, end, 6, &nameIndex))
                return false;
            HeaderField f;
            if (nameIndex) {
                HeaderField nf;
                if (!lookup(nameIndex, &nf))
                    return false;
                f.name = nf.name;
            } else if (!decodeString(p, end, &f.name)) {
                return false;
            }
            if (!decodeString(p, end, &f.value))
                return false;
            insert(f);
            out->append(f);
        } else if (first & 0x20) {
            // 6.3 Dynamic Table Size Update.
            quint64 size = 0;
            if (!decodeInteger(p, end, 5, &size))
                return false;
            setMaxTableSize(quint32(size));
        } else {
            // 6.2.2 / 6.2.3 Literal Header Field without / never Indexing.
            quint64 nameIndex = 0;
            if (!decodeInteger(p, end, 4, &nameIndex))
                return false;
            HeaderField f;
            if (nameIndex) {
                HeaderField nf;
                if (!lookup(nameIndex, &nf))
                    return false;
                f.name = nf.name;
            } else if (!decodeString(p, end, &f.name)) {
                return false;
            }
            if (!decodeString(p, end, &f.value))
                return false;
            out->append(f);
        }
    }
    return true;
}

} // namespace QHttp2

QT_END_NAMESPACE
