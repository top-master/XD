// Copyright (C) 2015 The XD Company Ltd.
// SPDX-License-Identifier: Apache-2.0

#ifndef RULES_STUB_LINE_H
#define RULES_STUB_LINE_H

#include "./comment.h"

#include <QtCore/QList>

class FormatStubLineRule : public FormatCommentRule
{
    typedef FormatCommentRule super;
public:
    inline FormatStubLineRule()
        : super("stub-line",
                "Reports wrapped comment lines of 17 characters or fewer.")
    {
    }

protected:
    void runGroup(FormatDocument *document,
                  const QList<int > & group) const Q_DECL_OVERRIDE;
};

#endif // RULES_STUB_LINE_H
