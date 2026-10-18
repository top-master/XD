// Copyright (C) 2015 The XD Company Ltd.
// SPDX-License-Identifier: Apache-2.0

#ifndef RULES_CONTROL_BRACES_H
#define RULES_CONTROL_BRACES_H

#include "../format-named-rule.h"

class FormatControlBracesRule : public FormatNamedRule
{
    typedef FormatNamedRule super;
public:
    inline FormatControlBracesRule()
        : super("control-braces",
                "Wraps the body of every control statement in braces.")
    {
    }

    void run(FormatDocument *document) const Q_DECL_OVERRIDE;

private:
    int bodyStart(const FormatDocument *document, int keyword) const;
    void brace(FormatDocument *document, int keyword, int head, int body) const;
    void unfold(FormatDocument *document, int keyword, int brace) const;
};

#endif // RULES_CONTROL_BRACES_H
