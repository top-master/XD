// Copyright (C) 2015 The XD Company Ltd.
// SPDX-License-Identifier: Apache-2.0

#ifndef RULES_PRIVATE_ACCESS_H
#define RULES_PRIVATE_ACCESS_H

#include "../format-named-rule.h"

class FormatPrivateAccessRule : public FormatNamedRule
{
    typedef FormatNamedRule super;
public:
    inline FormatPrivateAccessRule()
        : super("q-d-macro",
                "Reports d_func() and q_func() calls where Q_D or Q_Q fit.")
    {
    }

    void run(FormatDocument *document) const Q_DECL_OVERRIDE;
};

#endif // RULES_PRIVATE_ACCESS_H
