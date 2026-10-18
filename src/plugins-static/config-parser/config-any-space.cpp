// Copyright (C) 2015 The XD Company Ltd.
// SPDX-License-Identifier: Apache-2.0

#include "./config-any-space.h"

/*!
    @class Config::AnySpace
    @internal

    Matches any run of spaces and/or tabs, the empty one included, and
    keeps it as it is, so that a setting can ask for one space, two, a tab,
    or no space at all.
*/

/*!
    Builds a shared matcher of any run of spaces and/or tabs, for the
    matchers of a key.
*/
QSharedPointer<const Config::ValueMatcher > Config::AnySpace::create()
{
    return QSharedPointer<const Config::ValueMatcher >(new Config::AnySpace);
}

/*!
    Matches @a value when it holds only spaces and/or tabs.
*/
bool Config::AnySpace::match(const QString & value, QString *matched) const
{
    for (int i = 0; i < value.size(); ++i) {
        if (value.at(i) != QLatin1Char(' ')
            && value.at(i) != QLatin1Char('\t')
        ) {
            return false;
        }
    }
    *matched = value;
    return true;
}

/*!
    Builds the description of a run of spaces and/or tabs.
*/
QString Config::AnySpace::describe() const
{
    return QString(QLL("spaces and/or tabs (\"\" for none)"));
}
