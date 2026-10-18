// Copyright (C) 2015 The XD Company Ltd.
// SPDX-License-Identifier: Apache-2.0

#ifndef RULES_COMMENT_CASE_H
#define RULES_COMMENT_CASE_H

#include "./comment.h"

#include <QtCore/QList>

class FormatCommentCaseRule : public FormatCommentRule
{
    typedef FormatCommentRule super;
public:
    inline FormatCommentCaseRule()
        : super("comment-case",
                "Starts each comment with an upper-case letter.")
    {
    }

protected:
    void runGroup(FormatDocument *document,
                  const QList<int > & group) const Q_DECL_OVERRIDE;
};

#endif // RULES_COMMENT_CASE_H
