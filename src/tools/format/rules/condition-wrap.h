// Copyright (C) 2015 The XD Company Ltd.
// SPDX-License-Identifier: Apache-2.0

#ifndef RULES_CONDITION_WRAP_H
#define RULES_CONDITION_WRAP_H

#include "../format-named-rule.h"

class FormatConditionLayout;

class FormatConditionWrapRule : public FormatNamedRule
{
    typedef FormatNamedRule super;
public:
    inline FormatConditionWrapRule()
        : super("condition-wrap",
                "Breaks a long condition before each && and ||, by nesting level.")
    {
    }

    void run(FormatDocument *document) const Q_DECL_OVERRIDE;

private:
    bool isSplitTooLong(const FormatDocument *document,
                        const FormatConditionLayout & layout, int open,
                        int close) const;
};

#endif // RULES_CONDITION_WRAP_H
