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

#ifndef QMUTEX_RECURSIVE_H
#define QMUTEX_RECURSIVE_H

#include "./qmutex_p.h"

/// Same as QMutex, but won't dead-lock if local-thread already holds a lock.
///
/// Instead, this auto increases and decreases an internal lock-counter.
///
/// Before XD5, this feature was internal and was only available through a
/// separate allocation for QRecursiveMutexPrivate, then setting `d_ptr` to that.
class QRecursiveMutex : public QMutex, private QRecursiveMutexPrivate {
public:
    Q_ALWAYS_INLINE QRecursiveMutex()
    {
        this->d_ptr.store(static_cast<QMutexData *>(this));
    }

    Q_ALWAYS_INLINE ~QRecursiveMutex()
    {
        // Skips QMutex's checks.
        this->d_ptr.store(Q_NULLPTR);
    }

    Q_ALWAYS_INLINE void lock() QT_MUTEX_LOCK_NOEXCEPT { QMutex::lock(); }
    Q_ALWAYS_INLINE void unlock() Q_DECL_NOTHROW { QMutex::unlock(); }
};

#endif // QMUTEX_RECURSIVE_H
