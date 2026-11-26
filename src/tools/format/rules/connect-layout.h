// Copyright (C) 2015 The XD Company Ltd.
// SPDX-License-Identifier: Apache-2.0

#ifndef RULES_CONNECT_LAYOUT_H
#define RULES_CONNECT_LAYOUT_H

#include "../format-named-rule.h"

class FormatConnectLayoutRule : public FormatNamedRule
{
    typedef FormatNamedRule super;
public:
    inline FormatConnectLayoutRule()
        : super("connect-layout",
                "Lays connect() calls out by pairs, the type on its own line.")
    {
    }

    void run(FormatDocument *document) const Q_DECL_OVERRIDE;
};

#endif // RULES_CONNECT_LAYOUT_H
