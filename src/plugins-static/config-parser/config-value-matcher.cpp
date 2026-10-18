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
