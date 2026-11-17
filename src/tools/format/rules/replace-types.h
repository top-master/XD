// Copyright (C) 2015 The XD Company Ltd.
// SPDX-License-Identifier: Apache-2.0

#ifndef RULES_REPLACE_TYPES_H
#define RULES_REPLACE_TYPES_H

#include "../format-named-rule.h"

class FormatReplaceTypesRule : public FormatNamedRule
{
    typedef FormatNamedRule super;
public:
    inline FormatReplaceTypesRule()
        : super("replace-types",
                "Writes the types that [Replace.types] maps as their new names.")
    {
    }

    void run(FormatDocument *document) const Q_DECL_OVERRIDE;
};

#endif // RULES_REPLACE_TYPES_H
