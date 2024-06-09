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

#if !defined(QSUPPLIER_H) && defined(__cplusplus)
#define QSUPPLIER_H

#include "./qfunction.h"
#include "./qconsumer.h"


/// @warning The @ref get method non-const but will be
/// called even from @c const references.
template <typename T>
class QSupplier : public QDestructable {
    typedef QSupplier<T> Self;
public:
    typedef T Type;
private:
    QFunction<Type() > m_callback;
public:

    inline QSupplier()
    {
    }

    template <typename F>
    Q_ALWAYS_INLINE Q_IMPLICIT QSupplier(const F &callback)
        : m_callback(callback)
    {
    }

    inline virtual ~QSupplier()
    {
    }

    Q_ALWAYS_INLINE Type get() const {
        return qMove(const_cast<Self *>(this)->get());
    }

    inline virtual Type get()
    {
        if (m_callback) {
            return (m_callback)();
        }
    }

    Q_ALWAYS_INLINE void operator =(const std::function<Type() > &callback) Q_DECL_NOEXCEPT
    {
        m_callback = callback;
    }

private:
    Q_DISABLE_COPY(QSupplier)
};


#endif /* QSUPPLIER_H */
