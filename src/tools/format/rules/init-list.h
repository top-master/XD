// Copyright (C) 2015 The XD Company Ltd.
// SPDX-License-Identifier: Apache-2.0

#ifndef RULES_INIT_LIST_H
#define RULES_INIT_LIST_H

#include "../format-named-rule.h"

class FormatInitListRule : public FormatNamedRule
{
    typedef FormatNamedRule super;
public:
    inline FormatInitListRule()
        : super("init-list-lead",
                "Leads member-initializer lines with their colon or comma.")
    {
    }

    void run(FormatDocument *document) const Q_DECL_OVERRIDE;
};

#endif // RULES_INIT_LIST_H
