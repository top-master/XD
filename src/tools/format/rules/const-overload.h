// Copyright (C) 2015 The XD Company Ltd.
// SPDX-License-Identifier: Apache-2.0

#ifndef RULES_CONST_OVERLOAD_H
#define RULES_CONST_OVERLOAD_H

#include "../format-named-rule.h"

class FormatConstOverloadRule : public FormatNamedRule
{
    typedef FormatNamedRule super;
public:
    inline FormatConstOverloadRule()
        : super("const-overload-order",
                "Reports a non-const overload declared before its const twin.")
    {
    }

    void run(FormatDocument *document) const Q_DECL_OVERRIDE;
};

#endif // RULES_CONST_OVERLOAD_H
