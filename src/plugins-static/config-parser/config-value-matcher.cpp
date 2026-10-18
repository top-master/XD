// Copyright (C) 2015 The XD Company Ltd.
// SPDX-License-Identifier: Apache-2.0

#include "./config-value-matcher.h"

/*!
    \class Config::ValueMatcher
    \internal

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
    \fn bool Config::ValueMatcher::match(const QString & value, QString *matched) const

    Tells whether \a value is one this matcher takes, and when it is, gives
    \a matched the value to keep (such as a word in its own spelling).
*/

/*!
    \fn QString Config::ValueMatcher::describe() const

    Builds the description of the values this matcher takes, for the error of
    a value no matcher of the key takes.
*/
