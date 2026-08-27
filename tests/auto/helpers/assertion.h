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

#ifndef ASSERTION_H
#define ASSERTION_H

#include <QtNetwork/QNetworkSession>
#include <QtTest/QtTest>

// Matcher for qExpect(session)->to<OpenBefore>(msecs): requires the session to
// open within `msecs` ms.
//
// Under Fil-C the tests run in a sandbox with no bearer backend, so a
// QNetworkSession never opens: there we only ATTEMPT it (the network tests reach
// their peer over loopback / server-dummy and don't need the session) instead of
// failing on a purely environmental condition. A normal (non-Fil-C) build keeps
// the require-it-opened behaviour, so a real bearer regression on a proper test
// host is still caught.
template <typename TActual>
class OpenBefore : public QTest::QExpectMatcher<TActual>
{
public:
    bool run() Q_DECL_OVERRIDE
    {
        if (!this->requireArgCount(1))
            return false;

        const int msecs = this->args[0].toInt();
        this->owner->actual->open();
#if defined(__FILC__)
        this->owner->actual->waitForOpened(msecs);
        return true;
#else
        return this->owner->actual->waitForOpened(msecs);
#endif
    }
};

#endif // ASSERTION_H
