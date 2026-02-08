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

#ifndef QTHREAD_SLOTABLE_H
#define QTHREAD_SLOTABLE_H

#include <QtCore/qthread.h>
#include <QtCore/qsharedpointer.h>
#include <QtCore/qexception.h>
#include <QtCore/qatomicflags.h>


QT_BEGIN_NAMESPACE

#ifndef QT_NO_THREAD

/// Helper to ensure slots get called by this QThread sub-class's own-thread.
///
/// Otherwise, the way signal(s) and slot(s) connections work is that
/// the slots are run by the "calling-thread" which means
/// by the thread which called this class's constructor,
/// instead of being run by own-thread which this QThread sub-class creates.
///
/// @warning Don't directly @c delete QThreadSlotable, and
/// use @ref deleteSafe instead.
///
/// @warning This won't receive any signals unless this thread is running.
///
class Q_CORE_EXPORT QThreadSlotable : public QThread {
    typedef QThread super;
    Q_OBJECT

    enum Flag {
        FlagIsRunReached = 10,
        FlagIsDeletingSafely = 160
    };
public:
    /// @warning Forbids setting @ref parent in constructor, since
    /// the @ref start moves this to itself as thread, which is required because
    /// otherwise signals would be emitted from the thread which constructs this.
    /// Even later @ref setParent calls need to first move parent to this thread.
    QThreadSlotable();

    /// @warning Don't directly @c delete QThreadSlotable, and
    /// use @ref deleteSafe instead.
    ///
    /// @see preDestruct()
    ~QThreadSlotable();

    void start(Priority priority) Q_DECL_OVERRIDE;

    Q_ALWAYS_INLINE void start();

    /// @note QThread has NOT any "isRunReached" check and @ref isRunning is
    /// set to @c true the moment @ref start is called, however,
    /// that is enough for most use cases of QThread.
    inline bool isRunReached() const { return m_flags.includes(FlagIsRunReached); }

#ifndef QT_NO_EXCEPTIONS
    inline const std::exception_ptr &lastError() const { return m_lastError; }

    inline QString lastErrorString() const { return QExceptionWithMessage::fromPtr(m_lastError).message(); }
    inline void setLastErrorString(const QString &msg) {
        m_lastError = QExceptionWithMessage(msg).toPtr();
    }
#endif // QT_NO_EXCEPTIONS

protected:

    /// @warning Override @ref preRun instead.
    void run() Q_DECL_OVERRIDE Q_DECL_FINAL;
    /// Hook to run logic without reimplementing @ref run.
    ///
    /// @returns By default @c true, can be @c false to skip @ref exec.
    virtual bool preRun();
    /// Hook called to cleanup things even if @ref run throws.
    virtual void postRun();

    bool event(QEvent *event) Q_DECL_OVERRIDE;

    /// @warning Waits for own-thread to finish.
    virtual void preDestruct();
    /// Calls #qFatal if QEvent::DeferredDelete in not yet safely received, but
    /// does nothing if this object is already moved to out of own-thread.
    void requireDeleteSafe();

public Q_SLOTS:
    /// Calls `deleteLater` from given @p otherThread, but firstly ensures to
    /// move this QThreadSlotable to @p otherThread using @ref moveToThread.
    ///
    /// Otherwise, if you call @ref QThread::deleteLater directly, then
    /// it maybe would queue related ``delete` to run on own-thread,
    /// and a thread deleting itself may cause leak if not crash.
    ///
    /// @see QThreadSlotable
    bool deleteSafe(QThread *otherThread = Q_NULLPTR);

protected Q_SLOTS:
    /// @warning Called by the event-loop of thread which constructed this class,
    /// and NOT by this thread itself, meaning,
    /// if that thread never calls @ref exec, then this never gets called.
    virtual void onFinished();

private Q_SLOTS:
    /// @param disposeThreadPtr Pointer to QThread which called @ref deleteSafe.
    bool onDeleteSafe(void *disposeThreadPtr);

private:
    /// @warning Use @ref deleteSafe instead of this.
    inline void deleteLater() { super::deleteLater(); }

private:
    QAtomicFlags<Flag > m_flags;

#ifndef QT_NO_EXCEPTIONS
    std::exception_ptr m_lastError;
#endif // QT_NO_EXCEPTIONS
};


QSHAREDPOINTER_DELETER(QThreadSlotable, ptr->deleteSafe())


#ifndef Q_MOC_RUN
Q_ALWAYS_INLINE void QThreadSlotable::start() {
    // TRACE/build/MSVC 2010: may compile-crash if default-arg uses parent-class's sub-type #1,
    // hence either overload `start` here, but could use `Q_DEFAULT_ARG`.
    this->start(QThread::InheritPriority);
}
#endif // Q_MOC_RUN

#endif // QT_NO_THREAD

QT_END_NAMESPACE

#endif // QTHREAD_SLOTABLE_H
