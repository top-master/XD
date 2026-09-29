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

#include "qeventdispatcherdecor.h"

#include <private/qcoreapplication_p.h>
#include <private/qthread_p.h>
#include <QtCore/qexception.h>


QEventDispatcherDecor::QEventDispatcherDecor(QObject *parent)
    : super(parent)
{
}

QEventDispatcherDecor::~QEventDispatcherDecor()
{
    // Nothing to do (but required).
}

void QEventDispatcherDecor::postDecorLoad(QObject *loaded)
{
    // Casts before the load below, since once loaded, qobject_cast would
    // route back to this decorator.
    QAbstractEventDispatcher *dispatcher = qobject_cast<QAbstractEventDispatcher *>(loaded);
    // The plain QObjectDecor load instead of QObjectLazy's dissolving one, so
    // this decorator keeps its own identity and stays replaceable.
    QObjectDecor::postDecorLoad(loaded);
    loadedDefault = dispatcher;
}

void QEventDispatcherDecor::decorListener(PreDecorContext *ctx)
{
    // Unlike QEventDispatcherLazy, puts this decorator (not the loaded default)
    // back into the thread slot too, so that replace() keeps affecting the
    // thread's event loop.
    if (ctx->isUsedByThread) {
        Q_IF(ctx->thread) {
            ctx->thread->eventDispatcher.testAndSetOrdered(Q_NULLPTR, this);
        }
    }
    if (ctx->isUsedByApp) {
        QCoreApplicationPrivate::eventDispatcher = this;
    }

    delete ctx;
}

/*!
Swaps the held delegate for @p newValue and returns the previous one. Loads
the system default first when nothing is loaded yet, so the previous one
returned is never @c nullptr.
*/
QAbstractEventDispatcher *QEventDispatcherDecor::replace(QAbstractEventDispatcher *newValue)
{
    QObjectDecorLocker _(this);
    (void) toDecoratee();
    return reinterpret_cast<QAbstractEventDispatcher * >(decorSwapLoaded(newValue));
}

/*!
Swaps the system default back in (see systemDefault()), and returns the
delegate it replaced.
*/
QAbstractEventDispatcher *QEventDispatcherDecor::restore()
{
    QObjectDecorLocker _(this);
    (void) toDecoratee();
    return reinterpret_cast<QAbstractEventDispatcher * >(decorSwapLoaded(systemDefault()));
}
