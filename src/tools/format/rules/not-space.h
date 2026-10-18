// Copyright (C) 2015 The XD Company Ltd.
// SPDX-License-Identifier: Apache-2.0

#ifndef RULES_NOT_SPACE_H
#define RULES_NOT_SPACE_H

#include "../format-named-rule.h"

class FormatNotSpaceRule : public FormatNamedRule
{
    typedef FormatNamedRule super;
public:
    inline FormatNotSpaceRule()
        : super("not-space", "Puts a space on each side of a logical not (!).")
    {
    }

    void run(FormatDocument *document) const Q_DECL_OVERRIDE;
};

#endif // RULES_NOT_SPACE_H
