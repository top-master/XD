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

#ifndef QATOMICFLAGS_H
#define QATOMICFLAGS_H

#include <QtCore/qbasicatomic.h>

/**
 * Helper to atomically toggle bits of given @tparam IntType, like flags.
 *
 * This for example allows a single 32 bit integer to be treated as 32 atomic booleans.
 *
 * WARNING: this class needs to be manually initialized, hence
 * either use @ref QAtomicFlags class instead, or initialize like:
 * ```
 * static QBasicAtomicFlags<MyEnum> myGlobalFlags = Q_BASIC_ATOMIC_INITIALIZER(0);
 * ```
 * Or like:
 * ```
 * myFlags.storeUnsafe(0);
 * ```
 *
 * WARNING: only @ref append and @ref remove can be used without locking a mutex, and
 * otherwise, either your logic needs to work even with relaxed memory-order, or,
 * you should lock a mutex before calling ANY of this class's methods, because
 * although this class uses "Acquire" memory-order to force loading the
 * changes made by other threads, that can NOT force the other threads to run faster,
 * which may result to behaving same as relaxed memory-order and missing said changes.
 *
 * @warning The @tparam FlagType enum is not allowed to contain zero, and
 * zero means there is no single flag set yet.
 */
template <typename FlagType, typename IntType = int>
class QBasicAtomicFlags {
public:
    /// @warning Internal, only public to allow #Q_BASIC_ATOMIC_INITIALIZER usage.
    QBasicAtomicInteger<IntType> _q_value;


    /// Checks with relaxed memory-order whether given @p f flag is set.
    ///
    /// WARNING: does not support @p f being zero, to increase speed.
    Q_ALWAYS_INLINE bool includes(FlagType f) const Q_DECL_NOTHROW
    { return (this->_q_value.loadAcquire() & IntType(f)) == IntType(f); }

    /// Checks with relaxed memory-order whether given @p f flag is unset.
    ///
    /// WARNING: does not support @p f being zero, to increase speed.
    Q_ALWAYS_INLINE bool excludes(FlagType f) const Q_DECL_NOTHROW
    { return (this->_q_value.loadAcquire() & IntType(f)) != IntType(f); }

    Q_ALWAYS_INLINE IntType toInt() const Q_DECL_NOTHROW { return this->_q_value.loadAcquire(); }

    /// Races with other threads for being first to enable given @p f flag.
    /// @returns @c true if won said race.
    Q_ALWAYS_INLINE bool append(FlagType f) Q_DECL_NOTHROW
    {
        IntType tmp = this->_q_value.load();
        while ((tmp & IntType(f)) != IntType(f)) {
            if (this->_q_value.testAndSetRelaxed(tmp, (tmp | IntType(f)), tmp)) {
                return true;
            }
        }
        return false;
    }

    /// Races with other threads for being first to disable given @p f flag.
    /// @returns @c true if won said race.
    Q_ALWAYS_INLINE bool remove(FlagType f) Q_DECL_NOTHROW
    {
        IntType tmp = this->_q_value.load();
        while ((tmp & IntType(f)) == IntType(f)) {
            if (this->_q_value.testAndSetRelaxed(tmp, (tmp & ~IntType(f)), tmp)) {
                return true;
            }
        }
        return false;
    }


    /// Same as includes(FlagType), however, uses relaxed memory-order.
    Q_ALWAYS_INLINE bool cacheIncludes(FlagType f) const Q_DECL_NOTHROW
    { return (this->_q_value.load() & IntType(f)) == IntType(f); }

    /// Same as excludes(FlagType), however, uses relaxed memory-order.
    Q_ALWAYS_INLINE bool cacheExcludes(FlagType f) const Q_DECL_NOTHROW
    { return (this->_q_value.load() & IntType(f)) != IntType(f); }

    /// Same as toInt(), however, uses relaxed memory-order.
    Q_ALWAYS_INLINE IntType cacheToInt() const Q_DECL_NOTHROW { return this->_q_value.load(); }


    /// WARNING: sets raw-value without checking other threads, hence
    /// use the append(FlagType) or remove(FlagType) instead of this method.
    Q_ALWAYS_INLINE void storeUnsafe(IntType newValue) Q_DECL_NOTHROW
    {
        this->_q_value.store(newValue);
    }

#ifdef QT_BASIC_ATOMIC_HAS_CONSTRUCTORS
#  ifdef Q_COMPILER_DEFAULT_MEMBERS
    QAtomicFlags() = default;
#  else
    inline QAtomicFlags()
    {
    }
#  endif
    constexpr explicit QAtomicFlags(IntType value) Q_DECL_NOTHROW : _q_value(value) {}

private:
    Q_DISABLE_COPY(QAtomicFlags)
#endif // QT_BASIC_ATOMIC_HAS_CONSTRUCTORS
};

/**
 * Same as @ref QBasicAtomicFlags, however, this initializes to zero and
 * provides copy-constructor and copy-assign-operator.
 *
 * @note Use QBasicAtomicFlags which supports initializer-list, to
 * increase your static-variable performance.
 */
template <typename FlagType, typename IntType = int>
class QAtomicFlags : public QBasicAtomicFlags<FlagType, IntType > {
    typedef QBasicAtomicFlags<FlagType, IntType > super;
public:
#ifdef QT_BASIC_ATOMIC_HAS_CONSTRUCTORS
    constexpr explicit QAtomicFlags(IntType value = 0) Q_DECL_NOTHROW
        : super(value)
    {
    }
#else
    inline explicit QAtomicFlags(IntType value = 0) Q_DECL_NOTHROW
    {
        this->_q_value.store(value);
    }
#endif

    inline QAtomicFlags(const QAtomicFlags<FlagType, IntType > &other) Q_DECL_NOTHROW
    {
        this->_q_value.storeRelease(other._q_value.loadAcquire());
    }

    inline QAtomicFlags<FlagType, IntType > &operator=(const QAtomicFlags<FlagType, IntType > &other) Q_DECL_NOTHROW
    {
        this->_q_value.storeRelease(other.loadAcquire());
        return *this;
    }
};

#endif // QATOMICFLAGS_H
