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

#include <QtCore/QCoreApplication>
#include <QtCore/QThreadFunc>
#include <QtTest/qtest.h>


class tst_QThreadFunc : public QObject
{
    Q_OBJECT

private slots:
    void testCallback_shouldBeNullByDefault() {
        QThreadFunc t;
        qExpect(t.callback())->toBeNull()
            ->withContext("Should leave implementation to users.");
    }

    void testIsAutoDeletable_shouldBeFalseByDefault() {
        QThreadFunc t;
        qExpect(t.isAutoDeletable())->toBeFalsy()
            ->withContext("May be stack-allocated; set to false by default.");
    }

    void testKeepLooping_shouldBeFalseByDefault() {
        QThreadFunc t;
        qExpect(t.keepLooping())->toBeFalsy()
            ->withContext("May have nothing to do; set to false by default.");
    }

};

QTEST_MAIN(tst_QThreadFunc)

#include "tst_qthreadfunc.moc"
