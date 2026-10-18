// Copyright (C) 2015 The XD Company Ltd.
// SPDX-License-Identifier: Apache-2.0

#ifndef RULES_QPOINTER_GETTER_H
#define RULES_QPOINTER_GETTER_H

#include "../format-named-rule.h"

class FormatQPointerGetterRule : public FormatNamedRule
{
    typedef FormatNamedRule super;
public:
    inline FormatQPointerGetterRule()
        : super("qpointer-getter",
                "Reports getters that return a QPointer member's raw pointer.")
    {
    }

    void run(FormatDocument *document) const Q_DECL_OVERRIDE;
};

#endif // RULES_QPOINTER_GETTER_H
