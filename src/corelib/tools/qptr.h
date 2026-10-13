/****************************************************************************
**
** Copyright (C) 2026 The XD Company Ltd.
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

#include <QtCore/qglobal.h>

#ifndef QPTR_H
#define QPTR_H

#include <QtCore/qsharedpointer.h>

QT_BEGIN_NAMESPACE

/*!
    \class QPtr
    \inmodule QtCore
    \brief The QPtr class is a QRef that converts to a raw pointer by
    itself, for code written against raw pointers.

    Passing a QPtr where a `T *` is expected needs no @ref QRef::data "data()"
    call. The raw pointer is valid only while some QRef to the same object
    exists; hence code that keeps the pointer keeps a QRef (or QPtr) with it.

    If the program is written for XD 5.6.6 or later (see
    #QT_VERSION_TARGET), a #Q_GLOBAL_STATIC hands out a plain QRef;
    otherwise, a QPtr, which lets code written for raw pointers compile
    unchanged.
*/
template <class T>
class QPtr : public QRef<T >
{
public:
    Q_ALWAYS_INLINE QPtr() {}

    Q_ALWAYS_INLINE QPtr(const QPtr<T > & other)
        : QRef<T >(other)
    {}

    template <class X>
    Q_ALWAYS_INLINE QPtr(const QSharedPointer<X > & other)
        : QRef<T >(other)
    {}

    Q_ALWAYS_INLINE QPtr<T > &operator=(const QPtr<T > & other)
    {
        QRef<T >::operator=(other);
        return *this;
    }

#ifdef Q_COMPILER_RVALUE_REFS
    Q_ALWAYS_INLINE QPtr(QPtr<T > && other) Q_DECL_NOTHROW
    {
        this->swap(other);
    }

    Q_ALWAYS_INLINE QPtr<T > &operator=(QPtr<T > && other) Q_DECL_NOTHROW
    {
        this->swap(other);
        return *this;
    }
#endif

    /// Converts to the raw pointer, valid while some QRef to it exists.
    Q_ALWAYS_INLINE operator T *() const Q_DECL_NOTHROW { return this->data(); }

    /// Tells whether this holds an object; without it, a test such as
    /// `if (ptr)` could convert through either the raw pointer or the base's
    /// own test.
    Q_ALWAYS_INLINE operator bool() const Q_DECL_NOTHROW { return !this->isNull(); }
};

template<typename T> Q_DECLARE_TYPEINFO_BODY(QPtr<T >, Q_MOVABLE_TYPE);

QT_END_NAMESPACE

#endif // QPTR_H
