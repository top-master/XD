// License: Apache 2.0 without attribution need.
#ifndef HTTPS_HTTP2_H
#define HTTPS_HTTP2_H

#include <QtNetwork/QSslSocket>
#include <QtCore/QByteArray>
#include <QtCore/QFile>
#include <QtCore/QObject>
#include <QtCore/QString>

// The clean-room (Apache 2.0) HTTP/2 framing + HPACK the client also uses.
#include <private/qhttp2frame_p.h>
#include <private/qhpack_p.h>

// A minimal HTTP/2 ("h2") server for tst_http2. It completes the connection
// preface, answers SETTINGS, and for each client HEADERS frame decodes the
// :path and replies with a response HEADERS + DATA frame carrying the file (or a
// small default page), ending the stream.
class Http2Connection : public QObject
{
    Q_OBJECT
public:
    Http2Connection(QSslSocket *socket, const QString &root, QObject *parent = Q_NULLPTR)
        : QObject(parent), m_socket(socket), m_root(root), m_prefaceConsumed(false)
    {
        connect(m_socket, &QSslSocket::readyRead, this, &Http2Connection::onReadyRead);
        connect(m_socket, &QSslSocket::disconnected, this, &QObject::deleteLater);
        sendServerSettings();
    }

private slots:
    void onReadyRead()
    {
        using namespace QHttp2;

        // The 24-byte client connection preface precedes any frame (RFC 7540, 3.5).
        if (!m_prefaceConsumed) {
            if (m_socket->bytesAvailable() < PrefaceLength)
                return;
            const QByteArray magic = m_socket->read(PrefaceLength);
            if (magic != QByteArray(Preface, PrefaceLength)) {
                m_socket->disconnectFromHost();
                return;
            }
            m_prefaceConsumed = true;
        }

        for (;;) {
            Frame frame;
            bool protocolError = false;
            if (!m_reader.read(*m_socket, frame, &protocolError)) {
                if (protocolError)
                    m_socket->disconnectFromHost();
                return;
            }
            handleFrame(frame);
        }
    }

private:
    void sendServerSettings()
    {
        using namespace QHttp2;
        // Empty SETTINGS -- the defaults suit this server.
        FrameWriter::writeFrame(*m_socket, FrameSettings, FlagNone, ConnectionStreamID, QByteArray());
    }

    void sendSettingsAck()
    {
        using namespace QHttp2;
        FrameWriter::writeFrame(*m_socket, FrameSettings, FlagAck, ConnectionStreamID, QByteArray());
    }

    void handleFrame(QHttp2::Frame &frame)
    {
        using namespace QHttp2;
        switch (frame.type) {
        case FrameSettings:
            if (!frame.flag(FlagAck))
                sendSettingsAck();
            break;
        case FrameHeaders:
            respond(frame);
            break;
        default:
            break; // WINDOW_UPDATE / PRIORITY / RST_STREAM / PING / client DATA
        }
    }

    void respond(QHttp2::Frame &frame)
    {
        using namespace QHttp2;

        const quint32 streamID = frame.streamID;

        QByteArray path("/");
        HttpHeaders requestHeaders;
        if (m_decoder.decode(frame.payload, &requestHeaders)) {
            for (int i = 0; i < requestHeaders.size(); ++i) {
                if (requestHeaders.at(i).name == ":path")
                    path = requestHeaders.at(i).value;
            }
        }

        const QByteArray body = bodyFor(path);

        HttpHeaders header;
        header.append(HeaderField(":status", "200"));
        header.append(HeaderField("content-type", "text/html"));
        header.append(HeaderField("content-length", QByteArray::number(body.size())));
        const QByteArray block = m_encoder.encode(header);

        FrameWriter::writeFrame(*m_socket, FrameHeaders, FlagEndHeaders, streamID, block);
        // writeData splits into frames within the limit and sets END_STREAM last.
        FrameWriter::writeData(*m_socket, streamID, body, DefaultFrameSize, /*endStream*/ true);
    }

    QByteArray bodyFor(const QByteArray &path)
    {
        QString rel = QString::fromUtf8(path);
        const int q = rel.indexOf(QLatin1Char('?'));
        if (q >= 0)
            rel = rel.left(q);
        // Files are served under the "/qtest/" prefix, as buildReply() does for
        // HTTP/1 and SPDY.
        if (rel.startsWith(QLatin1String("/qtest/"))) {
            QFile f(m_root + QLatin1Char('/') + rel.mid(int(sizeof("/qtest/") - 1)));
            if (f.open(QIODevice::ReadOnly))
                return f.readAll();
        }
        return QByteArray("<html><body>hello http/2</body></html>");
    }

    QSslSocket *m_socket;
    QString m_root;
    bool m_prefaceConsumed;
    QHttp2::FrameReader m_reader;
    QHttp2::HpackDecoder m_decoder;
    QHttp2::HpackEncoder m_encoder;
};

#endif // HTTPS_HTTP2_H
