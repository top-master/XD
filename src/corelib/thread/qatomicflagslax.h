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

#ifndef QATOMICFLAGSLAX_H
#define QATOMICFLAGSLAX_H

#include <QtCore/qatomicflags.h>

/**
 * Same as @ref QBasicAtomicFlags, however, with the opposite memory-order
 * default: its methods without a suffix are the relaxed ones.
 *
 * WARNING: only use this class if your logic seems to mostly use
 * the @c Lax suffixed methods of QAtomicFlags and/or QBasicAtomicFlags,
 * and hence you want @c Lax meanining "Relaxed" memory-order to be the default.
 *
 * WARNING: all methods without @c Sync suffix use "Relaxed" memory-order.
 * Also, even @c Sync read operations can NOT be used as a guard for other reads,
 * unless the write was @c Sync as well.
 */
template <typename FlagType, typename IntType = int>
class QBasicAtomicFlagsLax {
public:
    /// @warning Internal, only public to allow #Q_BASIC_ATOMIC_INITIALIZER usage.
    QBasicAtomicInteger<IntType> _q_value;


    /// Checks whether given @p f flag is set.
    ///
    /// WARNING: does not support @p f being zero, to increase speed.
    Q_ALWAYS_INLINE bool includes(FlagType f) const Q_DECL_NOTHROW
    { return (this->_q_value.load() & IntType(f)) == IntType(f); }

    /// Checks whether given @p f flag is unset.
    ///
    /// WARNING: does not support @p f being zero, to increase speed.
    Q_ALWAYS_INLINE bool excludes(FlagType f) const Q_DECL_NOTHROW
    { return (this->_q_value.load() & IntType(f)) != IntType(f); }

    Q_ALWAYS_INLINE IntType toInt() const Q_DECL_NOTHROW { return this->_q_value.load(); }

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

    /// Short-hand for @ref append or @ref remove depending on given @p conditionMet status.
    Q_ALWAYS_INLINE bool setIf(bool conditionMet, FlagType f) Q_DECL_NOTHROW
    {
        if (conditionMet) {
            return this->append(f);
        }
        return this->remove(f);
    }


    /// Same as includes(FlagType), however, uses acquire memory-order.
    Q_ALWAYS_INLINE bool includesSync(FlagType f) const Q_DECL_NOTHROW
    { return (this->_q_value.loadAcquire() & IntType(f)) == IntType(f); }

    /// Same as excludes(FlagType), however, uses acquire memory-order.
    Q_ALWAYS_INLINE bool excludesSync(FlagType f) const Q_DECL_NOTHROW
    { return (this->_q_value.loadAcquire() & IntType(f)) != IntType(f); }

    /// Same as toInt(), however, uses acquire memory-order.
    Q_ALWAYS_INLINE IntType toIntSync() const Q_DECL_NOTHROW { return this->_q_value.loadAcquire(); }

    /// Same as append(FlagType), however, uses ordered memory-order.
    /// @returns @c true if won said race.
    Q_ALWAYS_INLINE bool appendSync(FlagType f) Q_DECL_NOTHROW
    {
        IntType tmp = this->_q_value.load();
        while ((tmp & IntType(f)) != IntType(f)) {
            if (this->_q_value.testAndSetOrdered(tmp, (tmp | IntType(f)), tmp)) {
                return true;
            }
        }
        return false;
    }

    /// Same as remove(FlagType), however, uses ordered memory-order.
    /// @returns @c true if won said race.
    Q_ALWAYS_INLINE bool removeSync(FlagType f) Q_DECL_NOTHROW
    {
        IntType tmp = this->_q_value.load();
        while ((tmp & IntType(f)) == IntType(f)) {
            if (this->_q_value.testAndSetOrdered(tmp, (tmp & ~IntType(f)), tmp)) {
                return true;
            }
        }
        return false;
    }

    /// Same as setIf(bool, FlagType), however, uses ordered memory-order.
    Q_ALWAYS_INLINE bool setIfSync(bool conditionMet, FlagType f) Q_DECL_NOTHROW
    {
        if (conditionMet) {
            return this->appendSync(f);
        }
        return this->removeSync(f);
    }


    /// WARNING: sets raw-value without checking other threads, hence
    /// use the append(FlagType) or remove(FlagType) instead of this method.
    Q_ALWAYS_INLINE void storeUnsafe(IntType newValue) Q_DECL_NOTHROW
    {
        this->_q_value.store(newValue);
    }

#ifdef QT_BASIC_ATOMIC_HAS_CONSTRUCTORS
#  ifdef Q_COMPILER_DEFAULT_MEMBERS
    QAtomicFlagsLax() = default;
#  else
    inline QAtomicFlagsLax()
    {
    }
#  endif
    constexpr explicit QAtomicFlagsLax(IntType value) Q_DECL_NOTHROW : _q_value(value) {}

private:
    Q_DISABLE_COPY(QAtomicFlagsLax)
#endif // QT_BASIC_ATOMIC_HAS_CONSTRUCTORS
};

/**
 * Same as @ref QAtomicFlags, however, based on @ref QBasicAtomicFlagsLax; hence
 * its plain methods default to relaxed memory-order.
 */
template <typename FlagType, typename IntType = int>
class QAtomicFlagsLax : public QBasicAtomicFlagsLax<FlagType, IntType > {
    typedef QBasicAtomicFlagsLax<FlagType, IntType > super;
public:
#ifdef QT_BASIC_ATOMIC_HAS_CONSTRUCTORS
    constexpr explicit QAtomicFlagsLax(IntType value = 0) Q_DECL_NOTHROW
        : super(value)
    {
    }
#else
    inline explicit QAtomicFlagsLax(IntType value = 0) Q_DECL_NOTHROW
    {
        this->_q_value.store(value);
    }
#endif

    inline QAtomicFlagsLax(const QAtomicFlagsLax<FlagType, IntType > &other) Q_DECL_NOTHROW
    {
        this->_q_value.storeRelease(other._q_value.loadAcquire());
    }

    inline QAtomicFlagsLax<FlagType, IntType > &operator=(const QAtomicFlagsLax<FlagType, IntType > &other) Q_DECL_NOTHROW
    {
        this->_q_value.storeRelease(other._q_value.loadAcquire());
        return *this;
    }
};

#endif // QATOMICFLAGSLAX_H
