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
#include <QtCore/qdatastream.h>

QT_BEGIN_NAMESPACE

/*!
    Helper to simplify number @c operator implementation(s), with the
    ability to cast from and to other number-types.

    For example, assume you need `int` to accept `unsigned` as value, and
    that without suppressing compile warnings all accross the project:
    ```
    class MyNumber : public QNumber<int> {
    public:
        typedef QNumber<quint32> super;
    public:
        Q_DECL_CONSTEXPR inline explicit MyNumber(ValueType v = 0) : super(v) {}

        inline static GraphTime fromUnsigned(const quint32 &t) { return MyNumber(int(t)); }
        inline qreal toReal() const { return val; }

        QNUMBER_OPERATORS(MyNumber)
        QNUMBER_OPERATOR_T(MyNumber, quint32, MyNumber::fromUnsigned)
    };
    ```
    In such case, even if there is any warning, you only need to
    suppress it inside a single method, the `fromUnsigned(...)` method.
*/
template <typename T>
class QNumber {
public:
    typedef T ValueType;
    typedef Q_DEREFABLE_T(T) ValueTypeDerefable;

    Q_DECL_CONSTEXPR inline QNumber() : val(T()) {}

    Q_DECL_CONSTEXPR inline ValueTypeDerefable value() const { return val; }
    inline void setValue(ValueTypeDerefable value) { val = value; }

    // This class can have non-built-in as type "T", else could do:
    // ```
    // Q_DECL_CONSTEXPR static QNumber<T> maximum() { return QNumber<T>( (std::numeric_limits<ValueType>::max)() ); }
    // Q_DECL_CONSTEXPR static QNumber<T> minimum() { return QNumber<T>( (std::numeric_limits<ValueType>::min)() ); }
    // ```

protected:
    Q_DECL_CONSTEXPR Q_ALWAYS_INLINE explicit QNumber(ValueTypeDerefable i)
        : val( i )
    {
    }

protected:
    ValueType val;
};

#ifndef QT_MOC_RUN
template <typename T>
inline qreal operator*(qreal i, const QNumber<T> &v) { return i * v.value(); }
template <typename T>
inline qreal operator/(qreal i, const QNumber<T> &v) { return i / v.value(); }

/*!
    @warning Can only be used inside a sub-class of QNumber, otherwise the
    compiler gives error for `&operator++()` having no formal parameter, see the
    example documented for QNumber class.

    @warning For implicit accessors, use #QNUMBER_OPS_ACCESSOR as well, and
    it's excluded from this macro only to allow explicit sub-classes.
*/
#  define QNUMBER_OPERATORS(CLASS) \
    inline CLASS &operator++() { val += 1; return *this;  } /*prefix (i.e. "++i")*/ \
    inline CLASS operator++(int) { CLASS tmp = *this; val += 1; return tmp;  } /*postfix (i.e. "i++")*/ \
    inline CLASS &operator--() { val -= 1; return *this;  } /*prefix (i.e. "--i")*/ \
    inline CLASS operator--(int) { CLASS tmp = *this; val -= 1; return tmp; } /*postfix (i.e. "i--")*/ \
    Q_DECL_CONSTEXPR inline CLASS operator-() const { return CLASS(-val); } /*invert operator*/ \
    \
    Q_DECL_CONSTEXPR inline bool operator!() const { return !val; } \
    \
    QNUMBER_OPERATOR_T(CLASS, CLASS, Q_VALUE_RETURN)

/// Same as #QNUMBER_OPERATORS, but for custom-type.
///
/// @param LEFT_CLASS Type-name of a custom sub-class of QNumber.
/// @param LEFT_CAST Should convert @p RIGHT_CLASS to @p LEFT_CLASS and return that.
/// @param RIGHT_CLASS The other-type, for which QNumber sub-class needs operators.
///
#  define QNUMBER_OPERATOR_T(LEFT_CLASS, RIGHT_CLASS, LEFT_CAST) \
    QNUMBER_OPS_ASSIGN_T(LEFT_CLASS, RIGHT_CLASS, LEFT_CAST) \
    \
    inline LEFT_CLASS &operator+=(Q_DEREFABLE_T(RIGHT_CLASS) right) { val += LEFT_CAST(right).value(); return *this; } \
    inline LEFT_CLASS &operator-=(Q_DEREFABLE_T(RIGHT_CLASS) right) { val -= LEFT_CAST(right).value(); return *this; } \
    Q_DECL_CONSTEXPR inline LEFT_CLASS operator+(Q_DEREFABLE_T(RIGHT_CLASS) right) const { return LEFT_CLASS(val + LEFT_CAST(right).value()); } \
    Q_DECL_CONSTEXPR inline LEFT_CLASS operator-(Q_DEREFABLE_T(RIGHT_CLASS) right) const { return LEFT_CLASS(val - LEFT_CAST(right).value()); } \
    \
    QNUMBER_OPS_COMPARE_T(LEFT_CLASS, RIGHT_CLASS, LEFT_CAST)\
    \
    inline LEFT_CLASS &operator/=(Q_DEREFABLE_T(RIGHT_CLASS) right) { val /= LEFT_CAST(right).value(); return *this; } \
    inline LEFT_CLASS &operator*=(Q_DEREFABLE_T(RIGHT_CLASS) right) { val *= LEFT_CAST(right).value(); return *this; } \
    inline LEFT_CLASS operator/(Q_DEREFABLE_T(RIGHT_CLASS) right) const { LEFT_CLASS n = *this; return (n /= LEFT_CAST(right)); } /* using existing byref operator/= */ \
    inline LEFT_CLASS operator*(Q_DEREFABLE_T(RIGHT_CLASS) right) const { LEFT_CLASS n = *this; return (n *= LEFT_CAST(right)); } /* using existing byref operator*= */ \
    \
    inline LEFT_CLASS operator>>(Q_DEREFABLE_T(RIGHT_CLASS) right) const { LEFT_CLASS n = *this; n.val >>= LEFT_CAST(right).value(); return n; } \
    inline LEFT_CLASS operator%(Q_DEREFABLE_T(RIGHT_CLASS) right) const { LEFT_CLASS n = *this; n.val %= LEFT_CAST(right).value(); return n; }

/// Defines implicit getters and setters.
///
/// @warning Use inside QNumber sub-class (not global scope).
///
#  define QNUMBER_OPS_ACCESSOR(CLASS) \
    QNUMBER_OPS_ACCESSOR_T(CLASS, CLASS::ValueType, Q_VALUE_RETURN)

/// Similar to #QNUMBER_OPS_ACCESSOR, but for custom-type.
///
/// @warning Requires QNumber sub-class's "setValue" method to have an
/// overload which accepts given @p TYPE as argument.
///
/// @param TYPE_CAST Should convert `val` field of @p CLASS to @p TYPE and return that.
///
#  define QNUMBER_OPS_ACCESSOR_T(CLASS, TYPE, TYPE_CAST) \
    inline operator Q_DEREFABLE_T(TYPE) () const { return TYPE_CAST(val); } \
    inline operator Q_DEREFABLE_MUTABLE_T(TYPE) () { return TYPE_CAST(val); } \
    inline typename QEnableIf<QtPrivate::is_pointer<TYPE >::value, typename QtPrivate::remove_relating<TYPE >::type >::Type &operator*() const { return *TYPE_CAST(val); } \
    inline typename QEnableIf<QtPrivate::is_pointer<TYPE >::value, Q_DEREFABLE_T(TYPE) >::Type operator->() const { return TYPE_CAST(val); } \
    inline typename QEnableIf<QtPrivate::is_pointer<TYPE >::value, TYPE >::Type operator->() { return TYPE_CAST(val); } \
    inline CLASS &operator=(Q_DEREFABLE_T(TYPE) right) { this->setValue( right ); return *this; }

/// Defines implicit setter(s).
#  define QNUMBER_OPS_ASSIGN(CLASS) \
    inline CLASS &operator=(ValueTypeDerefable right) { setValue( right ); return *this; } \
    QNUMBER_OPS_ASSIGN_T(CLASS, CLASS, Q_VALUE_RETURN)

#  define QNUMBER_OPS_ASSIGN_T(LEFT_CLASS, RIGHT_CLASS, LEFT_CAST) \
    inline LEFT_CLASS &operator=(Q_DEREFABLE_T(RIGHT_CLASS) right) { setValue( LEFT_CAST(right).value() ); return *this; }

/*!
    @warning Similar to #QNUMBER_OPERATORS, use only inside a QNumber sub-class.

    @see #QNUMBER_OPERATORS
*/
#  define QNUMBER_OPS_COMPARE(CLASS) \
    Q_DECL_CONSTEXPR inline bool operator!() const { return !val; } \
    QNUMBER_OPS_COMPARE_T(CLASS, CLASS, Q_VALUE_RETURN)

/// Same as #QNUMBER_COMPARE, but for custom-type.
///
/// @param LEFT_CAST Should convert @p RIGHT_CLASS to @p LEFT_CLASS and return that.
///
#  define QNUMBER_OPS_COMPARE_T(LEFT_CLASS, RIGHT_CLASS, LEFT_CAST) \
    Q_DECL_CONSTEXPR inline bool operator==(Q_DEREFABLE_T(RIGHT_CLASS) right) const { return val == LEFT_CAST(right).value(); } \
    Q_DECL_CONSTEXPR inline bool operator!=(Q_DEREFABLE_T(RIGHT_CLASS) right) const { return val != LEFT_CAST(right).value(); } \
    Q_DECL_CONSTEXPR inline bool operator<(Q_DEREFABLE_T(RIGHT_CLASS) right) const { return val < LEFT_CAST(right).value(); } \
    Q_DECL_CONSTEXPR inline bool operator>(Q_DEREFABLE_T(RIGHT_CLASS) right) const { return val > LEFT_CAST(right).value(); } \
    Q_DECL_CONSTEXPR inline bool operator<=(Q_DEREFABLE_T(RIGHT_CLASS) right) const { return val <= LEFT_CAST(right).value(); } \
    Q_DECL_CONSTEXPR inline bool operator>=(Q_DEREFABLE_T(RIGHT_CLASS) right) const { return val >= LEFT_CAST(right).value(); }

#  define QNUMBER_OPS_COMPARE_GLOBAL(LEFT_CLASS, RIGHT_CLASS) \
    Q_GLOBAL_OPS_COMPARE(LEFT_CLASS, RIGHT_CLASS, qNumberValue)

#  define Q_GLOBAL_OPS_COMPARE(LEFT_CLASS, RIGHT_CLASS, LEFT_CAST) \
    Q_DECL_CONSTEXPR inline bool operator==(Q_DEREFABLE_T(LEFT_CLASS) left, Q_DEREFABLE_T(RIGHT_CLASS) right) { return left == LEFT_CAST(right); } \
    Q_DECL_CONSTEXPR inline bool operator!=(Q_DEREFABLE_T(LEFT_CLASS) left, Q_DEREFABLE_T(RIGHT_CLASS) right) { return left != LEFT_CAST(right); } \
    Q_DECL_CONSTEXPR inline bool operator<(Q_DEREFABLE_T(LEFT_CLASS) left, Q_DEREFABLE_T(RIGHT_CLASS) right) { return left < LEFT_CAST(right); } \
    Q_DECL_CONSTEXPR inline bool operator>(Q_DEREFABLE_T(LEFT_CLASS) left, Q_DEREFABLE_T(RIGHT_CLASS) right) { return left > LEFT_CAST(right); } \
    Q_DECL_CONSTEXPR inline bool operator<=(Q_DEREFABLE_T(LEFT_CLASS) left, Q_DEREFABLE_T(RIGHT_CLASS) right) { return left <= LEFT_CAST(right); } \
    Q_DECL_CONSTEXPR inline bool operator>=(Q_DEREFABLE_T(LEFT_CLASS) left, Q_DEREFABLE_T(RIGHT_CLASS) right) { return left >= LEFT_CAST(right); }

template <typename T>
Q_ALWAYS_INLINE_T T qNumberValue(const QNumber<T> &x) {
    return x.value();
}

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
#  define QNUMBER_OPS_COMPARE(CLASS)
#  define QNUMBER_OPS_COMPARE_T(LEFT_CLASS, RIGHT_CLASS, LEFT_CAST)
#  define QNUMBER_OPS_COMPARE_GLOBAL(LEFT_CLASS, RIGHT_CLASS)
#  define Q_GLOBAL_OPS_COMPARE(LEFT_CLASS, RIGHT_CLASS, LEFT_CAST)
#endif //QT_MOC_RUN

QT_END_NAMESPACE

#endif //QNUMBER_H
