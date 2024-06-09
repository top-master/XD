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

#ifndef QOBJECT_DECOR_H
#define QOBJECT_DECOR_H

#include <QtCore/qobject.h>
#include <QtCore/qlazinessresolver.h>
#include <QtCore/qmutex.h>
#include <QtCore/qpointer.h>
#include <QtCore/qlinkedlist.h>
#include <QtCore/qfunction.h>


#ifndef QT_NO_QOBJECT

QT_BEGIN_NAMESPACE

class QThread;
class QObjectDecorLocker;

typedef QFunction<void (QObject *) > QObjectDecorListener;

/*!
Helper to decorate given @ref decorOwner with lazy-loading for another QObject.

@warning Call @ref decorAttach right after @ref decorOwner construction, since
else decor's signal/slot connections are **not** redirected to decoratee.

@warning Calls to @ref qobject_cast global helper load and return decoratee, if
said helper is called with @ref decorOwner as first argument, but as a
down-side, said helper can never cast to the real-class of @ref decorOwner, and
as an example, the @ref QEventDispatcherDecor class can be cast into the
`QEventDispatcherWin32`, although it never extends `QEventDispatcherWin32` class.

@warning Extending QObjectDecor forces MOC to change the implementation of
@ref QObject::qt_metacast helper into something slimilar to below:
```
void *QEventDispatcherDecor::qt_metacast(const char *className) Q_DECL_OVERRIDE
{
    if (qstrcmp(className, "QEventDispatcherDecor") == 0) {
        return static_cast<void *>(this);
    }

    return this->decorLoad()->qt_metacast(className);
}
```

@note The word "decor" or similar is injected to member names, to
prevent conflicting with sub-class's possible members, see the
@ref QEventDispatcherDecor
*/
class Q_CORE_EXPORT QObjectDecor : protected QPointerLazinessResolver {
    typedef QPointerLazinessResolver super;
public:
    inline QObjectDecor()
        : super(super::Immutable)
        , decorLocker(Q_NULLPTR)
        , decorLockThread(Q_NULLPTR)
        , decorOwner(Q_NULLPTR)
        , decorOwnerPrivate(Q_NULLPTR)
        , decorOwnerMeta(Q_NULLPTR)
        , decorLoaded(Q_NULLPTR)
    {
    }

    virtual ~QObjectDecor();

    void decorListen(const QObjectDecorListener &listener);

    static Q_ALWAYS_INLINE QPointerLazinessResolver *defaultResolver() {
        return &super::globalImmutableNonNull;
    }

    // MARK: Access helpers.

    Q_ALWAYS_INLINE const QPointer<QObject> &toDecoratee() const {
        if ( ! decorLoaded) {
            (void) const_cast<QObjectDecor *>(this)->decorLoad();
        }
        return decorLoaded;
    }

    Q_ALWAYS_INLINE bool isDecorLoaded() const {
        return decorLoaded.data() != Q_NULLPTR;
    }

    /// @warning May use @c reinterpret_cast() in some compilers, since the
    /// @c dynamic_cast() helper would not work in MSVC even if RTTI was enabled.
    static Q_ALWAYS_INLINE QObjectDecor *fromLazinessResolver(QLazinessResolver *resolver) {
#if defined(QT_NO_DYNAMIC_CAST) || defined(Q_CC_MSVC)
        return reinterpret_cast<QObjectDecor *>(resolver);
#else
        return dynamic_cast<QObjectDecor *>(resolver);
#endif
    }

    Q_ALWAYS_INLINE QLazinessResolver *toLazinessResolver() {
        return static_cast<QLazinessResolver *>(this);
    }

    Q_ALWAYS_INLINE const QMetaObject *toMetaObject() const {
        return this->decorOwnerMeta;
    }

    /// Returns QObjectData::isDecor field's value without causing decoratee to be loaded.
    static Q_ALWAYS_INLINE bool isDecor(const QObject *obj) {
        return reinterpret_cast<QObjectData *>(QScopedPointerLazyBase<QObjectData>::get(&const_cast<QObject *>(obj)->d_ptr)->d)->isDecor;
    }

    /// Returns given @p obj's current QObjectDecor if any, otherwise @c nullptr.
    ///
    /// @param obj Can be either @ref decorOwner or decoratee.
    ///
    /// @warning Use @ref qobject_cast instead, wherever the
    /// sub-class of QObjectDecor extends QObject as well.
    static Q_ALWAYS_INLINE QObjectDecor *fromDecorable(QObject *obj) {
        QObjectData *d = reinterpret_cast<QObjectData *>(QScopedPointerLazyBase<QObjectData>::get(&obj->d_ptr)->d);
        // TRACE/QObject: `d_ptr` should always be non-null, but let's check #2
        Q_IF (d) {
            QLazinessResolver *resolver = d->lazinessResolver.raw();
            if (resolver != defaultResolver()) {
                Q_ASSERT_X(0 == static_cast<QLazinessResolver *>(Q_PTR_CAST(QObjectDecor *, 0)),
                           "Decor", "Should be binary-castable to laziness-resolver.");
                return reinterpret_cast<QObjectDecor *>(resolver);
            }
        }
        return Q_NULLPTR;
    }

    Q_ALWAYS_INLINE const QObject *toDecor() const {
        return this->decorOwner.data();
    }

    Q_ALWAYS_INLINE QObject *toDecor() {
        return this->decorOwner.data();
    }

    Q_ALWAYS_INLINE const QObjectPrivate *toDecorPrivate() const {
        return this->decorOwnerPrivate;
    }

    Q_ALWAYS_INLINE QObjectPrivate *toDecorPrivate() {
        return this->decorOwnerPrivate;
    }

    /// @warning Binary-casts to given @tparam X type without checking.
    template <typename X>
    Q_ALWAYS_INLINE const QPointer<X> &toDecorPointer() {
        return *Q_PTR_CAST(QPointer<X> *, &this->decorOwner);
    }

    static Q_ALWAYS_INLINE QScopedPointerLazyImmutable<QObjectData> &d_ptr_from(const QObject *obj) {
        return const_cast<QObject * >(obj)->d_ptr;
    }

protected:
    void decorAttach(QObject *owner) Q_THROWS( QAtomicMismatchException );
    /// @warning After calling this, you should set `decorLoaded` to `Q_NULLPTR`
    /// and if needed, should handle delete of any loaded QObject.
    void decorDetach();

    /// @warning Implementations should call @ref preDecorLoad **before** the
    /// load happens, and should call @ref postDecorLoad **after** the
    /// load happens, but both times  while @ref decorMutex is locked using the
    /// QObjectDecorLocker helper.
    virtual void decorLoad() = 0;

    /// @warning Call this even if never overridden.
    ///
    /// @warning Caller should lock @ref decorMutex while calling.
    virtual void preDecorLoad();
    /// @warning Caller should lock @ref decorMutex while calling.
    ///
    /// @note Not virtual since we have @ref decorListen helper.
    void postDecorLoad(QObject *loaded) Q_THROWS( QRequirementErrorType::Usage );

    /// Triggers listeners (if any).
    /// @warning Caller should lock @ref decorMutex while calling.
    bool decorListenTrigger();

    bool lazyEvent(QLazyEvent *event) Q_DECL_OVERRIDE;

private:
    Q_DISABLE_COPY(QObjectDecor);

    friend class QObjectDecorLocker;

    mutable QMutex decorMutex;
    mutable QObjectDecorLocker *decorLocker;
    mutable QThread *decorLockThread;

protected:
    /// Interface which redirects to decoratee (@ref decorLoaded).
    ///
    /// Can be set by @ref decorAttach method's first argument, and
    /// can be unset by calling @ref decorDetach method.
    QPointer<QObject> decorOwner;
    /// Backup of original QObjectPrivate of @ref decorOwner.
    QObjectPrivate *decorOwnerPrivate;
    const QMetaObject *decorOwnerMeta;

    /// Decoratee.
    mutable QPointer<QObject> decorLoaded;

    typedef QLinkedList<QObjectDecorListener> Listeners;
    Listeners decorListeners;
};

class Q_CORE_EXPORT QObjectDecorLocker : public QMutexLocker {
    typedef QMutexLocker super;
public:
    explicit QObjectDecorLocker(QObjectDecor *owner) Q_THROWS( QNullPointerException );

    inline ~QObjectDecorLocker() {
        QObjectDecor *owner = this->decor();
        if (owner->decorLocker == this) {
            owner->decorLockThread = Q_NULLPTR;
            owner->decorLocker = Q_NULLPTR;
        }
    }

    Q_ALWAYS_INLINE QObjectDecor *decor() const {
        return Q_FIELDER(QObjectDecor, decorMutex, this->mutex());
    }

private:
    Q_DISABLE_COPY(QObjectDecorLocker)
};

QT_END_NAMESPACE

#endif // QT_NO_QOBJECT

#endif // QOBJECT_DECOR_H
