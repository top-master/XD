// Copyright (C) 2015 The XD Company Ltd.
// SPDX-License-Identifier: Apache-2.0

#ifndef RULES_MOC_RUN_H
#define RULES_MOC_RUN_H

#include "../format-named-rule.h"

class FormatMocRunRule : public FormatNamedRule
{
    typedef FormatNamedRule super;
public:
    inline FormatMocRunRule()
        : super("moc-run-macro",
                "Spells moc's macro Q_MOC_RUN, never QT_MOC_RUN.")
    {
    }

    void run(FormatDocument *document) const Q_DECL_OVERRIDE;
};

#endif // RULES_MOC_RUN_H
