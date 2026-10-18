// Copyright (C) 2015 The XD Company Ltd.
// SPDX-License-Identifier: Apache-2.0

#ifndef RULES_GLOBAL_STATIC_H
#define RULES_GLOBAL_STATIC_H

#include "../format-named-rule.h"

class FormatGlobalStaticRule : public FormatNamedRule
{
    typedef FormatNamedRule super;
public:
    inline FormatGlobalStaticRule()
        : super("global-static",
                "Reports static Qt objects that Q_GLOBAL_STATIC should hold.")
    {
    }

    void run(FormatDocument *document) const Q_DECL_OVERRIDE;
};

#endif // RULES_GLOBAL_STATIC_H
