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

#ifndef QNUMBER_H
#define QNUMBER_H

#include <QtCore/qdebug.h>

QT_BEGIN_NAMESPACE

template <typename T>
class QNumber {
public:
    typedef T ValueType;

    Q_DECL_CONSTEXPR QNumber() : val(T()) {}

    Q_DECL_CONSTEXPR inline ValueType value() const { return val; }
    inline void setValue(ValueType value) { val = value; }

    // This class can have non-built-in as type "T", else could do:
    // ```
    // Q_DECL_CONSTEXPR static QNumber<T> maximum() { return QNumber<T>( (std::numeric_limits<ValueType>::max)() ); }
    // Q_DECL_CONSTEXPR static QNumber<T> minimum() { return QNumber<T>( (std::numeric_limits<ValueType>::min)() ); }
    // ```

protected:
    Q_DECL_CONSTEXPR inline explicit QNumber(T i) : val(i) {} // 2nd param is just a dummy for disambiguation

protected:
    ValueType val;
};

#ifndef QT_MOC_RUN
template <typename T>
inline qreal operator*(qreal i, const QNumber<T> &v) { return i * v.value(); }
template <typename T>
inline qreal operator/(qreal i, const QNumber<T> &v) { return i / v.value(); }


#  define QNUMBER_OPERATORS(CLASS) \
    inline CLASS &operator++() { val += 1; return *this;  } /*prefix (i.e. "++i")*/ \
    inline CLASS operator++(int) { CLASS tmp = *this; val += 1; return tmp;  } /*postfix (i.e. "i++")*/ \
    inline CLASS &operator--() { val -= 1; return *this;  } /*prefix (i.e. "--i")*/ \
    inline CLASS operator--(int) { CLASS tmp = *this; val -= 1; return tmp; } /*postfix (i.e. "i--")*/ \
    Q_DECL_CONSTEXPR inline CLASS operator-() const { return CLASS(-val); } /*invert operator*/ \
    Q_DECL_CONSTEXPR inline bool operator!() const { return !val; } \
    \
    QNUMBER_OPERATOR_T(CLASS, CLASS, Q_VALUE_RETURN)

/// For custom-type.
///
/// @param LEFT_CAST Should convert @p RIGHT_CLASS to @p LEFT_CLASS and return that.
///
#  define QNUMBER_OPERATOR_T(LEFT_CLASS, RIGHT_CLASS, LEFT_CAST) \
    LEFT_CLASS &operator=(const RIGHT_CLASS &right) { val = LEFT_CAST(right).value(); return *this; } \
    \
    inline LEFT_CLASS &operator+=(const RIGHT_CLASS &right) { val += LEFT_CAST(right).value(); return *this; } \
    inline LEFT_CLASS &operator-=(const RIGHT_CLASS &right) { val -= LEFT_CAST(right).value(); return *this; } \
    Q_DECL_CONSTEXPR inline LEFT_CLASS operator+(const RIGHT_CLASS &right) const { return LEFT_CLASS(val + LEFT_CAST(right).value()); } \
    Q_DECL_CONSTEXPR inline LEFT_CLASS operator-(const RIGHT_CLASS &right) const { return LEFT_CLASS(val - LEFT_CAST(right).value()); } \
    \
    Q_DECL_CONSTEXPR inline bool operator==(const RIGHT_CLASS &right) const { return val == LEFT_CAST(right).value(); } \
    Q_DECL_CONSTEXPR inline bool operator!=(const RIGHT_CLASS &right) const { return val != LEFT_CAST(right).value(); } \
    Q_DECL_CONSTEXPR inline bool operator<(const RIGHT_CLASS &right) const { return val < LEFT_CAST(right).value(); } \
    Q_DECL_CONSTEXPR inline bool operator>(const RIGHT_CLASS &right) const { return val > LEFT_CAST(right).value(); } \
    Q_DECL_CONSTEXPR inline bool operator<=(const RIGHT_CLASS &right) const { return val <= LEFT_CAST(right).value(); } \
    Q_DECL_CONSTEXPR inline bool operator>=(const RIGHT_CLASS &right) const { return val >= LEFT_CAST(right).value(); } \
    \
    inline LEFT_CLASS &operator/=(const RIGHT_CLASS &right) { val /= LEFT_CAST(right).value(); return *this; } \
    inline LEFT_CLASS &operator*=(const RIGHT_CLASS &right) { val *= LEFT_CAST(right).value(); return *this; } \
    inline LEFT_CLASS operator/(const RIGHT_CLASS &right) const { LEFT_CLASS n = *this; return (n /= LEFT_CAST(right)); } /* using existing byref operator/= */ \
    inline LEFT_CLASS operator*(const RIGHT_CLASS &right) const { LEFT_CLASS n = *this; return (n *= LEFT_CAST(right)); } /* using existing byref operator*= */ \
    \
    inline LEFT_CLASS operator>>(const RIGHT_CLASS &right) const { LEFT_CLASS n = *this; n.val >>= LEFT_CAST(right).value(); return n; } \
    inline LEFT_CLASS operator%(const RIGHT_CLASS &right) const { LEFT_CLASS n = *this; n.val %= LEFT_CAST(right).value(); return n; }


template <typename T>
Q_DECL_CONSTEXPR inline bool operator==(int i, const QNumber<T> &n) { return i == n.value(); }
template <typename T>
Q_DECL_CONSTEXPR inline bool operator!=(int i, const QNumber<T> &n) { return i != n.value(); }
template <typename T>
Q_DECL_CONSTEXPR inline bool operator<=(int i, const QNumber<T> &n) { return i <= n.value(); }
template <typename T>
Q_DECL_CONSTEXPR inline bool operator>=(int i, const QNumber<T> &n) { return i >= n.value(); }
template <typename T>
Q_DECL_CONSTEXPR inline bool operator<(int i, const QNumber<T> &n) { return i < n.value(); }
template <typename T>
Q_DECL_CONSTEXPR inline bool operator>(int i, const QNumber<T> &n) { return i > n.value(); }

template <typename T>
Q_DECL_CONSTEXPR inline bool operator==(qreal i, const QNumber<T> &n) { return i == n.value(); }
template <typename T>
Q_DECL_CONSTEXPR inline bool operator!=(qreal i, const QNumber<T> &n) { return i != n.value(); }
template <typename T>
Q_DECL_CONSTEXPR inline bool operator<=(qreal i, const QNumber<T> &n) { return i <= n.value(); }
template <typename T>
Q_DECL_CONSTEXPR inline bool operator>=(qreal i, const QNumber<T> &n) { return i >= n.value(); }
template <typename T>
Q_DECL_CONSTEXPR inline bool operator<(qreal i, const QNumber<T> &n) { return i < n.value(); }
template <typename T>
Q_DECL_CONSTEXPR inline bool operator>(qreal i, const QNumber<T> &n) { return i > n.value(); }



template <typename T>
inline QDebug &operator<<(QDebug &dbg, const QNumber<T> &n)
{ return dbg << n.value(); }

template <typename T>
inline QDataStream &operator<<(QDataStream &s, const QNumber<T> &n)
{ return s << n.value(); }
template <typename T>
inline QDataStream &operator>>(QDataStream &s, QNumber<T> &n) {
    T t;
    s >> t;
    if(s.status() == QDataStream::Ok)
        n.setValue(t);
    return s;
}

template <typename T>
inline QTextStream &operator<<(QTextStream &s, const QNumber<T> &n)
{ return s << n.value(); }
template <typename T>
inline QTextStream &operator>>(QTextStream &s, QNumber<T> &n) {
    T t;
    s >> t;
    if (s.status() == QTextStream::Ok)
        n.setValue(t);
    return s;
}
#else
#  define QNUMBER_OPERATORS(CLASS)
#  define QNUMBER_OPERATOR_T(LEFT_CLASS, RIGHT_CLASS, LEFT_CAST)
#endif //QT_MOC_RUN

QT_END_NAMESPACE

#endif //QNUMBER_H
