// Copyright (C) 2015 The XD Company Ltd.
// SPDX-License-Identifier: Apache-2.0

#ifndef RULES_QLL_H
#define RULES_QLL_H

#include "../format-named-rule.h"

class FormatQllRule : public FormatNamedRule
{
    typedef FormatNamedRule super;
public:
    inline FormatQllRule()
        : super("qll",
                "Reports QLatin1String(\"...\") that new logic writes as QLL(\"...\").")
    {
    }

    void run(FormatDocument *document) const Q_DECL_OVERRIDE;
};

#endif // RULES_QLL_H
