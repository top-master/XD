// Copyright (C) 2015 The XD Company Ltd.
// SPDX-License-Identifier: Apache-2.0

#include "./format-rule.h"

/*!
    @class FormatRule
    @internal

    Finds one formatting rule's breaks in a FormatDocument, and queues the
    fix of each one it can fix safely with FormatDocument::replace(), or
    reports it with FormatDocument::report() otherwise.

    @fn QByteArray FormatRule::name() const

    Names the rule, the way `--rules` and the issues print it.

    @fn QString FormatRule::summary() const

    Says in one line what the rule checks, for `--list-rules`.

    @fn void FormatRule::run(FormatDocument *document) const

    Queues the fixes and reports the issues of this rule in @a document.
*/

/*!
    Destroys the rule.
*/
#include "./format-document.h"

FormatRule::~FormatRule()
{
}

QVariant FormatRule::cache(const FormatDocument *document, const QByteArray &key) const
{
    return document->ruleCache(this, key);
}

void FormatRule::setCache(FormatDocument *document, const QByteArray &key,
                          const QVariant &value) const
{
    document->setRuleCache(this, key, value);
}

/*!
    Tells whether the rule reads files other than the one it formats, such
    as the related files of a class; a run then hands each file of the run
    to the rule as the run found it, before any rule changed it.
*/
bool FormatRule::readsOtherFiles() const
{
    return false;
}
