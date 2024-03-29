/****************************************************************************
**
** Copyright (C) 2015 The XD Company Ltd.
** Copyright (C) 2015 The Qt Company Ltd.
** Copyright (C) 2015 Klarälvdalens Datakonsult AB, a KDAB Group company, info@kdab.com, author Marc Mutz <marc.mutz@kdab.com>
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

#ifndef QHASHFUNCTIONS_H
#define QHASHFUNCTIONS_H

#include <QtCore/qchar.h>
#include <QtCore/qpair.h>

#include <numeric> // for std::accumulate

#if 0
#pragma qt_class(QHashFunctions)
#endif

#if defined(Q_CC_MSVC)
#pragma warning( push )
#pragma warning( disable : 4311 ) // disable pointer truncation warning
                                  // (required for `qHash(const T *, ...)`).
#pragma warning( disable : 4127 ) // conditional expression is constant
#endif

QT_BEGIN_NAMESPACE

class QBitArray;
class QByteArray;
class QString;
class QStringRef;
class QLatin1String;

Q_CORE_EXPORT int qGlobalQHashSeed();
Q_CORE_EXPORT void qSetGlobalQHashSeed(int newSeed);

Q_CORE_EXPORT Q_DECL_PURE_FUNCTION uint qHashBits(const void *p, size_t size, uint seed = 0) Q_DECL_NOTHROW;

Q_DECL_CONST_FUNCTION Q_DECL_CONSTEXPR inline uint qHash(char key, uint seed = 0) Q_DECL_NOTHROW { return uint(key) ^ seed; }
Q_DECL_CONST_FUNCTION Q_DECL_CONSTEXPR inline uint qHash(uchar key, uint seed = 0) Q_DECL_NOTHROW { return uint(key) ^ seed; }
Q_DECL_CONST_FUNCTION Q_DECL_CONSTEXPR inline uint qHash(signed char key, uint seed = 0) Q_DECL_NOTHROW { return uint(key) ^ seed; }
Q_DECL_CONST_FUNCTION Q_DECL_CONSTEXPR inline uint qHash(ushort key, uint seed = 0) Q_DECL_NOTHROW { return uint(key) ^ seed; }
Q_DECL_CONST_FUNCTION Q_DECL_CONSTEXPR inline uint qHash(short key, uint seed = 0) Q_DECL_NOTHROW { return uint(key) ^ seed; }
Q_DECL_CONST_FUNCTION Q_DECL_CONSTEXPR inline uint qHash(uint key, uint seed = 0) Q_DECL_NOTHROW { return key ^ seed; }
Q_DECL_CONST_FUNCTION Q_DECL_CONSTEXPR inline uint qHash(int key, uint seed = 0) Q_DECL_NOTHROW { return uint(key) ^ seed; }
Q_DECL_CONST_FUNCTION Q_DECL_CONSTEXPR inline uint qHash(ulong key, uint seed = 0) Q_DECL_NOTHROW
{
    return (sizeof(ulong) > sizeof(uint))
        ? (uint(((key >> (8 * sizeof(uint) - 1)) ^ key) & (~0U)) ^ seed)
        : (uint(key & (~0U)) ^ seed);
}
Q_DECL_CONST_FUNCTION Q_DECL_CONSTEXPR inline uint qHash(long key, uint seed = 0) Q_DECL_NOTHROW { return qHash(ulong(key), seed); }
Q_DECL_CONST_FUNCTION Q_DECL_CONSTEXPR inline uint qHash(quint64 key, uint seed = 0) Q_DECL_NOTHROW
{
    return uint(((key >> (8 * sizeof(uint) - 1)) ^ key) & (~0U)) ^ seed;
}
Q_DECL_CONST_FUNCTION Q_DECL_CONSTEXPR inline uint qHash(qint64 key, uint seed = 0) Q_DECL_NOTHROW { return qHash(quint64(key), seed); }
Q_CORE_EXPORT Q_DECL_CONST_FUNCTION uint qHash(float key, uint seed = 0) Q_DECL_NOTHROW;
Q_CORE_EXPORT Q_DECL_CONST_FUNCTION uint qHash(double key, uint seed = 0) Q_DECL_NOTHROW;
#ifndef Q_OS_DARWIN
Q_CORE_EXPORT Q_DECL_CONST_FUNCTION uint qHash(long double key, uint seed = 0) Q_DECL_NOTHROW;
#endif
Q_DECL_CONST_FUNCTION Q_DECL_CONSTEXPR inline uint qHash(const QChar key, uint seed = 0) Q_DECL_NOTHROW { return qHash(key.unicode(), seed); }
Q_CORE_EXPORT Q_DECL_PURE_FUNCTION uint qHash(const QByteArray &key, uint seed = 0) Q_DECL_NOTHROW;
Q_CORE_EXPORT Q_DECL_PURE_FUNCTION uint qHash(const QString &key, uint seed = 0) Q_DECL_NOTHROW;
Q_CORE_EXPORT Q_DECL_PURE_FUNCTION uint qHash(const QStringRef &key, uint seed = 0) Q_DECL_NOTHROW;
Q_CORE_EXPORT Q_DECL_PURE_FUNCTION uint qHash(const QBitArray &key, uint seed = 0) Q_DECL_NOTHROW;
Q_CORE_EXPORT Q_DECL_PURE_FUNCTION uint qHash(QLatin1String key, uint seed = 0) Q_DECL_NOTHROW;
Q_CORE_EXPORT Q_DECL_PURE_FUNCTION uint qt_hash(const QString &key) Q_DECL_NOTHROW;
Q_CORE_EXPORT Q_DECL_PURE_FUNCTION uint qt_hash(const QStringRef &key) Q_DECL_NOTHROW;

template <class T> inline uint qHash(const T *key, uint seed = 0) Q_DECL_NOTHROW
{
    return qHash(reinterpret_cast<quintptr>(key), seed);
}
template<typename T> inline uint qHash(const T &t, uint seed)
    Q_DECL_NOEXCEPT_EXPR(noexcept(qHash(t)))
{ return qHash(t) ^ seed; }

namespace QtPrivate {

struct QHashCombine {
    typedef uint result_type;
    template <typename T>
    Q_DECL_CONSTEXPR result_type operator()(uint seed, const T &t) const Q_DECL_NOEXCEPT_EXPR(noexcept(qHash(t)))
    // combiner taken from N3876 / boost::hash_combine
    { return seed ^ (qHash(t) + 0x9e3779b9 + (seed << 6) + (seed >> 2)) ; }
};

struct QHashCombineCommutative {
    // QHashCombine is a good hash combiner, but is not commutative,
    // ie. it depends on the order of the input elements. That is
    // usually what we want: {0,1,3} should hash differently than
    // {1,3,0}. Except when it isn't (e.g. for QSet and
    // QHash). Therefore, provide a commutative combiner, too.
    typedef uint result_type;
    template <typename T>
    Q_DECL_CONSTEXPR result_type operator()(uint seed, const T &t) const Q_DECL_NOEXCEPT_EXPR(noexcept(qHash(t)))
    { return seed + qHash(t); } // don't use xor!
};

} // namespace QtPrivate

template <typename InputIterator>
inline uint qHashRange(InputIterator first, InputIterator last, uint seed = 0)
    Q_DECL_NOEXCEPT_EXPR(noexcept(qHash(*first))) // assume iterator operations don't throw
{
    return std::accumulate(first, last, seed, QtPrivate::QHashCombine());
}

template <typename InputIterator>
inline uint qHashRangeCommutative(InputIterator first, InputIterator last, uint seed = 0)
    Q_DECL_NOEXCEPT_EXPR(noexcept(qHash(*first))) // assume iterator operations don't throw
{
    return std::accumulate(first, last, seed, QtPrivate::QHashCombineCommutative());
}

template <typename T1, typename T2> inline uint qHash(const QPair<T1, T2> &key, uint seed = 0)
    Q_DECL_NOEXCEPT_EXPR(noexcept(qHash(key.first, seed)) && noexcept(qHash(key.second, seed)))
{
    uint h1 = qHash(key.first, seed);
    uint h2 = qHash(key.second, seed);
    return ((h1 << 16) | (h1 >> 16)) ^ h2 ^ seed;
}

QT_END_NAMESPACE

#if defined(Q_CC_MSVC)
#pragma warning( pop )
#endif

#endif // QHASHFUNCTIONS_H
