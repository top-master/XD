// Copyright (C) 2015 The XD Company Ltd.
// SPDX-License-Identifier: Apache-2.0

#ifndef RULES_OUTLINE_THIS_H
#define RULES_OUTLINE_THIS_H

#include "../format-named-rule.h"
#include "../lang/class-members.h"

#include <QtCore/QByteArray>
#include <QtCore/QHash>
#include <QtCore/QSet>

class FormatOutlineThisRule : public FormatNamedRule
{
    typedef FormatNamedRule super;
public:
    inline FormatOutlineThisRule()
        : super("outline-this",
                "Reaches own members with this-> in outline definitions.")
    {
    }

    void run(FormatDocument *document) const Q_DECL_OVERRIDE;

private:
    FormatClassMembers membersOf(const FormatDocument *document, const QByteArray & className,
                                 QHash<QByteArray, FormatClassMembers > *cache) const;
    QSet<QByteArray > localNames(const FormatDocument *document, int from,
                                 int to) const;
};

#endif // RULES_OUTLINE_THIS_H
