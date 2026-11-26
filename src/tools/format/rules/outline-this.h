// Copyright (C) 2015 The XD Company Ltd.
// SPDX-License-Identifier: Apache-2.0

#ifndef RULES_OUTLINE_THIS_H
#define RULES_OUTLINE_THIS_H

#include "../format-named-rule.h"

#include <QtCore/QByteArray>
#include <QtCore/QSet>

class FormatOutlineThisRule : public FormatNamedRule
{
    typedef FormatNamedRule super;
public:
    inline FormatOutlineThisRule()
        : super("outline-this",
                "Reaches members with this-> where [Relations] thisMode asks.")
    {
    }

    void run(FormatDocument *document) const Q_DECL_OVERRIDE;

private:
    QSet<QByteArray > localNames(
            const FormatDocument *document, int from,
            int to) const;
};

#endif // RULES_OUTLINE_THIS_H
