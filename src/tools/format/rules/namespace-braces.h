// Copyright (C) 2015 The XD Company Ltd.
// SPDX-License-Identifier: Apache-2.0

#ifndef RULES_NAMESPACE_BRACES_H
#define RULES_NAMESPACE_BRACES_H

#include "../format-named-rule.h"

class FormatNamespaceBracesRule : public FormatNamedRule
{
    typedef FormatNamedRule super;
public:
    inline FormatNamespaceBracesRule()
        : super("namespace-braces", "Ends a namespace's opening line at its { and puts its } on a line of its own, labeled with the namespace's name.")
    {
    }

    void run(FormatDocument *document) const Q_DECL_OVERRIDE;
};

#endif // RULES_NAMESPACE_BRACES_H
