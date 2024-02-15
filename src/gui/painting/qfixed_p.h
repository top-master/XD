/****************************************************************************
**
** Copyright (C) 2015 The XD Company Ltd.
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing/
**
** This file is part of the QtGui module of the Qt Toolkit.
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

#ifndef QFIXED_P_H
#define QFIXED_P_H

#include "QtCore/qdebug.h"
#include "QtCore/qpoint.h"
#include "QtCore/qsize.h"

QT_BEGIN_NAMESPACE

/// We use "QFixed" since the "plus, minus, divide and..." operations do
/// work faster on an integer than "qreal".
/// 
/// QFixed converts any "qreal" to integer but keeps their most decimal details
/// by multiplication of "qreal" with "64".
///
/// i.e. in most platforms, "QFixed" is a signed integer limited to "33,554,431"
/// instead of "2,147,483,647" which is the limit of a normal integer
/// since it holds a multiple by 64 of all original values.
///
/// @warning In case your code contains less "real numbers", and
/// mostly uses integers, use this class, otherwise,
/// if your code contains mostly "qreal" this may slow down instead of speed up
/// (unless you replace all with this class, cast to qreal not that much).
///
struct QFixed {
private:
    Q_DECL_CONSTEXPR inline QFixed(int i, Qt::Initialization) : val(i) {} // 2nd param is just a dummy for disambiguation
public:
    Q_DECL_CONSTEXPR QFixed() : val(0) {}
    /// @note `val(i<<6)` matches `val(i * 64)`.
    Q_DECL_CONSTEXPR Q_IMPLICIT QFixed(int i) : val(i * 64) {}
    Q_DECL_CONSTEXPR Q_IMPLICIT QFixed(long i) : val(i * 64) {}
    QFixed &operator=(int i) { val = i * 64; return *this; }
    QFixed &operator=(long i) { val = i * 64; return *this; }

    Q_DECL_CONSTEXPR static QFixed fromReal(qreal r) { return fromFixed((int)(r*qreal(64))); }
    /// Uses private ctor.
    Q_DECL_CONSTEXPR static QFixed fromFixed(int fixed) { return QFixed(fixed, Qt::Uninitialized); }

    Q_DECL_CONSTEXPR static QFixed maximum() { return fromFixed( (std::numeric_limits<int>::max)() ); }
    Q_DECL_CONSTEXPR static QFixed minimum() { return fromFixed( (std::numeric_limits<int>::min)() ); }

    Q_DECL_CONSTEXPR inline int value() const { return val; }
    inline void setValue(int value) { val = value; }

    Q_DECL_CONSTEXPR inline bool isNegative() const { return val < 0; }

    Q_DECL_CONSTEXPR inline int toInt() const { return (((val)+32) & -64)>>6; }
    Q_DECL_CONSTEXPR inline qreal toReal() const { return ((qreal)val)/(qreal)64; }

    Q_DECL_CONSTEXPR inline int truncate() const { return val>>6; }
    Q_DECL_CONSTEXPR inline QFixed round() const { return fromFixed(((val)+32) & -64); }
    /// The floor is the largest integer that is not greater than v. For example, if v is 41.2, then the floor is 41
    Q_DECL_CONSTEXPR inline QFixed floor() const { return fromFixed((val) & -64); }
    /// The ceiling is the smallest integer that is not less than v. For example, if v is 41.2, then the ceiling is 42
    Q_DECL_CONSTEXPR inline QFixed ceil() const { return fromFixed((val+63) & -64); }
    Q_DECL_CONSTEXPR inline QFixed absoluted() const { return fromFixed(val >= 0 ? val : -val); }
    inline void absolute() { if(val < 0) val = -val; }

    Q_DECL_CONSTEXPR inline QFixed operator+(int i) const { return fromFixed(val + i * 64); }
    Q_DECL_CONSTEXPR inline QFixed operator+(quint32 i) const { return fromFixed((val + (i<<6))); }
    Q_DECL_CONSTEXPR inline QFixed operator+(const QFixed &other) const { return fromFixed((val + other.val)); }
    inline QFixed &operator+=(int i) { val += i * 64; return *this; }
    inline QFixed &operator+=(quint32 i) { val += (i<<6); return *this; }
    inline QFixed &operator+=(const QFixed &other) { val += other.val; return *this; }
    /// Increment prefix (i.e. "++i").
    inline QFixed &operator++() { val += 64; return *this;  }
    // Increment postfix (i.e. "i++").
    inline QFixed operator++(int) { QFixed tmp = *this; val += 64; return tmp;  }

    Q_DECL_CONSTEXPR inline QFixed operator-(int i) const { return fromFixed(val - i * 64); }
    Q_DECL_CONSTEXPR inline QFixed operator-(quint32 i) const { return fromFixed((val - (i<<6))); }
    Q_DECL_CONSTEXPR inline QFixed operator-(const QFixed &other) const { return fromFixed((val - other.val)); }
    inline QFixed &operator-=(int i) { val -= i * 64; return *this; }
    inline QFixed &operator-=(quint32 i) { val -= (i<<6); return *this; }
    inline QFixed &operator-=(const QFixed &other) { val -= other.val; return *this; }
    /// Decrement prefix (i.e. "--i").
    inline QFixed &operator--() { val -= 64; return *this;  }
    /// Decrement postfix (i.e. "i--").
    inline QFixed operator--(int) { QFixed tmp = *this; val -= 64; return tmp; }

    // Invert operator.
    Q_DECL_CONSTEXPR inline QFixed operator-() const { return fromFixed(-val); }

    Q_DECL_CONSTEXPR inline bool operator==(const QFixed &other) const { return val == other.val; }
    Q_DECL_CONSTEXPR inline bool operator!=(const QFixed &other) const { return val != other.val; }
    Q_DECL_CONSTEXPR inline bool operator<(const QFixed &other) const { return val < other.val; }
    Q_DECL_CONSTEXPR inline bool operator>(const QFixed &other) const { return val > other.val; }
    Q_DECL_CONSTEXPR inline bool operator<=(const QFixed &other) const { return val <= other.val; }
    Q_DECL_CONSTEXPR inline bool operator>=(const QFixed &other) const { return val >= other.val; }
    Q_DECL_CONSTEXPR inline bool operator!() const { return !val; }

    inline QFixed &operator/=(int x) { val /= x; return *this; }
    inline QFixed &operator/=(const QFixed &o) {
        if (o.val == 0) {
            val = 0x7FFFFFFFL;
        } else {
            bool neg = false;
            qint64 a = val;
            qint64 b = o.val;
            if (a < 0) { a = -a; neg = true; }
            if (b < 0) { b = -b; neg = !neg; }

            int res = (int)(((a << 6) + (b >> 1)) / b);

            val = (neg ? -res : res);
        }
        return *this;
    }
    Q_DECL_CONSTEXPR inline QFixed operator/(int d) const { return fromFixed(val/d); }
    // Redirects to above by-ref `operator/=(const QFixed &o)`.
    inline QFixed operator/(QFixed b) const { QFixed f = *this; return (f /= b); }

    inline QFixed &operator%=(int x) { val %= (x * 64); return *this; }
    inline QFixed &operator%=(const QFixed &o) {
        if (o.val == 0) {
            val = 0x7FFFFFFFL;
        } else {
            val %= o.val;
        }
        return *this;
    }
    Q_DECL_CONSTEXPR inline QFixed operator%(int d) const { return fromFixed(val % d); }
    // Redirects to above by-ref `operator/=(const QFixed &o)`.
    inline QFixed operator%(QFixed b) const { QFixed f = *this; return (f %= b); }

    inline QFixed &operator*=(int i) { val *= i; return *this; }
    inline QFixed &operator*=(quint32 i) { val *= i; return *this; }
    inline QFixed &operator*=(const QFixed &o) {
        bool neg = false;
        qint64 a = val;
        qint64 b = o.val;
        if (a < 0) { a = -a; neg = true; }
        if (b < 0) { b = -b; neg = !neg; }

        int res = (int)((a * b + 0x20L) >> 6);
        val = neg ? -res : res;
        return *this;
    }
    Q_DECL_CONSTEXPR inline QFixed operator*(int i) const { return fromFixed(val * i); }
    Q_DECL_CONSTEXPR inline QFixed operator*(quint32 i) const { return fromFixed(val * i); }
    // Redirects to above by-ref `operator*=(const QFixed &o)`.
    inline QFixed operator*(const QFixed &o) const { QFixed f = *this; return (f *= o); }

    inline QFixed operator>>(int d) const { QFixed f = *this; f.val >>= d; return f; }

private:
    Q_DECL_CONSTEXPR QFixed(qreal i) : val((int)(i*qreal(64))) {}
    QFixed &operator=(qreal i) { val = (int)(i*qreal(64)); return *this; }
    Q_DECL_CONSTEXPR inline QFixed operator+(qreal i) const { return fromFixed((val + (int)(i*qreal(64)))); }
    inline QFixed &operator+=(qreal i) { val += (int)(i*64); return *this; }
    Q_DECL_CONSTEXPR inline QFixed operator-(qreal i) const { return fromFixed((val - (int)(i*qreal(64)))); }
    inline QFixed &operator-=(qreal i) { val -= (int)(i*64); return *this; }
    inline QFixed &operator/=(qreal r) { val = (int)(val/r); return *this; }
    Q_DECL_CONSTEXPR inline QFixed operator/(qreal d) const { return fromFixed((int)(val/d)); }
    inline QFixed &operator*=(qreal d) { val = (int) (val*d); return *this; }
    Q_DECL_CONSTEXPR inline QFixed operator*(qreal d) const { return fromFixed((int) (val*d)); }
    int val;
};
Q_DECLARE_TYPEINFO(QFixed, Q_PRIMITIVE_TYPE);

#define QFIXED_MAX (INT_MAX/256)

Q_DECL_CONSTEXPR inline int qRound(const QFixed &f) { return f.toInt(); }
Q_DECL_CONSTEXPR inline int qFloor(const QFixed &f) { return f.floor().truncate(); }

Q_DECL_CONSTEXPR inline QFixed operator*(int i, const QFixed &d) { return d*i; }
Q_DECL_CONSTEXPR inline QFixed operator+(int i, const QFixed &d) { return d+i; }
Q_DECL_CONSTEXPR inline QFixed operator-(int i, const QFixed &d) { return -(d-i); }
Q_DECL_CONSTEXPR inline QFixed operator*(quint32 i, const QFixed &d) { return d*i; }
Q_DECL_CONSTEXPR inline QFixed operator+(quint32 i, const QFixed &d) { return d+i; }
Q_DECL_CONSTEXPR inline QFixed operator-(quint32 i, const QFixed &d) { return -(d-i); }
// Q_DECL_CONSTEXPR inline QFixed operator*(qreal d, const QFixed &d2) { return d2*d; }

Q_DECL_CONSTEXPR inline bool operator==(const QFixed &f, int i) { return f.value() == i * 64; }
Q_DECL_CONSTEXPR inline bool operator==(int i, const QFixed &f) { return f.value() == i * 64; }
Q_DECL_CONSTEXPR inline bool operator!=(const QFixed &f, int i) { return f.value() != i * 64; }
Q_DECL_CONSTEXPR inline bool operator!=(int i, const QFixed &f) { return f.value() != i * 64; }
Q_DECL_CONSTEXPR inline bool operator<=(const QFixed &f, int i) { return f.value() <= i * 64; }
Q_DECL_CONSTEXPR inline bool operator<=(int i, const QFixed &f) { return i * 64 <= f.value(); }
Q_DECL_CONSTEXPR inline bool operator>=(const QFixed &f, int i) { return f.value() >= i * 64; }
Q_DECL_CONSTEXPR inline bool operator>=(int i, const QFixed &f) { return i * 64 >= f.value(); }
Q_DECL_CONSTEXPR inline bool operator<(const QFixed &f, int i) { return f.value() < i * 64; }
Q_DECL_CONSTEXPR inline bool operator<(int i, const QFixed &f) { return i * 64 < f.value(); }
Q_DECL_CONSTEXPR inline bool operator>(const QFixed &f, int i) { return f.value() > i * 64; }
Q_DECL_CONSTEXPR inline bool operator>(int i, const QFixed &f) { return i * 64 > f.value(); }

#ifndef QT_NO_DEBUG_STREAM
inline QDebug &operator<<(QDebug &dbg, const QFixed &f)
{ return dbg << f.toReal(); }
#endif

struct QFixedPoint {
    QFixed x;
    QFixed y;
    Q_DECL_CONSTEXPR inline QFixedPoint() {}
    Q_DECL_CONSTEXPR inline QFixedPoint(const QFixed &_x, const QFixed &_y) : x(_x), y(_y) {}
    Q_DECL_CONSTEXPR QPointF toPointF() const { return QPointF(x.toReal(), y.toReal()); }
    Q_DECL_CONSTEXPR static QFixedPoint fromPointF(const QPointF &p) {
        return QFixedPoint(QFixed::fromReal(p.x()), QFixed::fromReal(p.y()));
    }
};
Q_DECLARE_TYPEINFO(QFixedPoint, Q_PRIMITIVE_TYPE);

Q_DECL_CONSTEXPR inline QFixedPoint operator-(const QFixedPoint &p1, const QFixedPoint &p2)
{ return QFixedPoint(p1.x - p2.x, p1.y - p2.y); }
Q_DECL_CONSTEXPR inline QFixedPoint operator+(const QFixedPoint &p1, const QFixedPoint &p2)
{ return QFixedPoint(p1.x + p2.x, p1.y + p2.y); }

struct QFixedSize {
    QFixed width;
    QFixed height;
    Q_DECL_CONSTEXPR QFixedSize() {}
    Q_DECL_CONSTEXPR QFixedSize(QFixed _width, QFixed _height) : width(_width), height(_height) {}
    Q_DECL_CONSTEXPR QSizeF toSizeF() const { return QSizeF(width.toReal(), height.toReal()); }
    Q_DECL_CONSTEXPR static QFixedSize fromSizeF(const QSizeF &s) {
        return QFixedSize(QFixed::fromReal(s.width()), QFixed::fromReal(s.height()));
    }
};
Q_DECLARE_TYPEINFO(QFixedSize, Q_PRIMITIVE_TYPE);

QT_END_NAMESPACE

#endif // QTEXTENGINE_P_H
