// Copyright (C) 2015 The XD Company Ltd.
// SPDX-License-Identifier: Apache-2.0

#ifndef RULES_ARGS_WRAP_H
#define RULES_ARGS_WRAP_H

#include "../format-named-rule.h"

class FormatScopeConditions;

class FormatArgsWrapRule : public FormatNamedRule
{
    typedef FormatNamedRule super;
public:
    inline FormatArgsWrapRule()
        : super("args-wrap",
                "Breaks a wrapped call or brace list after its opening, keeping line groups.")
    {
    }

    void run(FormatDocument *document) const Q_DECL_OVERRIDE;

private:
    bool isCallParen(const FormatDocument *document, int paren) const;
    int controlKeyword(const FormatDocument *document, int paren) const;
    bool mayBreakAt(
            const FormatDocument *document, FormatScopeConditions *conditions,
            int open) const;
};

#endif // RULES_ARGS_WRAP_H
