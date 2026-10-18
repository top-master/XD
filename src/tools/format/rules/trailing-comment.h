// Copyright (C) 2015 The XD Company Ltd.
// SPDX-License-Identifier: Apache-2.0

#ifndef RULES_TRAILING_COMMENT_H
#define RULES_TRAILING_COMMENT_H

#include "./comment.h"

#include <QtCore/QList>

class FormatTrailingCommentRule : public FormatCommentRule
{
    typedef FormatCommentRule super;
public:
    inline FormatTrailingCommentRule()
        : super("trailing-comment",
                "Moves a comment at the end of a code line above that line.")
    {
    }

protected:
    void runGroup(FormatDocument *document,
                  const QList<int > & group) const Q_DECL_OVERRIDE;
};

#endif // RULES_TRAILING_COMMENT_H
