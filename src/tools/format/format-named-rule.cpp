// Copyright (C) 2015 The XD Company Ltd.
// SPDX-License-Identifier: Apache-2.0

#include "./format-named-rule.h"

/*!
    @class FormatNamedRule
    @internal

    Carries a rule's name and summary for its subclasses, which then only
    implement FormatRule::run().
*/

/*!
    Builds the rule named @a name, which @a summary describes in one line.
*/
FormatNamedRule::FormatNamedRule(const char *name, const char *summary)
    : m_name(name), m_summary(QString::fromLatin1(summary))
{
}

/*!
    Names the rule, the way `--rules` and the issues print it.
*/
QByteArray FormatNamedRule::name() const
{
    return this->m_name;
}

/*!
    Says in one line what the rule checks.
*/
QString FormatNamedRule::summary() const
{
    return this->m_summary;
}
