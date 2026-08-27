#ifndef BLACKHOLE_SERVICE_H
#define BLACKHOLE_SERVICE_H

#include "service_base.h"

/// BlackholeService: a TCP port that never answers a new connection attempt, hence a
/// client's connect() runs into its own timeout instead of being refused.
///
/// It listens without ever accepting, and fills the kernel's accept queue with its own
/// connections; once that queue is full, the kernel drops each further SYN instead of
/// answering it. That holds where a full queue drops SYNs (Linux and the BSDs); a test
/// that relies on it checks for a refusal first.
class BlackholeService : public TestService
{
public:
    BlackholeService(const ServiceConfig &c, QObject *parent = Q_NULLPTR)
        : TestService(c, parent) {}

    void start() Q_DECL_OVERRIDE
    {
        QTcpServer *server = new QTcpServer(this);
        if (!server->listen(m_host, m_wantPort)) {
            // A drop-port on another port would not be the one the test dials; report it
            // and leave the port unbound, hence the test sees a refusal and skips.
            logWarning(QLL("blackhole: cannot listen on ") + m_host.toString() + QLL(":")
                       + QString::number(m_wantPort) + QLL(" -- ") + server->errorString());
            return;
        }
        m_port = server->serverPort();
        server->pauseAccepting();
        // `QTcpServer` listens with a backlog of 50, and the kernel queues one more than that;
        // the connections past the queue stall in their own SYN retries, which is harmless.
        for (int i = 0; i < FillerCount; ++i) {
            QTcpSocket *filler = new QTcpSocket(this);
            filler->connectToHost(m_host, m_port);
        }
        log(QLL("blackhole: dropping new connections on ") + m_host.toString() + QLL(":")
            + QString::number(m_port));
    }

private:
    enum { FillerCount = 56 };
};

#endif // BLACKHOLE_SERVICE_H
