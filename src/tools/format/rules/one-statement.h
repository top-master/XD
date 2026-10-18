// Copyright (C) 2015 The XD Company Ltd.
// SPDX-License-Identifier: Apache-2.0

#ifndef RULES_ONE_STATEMENT_H
#define RULES_ONE_STATEMENT_H

#include "../format-named-rule.h"

class FormatOneStatementRule : public FormatNamedRule
{
    typedef FormatNamedRule super;
public:
    inline FormatOneStatementRule()
        : super("one-statement", "Puts one statement per line.")
    {
    }

    void run(FormatDocument *document) const Q_DECL_OVERRIDE;
};

#endif // RULES_ONE_STATEMENT_H
