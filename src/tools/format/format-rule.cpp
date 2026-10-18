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
FormatRule::~FormatRule()
{
}
