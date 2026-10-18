// Copyright (C) 2015 The XD Company Ltd.
// SPDX-License-Identifier: Apache-2.0

#ifndef RULES_COMMENT_H
#define RULES_COMMENT_H

#include "../format-named-rule.h"

#include <QtCore/QList>

class FormatCommentRule : public FormatNamedRule
{
    typedef FormatNamedRule super;
public:
    inline FormatCommentRule(const char *name, const char *summary)
        : super(name, summary)
    {
    }

    void run(FormatDocument *document) const Q_DECL_OVERRIDE;

protected:
    virtual void runGroup(FormatDocument *document,
                          const QList<int > & group) const = 0;
};

#endif // RULES_COMMENT_H
