/****************************************************************************
**
** Copyright (C) 2015 The XD Company Ltd.
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing/
**
** This file is part of the QtCore module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL21$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see http://www.qt.io/terms-conditions. For further
** information use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file. Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** As a special exception, The Qt Company gives you certain additional
** rights. These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef QSHAREDMEMORY_P_H
#define QSHAREDMEMORY_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include "qsharedmemory.h"

QT_WARNING_PUSH
QT_WARNING_SUPPRESS_UNUSED

#ifdef QT_NO_SHAREDMEMORY
#  ifndef QT_NO_SYSTEMSEMAPHORE
QT_BEGIN_NAMESPACE

namespace QSharedMemoryPrivate
{
    int createUnixKeyFile(const QString &fileName);
    QString makePlatformSafeKey(const QString &key,
            const QString &prefix = QLatin1Literal("qipc_sharedmemory_"));
}

QT_END_NAMESPACE
#  endif
#else

#include "qsystemsemaphore.h"
#include "private/qobject_p.h"

#if !defined(Q_OS_WIN) && !defined(Q_OS_ANDROID)
#  include <sys/sem.h>
#endif

QT_BEGIN_NAMESPACE

class QSharedMemoryLockModeHandler;

#ifndef QT_NO_SYSTEMSEMAPHORE
/*!
  Helper class, former QSharedMemoryLocker was renamed in favor of
  the public-API version of QSharedMemoryLocker.
 */
class QSharedMemoryUnlocker
{

public:
    inline QSharedMemoryUnlocker(QSharedMemory *sharedMemory) : q_sm(sharedMemory)
    {
        Q_ASSERT(q_sm);
    }

    inline ~QSharedMemoryUnlocker()
    {
        if (q_sm)
            q_sm->unlock();
    }

    inline bool lock()
    {
        if (q_sm && q_sm->lock())
            return true;
        q_sm = 0;
        return false;
    }

private:
    QSharedMemory *q_sm;
};
#endif // QT_NO_SYSTEMSEMAPHORE

class Q_AUTOTEST_EXPORT QSharedMemoryPrivate : public QObjectPrivate
{
    friend class QSharedMemoryLockModeHandler;
    Q_DECLARE_PUBLIC(QSharedMemory)

public:
    QSharedMemoryPrivate();

    static inline const QSharedMemoryPrivate *get(const QSharedMemory *o) { return o->d_func(); }
    static inline QSharedMemoryPrivate *get(QSharedMemory *o) { return o->d_func(); }

    static int createUnixKeyFile(const QString &fileName);
    static QString makePlatformSafeKey(const QString &key,
            const QString &prefix = QLatin1Literal("qipc_sharedmemory_"));
#ifdef Q_OS_WIN
    Qt::HANDLE handle();
#elif defined(QT_POSIX_IPC)
    int handle();
#else
    key_t handle();
#endif
    bool initKey();
    bool cleanHandle();
    bool create(int size);
    bool attach(QSharedMemory::AccessMode mode);
    bool detach();

    void setErrorString(QLatin1String function);

#ifndef QT_NO_SYSTEMSEMAPHORE
    inline bool tryLocker(QSharedMemoryUnlocker *locker, const QString &function) {
        if (!locker->lock()) {
            errorString = QSharedMemory::tr("%1: unable to lock").arg(function);
            error = QSharedMemory::LockError;
            return false;
        }
        return true;
    }

    /// @returns Exclusive "nativeKey" for system-semaphore since we can NOT share the same key
    /// also supports Windows "Global\\" prefix by not changing the begin of key.
    inline QString semaphoreKey() const {
        const QLL &suffix = QLL("qipc_memorylock_");
        if(key.length())
            return makePlatformSafeKey(key, suffix);
        else if(nativeKey.length()) {
            QString r;
            r.reserve(nativeKey.size() + suffix.size());
            r.append(nativeKey);
            r.append(QLatin1Char('_'));
            r.append(QLatin1String(suffix.data(), suffix.size() - 1));
            return r;
        }
        return QString();
    }
#endif // QT_NO_SYSTEMSEMAPHORE

private:
#ifdef Q_OS_WIN
    Qt::HANDLE hand;
#elif defined(QT_POSIX_IPC)
    int hand;
#else
    key_t unix_key;
#endif

public:
    void *memory;
    int size;
    QString key;
    QString nativeKey;

    QSharedMemory::SharedMemoryError error;
    QString errorString;

#ifndef QT_NO_SYSTEMSEMAPHORE
    QSystemSemaphore systemSemaphore;
    bool lockedByMe;
#endif
};


class QSharedMemoryLockModeHandler {
public:
    QSharedMemoryPrivate *dd;
    const QLatin1String function;
    int lockMode;
    bool success;

    Q_ALWAYS_INLINE QSharedMemoryLockModeHandler(QSharedMemoryPrivate *smp, QSharedMemory::LockMode mode)
        : dd(smp)
        , function(QLL("QSharedMemory::create"))
        , lockMode(mode)
        , success(false)
    {
    }

#ifndef QT_NO_SYSTEMSEMAPHORE

    Q_NEVER_INLINE ~QSharedMemoryLockModeHandler() {
        if (lockMode == -1) {
            // Critical error (see `createOrLock()` docs).
            return;
        }

        if ((lockMode == QSharedMemory::AlwaysLock && ! dd->lockedByMe)
            || ! success
            || lockMode == QSharedMemory::AlwaysUnlock
        ) {
            QString errorString = dd->errorString;
            QSharedMemory::SharedMemoryError error = dd->error;
            if (lockMode == QSharedMemory::AlwaysLock) {
                this->lock();
            } else {
                dd->q_func()->unlock();
            }
            if (error != QSharedMemory::NoError) {
                dd->errorString = errorString;
                dd->error = error;
            }
        }
    }

    bool lock();

#else

    inline bool lock() { return true }

#endif // QT_NO_SYSTEMSEMAPHORE

    Q_ALWAYS_INLINE QSharedMemoryPrivate *operator->() const { return this->dd; }
};

QT_END_NAMESPACE

#endif // QT_NO_SHAREDMEMORY

QT_WARNING_POP

#endif // QSHAREDMEMORY_P_H

