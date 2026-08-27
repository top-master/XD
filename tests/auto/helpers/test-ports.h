/****************************************************************************
**
** Copyright (C) 2015 The XD Company Ltd.
**
** This file is part of the test suite of the XD Toolkit.
**
** $QT_BEGIN_LICENSE:APACHE2$
**
** Licensed under the Apache License, Version 2.0 (the "License");
** you may not use this file except in compliance with the License.
** You may obtain a copy of the License at
**
**     http://www.apache.org/licenses/LICENSE-2.0
**
** Unless required by applicable law or agreed to in writing, software
** distributed under the License is distributed on an "AS IS" BASIS,
** WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
** See the License for the specific language governing permissions and
** limitations under the License.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef TEST_PORTS_H
#define TEST_PORTS_H

#include <QtCore/QCoreApplication>
#include <QtCore/qglobal.h>
#include <QtCore/QString>

/// Whether tests reach the bundled server at plain loopback (127.0.0.1) rather than by
/// the fake public host name qt-test-server.qt-test-net. XD ships no cross-platform way to
/// point that fake name at loopback, so an ordinary unprivileged run -- and every
/// memory-safe (Fil-C) run -- uses 127.0.0.1 directly. Only a privileged run, where the
/// environment is expected to resolve the fake name to the server out of band, returns
/// false. Static twin of TestServer::isLocalhost(), usable where no TestServer is in hand
/// (e.g. QtNetworkSettings::serverName()).
static inline bool isLoopbackOnly()
{
#if defined(__FILC__)
    return true;
#else
    return !QCoreApplication::isElevated();
#endif
}

// testPort() maps a service's canonical, privileged port (the CORRECT one a real
// deployment uses -- 443 for https, 80 for http, 21 for ftp, and so on) onto the
// port the tests should actually use right now.
//
// An elevated process (root on Unix, an elevated token on Windows -- see
// QCoreApplication::isElevated()) may bind those below-1024 ports, so it gets the
// real port verbatim. An unprivileged run cannot bind them, so it falls back to a
// high, unprivileged stand-in. The stand-ins below are exactly what server-dummy's
// fleet binds, so a client that asks testPort(443) and a server that listens on
// testPort(443) meet on the same port whether or not the run is elevated -- no
// remap layer in between.
static inline quint16 testPort(quint16 privileged)
{
    if (QCoreApplication::isElevated())
        return privileged; // correct port; an elevated process can bind it

    switch (privileged) {
    case 443: return 4433; // https  (correct: 443)
    case 80:  return 8080; // http   (correct: 80)
    case 21:  return 2100; // ftp    (correct: 21)
    case 143: return 1430; // imap   (correct: 143)
    case 993: return 9993; // imaps  (correct: 993)
    case 7:   return 7000; // echo   (correct: 7)
    case 13:  return 1300; // daytime(correct: 13)
    default:  return privileged; // already unprivileged (e.g. 3128, 1080)
    }
}

#endif // TEST_PORTS_H
