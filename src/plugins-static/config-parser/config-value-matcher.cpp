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
    Tells whether this matcher reads a value as the file writes it, with its
    double quotes, such as a condition that passes a text to a function
    (`hasStd("C++11")`); an `.ini` line then keeps the value's quotes and
    escapes for it. Most matchers read a value as `.ini` files spell one.
*/
bool Config::ValueMatcher::keepsText() const
{
    return false;
}
