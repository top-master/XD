// Copyright (C) 2015 The XD Company Ltd.
// SPDX-License-Identifier: Apache-2.0

#ifndef RULES_TEMPLATE_CLOSE_H
#define RULES_TEMPLATE_CLOSE_H

#include "../format-named-rule.h"

class FormatTemplateCloseRule : public FormatNamedRule
{
    typedef FormatNamedRule super;
public:
    inline FormatTemplateCloseRule()
        : super("template-close-space",
                "Puts a space before each > that closes a template or a cast.")
    {
    }

    void run(FormatDocument *document) const Q_DECL_OVERRIDE;

private:
    bool spaceArguments(FormatDocument *document, int open, int *end) const;
};

#endif // RULES_TEMPLATE_CLOSE_H
