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

#include <QtTest/QtTest>

#include <QtCore/qexception.h>

class tst_QException : public QObject
{
    Q_OBJECT
private slots:

    void testWithMessage_shouldHaveDefaultMessage() {
        // Dummy.
        QExceptionWithMessage obj;

        // Actual test.
        qExpect(obj.isEmpty())->toBeFalsy();
        qExpect(obj.message())->toEqualString(QLL("Unknown error."));
        qExpect(obj.what())->toEqualString("Unknown error.");
    }

    void testWithMessageIsEmpty_shouldBeEmptiable() {
        // Dummy.
        QExceptionWithMessage obj = qMove(QExceptionWithMessage(""));

        // Actual test.
        qExpect(obj.isEmpty())->toBeTruthy();
        qExpect(obj.message())->toBeEmpty();
        qExpect(obj.what())->toBeEmpty();
    }
};


QTEST_MAIN(tst_QException)
#include "tst_qexception.moc"
