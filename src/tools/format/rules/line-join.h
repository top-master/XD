// Copyright (C) 2015 The XD Company Ltd.
// SPDX-License-Identifier: Apache-2.0

#ifndef RULES_LINE_JOIN_H
#define RULES_LINE_JOIN_H

#include "../format-named-rule.h"

#include <QtCore/QList>

class FormatLineJoinRule : public FormatNamedRule
{
    typedef FormatNamedRule super;
public:
    inline FormatLineJoinRule()
        : super("line-join", "Joins the lines of each expression when [Structure] target is any, for the layout rules to lay it out anew.")
    {
    }

    void run(FormatDocument *document) const Q_DECL_OVERRIDE;

private:
    bool mayJoin(const FormatDocument *document, int before,
                 const QList<int > & brackets, int questions) const;
};

#endif // RULES_LINE_JOIN_H
