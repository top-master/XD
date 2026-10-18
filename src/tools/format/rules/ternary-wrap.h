// Copyright (C) 2015 The XD Company Ltd.
// SPDX-License-Identifier: Apache-2.0

#ifndef RULES_TERNARY_WRAP_H
#define RULES_TERNARY_WRAP_H

#include "../format-named-rule.h"

class FormatTernaryWrapRule : public FormatNamedRule
{
    typedef FormatNamedRule super;
public:
    inline FormatTernaryWrapRule()
        : super("ternary-wrap",
                "Breaks a long conditional expression before ?, then before :.")
    {
    }

    void run(FormatDocument *document) const Q_DECL_OVERRIDE;
};

#endif // RULES_TERNARY_WRAP_H
