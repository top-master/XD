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

#include <QtTest/qtest.h> // Minimal version of `#include <QtTest/QtTest>`.

#include <QtGui/private/qfixed_p.h>

Q_DECLARE_METATYPE(int);

class tst_QFixed : public QObject
{
    Q_OBJECT

public:
    qreal precession;

    inline tst_QFixed() {
        precession = 0.025;
    }

private slots:
    inline void divideByInt_data() {
        QTest::addColumn<int>("initial");
        QTest::addColumn<int>("divisor");
        QTest::addColumn<qreal>("expected");

        QTest::newRow("power of 10")
            << 92
            << 10
            << qreal(9.2);

        QTest::newRow("dividable once")
            << 52
            << 30
            << qreal(1.7333);
    }

    inline void divideByInt() {
        QFETCH(int, initial);
        QFETCH(int, divisor);
        QFETCH(qreal, expected);

        QFixed obj(initial);
        obj /= divisor;
        qExpect(obj.toReal())->toEqualFuzzy(expected, precession);
    }

    inline void divideByQFixed_data() {
        divideByInt_data();
    }

    inline void divideByQFixed() {
        QFETCH(int, initial);
        QFETCH(int, divisor);
        QFETCH(qreal, expected);

        QFixed obj(initial);
        QFixed other(divisor);
        obj /= other;
        qExpect(obj.toReal())->toEqualFuzzy(expected, precession);
    }

    inline void modulaByInt_data() {
        QTest::addColumn<int>("initial");
        QTest::addColumn<int>("divisor");
        QTest::addColumn<qreal>("expected");

        QTest::newRow("power of 10") << 92 << 10 << qreal(2);

        QTest::newRow("dividable once") << 52 << 30 << qreal(22.0);
    }

    inline void modulaByInt() {
        QFETCH(int, initial);
        QFETCH(int, divisor);
        QFETCH(qreal, expected);

        QFixed obj(initial);
        obj %= divisor;
        qExpect(obj.toReal())->toEqualFuzzy(expected, precession);
    }

    inline void modulaByQFixed_data() {
        modulaByInt_data();
    }

    inline void modulaByQFixed() {
        QFETCH(int, initial);
        QFETCH(int, divisor);
        QFETCH(qreal, expected);

        QFixed obj(initial);
        QFixed other(divisor);
        obj %= other;
        qExpect(obj.toReal())->toEqualFuzzy(expected, precession);
    }
};


QTEST_MAIN(tst_QFixed)
#include "tst_qfixed.moc"
