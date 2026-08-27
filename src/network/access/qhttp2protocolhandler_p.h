// License: Apache 2.0 without attribution need.
//
// A clean-room HTTP/2 (RFC 7540) client protocol handler. It plugs into the same
// QAbstractProtocolHandler machinery the SPDY handler uses: requests are taken
// from the channel's multiplexed queue, each becomes an HTTP/2 stream, and the
// decoded responses are pushed back into their QHttpNetworkReply. No third-party
// code -- the framing and HPACK live in the http2/ helpers.

#ifndef QHTTP2PROTOCOLHANDLER_P_H
#define QHTTP2PROTOCOLHANDLER_P_H

//
//  W A R N I N G
//  -------------
// This file is part of the Network access API. It exists purely as an
// implementation detail and may change from version to version without notice.
//

#include <private/qabstractprotocolhandler_p.h>
#include <private/qhttpnetworkconnection_p.h>

#include "http2/qhttp2frame_p.h"
#include "http2/qhpack_p.h"

#include <QtCore/qobject.h>
#include <QtCore/qhash.h>

QT_BEGIN_NAMESPACE

class QHttp2ProtocolHandler : public QObject, public QAbstractProtocolHandler
{
    Q_OBJECT
public:
    explicit QHttp2ProtocolHandler(QHttpNetworkConnectionChannel *channel);

    void _q_receiveReply() Q_DECL_OVERRIDE;
    void _q_readyRead() Q_DECL_OVERRIDE;
    bool sendRequest() Q_DECL_OVERRIDE;

private:
    Q_DISABLE_COPY(QHttp2ProtocolHandler)

    void sendConnectionPreface();
    void sendSettingsAck();
    bool sendRstStream(quint32 streamID, quint32 errorCode);
    void openStream(const HttpMessagePair &pair);
    void processFrames();

    void handleSETTINGS(const QHttp2::Frame &frame);
    void handleHEADERS(const QHttp2::Frame &frame);
    void handleDATA(const QHttp2::Frame &frame);
    void handleRST_STREAM(const QHttp2::Frame &frame);
    void handleGOAWAY(const QHttp2::Frame &frame);
    void handlePING(const QHttp2::Frame &frame);

    void deliverResponse(quint32 streamID, bool endStream);
    void finishStream(quint32 streamID);
    void connectionError(quint32 errorCode, const char *message);

    QHttp2::HpackEncoder m_encoder;
    QHttp2::HpackDecoder m_decoder;
    QHttp2::FrameReader m_reader;

    QHash<quint32, HttpMessagePair> m_inFlightStreams;
    QHash<quint32, QByteArray> m_headerBlocks;        // stream -> accumulating HEADERS/CONTINUATION
    QHash<quint32, bool> m_endStreamAfterHeaders;     // stream -> END_STREAM seen on HEADERS

    // Client streams are odd, hence m_nextStreamID starts at 1 (set in the ctor).
    quint32 m_nextStreamID;
    quint32 m_peerMaxFrameSize;
    bool m_prefaceSent;
    bool m_goawayReceived;
};

QT_END_NAMESPACE

#endif // QHTTP2PROTOCOLHANDLER_P_H
