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

#include "./qthreadslotable.h"

#include <QtCore/private/qobject_p.h>

QT_BEGIN_NAMESPACE

#ifndef QT_NO_THREAD

QThreadSlotable::QThreadSlotable()
    : m_isRunReached(false)
    , m_isDeletingSafely(false)
#ifndef QT_NO_EXCEPTIONS
    , m_lastError(Q_NULLPTR)
#endif
{
    QObjectData::get(this)->receiveChildEvents = false;

    // First of all, connect things we want to happen in separate-thread,
    // because `moveToThread` may cause undesired Qt::QueuedConnection.
    QObject::connect(this, &QThread::finished, this, &QThreadSlotable::onFinished);
}

QThreadSlotable::~QThreadSlotable() {
    requireDeleteSafe();
}

void QThreadSlotable::preDestruct()
{
    if (QThread::currentThread() != this) {
        // Waits for self.
        this->quit();
        this->wait(3000);
    } else {
        Q_ASSERT_X(false, "QThreadSlotable", "Thread-manager should not be deleted by own-thread.");
    }
}

void QThreadSlotable::start(Priority priority)
{
    m_isRunReached = false;

    // Below needs to occur before `start()`, else things connected to
    // this class's signals get triggered from calling-thread
    // (and never happen if said calling-thread is busy, or has no event-loop).
    this->moveToThread(this);

    super::start(priority);
}

void QThreadSlotable::run() {
    Q_DEFER {
        Q_ASSERT_X(qApp, "QThreadSlotable", "Should be destructed before QApplication.");
        this->postRun();
        // Allows destructor to wait for self.
        if (qApp
            // And without being already changed by `postRun` or `preRun`.
            && this->thread() == this
        ) {
            this->moveToThread(qApp->thread());
        }
    };

    m_isRunReached = true;

    QT_TRY {
        if ( ! qApp
             || this->isInterruptionRequested()
        ) {
            return;
        }

        if ( ! this->preRun()) {
            return;
        }

        this->exec();
    } QT_CATCHES(...,
        m_lastError = std::current_exception();
    )
}

bool QThreadSlotable::preRun()
{
    // Nothing to do (but required).
    return true;
}

void QThreadSlotable::postRun()
{
    // Nothing to do (but required).
}

/// @note If thread did run even for a milli-second,
/// then this will be called once it quits.
///
/// @warning Is NOT called by QThreadSlotable's own-thread, and instead
/// it will be triggered by the separate-thread that this class moves to on exit,
/// which is @ref QCoreApplication::thread by default; meaning,
/// if that thread never calls @ref exec, then this never gets called.
///
void QThreadSlotable::onFinished() {
    if ( ! m_isRunReached) {
        qFatal("QThreadSlotable: Usage error; override preRun instead of run.");
    }
    Q_ASSERT_X(QThread::currentThread() != this, "QThreadSlotable", "Should be called from a separate-thread");
}

bool QThreadSlotable::deleteSafe(QThread *otherThread) {
    if (otherThread == Q_NULLPTR) {
        otherThread = QThread::currentThread();
        if (otherThread == this) {
            otherThread = qApp->thread();
        }
    }
    return this->onDeleteSafe(Q_PTR_CAST(void *, otherThread));
}

/// @warning Needs to take `void *` as argument-type, since
/// we don't want to register `QThread *` as meta-type.
bool QThreadSlotable::onDeleteSafe(void *otherThreadPtr)
{
    QThread *currentThread = QThread::currentThread();
    if (this->thread() == this) {
        QThread *otherThread = Q_PTR_CAST(QThread *, otherThreadPtr);
        // Since `moveToThread` only works if called from
        // inside this QThread sub-class's own-thread, through `exec`.
        if (currentThread == this) {
            this->moveToThread(otherThread);
        } else if ( ! this->isRunning()) {
            // However, if own-thread's not running yet, we can skip events.
            QObjectPrivate *d = QObjectPrivate::get(this);
            d->moveToThread_unchecked(otherThread, false);
        }

        goto posInvokeQueued;
    } else if (currentThread == this) {
posInvokeQueued:
        // Calls `deleteSafe` from `this->thread()`, and
        // that repeats as long as `this->thread()` equals `this`.
        //
        // Note that if we pass lambda to `QTimer::singleShot`,
        // it would call from `QThread::currentThread`.
        QMetaObject::invokeMethod(
            this, "onDeleteSafe", Qt::QueuedConnection,
            Q_ARG(void*, otherThreadPtr)
        );
        return false;
    } else {
        // Should be called from another thread,
        // since calling it from this class's own-thread queues it on own-thread,
        // and a thread deleting itself may cause leak if not crash.
        this->deleteLater();
    }
    return true;
}

void QThreadSlotable::requireDeleteSafe()
{
    if ( ! m_isDeletingSafely
         && this->thread() == this
    ) {
        // Possible reasons:
        //
        // If you created a stack-allocated instance,
        // then create a heap-allocated instance instead, similar to:
        // ```
        // auto myThread = QSharedPointer<QThreadSlotable>(new QThreadSlotable);
        // ```
        //
        // If you wrap QThreadSlotable or sub-class with QSharedPointer,
        // then configure your class similar to:
        // ```
        // class MyThreadClass : public QThreadSlotable {
        //     // ...
        // };
        //
        // QSHAREDPOINTER_DELETER(MyThreadClass, ptr->deleteSafe())
        // ```
        qFatal("QThreadSlotable: Detected direct delete, use deleteSafe instead (for %p).", this);
    }

}

bool QThreadSlotable::event(QEvent *event)
{
    if (event->type() == QEvent::DeferredDelete) {
        // Validates DeferredDelete.
        QThread *currentThread = QThread::currentThread();
        if (currentThread == this || this->thread() == this) {
            if (qApp) {
                onDeleteSafe(qApp->thread());
            } else {
                qWarning("QThreadSlotable: Received DeferredDelete after QApplication was destructed, from thread %p.", currentThread);
            }
            return true;
        } else {
            // Was valid.
            m_isDeletingSafely = true;
            this->preDestruct();
            // Falls back to `super::event`.
        }
    }

    return super::event(event);
}

#endif // QT_NO_THREAD


QT_END_NAMESPACE
