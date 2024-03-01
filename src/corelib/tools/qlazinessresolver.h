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

#ifndef QLAZINESS_RESOLVER_H
#define QLAZINESS_RESOLVER_H

#include <QtCore/qglobal.h>
#include <QtCore/qcoreevent.h>

QT_BEGIN_NAMESPACE

class Q_CORE_EXPORT QLazyEvent {
public:
    Q_ALWAYS_INLINE explicit QLazyEvent(int type_, void *context_ = Q_NULLPTR, void *data_ = Q_NULLPTR)
        : m_context(context_)
        , m_data(data_)
        , m_type(type_)
    {
    }

    enum Type {
        /// Implementation should ensure value is loaded and ready to be get.
        LoadNow = QEvent::User - 5,
        /// Implementation should detach from current value, but without deleting it.
        ///
        /// Uses QLazyEventReplace.
        ///
        /// @warning Returning `true` means Deleter is allowed to be called for
        /// old value, and said Deleter is called even if the resolver was
        /// not triggered for %LoadNow yet.
        Replace,
        /// @warning If null-pointer is forbidden, then
        /// may always return @c false, without doing anything.
        Take,
        Destroy,
        /// Is optional if extending QPointerLazinessResolver, but
        /// only if null-pointer is allowed, because
        /// the default implementation simulates this by
        /// triggering both %Take and %Replace events.
        ///
        /// Uses QLazyEventReplace.
        ///
        /// @warning Returning @c true simply means success, and the Deleter is
        /// never triggered.
        Swap
    };

    /// @returns The `int` representation of Type, to
    /// support registering custom types, see @ref QEvent::registerEventType.
    Q_ALWAYS_INLINE int type() const { return m_type; }

    template <typename T>
    Q_ALWAYS_INLINE T &context() const {
        Q_ASSERT(m_context);
        return *reinterpret_cast<T *>(m_context);
    }

    Q_ALWAYS_INLINE void *rawContext() const { return m_context; }
    /// @warning Use @ref qLazyEventOverrideContext instead.
    Q_ALWAYS_INLINE void setRawContext(void *v) { m_context = v; }

    /// @warning Don't use unless sure rawData() gets set by caller, or
    /// check @ref rawData first.
    template <typename T>
    Q_ALWAYS_INLINE T &data() {
        Q_ASSERT(m_data);
        return *reinterpret_cast<T *>(m_data);
    }

    Q_ALWAYS_INLINE void *rawData() const { return m_data; }
    /// @warning Use @ref qLazyEventOverrideContext instead.
    Q_ALWAYS_INLINE void setRawData(void *v) { m_data = v; }

protected:
    void *m_context;
    void *m_data;
    int m_type;
};


inline void qLazyEventOverrideContext(void **newContext, void **newData, QLazyEvent &event)
{
    event.setRawContext(newContext);
    event.setRawData(newData);
}


class Q_CORE_EXPORT QLazyEventReplace : public QLazyEvent {
public:
    Q_ALWAYS_INLINE explicit QLazyEventReplace(int type_, void *context_ = Q_NULLPTR, void *data_ = Q_NULLPTR, void *newValue_ = Q_NULLPTR)
        : QLazyEvent(type_, context_, data_)
        , m_newValue(newValue_)
    {
    }

    /// @warning Don't use unless sure rawNewValue() gets set by caller, or
    /// check @ref rawNewValue first.
    template <typename T>
    Q_ALWAYS_INLINE T &newValue() const { return *reinterpret_cast<T *>(m_newValue); }

    Q_ALWAYS_INLINE void *rawNewValue() const { return m_newValue; }

    Q_ALWAYS_INLINE int overrideType(int newType) {
        int oldType = m_type;
        m_type = newType;
        return oldType;
    }

protected:
    void *m_newValue;
};

class Q_CORE_EXPORT QLazinessResolver {
public:
    virtual ~QLazinessResolver();

    /// @returns Depends on event Type.
    ///
    /// @warning Does not take ownership of @p context (caller should delete it).
    virtual bool lazyEvent(QLazyEvent *event) = 0;

    static void throwDuplicate();

    template <typename Container>
    static inline QLazinessResolver *get(Container &container) {
        return container.lazinessResolver().load();
    }

    template <typename Container>
    static inline bool set(Container &container, QLazinessResolver *expectedValue, QLazinessResolver *newValue, QLazinessResolver **current = Q_NULLPTR) {
        QLazinessResolver *tmp;
        if ( ! newValue) {
            qThrowNullPointer();
        }
        return container.lazinessResolver().testAndSetOrdered(expectedValue, newValue, current ? *current : tmp);
    }

    /// Returns reference to `lazinessResolver` field.
    ///
    /// @warning Will `static_cast` to ensure right base-type, but
    /// anything else is caller's responsibility.
    template <typename T, typename Container>
    static inline T &fieldCast(Container &container) {
        return *reinterpret_cast<T *>(static_cast<QBasicAtomicPointer<QLazinessResolver> *>(&container.lazinessResolver()));
    }
};

class Q_CORE_EXPORT QPointerLazinessResolver : public QLazinessResolver {
    typedef QPointerLazinessResolver Self;
public:
    enum Flag {
        /// Forbids being set to null-pointer, hence QLazyEvenr::Take is
        /// forbidden as well.
        NonNull = 0x01,
        Immutable = 0x02,
    };

    Q_DECL_CONSTEXPR explicit inline QPointerLazinessResolver(int flags = 0)
        : m_forbidNullPtr((flags & Self::NonNull) != 0)
        , m_immutable((flags & Self::Immutable) != 0)
    {
    }

    bool lazyEvent(QLazyEvent *event) Q_DECL_OVERRIDE;

    inline bool isNullAllowed() const Q_DECL_NOTHROW { return ! m_forbidNullPtr; }

public:
    static QPointerLazinessResolver globalNullable;
    static QPointerLazinessResolver globalNonNull;
    static QPointerLazinessResolver globalImmutable;

protected:
    bool m_forbidNullPtr;
    bool m_immutable;
};

class QPointerLazinessResolverAtomic : private QAtomicPointer<QLazinessResolver>
{
    friend class QLazinessResolver;
    typedef QAtomicPointer super;
public:
    inline QPointerLazinessResolverAtomic()
        : super(&QPointerLazinessResolver::globalNullable)
    {}

    inline explicit QPointerLazinessResolverAtomic(QPointerLazinessResolver *resolver)
        : super(resolver)
    {}

    inline QLazinessResolver *raw() const {
        return this->load();
    }

    inline QLazinessResolver *operator->() const
    {
        QLazinessResolver *d = this->load();
        Q_ASSERT(d);
        return d;
    }

    Q_ALWAYS_INLINE bool loadNow(void *context, void **data) const
    {
        QLazyEvent event(QLazyEvent::LoadNow, context, data);
        return (*this)->lazyEvent(&event);
    }

    Q_ALWAYS_INLINE void *take(void *context, void **data)
    {
        if (*data || loadNow(context, data)) {
            void *oldD = *data;
            QLazyEvent event(QLazyEvent::Take, context, data);
            if ((*this)->lazyEvent(&event)) {
                return oldD;
            }
        }
        return Q_NULLPTR;
    }

    Q_ALWAYS_INLINE bool replace(void *context, void **data, void **newValue)
    {
        if (*data == *newValue)
            return false;
        QLazyEventReplace event(QLazyEvent::Replace, context, data, newValue);
        return (*this)->lazyEvent(&event);
    }

    Q_ALWAYS_INLINE bool swap(void *context, void **data, void **newValue) {
        if (*data == *newValue)
            return false;
        QLazyEventReplace event(QLazyEvent::Swap, context, data, newValue);
        return (*this)->lazyEvent(&event);
    }

    Q_ALWAYS_INLINE bool destroy(void *context, void **data) {
        QLazyEvent event(QLazyEvent::Destroy, context, data);
        return (*this)->lazyEvent(&event);
    }
};

/// Helper to use @ref QPointerLazinessResolver::globalImmutable by default.
class QPointerLazinessResolverAtomicImmutable : public QPointerLazinessResolverAtomic
{
    typedef QPointerLazinessResolverAtomic super;
public:
    inline QPointerLazinessResolverAtomicImmutable()
        : super(&QPointerLazinessResolver::globalImmutable)
    {}

    inline explicit QPointerLazinessResolverAtomicImmutable(QPointerLazinessResolver *resolver)
        : super(resolver)
    {}
};

QT_END_NAMESPACE

#endif // QLAZINESS_RESOLVER_H
