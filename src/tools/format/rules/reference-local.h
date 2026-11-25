// Copyright (C) 2015 The XD Company Ltd.
// SPDX-License-Identifier: Apache-2.0

#ifndef RULES_REFERENCE_LOCAL_H
#define RULES_REFERENCE_LOCAL_H

#include "../format-named-rule.h"

class FormatReferenceLocalRule : public FormatNamedRule
{
    typedef FormatNamedRule super;
public:
    inline FormatReferenceLocalRule()
        : super("reference-local", "Reports a local that a non-const reference names, which a pointer should.")
    {
    }

    void run(FormatDocument *document) const Q_DECL_OVERRIDE;
};

#endif // RULES_REFERENCE_LOCAL_H
