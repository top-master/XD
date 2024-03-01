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
#include <QtCore/qexception.h>


#ifndef QT_NO_QOBJECT

QT_BEGIN_NAMESPACE

/*!
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
*/
class Q_CORE_EXPORT QObjectDecor : protected QPointerLazinessResolver {
    typedef QPointerLazinessResolver super;
public:
    inline QObjectDecor() Q_DECL_NOEXCEPT_EXPR(false)
        : super(super::Immutable)
        , decorOwner(Q_NULLPTR)
        , decorLoaded(Q_NULLPTR)
    {
    }

    inline ~QObjectDecor()
    {
        decorDetach();
    }

    void decorAttach(QObject *owner) Q_THROWS( QAtomicMismatchException );
    /// @warning After calling this, you should set `decorLoaded` to `Q_NULLPTR`
    /// and if needed, should handle delete of any loaded QObject.
    void decorDetach();

    /// @warning Implementations should call @ref postDecorLoad at least after the
    /// first load, and that while @ref decorMutex is locked.
    virtual QObject *decorLoad() = 0;

    /// @warning Caller should lock @ref decorMutex while calling.
    void postDecorLoad(QObject *loaded) Q_THROWS( QRequirementErrorType::Usage );

    // MARK: Access helpers (protected but not forbidden).

    inline QLazinessResolver *toLazinessResolver() {
        return static_cast<QLazinessResolver *>(this);
    }

    static Q_ALWAYS_INLINE QScopedPointerLazyImmutable<QObjectData> &d_ptr_from(const QObject *obj) {
        return const_cast<QObject * >(obj)->d_ptr;
    }

protected:
    bool lazyEvent(QLazyEvent *event) Q_DECL_OVERRIDE;

private:
    Q_DISABLE_COPY(QObjectDecor);

protected:
    /// Interface which redirects to decoratee (@ref decorLoaded).
    ///
    /// Can be set by @ref decorAttach method's first argument, and
    /// can be unset by calling @ref decorDetach method.
    QObject *decorOwner;
    /// Backup of original QObjectPrivate of @ref decorOwner.
    QObjectPrivate *decorOwnerPrivate;

    /// Decoratee.
    mutable QPointer<QObject> decorLoaded;
    mutable QMutex decorMutex;
};

QT_END_NAMESPACE

#endif // QT_NO_QOBJECT

#endif // QOBJECT_DECOR_H
