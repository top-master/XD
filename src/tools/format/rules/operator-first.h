// Copyright (C) 2015 The XD Company Ltd.
// SPDX-License-Identifier: Apache-2.0

#ifndef RULES_OPERATOR_FIRST_H
#define RULES_OPERATOR_FIRST_H

#include "../format-named-rule.h"

class FormatOperatorFirstRule : public FormatNamedRule
{
    typedef FormatNamedRule super;
public:
    inline FormatOperatorFirstRule()
        : super("operator-first",
                "Breaks a long expression before an operator, not after it.")
    {
    }

    void run(FormatDocument *document) const Q_DECL_OVERRIDE;
};

#endif // RULES_OPERATOR_FIRST_H
