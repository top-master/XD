// Copyright (C) 2015 The XD Company Ltd.
// SPDX-License-Identifier: Apache-2.0

#ifndef RULES_CODE_SHAPE_H
#define RULES_CODE_SHAPE_H

#include "../format-named-rule.h"

class FormatCodeShapeRule : public FormatNamedRule
{
    typedef FormatNamedRule super;
public:
    inline FormatCodeShapeRule()
        : super("code-shape",
                "Reports call derefs, direct-init, big inlines and init lists.")
    {
    }

    void run(FormatDocument *document) const Q_DECL_OVERRIDE;

private:
    void checkStatement(FormatDocument *document, int start) const;
    void checkBody(FormatDocument *document, int brace) const;
    void checkLines(FormatDocument *document) const;
};

#endif // RULES_CODE_SHAPE_H
