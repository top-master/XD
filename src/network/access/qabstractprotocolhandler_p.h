/****************************************************************************
**
** Copyright (C) 2014 BlackBerry Limited. All rights reserved.
** Contact: http://www.qt.io/licensing/
**
** This file is part of the QtNetwork module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL21$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see http://www.qt.io/terms-conditions. For further
** information use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file. Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** As a special exception, The Qt Company gives you certain additional
** rights. These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef QABSTRACTPROTOCOLHANDLER_H
#define QABSTRACTPROTOCOLHANDLER_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists for the convenience
// of the Network Access API.  This header file may change from
// version to version without notice, or even be removed.
//
// We mean it.
//

#ifndef QT_NO_HTTP

#include <QtCore/qglobal.h>
#include <QtCore/qatomicflags.h>
#include <QtCore/qset.h>
#include <QtCore/qobject.h>
#include <QtCore/qpointer.h>


QT_BEGIN_NAMESPACE

class QHttpNetworkConnectionChannel;
class QHttpNetworkReply;
class QAbstractSocket;
class QHttpNetworkConnection;
class QAbstractProtocolHandler;

class QProtocolRequestTracker : private QSet<QHttpNetworkReply *>
{
    typedef QSet<QHttpNetworkReply *> super;
public:
    typedef super Requests;

    Q_ALWAYS_INLINE const Requests &toSet() const { return *this; }
    Q_ALWAYS_INLINE Requests &toSet() { return *this; }

    Q_ALWAYS_INLINE void add(QHttpNetworkReply *reply);
    Q_ALWAYS_INLINE void remove(QHttpNetworkReply *reply);
    void removeDestroyed(QObject *reply);
    Q_ALWAYS_INLINE void clear();
};

class QAbstractProtocolHandler : public QObject
{
    Q_OBJECT
    typedef QObject super;
public:
    /// Names the bits of m_flags; a subclass adds its own from UserFlag on.
    enum HandlerFlag {
        /// Marks that untieReplies() ran since the last attachReply().
        RepliesUntiedFlag = 0x1,
        /// Asks attachReply() to file each request in m_inFlightRequests; off
        /// unless a subclass that needs the base to track them turns it on.
        TrackRepliesFlag = 0x2,
        /// Starts the bits a subclass may use for flags of its own.
        UserFlag = 0x10000
    };

    QAbstractProtocolHandler(QHttpNetworkConnectionChannel *channel);
    virtual ~QAbstractProtocolHandler();

    virtual void _q_receiveReply() = 0;
    virtual void _q_readyRead() = 0;
    virtual bool sendRequest() = 0;
    void setReply(QHttpNetworkReply *reply);
    virtual void untieReplies();

protected Q_SLOTS:
    virtual void _q_replyDestroyed(QObject *reply);

protected:
    virtual void attachReply(QHttpNetworkReply *reply);
    virtual void detachReply(QHttpNetworkReply *reply);
    void untieReply(QHttpNetworkReply *reply);
    template <typename Streams>
    Q_ALWAYS_INLINE void untieStreams(const Streams &streams);

    QPointer<QHttpNetworkConnectionChannel> m_channel;
    QPointer<QHttpNetworkReply> m_reply;
    QPointer<QAbstractSocket> m_socket;
    QPointer<QHttpNetworkConnection> m_connection;
    /// Files each request attachReply() tied here, under TrackRepliesFlag, and
    /// drops it once its reply dies; untieReplies() unties those still in flight.
    QProtocolRequestTracker m_inFlightRequests;
    /// Marks the handler's states by HandlerFlag bits, and by a subclass's own.
    QAtomicFlags<int > m_flags;
};

#ifndef Q_MOC_RUN
// MARK: Inlined at each call site.

Q_ALWAYS_INLINE void QProtocolRequestTracker::add(QHttpNetworkReply *reply)
{
    this->insert(reply);
}

Q_ALWAYS_INLINE void QProtocolRequestTracker::remove(QHttpNetworkReply *reply)
{
    super::remove(reply);
}

Q_ALWAYS_INLINE void QProtocolRequestTracker::clear()
{
    super::clear();
}

template <typename Streams>
Q_ALWAYS_INLINE void QAbstractProtocolHandler::untieStreams(const Streams &streams)
{
    typename Streams::const_iterator it = streams.constBegin();
    for (; it != streams.constEnd(); ++it) {
        if (it.value().second) {
            this->untieReply(it.value().second);
        }
    }
}
#endif // Q_MOC_RUN

QT_END_NAMESPACE

#endif // QT_NO_HTTP

#endif // QABSTRACTPROTOCOLHANDLER_H
