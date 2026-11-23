// Copyright (C) 2015 The XD Company Ltd.
// SPDX-License-Identifier: Apache-2.0

#include "./config-cache.h"

#include <QtCore/QThreadStorage>

/*!
    @class Config::Cache
    @internal

    Keeps resolved settings that do not read their place while running.
*/

namespace {

class CacheState
{
public:
    inline CacheState() : active(Q_NULLPTR), skipped(false) {}

    QString root;
    Config::CacheScope *active;
    bool skipped;
};

Q_GLOBAL_STATIC(QThreadStorage<CacheState >, cacheState)

} // namespace

/*!
    Returns the value kept for @a key and @a program, or an invalid value
    when absent. Sets @a found when it is not null.
*/
QVariant Config::Cache::get(
        const QString & key, const QStringList & program, bool *found) const
{
    const QHash<QString, Entry >::const_iterator entry =
        this->m_values.constFind(key);
    if (entry != this->m_values.constEnd()) {
        if (entry->program == program) {
            if (found) {
                *found = true;
            }
            return entry->value;
        }
    }
    if (found) {
        *found = false;
    }
    return QVariant();
}

/*!
    Keeps @a value for @a key and @a program when it does not depend on
    the place.
*/
void Config::Cache::add(
        const QString & key, const QStringList & program, const QVariant & value)
{
    if ( ! Cache::skipsFor(key)) {
        Entry entry;
        entry.program = program;
        entry.value = value;
        this->m_values.insert(key, entry);
    }
}

/*!
    Removes the values kept in the cache.
*/
void Config::Cache::clear()
{
    this->m_values.clear();
}

/*!
    Tracks the setting @a key while its program runs. An empty key leaves
    the current tracking scope unchanged.
*/
Config::CacheScope::CacheScope(const QString & key)
    : m_target(key), m_parent(Q_NULLPTR), m_active( ! key.isEmpty())
{
    if ( ! this->m_active) {
        return;
    }
    CacheState *state = &(cacheState()->localData());
    if ( ! state->active) {
        state->root = key;
        state->skipped = false;
    }
    this->m_parent = state->active;
    state->active = this;
}

/*!
    Restores the enclosing tracking scope.
*/
Config::CacheScope::~CacheScope()
{
    if (this->m_active) {
        CacheState *state = &(cacheState()->localData());
        state->active = this->m_parent;
    }
}

/*!
    Marks the setting running in this thread as dependent on its place.
*/
void Config::Cache::skip()
{
    CacheState *state = &(cacheState()->localData());
    if (state->active) {
        state->skipped = true;
    }
}

/*!
    Tells whether @a key is being resolved in this thread.
*/
bool Config::Cache::isResolving(const QString & key)
{
    const CacheState *state = &(cacheState()->localData());
    const CacheScope *entry = state->active;
    for (; entry; entry = entry->m_parent) {
        if (entry->m_target.compare(key, Qt::CaseInsensitive) == 0) {
            return true;
        }
    }
    return false;
}

/*!
    Tells whether the last setting evaluated in this thread was @a key and
    read its place.
*/
bool Config::Cache::skipsFor(const QString & key)
{
    const CacheState *state = &(cacheState()->localData());
    if ( ! state->skipped) {
        return false;
    }
    const CacheScope *entry = state->active;
    for (; entry; entry = entry->m_parent) {
        if (entry->m_target.compare(key, Qt::CaseInsensitive) == 0) {
            return true;
        }
    }
    return state->root.compare(key, Qt::CaseInsensitive) == 0;
}
