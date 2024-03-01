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

#include <QtCore/private/qobject_p.h>


QT_BEGIN_NAMESPACE

#ifndef QT_NO_QOBJECT

void QObjectDecor::decorAttach(QObject *owner)
{
    if (decorOwner != Q_NULLPTR) {
        decorDetach();
    }
    if ( ! QLazinessResolver::set(owner->d_ptr, &globalImmutable, this)) {
        qThrowAtomicMismatch();
    }
    // Needs to be last, because if above throws, then
    // the QObject's destructor may cause crash.
    decorOwner = owner;
    QObjectPrivate *ownerPrivate = static_cast<QObjectPrivate *>(
                QScopedPointerBase::get(&owner->d_ptr)->d);
    decorOwnerPrivate = ownerPrivate;
    ownerPrivate->isLazy = true;
}

void QObjectDecor::decorDetach()
{
    QMutexLocker _(&decorMutex);
    if (decorOwner) {
        QObjectPrivate *ownerPrivate = decorOwnerPrivate;
        QScopedPointerBase *base = QScopedPointerBase::get(&decorOwner->d_ptr);
        base->d = ownerPrivate;
        QLazinessResolver::set(decorOwner->d_ptr, this, &globalImmutable);
        ownerPrivate->isLazy = false;
        if (decorLoaded) {
            QObjectPrivate *loadedPrivate = QObjectPrivate::get(decorLoaded.data());
            loadedPrivate->isDecoratee = false;
            // Note that `decorOwner` may handle delete of any loaded QObject,
            // else could do:
            // decorLoaded = Q_NULLPTR;
            // ```
        }
    }
    decorOwner = Q_NULLPTR;
}

void QObjectDecor::postDecorLoad(QObject *loaded) {
    if (loaded == decorOwner) {
        qThrow(QRequirementErrorType::Usage,  "Recursion detected,"
               " ensure has different resolver.");
    }

    QObjectPrivate *loadedPrivate = QObjectPrivate::get(loaded);
    loadedPrivate->isDecoratee = true;
    QScopedPointerBase *base = QScopedPointerBase::get(&decorOwner->d_ptr);
    base->d = loadedPrivate;
    this->decorLoaded = QPointer<QObject>(loaded);
    decorOwnerPrivate->isLazy = false;
}

bool QObjectDecor::lazyEvent(QLazyEvent *event)
{
    // Validation.
    QScopedPointerBase *base = QScopedPointerBase::get(&decorOwner->d_ptr);
    QObjectData *&dataRef = event->data<QObjectData *>();
    if (&dataRef != reinterpret_cast<QObjectData **>(&base->d)) {
        // Some resolvers may support different data, hence don't throw.
        return false;
    }
    if ( ! decorOwner) {
        goto posEndFunc;
    }

    // Handling.
    switch (event->type()) {
    case QLazyEvent::LoadNow: {
        QObject *result = decorLoad();
        return result != Q_NULLPTR;
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

#endif // QT_NO_QOBJECT

QT_END_NAMESPACE
