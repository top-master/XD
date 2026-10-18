// Copyright (C) 2015 The XD Company Ltd.
// SPDX-License-Identifier: Apache-2.0

#ifndef RULES_OPERATOR_WRAP_H
#define RULES_OPERATOR_WRAP_H

#include "../format-named-rule.h"


class FormatOperatorWrapRule : public FormatNamedRule
{
    typedef FormatNamedRule super;
public:
    inline FormatOperatorWrapRule()
        : super("operator-wrap",
                "Breaks a code line past the column limit before the operators of a chain.")
    {
    }

    void run(FormatDocument *document) const Q_DECL_OVERRIDE;

private:
    bool wholeStatement(const FormatDocument *document, int line, int *first,
                        int *last) const;
};

#endif // RULES_OPERATOR_WRAP_H
