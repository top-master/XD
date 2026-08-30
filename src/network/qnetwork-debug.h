/****************************************************************************
**
** Copyright (C) 2015 The XD Company Ltd.
** Copyright (C) 2015 The Qt Company Ltd.
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

#ifndef QNETWORK_DEBUG_H
#define QNETWORK_DEBUG_H

#include <QtCore/qdebug.h>
#include <QtCore/qformatter.h>

// One switchboard for every QtNetwork debug trace: uncomment a switch below to turn that layer's
// qDebug() output on, so you never open a .cpp to hunt down its switch. These qDebug_* switches live
// ONLY here; each maps (further down) to the internal guard macro its layer already #ifdef's on, and
// the per-layer trace macros that expand to qDebug() are defined here too -- so no .cpp spells
// qDebug() itself.

// -- HTTP --
//#define qDebug_http_connectionChannel
//#define qDebug_http_connection
//#define qDebug_http_socketEngine
//#define qDebug_http_backend
//#define qDebug_http_threadDelegate
// -- Sockets --
//#define qDebug_abstractSocket
//#define qDebug_nativeSocketEngine
//#define qDebug_tcpSocket
//#define qDebug_udpSocket
//#define qDebug_tcpServer
//#define qDebug_socks5
//#define qDebug_tcpSocketEngine
//#define qDebug_localServer
// -- SSL / DNS --
//#define qDebug_ssl
//#define qDebug_hostInfo
//#define qDebug_dnsLookup
// -- FTP / cache / cookies --
//#define qDebug_ftp_pi
//#define qDebug_ftp_dtp
//#define qDebug_diskCache
//#define qDebug_cacheBackend
//#define qDebug_cookie

// Map each switch above to the internal guard macro the layer's sources test with #ifdef.
#ifdef qDebug_http_connectionChannel
#  define QHTTPNETWORKCONNECTIONCHANNEL_DEBUG
#endif
#ifdef qDebug_http_connection
#  define QHTTPNETWORKCONNECTION_DEBUG
#endif
#ifdef qDebug_http_socketEngine
#  define QHTTPSOCKETENGINE_DEBUG
#endif
#ifdef qDebug_http_backend
#  define QNETWORKACCESSHTTPBACKEND_DEBUG
#endif
#ifdef qDebug_http_threadDelegate
#  define QHTTPTHREADDELEGATE_DEBUG
#endif
#ifdef qDebug_abstractSocket
#  define QABSTRACTSOCKET_DEBUG
#endif
#ifdef qDebug_nativeSocketEngine
#  define QNATIVESOCKETENGINE_DEBUG
#endif
#ifdef qDebug_tcpSocket
#  define QTCPSOCKET_DEBUG
#endif
#ifdef qDebug_udpSocket
#  define QUDPSOCKET_DEBUG
#endif
#ifdef qDebug_tcpServer
#  define QTCPSERVER_DEBUG
#endif
#ifdef qDebug_socks5
#  define QSOCKS5SOCKETLAYER_DEBUG
#endif
#ifdef qDebug_ssl
#  define QSSLSOCKET_DEBUG
#endif
#ifdef qDebug_hostInfo
#  define QHOSTINFO_DEBUG
#endif
#ifdef qDebug_ftp_pi
#  define QFTPPI_DEBUG
#endif
#ifdef qDebug_ftp_dtp
#  define QFTPDTP_DEBUG
#endif
#ifdef qDebug_diskCache
#  define QNETWORKDISKCACHE_DEBUG
#endif
#ifdef qDebug_cacheBackend
#  define QNETWORKACCESSCACHEBACKEND_DEBUG
#endif
#ifdef qDebug_tcpSocketEngine
#  define QTCPSOCKETENGINE_DEBUG
#endif
#ifdef qDebug_localServer
#  define QLOCALSERVER_DEBUG
#endif
#ifdef qDebug_dnsLookup
#  define QDNSLOOKUP_DEBUG
#endif
#ifdef qDebug_cookie
#  define PARSEDATESTRINGDEBUG
#endif

// Per-layer trace macros: each expands to a live qDebug() with the class prefix when its guard above
// is defined, or to a compiled-out `if (0) qDebug()` otherwise. The sources use these names and never
// spell qDebug() themselves.

// Traces connection setup and the retry/resend decisions.
#ifdef QHTTPNETWORKCONNECTIONCHANNEL_DEBUG
#  define qDebug_HTTPNCC qDebug() << "QHttpNetworkConnectionChannel:"
#else
#  define qDebug_HTTPNCC if (0) qDebug()
#endif

// Traces request dispatch and resend scheduling (_q_startNextRequest).
#ifdef QHTTPNETWORKCONNECTION_DEBUG
#  define qDebug_HTTPNC qDebug() << "QHttpNetworkConnection:"
#else
#  define qDebug_HTTPNC if (0) qDebug()
#endif

// Traces the proxy CONNECT tunnel: its setup, the reply parsing, and the close that ends it.
#ifdef QHTTPSOCKETENGINE_DEBUG
#  define qDebug_HTTPSE qDebug() << "QHttpSocketEngine:"
#else
#  define qDebug_HTTPSE if (0) qDebug()
#endif

#ifdef QNETWORKACCESSHTTPBACKEND_DEBUG
#  define qDebug_HTTPBACKEND qDebug() << "QNetworkAccessHttpBackend:"
#else
#  define qDebug_HTTPBACKEND if (0) qDebug()
#endif

#ifdef QHTTPTHREADDELEGATE_DEBUG
#  define qDebug_HTTPTD qDebug() << "QHttpThreadDelegate:"
#else
#  define qDebug_HTTPTD if (0) qDebug()
#endif

#ifdef QABSTRACTSOCKET_DEBUG
#  define qDebug_ABS qDebug() << "QAbstractSocket:"
#else
#  define qDebug_ABS if (0) qDebug()
#endif

#ifdef QNATIVESOCKETENGINE_DEBUG
#  define qDebug_NSE qDebug() << "QNativeSocketEngine:"
#else
#  define qDebug_NSE if (0) qDebug()
#endif

#ifdef QTCPSOCKET_DEBUG
#  define qDebug_TCPSOCK qDebug() << "QTcpSocket:"
#else
#  define qDebug_TCPSOCK if (0) qDebug()
#endif

#ifdef QUDPSOCKET_DEBUG
#  define qDebug_UDPSOCK qDebug() << "QUdpSocket:"
#else
#  define qDebug_UDPSOCK if (0) qDebug()
#endif

#ifdef QTCPSERVER_DEBUG
#  define qDebug_TCPSRV qDebug() << "QTcpServer:"
#else
#  define qDebug_TCPSRV if (0) qDebug()
#endif

// QSocks5 keeps three prefixes: a fixed tag, and the owning object from either side (q_ptr / this).
#ifdef QSOCKS5SOCKETLAYER_DEBUG
#  define qDebug_SOCKS5 qDebug() << "[QSocks5]"
#  define qDebug_SOCKS5_Q qDebug() << this
#  define qDebug_SOCKS5_D qDebug() << q_ptr
#else
#  define qDebug_SOCKS5 if (0) qDebug()
#  define qDebug_SOCKS5_Q if (0) qDebug()
#  define qDebug_SOCKS5_D if (0) qDebug()
#endif

#ifdef QSSLSOCKET_DEBUG
#  define qDebug_SSL qDebug() << "QSslSocket:"
#else
#  define qDebug_SSL if (0) qDebug()
#endif

#ifdef QHOSTINFO_DEBUG
#  define qDebug_HINFO qDebug() << "QHostInfo:"
#else
#  define qDebug_HINFO if (0) qDebug()
#endif

#ifdef QFTPPI_DEBUG
#  define qDebug_FPI qDebug() << "QFtpPI:"
#else
#  define qDebug_FPI if (0) qDebug()
#endif

#ifdef QFTPDTP_DEBUG
#  define qDebug_FDTP qDebug() << "QFtpDTP:"
#else
#  define qDebug_FDTP if (0) qDebug()
#endif

#ifdef QNETWORKDISKCACHE_DEBUG
#  define qDebug_DISKCACHE qDebug() << "QNetworkDiskCache:"
#else
#  define qDebug_DISKCACHE if (0) qDebug()
#endif

#ifdef QNETWORKACCESSCACHEBACKEND_DEBUG
#  define qDebug_CACHEBACKEND qDebug() << "QNetworkAccessCacheBackend:"
#else
#  define qDebug_CACHEBACKEND if (0) qDebug()
#endif

#ifdef QTCPSOCKETENGINE_DEBUG
#  define qDebug_TCPSE qDebug() << "QTcpSocketEngine:"
#else
#  define qDebug_TCPSE if (0) qDebug()
#endif

#ifdef QLOCALSERVER_DEBUG
#  define qDebug_LOCSRV qDebug() << "QLocalServer:"
#else
#  define qDebug_LOCSRV if (0) qDebug()
#endif

#ifdef QDNSLOOKUP_DEBUG
#  define qDebug_DNS qDebug() << "QDnsLookup:"
#else
#  define qDebug_DNS if (0) qDebug()
#endif

// qnetworkcookie.cpp's date parser already gates its traces on PARSEDATESTRINGDEBUG; reuse that as the
// cookie guard so qDebug_cookie is the single control (its local //#define was removed).
#ifdef PARSEDATESTRINGDEBUG
#  define qDebug_COOKIE qDebug() << "QNetworkCookie:"
#else
#  define qDebug_COOKIE if (0) qDebug()
#endif

#endif // QNETWORK_DEBUG_H
