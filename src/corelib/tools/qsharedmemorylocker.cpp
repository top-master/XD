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

#include "qsharedmemorylocker.h"


QSharedMemoryLockerBase::QSharedMemoryLockerBase(QSharedMemory *ptr, const void *owner)
    : o(owner)
{
    // Intentionally NOT in header, else for some compilers the
    // assert may fail even if `ptr` is aligned correctly.
    Q_ASSERT_X((reinterpret_cast<quintptr>(ptr) & quintptr(1u)) == quintptr(0),
               "QSharedMemoryLocker", "QSharedMemory pointer is misaligned");
    val = reinterpret_cast<quintptr>(ptr);
}

class QSharedMemoryLockerList : public QList< QPair< const void* , quintptr > >
{
public:
    QMutex mutex;
};
Q_GLOBAL_STATIC(QSharedMemoryLockerList, staticList)

#ifndef QT_NO_DEBUG
// Comment below only for debug.
#   define QT_NO_DEBUG
#endif

#undef Q_ASSERT
#ifndef QT_NO_DEBUG
#   define Q_ASSERT(cond, x) ((!(cond)) ? qt_assert(x, "QSharedMemoryLocker.cpp", __LINE__) : qt_noop())
#else
#   define Q_ASSERT(cond, x) do {} while(0)
#endif

bool QSharedMemoryLockerBase::connect(QSharedMemoryLockerBase::Constructor constructor, int size)
{
    bool locked = false;
    if (memory()) {
        QMutexLocker _(o ? &staticList()->mutex : Q_NULLPTR);
        if ( ! o || ! staticList()->contains(qMakePair(o, val))) {
            if (memory()->isAttached()) {
                locked = memory()->lock();
                Q_ASSERT(locked, "lock attached failed.");
            } else if (memory()->create(size)) {
                locked = memory()->lock();
                Q_ASSERT(locked, "lock after creation failed.");
                if (locked)
                    constructor(memory()->data());
            } else if(memory()->error() == QSharedMemory::AlreadyExists
                && memory()->attach())
            {
                locked = memory()->lock();
                Q_ASSERT(locked, "lock already-existing failed.");
            } else {
                Q_ASSERT(false, qPrintable(memory()->errorString()));
            }
            if (locked) {
                if (o)
                    staticList()->append(qMakePair(o, val));
                val |= quintptr(1u);
            }
        }
    }
    return locked;
}

bool QSharedMemoryLockerBase::unlock()
{
    if (isLocked()) {
        QMutexLocker _(&staticList()->mutex);
        if (isLocked()) {
            val &= ~quintptr(1u);
            if (o) staticList()->removeOne(qMakePair(o, val));
            return memory()->unlock();
        }
    }
    return false;
}
