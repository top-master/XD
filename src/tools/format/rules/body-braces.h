// Copyright (C) 2015 The XD Company Ltd.
// SPDX-License-Identifier: Apache-2.0

#ifndef RULES_BODY_BRACES_H
#define RULES_BODY_BRACES_H

#include "../format-named-rule.h"

class FormatBodyBracesRule : public FormatNamedRule
{
    typedef FormatNamedRule super;
public:
    inline FormatBodyBracesRule()
        : super("body-braces", "Gives a function, class, or enum body that closes in a chain of brackets (`} }`) its own lines.")
    {
    }

    void run(FormatDocument *document) const Q_DECL_OVERRIDE;
};

#endif // RULES_BODY_BRACES_H
