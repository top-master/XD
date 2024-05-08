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

#ifndef QPOINTER_H
#define QPOINTER_H

#include <QtCore/qsharedpointer.h>
#include <QtCore/qtypeinfo.h>

#ifndef QT_NO_QOBJECT

QT_BEGIN_NAMESPACE

class QVariant;

template <class T>
class QPointer
{
    Q_STATIC_ASSERT_X(!QtPrivate::is_pointer<T>::value, "QPointer's template type must not be a pointer type");

protected:
    template<typename U>
    struct TypeSelector
    {
        typedef QObject Type;
    };
    template<typename U>
    struct TypeSelector<const U>
    {
        typedef const QObject Type;
    };
    typedef typename TypeSelector<T>::Type QObjectType;
    QWeakPointer<QObjectType> wp;
public:
    Q_DECL_CONSTEXPR inline QPointer() { }
    Q_DECL_CONSTEXPR inline QPointer(T *p) : wp(p, true) { }
    // compiler-generated copy/move ctor/assignment operators are fine!
    // compiler-generated dtor is fine!

#ifdef Q_QDOC
    // Stop qdoc from complaining about missing function
    ~QPointer();
#endif

    inline void swap(QPointer &other) { wp.swap(other.wp); }

    inline QPointer<T> &operator=(T* p)
    { wp.assign(static_cast<QObjectType*>(p)); return *this; }

    inline T* data() const Q_DECL_NOTHROW
    { return static_cast<T*>( wp.data()); }
    inline T* operator->() const
    { return data(); }
    inline T& operator*() const
    { return *data(); }
    inline operator T*() const
    { return data(); }

    inline bool isNull() const
    { return wp.isNull(); }

    inline void clear()
    { wp.clear(); }

    Q_ALWAYS_INLINE QSharedPointer<T> toStrongRef(Qt::LogMode logMode = Qt::LogWarning) const
    { return qSharedPointerCast<T, QObjectType>(wp, logMode); }
    // std::weak_ptr compatibility:
    Q_ALWAYS_INLINE QSharedPointer<T> lock() const
    { return qSharedPointerCast<T, QObjectType>(wp); }
};
template <class T> Q_DECLARE_TYPEINFO_BODY(QPointer<T>, Q_MOVABLE_TYPE);

template <class T>
inline bool operator==(const T *o, const QPointer<T> &p)
{ return o == p.operator->(); }

template<class T>
inline bool operator==(const QPointer<T> &p, const T *o)
{ return p.operator->() == o; }

template <class T>
inline bool operator==(T *o, const QPointer<T> &p)
{ return o == p.operator->(); }

template<class T>
inline bool operator==(const QPointer<T> &p, T *o)
{ return p.operator->() == o; }

template<class T>
inline bool operator==(const QPointer<T> &p1, const QPointer<T> &p2)
{ return p1.operator->() == p2.operator->(); }

template <class T>
inline bool operator!=(const T *o, const QPointer<T> &p)
{ return o != p.operator->(); }

template<class T>
inline bool operator!= (const QPointer<T> &p, const T *o)
{ return p.operator->() != o; }

template <class T>
inline bool operator!=(T *o, const QPointer<T> &p)
{ return o != p.operator->(); }

template<class T>
inline bool operator!= (const QPointer<T> &p, T *o)
{ return p.operator->() != o; }

template<class T>
inline bool operator!= (const QPointer<T> &p1, const QPointer<T> &p2)
{ return p1.operator->() != p2.operator->() ; }

template<typename T>
QPointer<T>
qPointerFromVariant(const QVariant &variant)
{
    return QPointer<T>(qobject_cast<T*>(QtSharedPointer::weakPointerFromVariant_internal(variant).data()));
}

/// Same as QPointer, but simulates QAtomicPointer API.
template <typename T>
class QObjectPointerAtomic : public QPointer<T> {
    typedef QPointer<T> super;
    typedef QObjectPointerAtomic<T> Self;
public:
    typedef T *Type;
    typedef QAtomicOps<Type> Ops;
    typedef typename Ops::Type AtomicType;

    Q_DECL_CONSTEXPR inline QObjectPointerAtomic() { }
    Q_DECL_CONSTEXPR inline QObjectPointerAtomic(Type p) : super(p) { }

    template <typename X>
    Q_DECL_CONSTEXPR inline Q_IMPLICIT QObjectPointerAtomic(const QPointer<X> &other)
        : super(other)
    { }

    template <typename X>
    Q_DECL_CONSTEXPR inline QObjectPointerAtomic &operator=(const QPointer<X> &other)
    { super::operator=(other); return *this; }

    /// Same as @ref data.
    Q_ALWAYS_INLINE Type load() const Q_DECL_NOTHROW { return this->data(); }
    Q_ALWAYS_INLINE void store(Type newValue) { *this = Self(newValue); }

    Q_ALWAYS_INLINE Type loadAcquire() const Q_DECL_NOTHROW { (void) this->wp.value.loadAcquire(); return this->data(); }
    Q_ALWAYS_INLINE void storeRelease(Type newValue) {
        *this = Self(newValue);
        // Repeats to release memory-fence.
        this->wp.value.storeRelease(newValue);
    }

    static Q_ALWAYS_INLINE Q_DECL_CONSTEXPR bool isTestAndSetNative() { return Ops::isTestAndSetNative(); }
    static Q_ALWAYS_INLINE Q_DECL_CONSTEXPR bool isTestAndSetWaitFree() { return Ops::isTestAndSetWaitFree(); }

    inline bool testAndSetRelaxed(Type expectedValue, Type newValue) Q_DECL_NOTHROW {
        if (this->wp.value.testAndSetRelaxed(expectedValue, newValue)) {
            *this = Self(newValue);
            return true;
        }
        return false;
    }

    inline bool testAndSetAcquire(Type expectedValue, Type newValue) Q_DECL_NOTHROW {
        if (this->wp.value.testAndSetAcquire(expectedValue, newValue)) {
            *this = Self(newValue);
            return true;
        }
        return false;
    }

    inline bool testAndSetRelease(Type expectedValue, Type newValue) Q_DECL_NOTHROW {
        if (this->wp.value.testAndSetRelease(expectedValue, newValue)) {
            *this = Self(newValue);
            return true;
        }
        return false;
    }

    inline bool testAndSetOrdered(Type expectedValue, Type newValue) Q_DECL_NOTHROW {
        if (this->wp.value.testAndSetOrdered(expectedValue, newValue)) {
            *this = Self(newValue);
            return true;
        }
        return false;
    }



    inline bool testAndSetRelaxed(Type expectedValue, Type newValue, Type &currentValue) Q_DECL_NOTHROW {
        if (this->wp.value.testAndSetRelaxed(expectedValue, newValue, currentValue)) {
            *this = Self(newValue);
            return true;
        }
        return false;
    }

    inline bool testAndSetAcquire(Type expectedValue, Type newValue, Type &currentValue) Q_DECL_NOTHROW {
        if (this->wp.value.testAndSetAcquire(expectedValue, newValue, currentValue)) {
            *this = Self(newValue);
            return true;
        }
        return false;
    }

    inline bool testAndSetRelease(Type expectedValue, Type newValue, Type &currentValue) Q_DECL_NOTHROW {
        if (this->wp.value.testAndSetRelease(expectedValue, newValue, currentValue)) {
            *this = Self(newValue);
            return true;
        }
        return false;
    }

    inline bool testAndSetOrdered(Type expectedValue, Type newValue, Type &currentValue) Q_DECL_NOTHROW {
        if (this->wp.value.testAndSetOrdered(expectedValue, newValue, currentValue)) {
            *this = Self(newValue);
            return true;
        }
        return false;
    }

};

QT_END_NAMESPACE

#endif // QT_NO_QOBJECT

#endif // QPOINTER_H
