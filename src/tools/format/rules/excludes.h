// Copyright (C) 2015 The XD Company Ltd.
// SPDX-License-Identifier: Apache-2.0

#ifndef RULES_EXCLUDES_H
#define RULES_EXCLUDES_H

#include "../format-named-rule.h"

class FormatExcludesRule : public FormatNamedRule
{
    typedef FormatNamedRule super;
public:
    inline FormatExcludesRule()
        : super("excludes",
                "Reports !flags.includes(...) checks that excludes() states.")
    {
    }

    void run(FormatDocument *document) const Q_DECL_OVERRIDE;
};

#endif // RULES_EXCLUDES_H
