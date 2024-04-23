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

#ifndef QSCOPEDPOINTER_LAZY_H
#define QSCOPEDPOINTER_LAZY_H

#include "./qscopedpointerlazy_p.h"


QT_BEGIN_NAMESPACE

/// Simulates QScopedPointer's API, but provides lazy-loading support.
///
/// @warning Ensure given @tparam Deleter handles null-pointer, because
/// said Deleter is called even if the QLazinessResolver was not triggered for
/// the @ref QLazyEvent::LoadNow event yet; but default Deleter already handles.
///
/// @see QLazinessResolver
template <typename T, typename Deleter = QScopedPointerDeleter<T> >
class QScopedPointerLazy : protected QScopedPointerLazyBase<T> {
    typedef T *QScopedPointerLazy:: *RestrictedBool;
    typedef QScopedPointerLazy<T, Deleter> Self;
    typedef QScopedPointerLazyBase<T> super;
public:
    Q_DECL_CONSTEXPR explicit inline QScopedPointerLazy(Qt::Initialization)
        : super(Qt::Uninitialized)
    {}

    Q_DECL_CONSTEXPR explicit inline QScopedPointerLazy(T *p = Q_NULLPTR)
        : super(p)
    {
    }

    inline ~QScopedPointerLazy()
    {
        if (super::lazinessResolver().destroy(this, reinterpret_cast<void **>(&this->d))) {
            Deleter::cleanup(data());
        }
    }

    inline T &operator*() const
    {
        T *p = data();
        Q_ASSERT(p);
        return *p;
    }

    inline T *operator->() const
    {
        T *p = data();
        Q_ASSERT(p);
        return p;
    }

    inline bool operator!() const
    {
        T *p = data();
        return !p;
    }

#if defined(Q_QDOC)
    inline operator bool() const
    {
        return isNull() ? Q_NULLPTR : &QScopedPointerLazy::d;
    }
#else
    inline operator RestrictedBool() const
    {
        return isNull() ? Q_NULLPTR : &QScopedPointerLazy::d;
    }
#endif

    inline T *data() const
    {
        if (super::isLoadPending()) {
            super::lazinessResolver().loadNow(
                    const_cast<Self *>(this),
                    Q_PTR_CAST(void **, &this->d));
        }
        return Q_PTR_CAST(T *, this->d);
    }

    Q_ALWAYS_INLINE bool isNull() const
    {
        return !data();
    }

    inline bool reset(T *other = Q_NULLPTR)
    {
        T *oldD = Q_PTR_CAST(T *, this->d);
        if (super::lazinessResolver().replace(
                    this, reinterpret_cast<void **>(&this->d),
                    reinterpret_cast<void **>(&other))
        ) {
            Deleter::cleanup(oldD);
        }
        return Q_PTR_CAST(T *, this->d) == other;
    }

    Q_ALWAYS_INLINE T *take()
    {
        return Q_PTR_CAST(T *, super::lazinessResolver().take(this, reinterpret_cast<void **>(&this->d)));
    }

    Q_ALWAYS_INLINE bool swap(Self &other)
    {
        return super::lazinessResolver().swap(
                    this, reinterpret_cast<void **>(&this->d),
                    reinterpret_cast<void **>(&other.d));
    }

    typedef T *pointer;

protected:
    Q_DECL_CONSTEXPR explicit inline QScopedPointerLazy(T *p, QPointerLazinessResolver *resolver)
        : super(p, resolver)
    {
    }

private:
    Q_DISABLE_COPY(QScopedPointerLazy)

    friend class QLazinessResolver;
};


#ifndef Q_MOC_RUN
template <typename T>
Q_INLINE_TEMPLATE void qLazyEventOverrideContext(QScopedPointerLazy<T> &newContext, QLazyEvent &event)
{
    event.setRawContext(&newContext);
    QScopedPointerBase<T> *base = QScopedPointerBase<T>::get(&newContext);
    event.setRawData(&base->d);
}

template <class T, class Deleter>
inline bool operator==(const QScopedPointerLazy<T, Deleter> &lhs, const QScopedPointerLazy<T, Deleter> &rhs)
{
    return lhs.data() == rhs.data();
}

template <class T, class Deleter>
inline bool operator!=(const QScopedPointerLazy<T, Deleter> &lhs, const QScopedPointerLazy<T, Deleter> &rhs)
{
    return lhs.data() != rhs.data();
}

template <class T, class Deleter>
Q_INLINE_TEMPLATE void qSwap(QScopedPointerLazy<T, Deleter> &p1, QScopedPointerLazy<T, Deleter> &p2)
{ p1.swap(p2); }

QT_END_NAMESPACE
namespace std {
    template <class T, class Deleter>
    Q_INLINE_TEMPLATE void swap(QT_PREPEND_NAMESPACE(QScopedPointerLazy)<T, Deleter> &p1, QT_PREPEND_NAMESPACE(QScopedPointerLazy)<T, Deleter> &p2)
    { p1.swap(p2); }
}
QT_BEGIN_NAMESPACE

#endif // Q_MOC_RUN

template <typename T, typename Deleter = QScopedPointerDeleter<T> >
class QScopedPointerLazyNonNull : public QScopedPointerLazy<T, Deleter>
{
    typedef QScopedPointerLazy<T, Deleter> super;
public:
    Q_DECL_CONSTEXPR explicit inline QScopedPointerLazyNonNull(Qt::Initialization)
        : super(Qt::Uninitialized)
    {}

    Q_DECL_CONSTEXPR explicit inline QScopedPointerLazyNonNull(T *p = Q_NULLPTR)
        : super(p, &QPointerLazinessResolver::globalNonNull)
    {
    }

};

template <typename T, typename Deleter = QScopedPointerDeleter<T> >
class QScopedPointerLazyImmutable : public QScopedPointerLazy<T, Deleter>
{
    typedef QScopedPointerLazy<T, Deleter> super;
public:
    Q_DECL_CONSTEXPR explicit inline QScopedPointerLazyImmutable(Qt::Initialization)
        : super(Qt::Uninitialized)
    {}

    Q_DECL_CONSTEXPR explicit inline QScopedPointerLazyImmutable(T *p = Q_NULLPTR)
        : super(p, &QPointerLazinessResolver::globalImmutable)
    {
    }

};

QT_END_NAMESPACE

#endif // QSCOPEDPOINTER_LAZY_H
