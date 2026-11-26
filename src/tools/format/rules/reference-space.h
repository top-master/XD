// Copyright (C) 2015 The XD Company Ltd.
// SPDX-License-Identifier: Apache-2.0

#ifndef RULES_REFERENCE_SPACE_H
#define RULES_REFERENCE_SPACE_H

#include "../format-named-rule.h"

class FormatReferenceSpaceRule : public FormatNamedRule
{
    typedef FormatNamedRule super;
public:
    inline FormatReferenceSpaceRule()
        : super("reference-space",
                "Writes a named reference declaration as Type & name.")
    {
    }

    void run(FormatDocument *document) const Q_DECL_OVERRIDE;

private:
    bool isDeclaration(const FormatDocument *document, int ampersand) const;
};

#endif // RULES_REFERENCE_SPACE_H
