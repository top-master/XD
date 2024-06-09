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

#if !defined(QCONSUMER_H) && defined(__cplusplus)
#define QCONSUMER_H

#include "./qfunction.h"
#include "./qdestructable.h"


/// @warning The @ref accept method non-const but will be
/// called even from @c const references.
template <typename T>
class QConsumer : public QDestructable {
    typedef QConsumer<T> Self;
public:
    typedef T Type;
private:
    QFunction<void(Type &)> m_callback;
public:

    inline QConsumer()
    {
    }

    template <typename F>
    Q_ALWAYS_INLINE Q_IMPLICIT QConsumer(const F &callback)
        : m_callback(callback)
    {
    }

    inline virtual ~QConsumer()
    {
    }

    Q_ALWAYS_INLINE void accept(Type &value) const {
        const_cast<Self *>(this)->accept(value);
    }

    inline virtual void accept(Type &value)
    {
        if (m_callback) {
            (m_callback)(value);
        }
    }

    Q_ALWAYS_INLINE void operator =(const std::function<void(Type &)> &callback) Q_DECL_NOEXCEPT
    {
        m_callback = callback;
    }

private:
    Q_DISABLE_COPY(QConsumer)
};


#endif /* QCONSUMER_H */
