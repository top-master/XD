// Copyright (C) 2015 The XD Company Ltd.
// SPDX-License-Identifier: Apache-2.0

#include "./config-evaluator-context.h"

/*!
    @class Config::Evaluator::Context
    @internal

    Answers what a condition asks for at the place its caller evaluates it
    (see Config::Condition::eval()): its names, before the caller's set
    of names does; the objects a tool adds to a scope of Config::Scopes reach
    it as well, and answer from it (see Config::Object::context()). Construct
    a context with `false` when it has no place to answer for. Each context
    belongs to one thread; each operation records its final status after any
    inner call that reused the context finishes.
*/

/*!
    Lists the predicate names this context can resolve.
*/
QStringList Config::Evaluator::CompileContext::names() const
{
    if ( ! this->m_isOnNamesCalled) {
        this->m_names = this->onNames();
        this->m_isOnNamesCalled = true;
    }
    return this->m_names;
}

/*!
    Builds the predicate names that names() caches.
*/
QStringList Config::Evaluator::CompileContext::onNames() const
{
    return this->m_names;
}

/*!
    Lists the allowed words in a choice. An empty list takes no choice;
    anyEnumKeys() takes any word.
*/
QStringList Config::Evaluator::CompileContext::enumKeys() const
{
    if ( ! this->m_isOnEnumKeysCalled) {
        this->m_enumKeys = this->onEnumKeys();
        this->m_isOnEnumKeysCalled = true;
    }
    return this->m_enumKeys;
}

/*!
    Builds the choice words that enumKeys() caches.
*/
QStringList Config::Evaluator::CompileContext::onEnumKeys() const
{
    return this->m_enumKeys;
}

/*!
    Tells in @a value whether the name @a name (in the form without `_` and
    `-` that Config::Condition::canonicalName() gives it) holds where the
    condition is evaluated, before the caller's set of names is asked, and
    for a call `name()`; returns false when the context does not know the
    name, which leaves it to that set.
*/
bool Config::Evaluator::Context::predicate(const QString &, bool *) const
{
    return false;
}

/*! Builds the effective settings at this place, when available. */
const Config::Settings *Config::Evaluator::Context::settings() const
{
    return Q_NULLPTR;
}

/*!
    Resolves a threshold program at a given percentage, when the place
    supports measuring one; an invalid value means it cannot be resolved.
*/
QVariant Config::Evaluator::Context::resolveAtPercent(const QStringList &, int) const
{
    return QVariant();
}
