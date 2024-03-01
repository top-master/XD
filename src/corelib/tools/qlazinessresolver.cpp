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

#include "qlazinessresolver.h"

#include <QtCore/qexception.h>


QT_BEGIN_NAMESPACE

QPointerLazinessResolver QPointerLazinessResolver::globalNullable;
QPointerLazinessResolver QPointerLazinessResolver::globalNonNull(QPointerLazinessResolver::NonNull);
QPointerLazinessResolver QPointerLazinessResolver::globalImmutable(QPointerLazinessResolver::Immutable);


QLazinessResolver::~QLazinessResolver()
{
    // Nothing to do (but required).
}

void QLazinessResolver::throwDuplicate()
{
    qThrow(QRequirementErrorType::Usage, "Has already another Laziness-Resolver.");
}

bool QPointerLazinessResolver::lazyEvent(QLazyEvent *event)
{
    switch (event->type()) {
    case QLazyEvent::LoadNow: {
        // By default loads nothing, but
        // implementations could do:
        // ```
        // event->data<void *>() = myLoadedPointer;
        // ```
        return ! m_forbidNullPtr || (event->rawData() != Q_NULLPTR);
    }
    case QLazyEvent::Take: {
        if (m_forbidNullPtr || m_immutable) {
            return false;
        }
        event->data<void *>() = Q_NULLPTR;
        return true;
    }
    case QLazyEvent::Replace: {
        if (m_immutable) {
            return false;
        }
        QLazyEventReplace *replaceEvent = static_cast<QLazyEventReplace *>(event);
        void *&oldValueRef = replaceEvent->data<void *>();
        void *&newValueRef = replaceEvent->newValue<void *>();
        void *oldValue = oldValueRef;
        void *newValue = newValueRef;
        if (newValue != oldValue) {
            if (m_forbidNullPtr && ! newValue) {
                return false;
            }
            oldValueRef = newValue;
            return true; // Means delete old.
        }
        return false; // Don't delete old.
    }
    case QLazyEvent::Swap: {
        if (m_immutable) {
            return false;
        }
        QLazyEventReplace *castedEvent = static_cast<QLazyEventReplace *>(event);
        void *&oldValueRef = castedEvent->data<void *>();
        void *&newValueRef = castedEvent->newValue<void *>();

        // We want implementation of this to be optional for sub-classes,
        // else we would do:
        // ```
        // if (m_forbidNullPtr && ! newValueRef) {
        //     return false;
        // }
        // qSwap(oldValueRef, newValueRef);
        // return true;
        // ```
        // instead of following lines till case-end.

        if (m_forbidNullPtr) {
            if ( ! newValueRef)
                return false;
            // Docs for Swap already mention it's only optional if null-pointer is
            // allowed, hence if sub-class let's below be reached, then
            // it means a binary swap is enough.
            qSwap(oldValueRef, newValueRef);
            return true;
        }

        QT_FINALLY([&] {
            castedEvent->overrideType(QLazyEvent::Swap);
        });
        // Ensures already loaded.
        void *oldD = oldValueRef;
        if ( ! oldD) {
            castedEvent->overrideType(QLazyEvent::LoadNow);
            if ( ! lazyEvent(castedEvent)) {
                return false;
            }
            oldD = oldValueRef;
        }
        // Protects current value from possible deletion
        // (which triggering QLazyEvent::Replace could cause).
        castedEvent->overrideType(QLazyEvent::Take);
        if (lazyEvent(castedEvent)) {
            // Replaces null-pointer with new value.
            castedEvent->overrideType(QLazyEvent::Replace);
            if (lazyEvent(castedEvent)) {
                // Actual swap.
                newValueRef = oldD;
                return true;
            } else {
                qWarning("Default Swap failed to replace existing value.");
                void *newValue = newValueRef;
                newValueRef = oldD;
                QT_FINALLY([&] {
                    newValueRef = newValue;
                });
                lazyEvent(castedEvent);
            }
        }
        break;
    }
    case QLazyEvent::Destroy:
        // By default always calls deleter.
        return true;
    default: break;
    }

    return false;
}

QT_END_NAMESPACE
