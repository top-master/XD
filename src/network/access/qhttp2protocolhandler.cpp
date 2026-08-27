// License: Apache 2.0 without attribution need.

#include "qhttp2protocolhandler_p.h"

#include "qhttpnetworkconnection_p.h"
#include "qhttpnetworkconnectionchannel_p.h"

#include <private/qhttpnetworkreply_p.h>
#include <private/qhttpnetworkrequest_p.h>
#include <private/qnoncontiguousbytedevice_p.h>

#include <QtNetwork/qabstractsocket.h>
#include <QtCore/qurl.h>

QT_BEGIN_NAMESPACE

using namespace QHttp2;

static QByteArray u16(quint16 v)
{
    QByteArray b;
    b.append(char((v >> 8) & 0xff));
    b.append(char(v & 0xff));
    return b;
}

static QByteArray u32(quint32 v)
{
    QByteArray b;
    b.append(char((v >> 24) & 0xff));
    b.append(char((v >> 16) & 0xff));
    b.append(char((v >> 8) & 0xff));
    b.append(char(v & 0xff));
    return b;
}

static quint32 read32(const char *p)
{
    const uchar *u = reinterpret_cast<const uchar *>(p);
    return (quint32(u[0]) << 24) | (quint32(u[1]) << 16) | (quint32(u[2]) << 8) | quint32(u[3]);
}

QHttp2ProtocolHandler::QHttp2ProtocolHandler(QHttpNetworkConnectionChannel *channel)
    : QAbstractProtocolHandler(channel)
    , m_nextStreamID(1)
    , m_peerMaxFrameSize(QHttp2::DefaultFrameSize)
    , m_prefaceSent(false)
    , m_goawayReceived(false)
{
}

void QHttp2ProtocolHandler::_q_receiveReply()
{
    processFrames();
}

void QHttp2ProtocolHandler::_q_readyRead()
{
    processFrames();
}

bool QHttp2ProtocolHandler::sendRequest()
{
    if (m_goawayReceived)
        return false;
    if (!m_prefaceSent)
        sendConnectionPreface();

    const QList<HttpMessagePair> requests = m_channel->spdyRequestsToSend.values();
    m_channel->spdyRequestsToSend.clear();
    for (int i = 0; i < requests.size(); ++i)
        openStream(requests.at(i));
    return true;
}

void QHttp2ProtocolHandler::sendConnectionPreface()
{
    m_socket->write(Preface, PrefaceLength);

    // Our SETTINGS: disable server push, advertise our initial window.
    QByteArray settings;
    settings += u16(SettingEnablePush)        + u32(0);
    settings += u16(SettingInitialWindowSize) + u32(DefaultWindowSize);
    FrameWriter::writeFrame(*m_socket, FrameSettings, FlagNone, ConnectionStreamID, settings);
    m_prefaceSent = true;
}

void QHttp2ProtocolHandler::sendSettingsAck()
{
    FrameWriter::writeFrame(*m_socket, FrameSettings, FlagAck, ConnectionStreamID, QByteArray());
}

bool QHttp2ProtocolHandler::sendRstStream(quint32 streamID, quint32 errorCode)
{
    return FrameWriter::writeFrame(*m_socket, FrameRstStream, FlagNone, streamID, u32(errorCode));
}

void QHttp2ProtocolHandler::openStream(const HttpMessagePair &pair)
{
    const QHttpNetworkRequest &request = pair.first;
    QHttpNetworkReply *reply = pair.second;
    const quint32 streamID = m_nextStreamID;
    m_nextStreamID += 2;

    reply->d_func()->connection = m_connection;
    reply->d_func()->connectionChannel = m_channel;
    // A multiplexed handler ran; the reply-impl maps this onto Http2WasUsed for
    // requests that opted into HTTP/2 (and SpdyWasUsed otherwise).
    reply->setSpdyWasUsed(true);
    m_inFlightStreams.insert(streamID, pair);

    HttpHeaders headers;
    headers.append(HeaderField(":method", request.methodName()));
    headers.append(HeaderField(":path", request.uri(false)));
    headers.append(HeaderField(":scheme", request.url().scheme().toLatin1()));
    headers.append(HeaderField(":authority",
                               request.url().authority(QUrl::FullyEncoded | QUrl::RemoveUserInfo).toLatin1()));
    for (int a = 0; a < request.header().count(); ++a) {
        const QByteArray key = request.header().at(a).first;
        if (key == "Connection" || key == "Host" || key == "Keep-Alive"
                || key == "Proxy-Connection" || key == "Transfer-Encoding")
            continue; // hop-by-hop headers are not allowed in HTTP/2 (RFC 7540, 8.1.2.2)
        headers.append(HeaderField(key.toLower(), request.header().at(a).second));
    }

    const QByteArray block = m_encoder.encode(headers);

    // Best-effort request body: send whatever the upload device currently holds as
    // DATA. Adequate for the small in-memory bodies these tests use; a GET carries
    // none, so END_STREAM rides on the HEADERS frame.
    QByteArray body;
    if (QNonContiguousByteDevice *device = request.uploadByteDevice()) {
        qint64 size = 0;
        while (const char *p = device->readPointer(-1, size)) {
            if (size <= 0)
                break;
            body.append(p, int(size));
            device->advanceReadPointer(size);
        }
    }

    quint8 headerFlags = FlagEndHeaders;
    if (body.isEmpty())
        headerFlags |= FlagEndStream;
    FrameWriter::writeFrame(*m_socket, FrameHeaders, headerFlags, streamID, block);

    if (!body.isEmpty())
        FrameWriter::writeData(*m_socket, streamID, body, m_peerMaxFrameSize, /*endStream*/ true);

    reply->d_func()->state = QHttpNetworkReplyPrivate::SPDYHalfClosed;
}

void QHttp2ProtocolHandler::processFrames()
{
    // The client connection preface MUST be the very first thing we send (RFC 7540,
    // 3.5) -- before we ACK the peer's SETTINGS or send anything else. _q_readyRead
    // can fire before sendRequest(), so guard it here too.
    if (!m_prefaceSent)
        sendConnectionPreface();

    for (;;) {
        Frame frame;
        bool protocolError = false;
        if (!m_reader.read(*m_socket, frame, &protocolError)) {
            if (protocolError)
                connectionError(ProtocolError, "invalid frame");
            return;
        }

        switch (frame.type) {
        case FrameSettings:      handleSETTINGS(frame); break;
        case FrameHeaders:
        case FrameContinuation:  handleHEADERS(frame); break;
        case FrameData:          handleDATA(frame); break;
        case FrameRstStream:     handleRST_STREAM(frame); break;
        case FrameGoaway:        handleGOAWAY(frame); break;
        case FramePing:          handlePING(frame); break;
        case FramePushPromise:   sendRstStream(frame.streamID, RefusedStream); break;
        case FrameWindowUpdate:  /* our client is receive-mostly; nothing to do */ break;
        case FramePriority:      break;
        default:                 break;
        }
        if (m_goawayReceived && m_inFlightStreams.isEmpty())
            return;
    }
}

void QHttp2ProtocolHandler::handleSETTINGS(const QHttp2::Frame &frame)
{
    if (frame.flag(FlagAck))
        return; // our SETTINGS were acknowledged

    const char *p = frame.payload.constData();
    int n = frame.payload.size();
    for (int i = 0; i + 6 <= n; i += 6) {
        const quint16 id = (quint16(quint8(p[i])) << 8) | quint8(p[i + 1]);
        const quint32 value = read32(p + i + 2);
        if (id == SettingMaxFrameSize && value >= DefaultFrameSize && value <= MaxAllowedFrameSize)
            m_peerMaxFrameSize = value;
    }
    sendSettingsAck();
}

void QHttp2ProtocolHandler::handleHEADERS(const QHttp2::Frame &frame)
{
    const quint32 streamID = frame.streamID;
    if (streamID == ConnectionStreamID)
        return connectionError(ProtocolError, "HEADERS on stream 0");

    m_headerBlocks[streamID].append(frame.payload);
    if (frame.type == FrameHeaders && frame.flag(FlagEndStream))
        m_endStreamAfterHeaders[streamID] = true;

    if (frame.flag(FlagEndHeaders))
        deliverResponse(streamID, m_endStreamAfterHeaders.value(streamID, false));
}

void QHttp2ProtocolHandler::deliverResponse(quint32 streamID, bool endStream)
{
    const QByteArray block = m_headerBlocks.take(streamID);
    m_endStreamAfterHeaders.remove(streamID);

    if (!m_inFlightStreams.contains(streamID))
        return;

    HttpHeaders headers;
    if (!m_decoder.decode(block, &headers))
        return connectionError(CompressionError, "HPACK decoding failed");

    QHttpNetworkReply *reply = m_inFlightStreams.value(streamID).second;
    QHttpNetworkReplyPrivate *priv = reply->d_func();

    for (int i = 0; i < headers.size(); ++i) {
        const QByteArray &name = headers.at(i).name;
        const QByteArray &value = headers.at(i).value;
        if (name == ":status") {
            reply->setStatusCode(value.left(3).toInt());
            priv->reasonPhrase.clear();
        } else if (!name.isEmpty() && name.at(0) != ':') {
            reply->setHeaderField(name, value);
        }
    }
    priv->majorVersion = 2;
    priv->minorVersion = 0;

    emit reply->headerChanged();

    if (endStream)
        finishStream(streamID);
}

void QHttp2ProtocolHandler::handleDATA(const QHttp2::Frame &frame)
{
    const quint32 streamID = frame.streamID;
    if (!m_inFlightStreams.contains(streamID))
        return; // stream already closed/unknown

    QHttpNetworkReply *reply = m_inFlightStreams.value(streamID).second;
    QHttpNetworkReplyPrivate *priv = reply->d_func();

    if (!frame.payload.isEmpty()) {
        priv->compressedData.append(frame.payload);
        priv->totalProgress += frame.payload.size();

        const QHttpNetworkRequest &request = m_inFlightStreams.value(streamID).first;
        if (request.d->autoDecompress && priv->isCompressed()) {
            QByteDataBuffer in;
            in.append(frame.payload);
            priv->uncompressBodyData(&in, &priv->responseData);
        } else {
            priv->responseData.append(frame.payload);
        }

        if (priv->shouldEmitSignals()) {
            emit reply->readyRead();
            emit reply->dataReadProgress(priv->totalProgress, priv->bodyLength);
        }

        // Replenish flow control so downloads larger than the initial window do
        // not stall (RFC 7540, 6.9).
        const QByteArray delta = u32(quint32(frame.payload.size()));
        FrameWriter::writeFrame(*m_socket, FrameWindowUpdate, FlagNone, ConnectionStreamID, delta);
        FrameWriter::writeFrame(*m_socket, FrameWindowUpdate, FlagNone, streamID, delta);
    }

    if (frame.flag(FlagEndStream))
        finishStream(streamID);
}

void QHttp2ProtocolHandler::handleRST_STREAM(const QHttp2::Frame &frame)
{
    const quint32 streamID = frame.streamID;
    if (!m_inFlightStreams.contains(streamID))
        return;
    QHttpNetworkReply *reply = m_inFlightStreams.value(streamID).second;
    reply->d_func()->state = QHttpNetworkReplyPrivate::SPDYClosed;
    reply->disconnect(this);
    m_inFlightStreams.remove(streamID);
    m_headerBlocks.remove(streamID);
    m_endStreamAfterHeaders.remove(streamID);
    emit reply->finishedWithError(QNetworkReply::ProtocolFailure, tr("HTTP/2 stream reset by peer"));
}

void QHttp2ProtocolHandler::handleGOAWAY(const QHttp2::Frame &frame)
{
    Q_UNUSED(frame);
    m_goawayReceived = true;
}

void QHttp2ProtocolHandler::handlePING(const QHttp2::Frame &frame)
{
    if (frame.flag(FlagAck))
        return;
    // Echo the 8-octet opaque data with the ACK flag set (RFC 7540, 6.7).
    FrameWriter::writeFrame(*m_socket, FramePing, FlagAck, ConnectionStreamID, frame.payload);
}

void QHttp2ProtocolHandler::finishStream(quint32 streamID)
{
    if (!m_inFlightStreams.contains(streamID))
        return;
    QHttpNetworkReply *reply = m_inFlightStreams.value(streamID).second;
    reply->d_func()->state = QHttpNetworkReplyPrivate::SPDYClosed;
    reply->disconnect(this);
    m_inFlightStreams.remove(streamID);
    m_headerBlocks.remove(streamID);
    m_endStreamAfterHeaders.remove(streamID);
    // TRACE/network http-reply dangling-connection: clear the reply's connection on finish #1,
    // A finished stream's reply leaves m_inFlightStreams but is destroyed later (deleteLater),
    // possibly after the connection is gone. Its connection back-pointer is a QPointer, but on
    // a torn-down / cross-thread path that pointer is not always cleared in time, so
    // ~QHttpNetworkReply would call removeReply() into freed connection storage (a Fil-C
    // use-after-free; a garbage read natively). The reply is done talking to the connection
    // now, so drop the link here, on the connection's own thread, before handing the reply on.
    // Cleared before emit finished() because a slot may delete the reply during that emit.
    reply->d_func()->connection = 0;
    emit reply->finished();
}

void QHttp2ProtocolHandler::connectionError(quint32 errorCode, const char *message)
{
    QByteArray payload = u32(ConnectionStreamID) + u32(errorCode);
    FrameWriter::writeFrame(*m_socket, FrameGoaway, FlagNone, ConnectionStreamID, payload);
    m_goawayReceived = true;
    m_channel->emitFinishedWithError(QNetworkReply::ProtocolFailure, message);
    m_socket->close();
}

QT_END_NAMESPACE
