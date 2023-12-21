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

#ifndef QRANGE_H
#define QRANGE_H

#include <QtCore/qdatastream.h>

QT_BEGIN_NAMESPACE


template <class T>
class QRangeT {
    typedef QRangeT<T> Self;
public:
    inline QRangeT() : start(T()), length(T()) {}
    inline QRangeT(Qt::Initialization) {} //warning this does not set "start" and "length" to zero
    inline QRangeT(T start_, T length_) : start(start_), length(length_) {}

    static inline Self fromLimit(T minimum_, T maximum_)
        { return QRangeT(minimum_, maximum_ - minimum_); }

    T start;
    T length;
    inline T begin() const { return start; }
    inline T end() const { return start + length; }

    inline void set(T minimum_, T maximum_) { start = minimum_; length = maximum_ - minimum_; }
    inline T minimum() const { return start; }
    inline T maximum() const { return start + length; } //we count trailing null as max possible

    inline bool isNull() const { return start == 0 && length == 0; }
    inline bool isEmpty() const { return length <= 0; }
    inline bool isValid(int minSize = 0) const { return start >= 0 && length >= minSize; }
    inline bool isFilled() const { return length > 0; }
    inline const T &size() const { return length; } // returns "maximum" minus "minimum"

    inline bool contains(T i) const { return i >= start && i < start + length; }

    inline bool intersects(const QRangeT<T> &r) const //this works until "length" is positive
        { return qMax(start, r.start) < qMin(start + length,  r.start + r.length); }

    inline void translate(T i) { start += i; }
    inline Self translated(T i) const { return Self(start + i, length); }

    // TRACE/corelib support: MSVC's default copy-assign may miss `const`,
    // but the bug is hidden unless `other` is a `typedef`.
    Q_ALWAYS_INLINE Self &operator=(const Self &other) {
        start = other.start;
        length = other.length;
        return *this;
    }

#ifdef Q_COMPILER_DEFAULT_MEMBERS
    QRangeT(const Self &) = default;
#endif
};

template <class T>
inline bool operator==(const QRangeT<T> &r1, const QRangeT<T> &r2) { return r1.start == r2.start && r1.length == r2.length; }
template <class T>
inline bool operator!=(const QRangeT<T> &r1, const QRangeT<T> &r2) { return r1.start != r2.start || r1.length != r2.length; }

typedef QRangeT<int> QRange;
typedef QRangeT<qreal> QRangeF;


template <class T>
inline QDataStream& operator>>(QDataStream& s, QRangeT<T> &v)
{
    s >> v.start;
    s >> v.length;
    return s;
}

template<typename T>
inline QDataStream& operator<<(QDataStream& s, const QRangeT<T> &v)
{
    s << v.start;
    s << v.length;
    return s;
}

QT_END_NAMESPACE

#endif // QRANGE_H
