// License: Apache 2.0 without attribution need.
//
// A clean-room HPACK (RFC 7541) header-compression codec for HTTP/2. Written
// from the RFC: static table (Appendix A), integer (5.1) and string (5.2)
// primitives, the Huffman code (Appendix B) for decoding, and the field
// representations (6.x) plus a dynamic table (2.3.2). No third-party code.

#ifndef QHPACK_P_H
#define QHPACK_P_H

//
//  W A R N I N G
//  -------------
// This file is part of the Network access API. It exists purely as an
// implementation detail and may change from version to version without notice.
//

#include <QtCore/qbytearray.h>
#include <QtCore/qvector.h>

QT_BEGIN_NAMESPACE

namespace QHttp2
{

struct HeaderField
{
    HeaderField() {}
    HeaderField(const QByteArray &n, const QByteArray &v) : name(n), value(v) {}
    QByteArray name;
    QByteArray value;
};
typedef QVector<HeaderField> HttpHeaders;

// Encodes header lists. For simplicity and full interoperability it uses the
// "literal header field without indexing" representation (never Huffman), with a
// static-table index for the field name when one exists. This is always valid.
class HpackEncoder
{
public:
    QByteArray encode(const HttpHeaders &headers) const;

private:
    void appendString(QByteArray &out, const QByteArray &s) const;
};

// Decodes header blocks, supporting every RFC 7541 representation the peer may
// send: indexed fields (static + dynamic), the three literal forms, dynamic
// table size updates, and Huffman-coded strings.
class HpackDecoder
{
public:
    explicit HpackDecoder(quint32 maxDynamicTableSize = 4096);

    bool decode(const QByteArray &block, HttpHeaders *out);

private:
    bool decodeInteger(const uchar *&p, const uchar *end, int prefixBits, quint64 *value);
    bool decodeString(const uchar *&p, const uchar *end, QByteArray *out);
    bool lookup(quint64 index, HeaderField *out) const;
    void insert(const HeaderField &field);
    void evictToFit();
    void setMaxTableSize(quint32 size);

    QVector<HeaderField> m_dynamicTable; // most-recently-added at index 0
    quint32 m_maxTableSize;
    quint32 m_tableSize;
};

} // namespace QHttp2

QT_END_NAMESPACE

#endif // QHPACK_P_H
