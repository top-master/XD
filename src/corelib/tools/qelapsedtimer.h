/****************************************************************************
**
** Copyright (C) 2015 The XD Company Ltd.
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing/
**
** This file is part of the QtCore module of the Qt Toolkit.
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

#ifndef QELAPSEDTIMER_H
#define QELAPSEDTIMER_H

#include <QtCore/qflags.h>
#include <QtCore/qstring.h>


QT_BEGIN_NAMESPACE


class Q_CORE_EXPORT QElapsedTimer
{
public:
    enum ClockType {
        SystemTime,
        MonotonicClock,
        TickCounter,
        MachAbsoluteTime,
        PerformanceCounter
    };

    Q_DECL_CONSTEXPR QElapsedTimer()
        : t1(Q_INT64_C(0x8000000000000000)),
          t2(Q_INT64_C(0x8000000000000000))
    {
    }

    static ClockType clockType() Q_DECL_NOTHROW;
    static bool isMonotonic() Q_DECL_NOTHROW;

    void start() Q_DECL_NOTHROW;
    qint64 restart() Q_DECL_NOTHROW;
    void invalidate() Q_DECL_NOTHROW;
    bool isValid() const Q_DECL_NOTHROW;

    qint64 nsecsElapsed() const Q_DECL_NOTHROW;
    qint64 elapsed() const Q_DECL_NOTHROW;
    bool hasExpired(qint64 timeout) const Q_DECL_NOTHROW;

    qint64 msecsSinceReference() const Q_DECL_NOTHROW;
    qint64 msecsTo(const QElapsedTimer &other) const Q_DECL_NOTHROW;
    qint64 secsTo(const QElapsedTimer &other) const Q_DECL_NOTHROW;

    enum FormatFlag {
        NoFlags = 0,
        /// Show milli-seconds.
        Precise = 0x0001,
        ForceEnglish = 0x1000,
    };
    typedef QFlags<FormatFlag> FormatFlags;

    static QString toString(qint64 msec, FormatFlags flags = NoFlags);
    inline QString toString(FormatFlags flags = NoFlags) const { return QElapsedTimer::toString(elapsed(), flags); }
    /// Combines @ref toString with @ref label to prevent mismatch.
    static QString toStringLabel(qint64 msec, FormatFlags flags = Precise);
    inline QString toStringLabel(FormatFlags flags = Precise) const
        { return QElapsedTimer::toStringLabel(elapsed(), flags); }
    static QString label(qint64 msec, FormatFlags flags = NoFlags);

    bool operator==(const QElapsedTimer &other) const Q_DECL_NOTHROW
    { return t1 == other.t1 && t2 == other.t2; }
    bool operator!=(const QElapsedTimer &other) const Q_DECL_NOTHROW
    { return !(*this == other); }

    friend bool Q_CORE_EXPORT operator<(const QElapsedTimer &v1, const QElapsedTimer &v2) Q_DECL_NOTHROW;

    static int timeLeft(int timeout, int elapsed);
    inline int timeLeft(int timeout) const
    {
        // Maybe 2147483647.
        const int maxElapsed = (std::numeric_limits<int>::max)();
        return timeLeft(timeout, qMin(elapsed(), qint64(maxElapsed)));
    }

private:
    qint64 t1;
    qint64 t2;
};

QT_END_NAMESPACE

#endif // QELAPSEDTIMER_H
