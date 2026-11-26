// Copyright (C) 2015 The XD Company Ltd.
// SPDX-License-Identifier: Apache-2.0

#ifndef RULES_CONTINUATION_INDENT_H
#define RULES_CONTINUATION_INDENT_H

#include "../format-named-rule.h"

class FormatContinuationIndentRule : public FormatNamedRule
{
    typedef FormatNamedRule super;
public:
    inline FormatContinuationIndentRule()
        : super("continuation-indent",
                "Indents operator-led lines 4 spaces past the line of their group.")
    {
    }

    void run(FormatDocument *document) const Q_DECL_OVERRIDE;
};

#endif // RULES_CONTINUATION_INDENT_H
