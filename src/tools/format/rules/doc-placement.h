// Copyright (C) 2015 The XD Company Ltd.
// SPDX-License-Identifier: Apache-2.0

#ifndef RULES_DOC_PLACEMENT_H
#define RULES_DOC_PLACEMENT_H

#include "./comment.h"

#include <QtCore/QList>
#include <QtCore/QString>

class FormatDocPlacementRule : public FormatCommentRule
{
    typedef FormatCommentRule super;
public:
    inline FormatDocPlacementRule()
        : super("doc-placement",
                "Reports misplaced docs: // on symbols, header docs, orphans.")
    {
    }

protected:
    void runGroup(FormatDocument *document,
                  const QList<int > & group) const Q_DECL_OVERRIDE;

private:
    QString headerSource(const QString & path) const;
};

#endif // RULES_DOC_PLACEMENT_H
