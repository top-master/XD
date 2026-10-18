// Copyright (C) 2015 The XD Company Ltd.
// SPDX-License-Identifier: Apache-2.0

#ifndef RULES_MARK_BANNER_H
#define RULES_MARK_BANNER_H

#include "./comment.h"

#include <QtCore/QList>

class FormatMarkBannerRule : public FormatCommentRule
{
    typedef FormatCommentRule super;
public:
    inline FormatMarkBannerRule()
        : super("mark-banner", "Writes a section divider as // MARK: <label>.")
    {
    }

protected:
    void runGroup(FormatDocument *document,
                  const QList<int > & group) const Q_DECL_OVERRIDE;
};

#endif // RULES_MARK_BANNER_H
