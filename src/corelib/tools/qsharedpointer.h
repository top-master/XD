/****************************************************************************
**
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

#include <QtCore/qglobal.h>

#ifndef QSHAREDPOINTER_H
#define QSHAREDPOINTER_H

#include <QtCore/qatomic.h>

#ifndef Q_QDOC
# include "qsharedpointer_impl.h"
#else

QT_BEGIN_NAMESPACE


// These classes are here to fool qdoc into generating a better documentation

template <class T>
class QSharedPointer
{
public:
    // basic accessor functions
    T *data() const;
    bool isNull() const;
    operator bool() const;
    bool operator!() const;
    T &operator*() const;
    T *operator ->() const;

    // constructors
    QSharedPointer();
    explicit QSharedPointer(T *ptr);
    QSharedPointer(T *ptr, Deleter d);
    QSharedPointer(const QSharedPointer<T> &other);
    QSharedPointer(const QWeakPointer<T> &other);

    ~QSharedPointer() { }

    QSharedPointer<T> &operator=(const QSharedPointer<T> &other);
    QSharedPointer<T> &operator=(const QWeakPointer<T> &other);

    void swap(QSharedPointer<T> &other);

    QWeakPointer<T> toWeakRef() const;

    void clear();

    void reset();
    void reset(T *t);
    template <typename Deleter>
    void reset(T *t, Deleter deleter);

    // casts:
    template <class X> QSharedPointer<X> staticCast() const;
    template <class X> QSharedPointer<X> dynamicCast() const;
    template <class X> QSharedPointer<X> constCast() const;
    template <class X> QSharedPointer<X> objectCast() const;

    static inline QSharedPointer<T> create();
    static inline QSharedPointer<T> create(...);
};

template <class T>
class QWeakPointer
{
public:
    // basic accessor functions
    bool isNull() const;
    operator bool() const;
    bool operator!() const;

    // constructors:
    QWeakPointer();
    QWeakPointer(const QWeakPointer<T> &other);
    QWeakPointer(const QSharedPointer<T> &other);

    ~QWeakPointer();

    QWeakPointer<T> &operator=(const QWeakPointer<T> &other);
    QWeakPointer<T> &operator=(const QSharedPointer<T> &other);

    QWeakPointer(const QObject *other);
    QWeakPointer<T> &operator=(const QObject *other);

    void swap(QWeakPointer<T> &other);

    T *data() const;
    void clear();

    QSharedPointer<T> toStrongRef() const;
    QSharedPointer<T> lock() const;
};

template <class T>
class QEnableSharedFromThis
{
public:
    QSharedPointer<T> sharedFromThis();
    QSharedPointer<const T> sharedFromThis() const;
};

template<class T, class X> bool operator==(const QSharedPointer<T> &ptr1, const QSharedPointer<X> &ptr2);
template<class T, class X> bool operator!=(const QSharedPointer<T> &ptr1, const QSharedPointer<X> &ptr2);
template<class T, class X> bool operator==(const QSharedPointer<T> &ptr1, const X *ptr2);
template<class T, class X> bool operator!=(const QSharedPointer<T> &ptr1, const X *ptr2);
template<class T, class X> bool operator==(const T *ptr1, const QSharedPointer<X> &ptr2);
template<class T, class X> bool operator!=(const T *ptr1, const QSharedPointer<X> &ptr2);
template<class T, class X> bool operator==(const QWeakPointer<T> &ptr1, const QSharedPointer<X> &ptr2);
template<class T, class X> bool operator!=(const QWeakPointer<T> &ptr1, const QSharedPointer<X> &ptr2);
template<class T, class X> bool operator==(const QSharedPointer<T> &ptr1, const QWeakPointer<X> &ptr2);
template<class T, class X> bool operator!=(const QSharedPointer<T> &ptr1, const QWeakPointer<X> &ptr2);

template <class X, class T> QSharedPointer<X> qSharedPointerCast(const QSharedPointer<T> &other);
template <class X, class T> QSharedPointer<X> qSharedPointerCast(const QWeakPointer<T> &other);
template <class X, class T> QSharedPointer<X> qSharedPointerDynamicCast(const QSharedPointer<T> &src);
template <class X, class T> QSharedPointer<X> qSharedPointerDynamicCast(const QWeakPointer<T> &src);
template <class X, class T> QSharedPointer<X> qSharedPointerConstCast(const QSharedPointer<T> &src);
template <class X, class T> QSharedPointer<X> qSharedPointerConstCast(const QWeakPointer<T> &src);
template <class X, class T> QSharedPointer<X> qSharedPointerObjectCast(const QSharedPointer<T> &src);
template <class X, class T> QSharedPointer<X> qSharedPointerObjectCast(const QWeakPointer<T> &src);

template <class X, class T> QWeakPointer<X> qWeakPointerCast(const QWeakPointer<T> &src);

QT_END_NAMESPACE

#endif // Q_QDOC

#endif // QSHAREDPOINTER_H

// The part `qglobal.h` leaves out while it defines `Q_GLOBAL_STATIC`, since the
// headers below include `qglobal.h` themselves; any later include of this
// header adds it.
#if !defined(QSHAREDPOINTER_FULL_H) && !defined(QT_SHAREDPOINTER_CORE_ONLY)
#define QSHAREDPOINTER_FULL_H

#include <QtCore/qshareddata.h>
#include <QtCore/qhashfunctions.h>
#if QT_DEPRECATED_SINCE(5, 6)
#include <QtCore/qhash.h>
#endif

#ifndef Q_QDOC
QT_BEGIN_NAMESPACE

//
// qHash
//

// Uses helper defined in `qhashfunctions.h` file:
// ```
// template <class T> inline uint qHash(const T *key, uint seed = 0) Q_DECL_NOTHROW;
// ```
template <class T>
Q_INLINE_TEMPLATE uint qHash(const QSharedPointer<T> &ptr, uint seed = 0)
{
    return QT_PREPEND_NAMESPACE(qHash)<T>(ptr.data(), seed);
}

QT_END_NAMESPACE
#endif // Q_QDOC

#endif // QSHAREDPOINTER_FULL_H
