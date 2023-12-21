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

#if !defined(QFUNCTION_H) && defined(__cplusplus)
#define QFUNCTION_H

#include <QtCore/qglobal.h>
#include <QtCore/qtypetraits.h>

#include <memory>
#include <functional>

QT_BEGIN_NAMESPACE


template <typename Func1>
class QFunction;

namespace QtPrivate {
    template <typename T> struct remove_std_function { typedef T type; };
    template <typename T> struct remove_std_function<std::function<T > > { typedef T type; };
    template <typename T> struct remove_qfunction { typedef T type; };
    template <typename T> struct remove_qfunction<QFunction<T > > { typedef T type; };

    template <class T> struct remove_function_pointer;
    // Combo of "remove_relating" and "remove_std_function"
    template <typename T> struct remove_function_pointer {
        typedef typename remove_relating<typename remove_std_function<typename remove_qfunction<T>::type >::type >::type type;
    };
} //namespace QtPrivate


QT_WARNING_PUSH

/**
 * Backward compatible std::function.
 *
 * Some MSVC compilers have problems accepting a simple "void (*) ()" as type,
 * but "void ()" works, hence this class silently handles the required casts.
 */
template <typename Func1>
class QFunction : public std::function<typename QtPrivate::remove_function_pointer<Func1 >::type >
{
    typedef std::function<typename QtPrivate::remove_function_pointer<Func1 >::type > super;
public:
    inline QFunction() Q_DECL_NOEXCEPT {}

#ifdef _MSC_VER
    // Copy; MSVC's move constructor causes stack-overflow crash.
    template <typename F>
    Q_INLINE_TEMPLATE QFunction(const F &f) Q_DECL_NOEXCEPT
        : super(f)
    {}
#else
    // Move or copy.
    template <typename F>
    Q_INLINE_TEMPLATE QFunction(F && f) Q_DECL_NOEXCEPT
        : super(f)
    {}
#endif

    inline QFunction &operator=(const QFunction &other) Q_DECL_NOEXCEPT
    { this->super::operator= (static_cast<const super &>(other)); return *this; }

    inline QFunction(const QFunction &other) Q_DECL_NOEXCEPT
    { this->operator= (other); }
};

QT_WARNING_POP

QT_END_NAMESPACE

#endif //QFUNCTION_H
