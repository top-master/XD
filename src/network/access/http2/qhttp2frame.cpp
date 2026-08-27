// License: Apache 2.0 without attribution need.

#include "qhttp2frame_p.h"

#include <QtNetwork/qabstractsocket.h>

QT_BEGIN_NAMESPACE

namespace QHttp2
{

const char Preface[] = "PRI * HTTP/2.0\r\n\r\nSM\r\n\r\n";

static quint32 readUint24(const uchar *p)
{
    return (quint32(p[0]) << 16) | (quint32(p[1]) << 8) | quint32(p[2]);
}

static quint32 readUint31(const uchar *p)
{
    return ((quint32(p[0]) & 0x7f) << 24) | (quint32(p[1]) << 16)
            | (quint32(p[2]) << 8) | quint32(p[3]);
}

bool FrameReader::read(QAbstractSocket &socket, Frame &frame, bool *protocolError)
{
    if (protocolError)
        *protocolError = false;

    if (socket.bytesAvailable() > 0)
        m_buffer.append(socket.readAll());

    if (m_buffer.size() < FrameHeaderSize)
        return false;

    const uchar *hdr = reinterpret_cast<const uchar *>(m_buffer.constData());
    const quint32 length = readUint24(hdr);
    if (length > MaxAllowedFrameSize) {
        if (protocolError)
            *protocolError = true;
        return false;
    }
    if (quint32(m_buffer.size()) < FrameHeaderSize + length)
        return false; // wait for the rest of the payload

    frame.type = FrameType(hdr[3]);
    frame.flags = hdr[4];
    frame.streamID = readUint31(hdr + 5);

    QByteArray payload = m_buffer.mid(FrameHeaderSize, int(length));
    m_buffer.remove(0, int(FrameHeaderSize + length));

    // Strip padding for the frame types that carry it.
    if ((frame.type == FrameData || frame.type == FrameHeaders)
            && (frame.flags & FlagPadded)) {
        if (payload.isEmpty()) {
            if (protocolError)
                *protocolError = true;
            return false;
        }
        const int padLength = quint8(payload.at(0));
        payload.remove(0, 1);
        if (padLength > payload.size()) {
            if (protocolError)
                *protocolError = true;
            return false;
        }
        payload.chop(padLength);
    }

    // A HEADERS frame may be prefixed by a 5-octet priority block.
    if (frame.type == FrameHeaders && (frame.flags & FlagPriority)) {
        if (payload.size() < 5) {
            if (protocolError)
                *protocolError = true;
            return false;
        }
        payload.remove(0, 5);
    }

    frame.payload = payload;
    return true;
}

bool FrameWriter::writeFrame(QAbstractSocket &socket, FrameType type, quint8 flags,
                             quint32 streamID, const QByteArray &payload)
{
    const int length = payload.size();
    char hdr[FrameHeaderSize];
    hdr[0] = char((length >> 16) & 0xff);
    hdr[1] = char((length >> 8) & 0xff);
    hdr[2] = char(length & 0xff);
    hdr[3] = char(type);
    hdr[4] = char(flags);
    hdr[5] = char((streamID >> 24) & 0x7f);
    hdr[6] = char((streamID >> 16) & 0xff);
    hdr[7] = char((streamID >> 8) & 0xff);
    hdr[8] = char(streamID & 0xff);

    if (socket.write(hdr, FrameHeaderSize) != FrameHeaderSize)
        return false;
    if (length && socket.write(payload) != length)
        return false;
    return true;
}

bool FrameWriter::writeData(QAbstractSocket &socket, quint32 streamID,
                            const QByteArray &data, quint32 maxFrameSize, bool endStream)
{
    if (maxFrameSize == 0 || maxFrameSize > MaxAllowedFrameSize)
        maxFrameSize = DefaultFrameSize;

    int offset = 0;
    do {
        const int chunk = qMin(int(maxFrameSize), data.size() - offset);
        const bool last = (offset + chunk >= data.size());
        const quint8 flags = (last && endStream) ? FlagEndStream : FlagNone;
        if (!writeFrame(socket, FrameData, flags, streamID, data.mid(offset, chunk)))
            return false;
        offset += chunk;
    } while (offset < data.size());

    return true;
}

} // namespace QHttp2

QT_END_NAMESPACE
