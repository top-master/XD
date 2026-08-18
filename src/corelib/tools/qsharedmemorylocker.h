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

#ifndef EXTRAS_QSHAREDMEMORYLOCKER_H
#define EXTRAS_QSHAREDMEMORYLOCKER_H

#include <QtCore/QSharedMemory>
#include <QtCore/QMutex>
#include <QtCore/QList>
#include <QtCore/QPair>


QT_BEGIN_NAMESPACE

class Q_CORE_EXPORT QSharedMemoryLockerBase
{
protected:
    QSharedMemoryLockerBase(QSharedMemory *ptr, const void* owner = 0);
    ~QSharedMemoryLockerBase()
    {
        unlock();
        this->val = 0;
#if !QT_PTR_TAGGING
        this->m_locked = false;
#endif
    }

    typedef void *(*Constructor)(void *);

    /// @note Generates "N" binery outputs if "N = types * files"
    /// (i.e. generates compile/binery output once per type name "T" for every
    /// .exe or .dll file using that type like "QSharedMemoryLocker<TypeName>").
    template <typename T>
    static T *constructType(void *where)
        { return new (where) T(); }

    bool connect(Constructor constructor, int size);

    inline QSharedMemory *memory() const {
#if QT_PTR_TAGGING
        return reinterpret_cast<QSharedMemory *>(val & ~quintptr(1u));
#else
        return val;
#endif
    }

public:

    bool unlock();

#if QT_PTR_TAGGING
    inline bool isLocked() const { return((val & quintptr(1u)) == quintptr(1u)); }
#else
    inline bool isLocked() const { return m_locked; }
#endif

    inline bool isNull() const
        { return !memory()->constData(); }

    // TODO: maybe support override, like:
    // ```
    // void reset(QSharedMemory *other = Q_NULLPTR);
    // ```

    /// @warning After calling this, all methods will have undefined-behavior, but
    /// the result should be safe to use as long as it's non-null.
    inline QSharedMemory *take()
    {
        QSharedMemory *oldD = memory();
        val = 0;
#if !QT_PTR_TAGGING
        m_locked = false;
#endif
        return oldD;
    }

    inline QString lastError() const { return memory()->errorString(); }

private:
    Q_DISABLE_COPY(QSharedMemoryLockerBase)

protected:
    const void* o;
#if QT_PTR_TAGGING
    quintptr val;
#else
    QSharedMemory *val;
    bool m_locked;
#endif
};

template <typename T>
class QSharedMemoryLocker : public QSharedMemoryLockerBase
{
    typedef QSharedMemoryLockerBase super;
#ifndef Q_CC_NOKIAX86
#if QT_PTR_TAGGING
    typedef quintptr QSharedMemoryLocker:: *SafeBool;
#else
    typedef QSharedMemory *QSharedMemoryLocker:: *SafeBool;
#endif
#endif
public:
    /// Constructs the class of type "T" at given QSharedMemory and locks it, or
    /// waits until already existing can be locked.
    ///
    /// If @p owner is set, then QSharedMemoryLocker waits only for
    /// lock if not already locked by same owner.
    ///
    inline explicit QSharedMemoryLocker(const QSharedMemory *ptr = 0, const void* owner = 0)
        : super(const_cast<QSharedMemory *>(ptr), owner)
    {
        relock();
    }

    template <typename OwnerClass>
    inline explicit QSharedMemoryLocker(const OwnerClass *owner)
        : super(const_cast<QSharedMemory *>(owner->ds_func()), owner)
    {
        relock();
    }

    inline ~QSharedMemoryLocker()
    {}

    /// Tries to lock again, after previous @ref unlock.
    ///
    /// @see super::lock
    /// @see super::isLocked
    inline bool relock() {
        typedef T*(*ConstructPtr)(void*);
        ConstructPtr cptr = constructType<T>; //supports mingw compiler by using cptr variable
        return connect(reinterpret_cast<Constructor>(cptr), sizeof(T));
    }

    inline T &operator*() const
    {
        //Q_ASSERT(memory());
        return *reinterpret_cast<T *>(super::memory()->data());
    }

    inline T *operator->() const
    {
        //Q_ASSERT(memory());
        return reinterpret_cast<T *>(super::memory()->data());
    }

    inline bool operator!() const
        { return ! super::memory()->constData(); }

#if defined(Q_CC_NOKIAX86) || defined(Q_QDOC)
    inline operator bool() const
        { return super::memory()->constData() != Q_NULLPTR; }
#else
    inline operator SafeBool() const
        { return super::memory()->constData() ? &QSharedMemoryLocker::val : Q_NULLPTR; }
#endif

    inline T *data() const
        { return reinterpret_cast<T *>(super::memory()->data()); }

    inline void swap(QSharedMemoryLocker<T> &other)
    {
        qSwap(val, other.val);
#if !QT_PTR_TAGGING
        qSwap(m_locked, other.m_locked);
#endif
    }

    typedef T *pointer;
    typedef T type;

private:
    Q_DISABLE_COPY(QSharedMemoryLocker)
};

/// @param SharedDataClass Type of the class to share.
/// @param Dptr Any logic that results to pointer of a QSharedMemory.
#define Q_DECLARE_SHARED_MEMORY_D(SharedDataClass, Dptr) \
    inline QSharedMemory *ds_func() { return Dptr; } \
    inline const QSharedMemory* ds_func() const { return Dptr; } \
    typedef QSharedMemoryLocker<SharedDataClass> SharedMemoryLocker; \
    friend class QSharedMemoryLocker<SharedDataClass>;

#define Q_DECLARE_SHARED_MEMORY(SharedDataClass) Q_DECLARE_SHARED_MEMORY_D(SharedDataClass, this)

#define Q_DS(Class) Class::SharedMemoryLocker ds(this->ds_func(), this)

QT_END_NAMESPACE

#endif //EXTRAS_QSHAREDMEMORYLOCKER_H
