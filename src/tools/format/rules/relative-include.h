// Copyright (C) 2015 The XD Company Ltd.
// SPDX-License-Identifier: Apache-2.0

#ifndef RULES_RELATIVE_INCLUDE_H
#define RULES_RELATIVE_INCLUDE_H

#include "../format-named-rule.h"

class FormatRelativeIncludeRule : public FormatNamedRule
{
    typedef FormatNamedRule super;
public:
    inline FormatRelativeIncludeRule()
        : super("relative-include",
                "Writes #include \"./name\" for a header next to the file.")
    {
    }

    void run(FormatDocument *document) const Q_DECL_OVERRIDE;
};

#endif // RULES_RELATIVE_INCLUDE_H
