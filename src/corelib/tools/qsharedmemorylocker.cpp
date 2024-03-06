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

#include <QtCore/private/qsharedmemory_p.h>


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
    QSharedMemory *mem = this->memory();
    if (mem) {
        QSharedMemoryLockerList *list = staticList();
        QMutexLocker _(o ? &list->mutex : Q_NULLPTR);
        if ( ! o || ! list->contains(qMakePair(o, val))) {
            QLatin1String error;
            if (mem->isAttached()) {
                locked = mem->lock();
                if ( ! locked) {
                    error = QLL("Failed lock already-attached shared-memory.");
                }
            } else if (mem->createAndLock(size)) {
                locked = true;
                void *data = mem->data();
                if (data) {
                    constructor(data);
                } else {
                    error = QLL("Failed to access shared-memory after creation.");
                }
            } else if(mem->error() == QSharedMemory::AlreadyExists
                && mem->attach())
            {
                locked = mem->lock();
                if ( ! locked) {
                    error = QLL("Failed to lock already-existing shared-memory.");
                }
            }
            if (locked) {
                if (o)
                    list->append(qMakePair(o, val));
                val |= quintptr(1u);
            } else {
                // Error reporting.
                QSharedMemoryPrivate *memD = QSharedMemoryPrivate::get(mem);
                if ( ! error.isEmpty()) {
                    QString oldError = memD->errorString;
                    if (oldError.isEmpty()) {
                        memD->errorString = error;
                        memD->error = QSharedMemory::LockError;
                    } else {
                        QLL separator("\n");
                        QString msg;
                        msg.reserve(oldError.size() + separator.size() + error.size());
                        msg += error;
                        msg += separator;
                        msg += oldError;
                        memD->errorString = msg;
                        memD->error = memD->error != QSharedMemory::NoError
                                ? memD->error
                                : QSharedMemory::LockError;
                    }
                }
                Q_ASSERT(false, qPrintable(memD->errorString));
            }
        }
    }
    return locked;
}

bool QSharedMemoryLockerBase::unlock()
{
    if (isLocked()) {
        QSharedMemoryLockerList *list = staticList();
        QMutexLocker _(&list->mutex);
        if (isLocked()) {
            val &= ~quintptr(1u);
            if (o) list->removeOne(qMakePair(o, val));
            return memory()->unlock();
        }
    }
    return false;
}
