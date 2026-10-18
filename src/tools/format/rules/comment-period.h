// Copyright (C) 2015 The XD Company Ltd.
// SPDX-License-Identifier: Apache-2.0

#ifndef RULES_COMMENT_PERIOD_H
#define RULES_COMMENT_PERIOD_H

#include "./comment.h"

#include <QtCore/QList>

class FormatCommentPeriodRule : public FormatCommentRule
{
    typedef FormatCommentRule super;
public:
    inline FormatCommentPeriodRule()
        : super("comment-period", "Ends each comment with a period.")
    {
    }

protected:
    void runGroup(FormatDocument *document,
                  const QList<int > & group) const Q_DECL_OVERRIDE;
};

#endif // RULES_COMMENT_PERIOD_H
