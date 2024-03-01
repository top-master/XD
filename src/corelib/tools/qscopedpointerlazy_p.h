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

#ifndef QSCOPEDPOINTER_LAZY_P_H
#define QSCOPEDPOINTER_LAZY_P_H

#include <QtCore/qscopedpointer.h>


QT_BEGIN_NAMESPACE

class QObjectData;

template <typename T>
class QScopedPointerLazyBase : public QScopedPointerBase {
    typedef QScopedPointerLazyBase<T> Self;
    QPointerLazinessResolverField m_lazinessResolver;

public:
    Q_DECL_CONSTEXPR explicit inline QScopedPointerLazyBase(Qt::Initialization)
    {}

    Q_DECL_CONSTEXPR explicit inline QScopedPointerLazyBase(T *p = Q_NULLPTR)
        : QScopedPointerBase(p)
    {
    }

    Q_DECL_CONSTEXPR explicit inline QScopedPointerLazyBase(T *p, QPointerLazinessResolver *resolver)
        : QScopedPointerBase(p)
        , m_lazinessResolver(resolver)
    {
    }

    Q_ALWAYS_INLINE QPointerLazinessResolverField &lazinessResolver() const {
        return const_cast<Self *>(this)->m_lazinessResolver;
    }

    Q_DECL_CONSTEXPR bool isLoadPending() const {
        return d == Q_NULLPTR;
    }
};


template <>
class QScopedPointerLazyBase<QObjectData> : public QScopedPointerBase {
    typedef QScopedPointerLazyBase<QObjectData> Self;
public:
    Q_DECL_CONSTEXPR explicit inline QScopedPointerLazyBase(Qt::Initialization)
    {}

    Q_DECL_CONSTEXPR explicit inline QScopedPointerLazyBase(QObjectData *p = Q_NULLPTR)
        : QScopedPointerBase(p)
    {
    }

    /// @warning QObjectData always remains immutable, but
    /// change could be forced using @ref QLazinessResolver::set helper.
    Q_DECL_CONSTEXPR explicit inline QScopedPointerLazyBase(QObjectData *p, QPointerLazinessResolver *resolver)
        : QScopedPointerBase(p)
    {
        Q_UNUSED(resolver);
    }

    // Defined in QObject header.
    inline QPointerLazinessResolverField &lazinessResolver() const;

    Q_DECL_CONSTEXPR inline bool isLoadPending() const;

};


QT_END_NAMESPACE

#endif // QSCOPEDPOINTER_LAZY_P_H
