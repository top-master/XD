// Copyright (C) 2015 The XD Company Ltd.
// SPDX-License-Identifier: Apache-2.0

#include "./config-evaluator-context.h"

/*!
    @class Config::Evaluator::Context
    @internal

    Answers what a condition asks for at the place its caller evaluates it
    (see Config::Condition::evaluate()): its names, before the caller's set
    of names does, and the percent value; the objects a tool adds to a scope
    of Config::Scopes reach it as well, and answer from it (see
    Config::Object::context()).
*/

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

/*!
    Builds in @a value the value that the percent terms of a condition compare
    with (from 0 to 100, or past it); returns false when the context has none.
*/
bool Config::Evaluator::Context::percent(qreal *) const
{
    return false;
}
