/****************************************************************************
**
** Copyright (C) 2015 The XD Company Ltd.
**
** This file is part of the QtCore module of the XD Toolkit.
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

#ifndef QTIMESTAMP_H
#define QTIMESTAMP_H

#include <QtCore/qdatetime.h>
#include <QtCore/qstring.h>


QT_BEGIN_NAMESPACE


class Q_CORE_EXPORT QTimestamp {
public:
    typedef qint64 RawType;

    Q_DECL_CONSTEXPR inline QTimestamp() : raw(0) {}

    Q_DECL_CONSTEXPR inline explicit QTimestamp(RawType milliSeconds)
        : raw(milliSeconds)
    {}

    inline QDateTime toDateTime() const
    {
        return QDateTime::fromMSecsSinceEpoch(raw);
    }

    Q_DECL_CONSTEXPR inline bool isNull() const Q_DECL_NOTHROW { return raw <= 0; }

    Q_DECL_CONSTEXPR inline bool isAfter(const QTimestamp &other) Q_DECL_NOTHROW
        { return !(raw < other.raw); }

    Q_DECL_CONSTEXPR inline bool isBefore(const QTimestamp &other) Q_DECL_NOTHROW
        { return (raw < other.raw); }

    Q_DECL_CONSTEXPR inline RawType getTime() const Q_DECL_NOTHROW { return raw; }

    enum Unit {
        MilliSecond = 1,
        Second = 1000 * MilliSecond,
        Minute = 60 * Second,
        Hour = 60 * Minute,
        Day = 24u * Hour,

        /// @warning The actual value *would* exceed `int`, hence
        /// use @ref Month instead, unless value isn't used in calculations, and
        /// is just used as `enum` (like in `switch`).
        Monthly = 31
    };

    /// Adds one Day grace-period for each Month (value is 31 days), hence
    /// don't use if grace-period is not allowed.
    ///
    /// Value is same as `qint64(31u * QTimestamp::Day) & 0xffffffff`.
    static const qint64 Month;

    static Q_ALWAYS_INLINE Q_DECL_CONSTEXPR qint64 month() Q_DECL_NOTHROW
        { return Q_INT64_C(0x000000009FA52400); }

private:
    RawType raw;
};

QT_END_NAMESPACE

#endif // QTIMESTAMP_H
