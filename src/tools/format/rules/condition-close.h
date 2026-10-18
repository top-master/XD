// Copyright (C) 2015 The XD Company Ltd.
// SPDX-License-Identifier: Apache-2.0

#ifndef RULES_CONDITION_CLOSE_H
#define RULES_CONDITION_CLOSE_H

#include "../format-named-rule.h"

class FormatConditionCloseRule : public FormatNamedRule
{
    typedef FormatNamedRule super;
public:
    inline FormatConditionCloseRule()
        : super("condition-close",
                "Closes a multi-line condition with \") {\" on its own line.")
    {
    }

    void run(FormatDocument *document) const Q_DECL_OVERRIDE;
};

#endif // RULES_CONDITION_CLOSE_H
