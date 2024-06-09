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

#include "qobjectdecor.h"

#include <QtCore/qexception.h>
#include <QtCore/qthread.h>

#include <QtCore/private/qobject_p.h>


QT_BEGIN_NAMESPACE

#ifndef QT_NO_QOBJECT

QObjectDecor::~QObjectDecor()
{
    decorDetach();
}

void QObjectDecor::decorAttach(QObject *owner)
{
    Q_ASSERT(owner);
    if (decorOwner != Q_NULLPTR) {
        decorDetach();
    }

    const QMetaObject *m = owner->metaObject();
    if ( ! QLazinessResolver::set(owner->d_ptr, defaultResolver(), this)) {
        qThrowAtomicMismatch();
    }
    // Needs to be last, because if above throws, then
    // the QObject's destructor may cause crash.
    decorOwner = owner;
    QObjectPrivate *ownerPrivate = static_cast<QObjectPrivate *>(
                QScopedPointerLazyBase<QObjectData>::get(&owner->d_ptr)->d);
    decorOwnerPrivate = ownerPrivate;
    decorOwnerMeta = m;
    ownerPrivate->isLazy = true;
    ownerPrivate->isDecor = true;
}

void QObjectDecor::decorDetach()
{
    QObjectDecorLocker _(this);
    if (decorOwner) {
        QObjectPrivate *ownerPrivate = decorOwnerPrivate;
        QScopedPointerLazyBase<QObjectData> *base = QScopedPointerLazyBase<QObjectData>::get(&decorOwner->d_ptr);
        base->d = ownerPrivate;
        QLazinessResolver::set(decorOwner->d_ptr, this, defaultResolver());
        ownerPrivate->isLazy = false;
        ownerPrivate->isDecor = false;
        QObject *loaded = decorLoaded.data();
        if (loaded) {
            QObjectPrivate *loadedPrivate = QObjectPrivate::get(loaded);
            loadedPrivate->isDecoratee = false;
            QLazinessResolver::set(loaded->d_ptr, this, defaultResolver());
            // If `decorOwner` if handles delete of any loaded QObject, then
            // it should copy the decorLoaded QPointer before calling this.
            decorLoaded = QPointer<QObject>();
        }
    }
    decorOwner = Q_NULLPTR;
    decorOwnerPrivate = Q_NULLPTR;
    decorOwnerMeta = Q_NULLPTR;
}

void QObjectDecor::preDecorLoad() {
    // Nothing to do (but required to ease implementations).
}

void QObjectDecor::postDecorLoad(QObject *loaded) {
    Q_ASSERT_X(decorMutex.tryLock() == false, "Decor", "Caller should lock the mutex.");

    if (loaded == decorOwner) {
        qThrow(QRequirementErrorType::Usage,  "Recursion detected,"
               " ensure has different resolver.");
    }

    if ( ! QLazinessResolver::set(loaded->d_ptr, defaultResolver(), this)) {
        qThrow(QRequirementErrorType::Usage,  "Freshly loaded decoratee should have default resolver.");
    }

    QObjectPrivate *loadedPrivate = QObjectPrivate::get(loaded);
    loadedPrivate->isDecoratee = true;
    QScopedPointerLazyBase<QObjectData> *base = QScopedPointerLazyBase<QObjectData>::get(&decorOwner->d_ptr);
    base->d = loadedPrivate;
    this->decorLoaded = QPointer<QObject>(loaded);
    decorOwnerPrivate->isLazy = false;

    decorListenTrigger();
}

void QObjectDecor::decorListen(const QObjectDecorListener &listener) {
    if ( ! listener) return;
    QObjectDecorLocker _(this);
    if (decorLoaded) {
        listener(decorLoaded);
    } else {
        decorListeners.append(listener);
    }
}

bool QObjectDecor::decorListenTrigger() {
    QObject *loaded = decorLoaded.data();
    if ( ! loaded) {
        return false;
    }

    // Maybe unlock `decorMutex`.
    Listeners oldListeners = decorListeners;
    decorListeners.clear();

#ifdef QT_DEBUG
    int index = 0;
    Q_UNUSED(index) // Debug porpuses only.
#endif
    Listeners::const_iterator it = oldListeners.constBegin();
    Listeners::const_iterator end = oldListeners.constEnd();
    for (; it != end; ++it) {
        const QObjectDecorListener &listener = *it;
        listener(loaded);
        QT_DEBUG_SCOPE(++index;)
    }

    // Re-lock `decorMutex`.
    oldListeners.clear();

    return true;
}

bool QObjectDecor::lazyEvent(QLazyEvent *event)
{
    // Validation.
    QScopedPointerLazyBase<QObjectData> *base = QScopedPointerLazyBase<QObjectData>::get(&decorOwner->d_ptr);
    QObjectPrivate *&dataRef = event->data<QObjectPrivate *>();
    if (&dataRef != &base->d) {
        // Some resolvers may support different data, hence don't throw.
        return false;
    }
    if ( ! decorOwner) {
        goto posEndFunc;
    }

    // Handling.
    switch (event->type()) {
    case QLazyEvent::LoadNow: {
        decorLoad();
        return ! decorLoaded.isNull();
    }
    case QLazyEvent::Replace:
    case QLazyEvent::Take:
    case QLazyEvent::Swap:
        // QObject forbids replacing and/or taking QObjectPrivate, under other because
        // some Qt smart-pointers set and need fields of QObjectPrivate, and
        // replacement may not have those, and copying QObjectPrivate is too complicated.
        return false;
    case QLazyEvent::Destroy:
        // Never deletes, since we just borrow QObjectPrivate.
        return false;
    default: break;
    };

posEndFunc:
    return super::lazyEvent(event);
}

QObjectDecorLocker::QObjectDecorLocker(QObjectDecor *owner) Q_THROWS(!)
    : super(Qt::Uninitialized)
{
    if ( ! owner) {
        qThrowNullPointer();
    }
    // Never locks for same thread.
    QThread *current = QThread::currentThread();
    if (current != owner->decorLockThread) {
        this->init(&owner->decorMutex);
        owner->decorLocker = this;
        owner->decorLockThread = current;
    }
}

#endif // QT_NO_QOBJECT

QT_END_NAMESPACE
