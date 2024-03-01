/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing/
**
** This file is part of the test suite of the Qt Toolkit.
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

#include <QtCore/QString>
#include <QtCore/QTime>
#include <QtCore/QTimestamp>
#include <QtCore/QElapsedTimer>
#include <QtTest/QtTest>


#define LLS(x) QString(QLL(x))

static const int minResolution = 50; // the minimum resolution for the tests

QDebug operator<<(QDebug s, const QElapsedTimer &t)
{
    s.nospace() << "(" << t.msecsSinceReference() << ")";
    return s.space();
}

class tst_QElapsedTimer : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void statics();
    void validity();
    void basics();
    void elapsed();
    void toString_data();
    void toString();
};

void tst_QElapsedTimer::statics()
{
    qDebug() << "Clock type is" << QElapsedTimer::clockType();
    qDebug() << "Said clock is" << (QElapsedTimer::isMonotonic() ? "monotonic" : "not monotonic");
    QElapsedTimer t;
    t.start();
    qDebug() << "Current time is" << t.msecsSinceReference();
}

void tst_QElapsedTimer::validity()
{
    QElapsedTimer t;

    QVERIFY(!t.isValid()); // non-POD now, it should always start invalid

    t.start();
    QVERIFY(t.isValid());

    t.invalidate();
    QVERIFY(!t.isValid());
}

void tst_QElapsedTimer::basics()
{
    QElapsedTimer t1;
    t1.start();

    QVERIFY(t1.msecsSinceReference() != 0);

    QCOMPARE(t1, t1);
    QVERIFY(!(t1 != t1));
    QVERIFY(!(t1 < t1));
    QCOMPARE(t1.msecsTo(t1), qint64(0));
    QCOMPARE(t1.secsTo(t1), qint64(0));

    quint64 value1 = t1.msecsSinceReference();
    qDebug() << "value1:" << value1 << "t1:" << t1;
    qint64 nsecs = t1.nsecsElapsed();
    qint64 elapsed = t1.restart();
    QVERIFY(elapsed < minResolution);
    QVERIFY(nsecs / 1000000 < minResolution);

    quint64 value2 = t1.msecsSinceReference();
    qDebug() << "value2:" << value2 << "t1:" << t1
             << "elapsed:" << elapsed << "nsecs:" << nsecs;
    // in theory, elapsed == value2 - value1

    // However, since QElapsedTimer keeps internally the full resolution,
    // we have here a rounding error due to integer division
    QVERIFY(qAbs(elapsed - qint64(value2 - value1)) <= 1);
}

void tst_QElapsedTimer::elapsed()
{
    QElapsedTimer t1;
    t1.start();

    QTest::qSleep(4*minResolution);
    QElapsedTimer t2;
    t2.start();

    QVERIFY(t1 != t2);
    QVERIFY(!(t1 == t2));
    QVERIFY(t1 < t2);
    QVERIFY(t1.msecsTo(t2) > 0);

    QVERIFY(t1.nsecsElapsed() > 0);
    QVERIFY(t1.elapsed() > 0);
    // the number of elapsed nanoseconds and milliseconds should match
    QVERIFY(t1.nsecsElapsed() - t1.elapsed() * 1000000 < 1000000);
    QVERIFY(t1.hasExpired(minResolution));
    QVERIFY(!t1.hasExpired(8*minResolution));
    QVERIFY(!t2.hasExpired(minResolution));

    QVERIFY(!t1.hasExpired(-1));
    QVERIFY(!t2.hasExpired(-1));

    qint64 elapsed = t1.restart();
    QVERIFY(elapsed > 3*minResolution);
    QVERIFY(elapsed < 5*minResolution);
    qint64 diff = t2.msecsTo(t1);
    QVERIFY(diff < minResolution);
}

void tst_QElapsedTimer::toString_data()
{
    QTest::addColumn<qint64>("elapsed");
    QTest::addColumn<QString>("expected");
    QTest::addColumn<QString>("expectedLabel");

    QTest::newRow("Min milli-second.")
            << qint64(1 * QTimestamp::MilliSecond)
            << LLS("00:00") << LLS("00:00.001 milli-second(s)");

    QTest::newRow("Max milli-second.")
            << qint64(999 * QTimestamp::MilliSecond)
            << LLS("00:01") << LLS("00:00.999 milli-second(s)");

    QTest::newRow("Min second.")
            << qint64(1 * QTimestamp::Second)
            << LLS("00:01") << LLS("00:01.000 second(s)");

    QTest::newRow("Max second.")
            << qint64(59 * QTimestamp::Second + 999 * QTimestamp::MilliSecond)
            << LLS("01:00") << LLS("00:59.999 second(s)");

    QTest::newRow("Min minute.")
            << qint64(1 * QTimestamp::Minute) << LLS("01:00") << LLS("01:00.000 minute(s)");

    QTest::newRow("Max minute.")
            << qint64(59 * QTimestamp::Minute + 59 * QTimestamp::Second + 999 * QTimestamp::MilliSecond)
            << LLS("1:00:00") << LLS("59:59.999 minute(s)");

    QTest::newRow("Min hour.")
            << qint64(1 * QTimestamp::Hour) << LLS("1:00:00") << LLS("1:00:00.000 hour(s)");
    QTest::newRow("Two hours.")
            << qint64(2 * QTimestamp::Hour) << LLS("2:00:00") << LLS("2:00:00.000 hour(s)");
    QTest::newRow("Max hour.")
            << qint64(QTimestamp::Day - QTimestamp::MilliSecond)
            << LLS("24:00:00") << LLS("23:59:59.999 hour(s)");

    QTest::newRow("Half of day.")
            << qint64(12 * QTimestamp::Hour) << LLS("12:00:00") << LLS("12:00:00.000 hour(s)");
    QTest::newRow("Min day.")
            << qint64(1 * QTimestamp::Day) << LLS("24:00:00") << LLS("24:00:00.000 hour(s)");

    qint64 time = QTimestamp::Month;
    time -= QTimestamp::MilliSecond;
    QTest::newRow("Max day.")
            << time << LLS("744:00:00") << LLS("743:59:59.999 hour(s)");

    time = Q_INT64_C(3) * QTimestamp::Month;
    time += 5 * QTimestamp::Minute;
    time += 7 * QTimestamp::Second;
    time += 21 * QTimestamp::MilliSecond;
    QTest::newRow("Months with minutes and seconds.")
            << time << LLS("2232:05:07") << LLS("2232:05:07.021 hour(s)");
}

void tst_QElapsedTimer::toString()
{
    const QFETCH(qint64, elapsed);
    const QFETCH(QString, expected);
    const QFETCH(QString, expectedLabel);

    QCOMPARE(QElapsedTimer::toString(elapsed), expected);
    QCOMPARE(QElapsedTimer::toStringLabel(elapsed), expectedLabel);
}

QTEST_MAIN(tst_QElapsedTimer);

#include "tst_qelapsedtimer.moc"
