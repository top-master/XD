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

// The canonical->stand-in port map that pairs with isLoopbackOnly() lives on the test-server
// handle as the static TestServer::port(quint16) (helpers/testserver.h), so a test spells one
// canonical port (443/80/21/...) and reaches server-dummy's fleet whether or not the run is
// elevated. It is a class member rather than a free function here only so call sites read
// TestServer::port(443); it needs no handle (it is static).

#endif // TEST_PORTS_H
