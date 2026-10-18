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

#include <private/qabstractprotocolhandler_p.h>
#include <private/qhttpnetworkconnectionchannel_p.h>
#include <private/qhttpnetworkreply_p.h>

#ifndef QT_NO_HTTP

QT_BEGIN_NAMESPACE

/*!
    \class QProtocolRequestTracker
    \internal

    Files the requests in flight on a handler that asks for it with
    QAbstractProtocolHandler::TrackRepliesFlag, as the set of their replies.
*/

/*!
    \fn const QProtocolRequestTracker::Requests &QProtocolRequestTracker::toSet() const
    \internal

    Hands out the replies in flight as the set they are.
*/

/*!
    \fn QProtocolRequestTracker::Requests &QProtocolRequestTracker::toSet()
    \internal

    Hands out the replies in flight as the set they are, to change.
*/

/*!
    \fn void QProtocolRequestTracker::add(QHttpNetworkReply *reply)
    \internal

    Files the request of \a reply.
*/

/*!
    \fn void QProtocolRequestTracker::remove(QHttpNetworkReply *reply)
    \internal

    Drops the request of \a reply, which left the handler; does nothing when
    it is not filed.
*/

/*!
    \internal

    Drops the request of \a reply, which is being destroyed; hence only its
    address is used.
*/
void QProtocolRequestTracker::removeDestroyed(QObject *reply)
{
    remove(static_cast<QHttpNetworkReply * >(reply));
}

/*!
    \fn void QProtocolRequestTracker::clear()
    \internal

    Drops every request still filed.
*/

QAbstractProtocolHandler::QAbstractProtocolHandler(QHttpNetworkConnectionChannel *channel)
    : QObject(0)
    , m_channel(channel)
    , m_reply(0)
    , m_socket(m_channel->socket)
    , m_connection(m_channel->connection)
{
    Q_ASSERT(m_channel);
    Q_ASSERT(m_socket);
    Q_ASSERT(m_connection);
}

QAbstractProtocolHandler::~QAbstractProtocolHandler()
{
    QAbstractProtocolHandler::untieReplies();
}

/*!
    \internal

    Unties every request still in flight on this handler from its connection
    and channel. The destructor calls it, and an owner may call it earlier, at
    the start of its own teardown; it does its work once, until attachReply()
    ties another reply.

    A handler that overrides it calls this base version from its override, and
    calls the override from its own destructor: the base destructor runs only
    this base version, since the handler's part is gone by then, and skips it
    once it ran.
*/
void QAbstractProtocolHandler::untieReplies()
{
    if (m_flags.includesLax(RepliesUntiedFlag)) {
        return;
    }
    // TRACE/network http-reply dangling-connection: untie the replies a handler holds #2,
    // a request still in flight outlives this handler, and the channel goes
    // with it; hence its reply drops both back-pointers here, whatever order
    // the owner tears down in.
    typedef QProtocolRequestTracker::Requests Requests;
    const Requests &requests = m_inFlightRequests.toSet();
    Requests::const_iterator it = requests.constBegin();
    for (; it != requests.constEnd(); ++it) {
        untieReply(*it);
    }
    m_inFlightRequests.clear();
    m_flags.appendLax(RepliesUntiedFlag);
}

/*!
    \internal

    Ties \a reply to this handler's connection and channel while its request
    runs; detachReply() unties it once the request ends, and untieReplies()
    unties every request still in flight (the destructor calls it at the
    latest). A handler that overrides it calls this base version too, which
    routes the reply's death to _q_replyDestroyed(), and files the request
    under TrackRepliesFlag.
*/
void QAbstractProtocolHandler::attachReply(QHttpNetworkReply *reply)
{
    reply->d_func()->connection = m_connection;
    reply->d_func()->connectionChannel = m_channel;
    m_flags.removeLax(RepliesUntiedFlag);
    connect(reply, &QObject::destroyed,
            this, &QAbstractProtocolHandler::_q_replyDestroyed,
            Qt::UniqueConnection);
    if (m_flags.includesLax(TrackRepliesFlag)) {
        m_inFlightRequests.add(reply);
    }
}

/*!
    \internal

    Unties \a reply from this handler's connection once its request ended,
    before the handler announces the end. The channel back-pointer stays,
    since a finished reply still reads its SSL details through it. A handler
    that overrides it calls this base version too, which stops hearing the
    reply's death and drops the request from the in-flight ones.
*/
void QAbstractProtocolHandler::detachReply(QHttpNetworkReply *reply)
{
    // TRACE/network http-reply dangling-connection: untie the replies a handler holds #1,
    // a finished reply lives on with its owner, often past this connection,
    // and its destructor calls `removeReply()` on the connection it still
    // names. Untying it here, on the connection's own thread, keeps that call
    // out of a connection being torn down; it comes before the handler emits
    // the end, since a slot may delete the reply.
    reply->d_func()->connection = 0;
    disconnect(reply, &QObject::destroyed,
               this, &QAbstractProtocolHandler::_q_replyDestroyed);
    m_inFlightRequests.remove(reply);
}

/*!
    \internal

    Drops the request of \a reply, which its owner destroyed while it was in
    flight; every reply attachReply() tied here reaches it. A handler that keeps
    state of its own per request overrides it, drops that state, and calls this
    base version too.

    The reply is already being destroyed; hence only its address is used.
*/
void QAbstractProtocolHandler::_q_replyDestroyed(QObject *reply)
{
    m_inFlightRequests.removeDestroyed(reply);
}

/*!
    \internal

    Drops both back-pointers of \a reply, its connection and its channel.
*/
void QAbstractProtocolHandler::untieReply(QHttpNetworkReply *reply)
{
    reply->d_func()->connection = 0;
    reply->d_func()->connectionChannel = 0;
}

/*!
    \fn template <typename Streams> void QAbstractProtocolHandler::untieStreams(const Streams &streams)
    \internal

    Unties the reply of each request in \a streams, a map of the requests a
    handler keeps in flight itself (an HttpMessagePair per stream).
*/

void QAbstractProtocolHandler::setReply(QHttpNetworkReply *reply)
{
    m_reply = reply;
}

QT_END_NAMESPACE

#endif // QT_NO_HTTP
