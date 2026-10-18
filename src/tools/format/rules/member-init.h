// Copyright (C) 2015 The XD Company Ltd.
// SPDX-License-Identifier: Apache-2.0

#ifndef RULES_MEMBER_INIT_H
#define RULES_MEMBER_INIT_H

#include "../format-named-rule.h"

class FormatMemberInitRule : public FormatNamedRule
{
    typedef FormatNamedRule super;
public:
    inline FormatMemberInitRule()
        : super("member-init",
                "Reports data members initialized in the class body.")
    {
    }

    void run(FormatDocument *document) const Q_DECL_OVERRIDE;
};

#endif // RULES_MEMBER_INIT_H
