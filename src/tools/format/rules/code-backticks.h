// Copyright (C) 2015 The XD Company Ltd.
// SPDX-License-Identifier: Apache-2.0

#ifndef RULES_CODE_BACKTICKS_H
#define RULES_CODE_BACKTICKS_H

#include "./comment.h"

#include <QtCore/QList>

class FormatCodeBackticksRule : public FormatCommentRule
{
    typedef FormatCommentRule super;
public:
    inline FormatCodeBackticksRule()
        : super("code-backticks",
                "Wraps each code name of a plain comment, and each access chain of "
                "a doc-comment, in backticks.")
    {
    }

protected:
    void runGroup(FormatDocument *document,
                  const QList<int > & group) const Q_DECL_OVERRIDE;
};

#endif // RULES_CODE_BACKTICKS_H
