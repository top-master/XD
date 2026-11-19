// Copyright (C) 2015 The XD Company Ltd.
// SPDX-License-Identifier: Apache-2.0

#include "./config-value-matcher.h"

/*!
    @class Config::ValueMatcher
    @internal

    Decides which values a setting takes: a key lists its matchers,
    and a value counts when one of them matches it.
    A tool subclasses it for a value form of its own;
    Config::Word and Config::AnySpace cover the common ones.
*/

/*!
    Destroys the matcher.
*/
Config::ValueMatcher::~ValueMatcher()
{
}

/*!
    Builds why this matcher does not take @a value, for the error of a value
    no matcher of the key takes, such as an undefined name a sign follows;
    empty when the description of the values it takes says enough (see
    describe()).
*/
QString Config::ValueMatcher::explain(const QString &) const
{
    return QString();
}

/*!
    Converts @a answer, what a key's compiled program gives (see
    Config::Condition::answer()), into @a value, the value of the type the
    key takes; returns false when this matcher compiles no program, or
    takes no such answer, hence the next matcher of the key may.
*/
bool Config::ValueMatcher::convert(const QVariant &, QVariant *) const
{
    return false;
}
