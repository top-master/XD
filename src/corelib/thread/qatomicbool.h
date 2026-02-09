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

#ifndef QATOMICBOOL_H
#define QATOMICBOOL_H

#include <QtCore/qatomic.h>
#include "./qbasicatomic-config.h"

QT_BEGIN_NAMESPACE

QT_WARNING_PUSH
QT_WARNING_DISABLE_GCC("-Wextra")

class QAtomicBool : public QAtomicInt
{
    typedef QAtomicInt super;
    typedef super::Ops::Type QAtomicBool:: *RestrictedBool;

public:

#ifdef QT_BASIC_ATOMIC_HAS_CONSTRUCTORS
    constexpr QAtomicBool(bool value = false) Q_DECL_NOTHROW
        : super(value ? 1 : 0)
    {
    }
#else
    inline QAtomicBool(bool value = false) Q_DECL_NOTHROW
    {
        this->storeRelease(value ? 1 : 0);
    }
#endif

    // MARK: copy.

    inline QAtomicBool(const QAtomicBool &other) Q_DECL_NOTHROW
#ifdef QT_BASIC_ATOMIC_HAS_CONSTRUCTORS
        : super()
#endif
    {
        this->storeRelease(other.loadAcquire());
    }

    inline QAtomicBool &operator=(const QAtomicBool &other) Q_DECL_NOTHROW
    {
        this->storeRelease(other.loadAcquire());
        return *this;
    }

    inline QAtomicBool &operator=(bool newValue) Q_DECL_NOTHROW
    {
        this->storeRelease(newValue ? 1 : 0);
        return *this;
    }

    // MARK: comparisons.

    inline bool operator==(bool other) const Q_DECL_NOTHROW
    { return this->loadAcquire() == (other ? 1 : 0); }
    inline bool operator!=(bool other) const Q_DECL_NOTHROW
    { return this->loadAcquire() != (other ? 1 : 0); }
    inline bool operator<=(bool other) const Q_DECL_NOTHROW
    { return this->loadAcquire() <= (other ? 1 : 0); }
    inline bool operator>=(bool other) const Q_DECL_NOTHROW
    { return this->loadAcquire() >= (other ? 1 : 0); }

    inline bool operator==(const QAtomicBool &other) const Q_DECL_NOTHROW
    { return this->loadAcquire() == other.loadAcquire(); }
    inline bool operator!=(const QAtomicBool &other) const Q_DECL_NOTHROW
    { return this->loadAcquire() != other.loadAcquire(); }
    inline bool operator<=(const QAtomicBool &other) const Q_DECL_NOTHROW
    { return this->loadAcquire() <= other.loadAcquire(); }
    inline bool operator>=(const QAtomicBool &other) const Q_DECL_NOTHROW
    { return this->loadAcquire() >= other.loadAcquire(); }

    inline bool operator!() const Q_DECL_NOTHROW
    { return this->loadAcquire() == 0; }

#if defined(Q_QDOC)
    inline operator bool() const
    {
        return loadAcquire() != 0;
    }
#else
    inline operator RestrictedBool() const Q_DECL_NOTHROW
    {
        return this->loadAcquire() ? &super::_q_value : Q_NULLPTR;
    }
#endif

};

QT_WARNING_POP

#ifdef QT_BASIC_ATOMIC_HAS_CONSTRUCTORS
#  undef QT_BASIC_ATOMIC_HAS_CONSTRUCTORS
#endif

QT_END_NAMESPACE
#endif // QATOMICBOOL_H
