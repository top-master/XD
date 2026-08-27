// License: Apache 2.0 without attribution need.
//
// A clean-room HTTP/2 (RFC 7540) framing layer: the fixed 9-octet frame header
// plus helpers to read one frame from a socket and to serialize frames out. No
// third-party code -- written from the RFC.

#ifndef QHTTP2FRAME_P_H
#define QHTTP2FRAME_P_H

//
//  W A R N I N G
//  -------------
// This file is part of the Network access API. It exists purely as an
// implementation detail and may change from version to version without notice.
//

#include <QtCore/qbytearray.h>
#include <QtCore/qglobal.h>

QT_BEGIN_NAMESPACE

class QAbstractSocket;

namespace QHttp2
{
// The 24-octet client connection preface (RFC 7540, 3.5).
extern const char Preface[];
enum { PrefaceLength = 24 };

// Protocol defaults (RFC 7540, 6.5.2 / 6.9.1).
enum {
    FrameHeaderSize     = 9,
    DefaultFrameSize    = 16384,
    MaxAllowedFrameSize = 16777215,
    DefaultWindowSize   = 65535,
    ConnectionStreamID  = 0
};

enum FrameType : quint8 {
    FrameData         = 0x0,
    FrameHeaders      = 0x1,
    FramePriority     = 0x2,
    FrameRstStream    = 0x3,
    FrameSettings     = 0x4,
    FramePushPromise  = 0x5,
    FramePing         = 0x6,
    FrameGoaway       = 0x7,
    FrameWindowUpdate = 0x8,
    FrameContinuation = 0x9
};

// Flag bits are per-frame-type; the numeric values are shared (RFC 7540, 6.*).
enum FrameFlag : quint8 {
    FlagNone       = 0x0,
    FlagAck        = 0x1, // SETTINGS, PING
    FlagEndStream  = 0x1, // DATA, HEADERS
    FlagEndHeaders = 0x4, // HEADERS, PUSH_PROMISE, CONTINUATION
    FlagPadded     = 0x8, // DATA, HEADERS, PUSH_PROMISE
    FlagPriority   = 0x20 // HEADERS
};

enum SettingId : quint16 {
    SettingHeaderTableSize      = 0x1,
    SettingEnablePush           = 0x2,
    SettingMaxConcurrentStreams = 0x3,
    SettingInitialWindowSize    = 0x4,
    SettingMaxFrameSize         = 0x5,
    SettingMaxHeaderListSize    = 0x6
};

enum ErrorCode : quint32 {
    NoError            = 0x0,
    ProtocolError      = 0x1,
    InternalError      = 0x2,
    FlowControlError   = 0x3,
    SettingsTimeout    = 0x4,
    StreamClosed       = 0x5,
    FrameSizeError     = 0x6,
    RefusedStream      = 0x7,
    Cancel             = 0x8,
    CompressionError   = 0x9,
    ConnectError       = 0xa,
    EnhanceYourCalm    = 0xb,
    InadequateSecurity = 0xc,
    Http11Required     = 0xd
};

// A decoded inbound frame. The payload is already stripped of any padding.
struct Frame
{
    inline Frame()
        : type(FrameData)
        , flags(FlagNone)
        , streamID(0)
    {}

    FrameType type;
    quint8 flags;
    quint32 streamID;
    QByteArray payload; // frame payload without the 9-octet header (and no padding)

    bool flag(FrameFlag f) const { return (flags & quint8(f)) != 0; }
};

// Reads whole frames out of a socket's buffer incrementally.
class FrameReader
{
public:
    // Tries to read one complete frame. Returns true and fills 'frame' when a
    // full frame is available; returns false (leaving the partial data buffered)
    // when more bytes are needed. Sets 'error' on a malformed frame.
    bool read(QAbstractSocket &socket, Frame &frame, bool *protocolError);

private:
    QByteArray m_buffer;
};

// Serializes and writes a single frame, splitting DATA payloads that exceed the
// peer's maximum frame size into several DATA frames.
class FrameWriter
{
public:
    static bool writeFrame(QAbstractSocket &socket, FrameType type, quint8 flags,
                           quint32 streamID, const QByteArray &payload);
    // Writes 'data' as one or more DATA frames of at most 'maxFrameSize' octets;
    // END_STREAM is set only on the final frame when 'endStream' is true.
    static bool writeData(QAbstractSocket &socket, quint32 streamID,
                          const QByteArray &data, quint32 maxFrameSize, bool endStream);
};

} // namespace QHttp2

QT_END_NAMESPACE

#endif // QHTTP2FRAME_P_H
