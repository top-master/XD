// Copyright (C) 2015 The XD Company Ltd.
// SPDX-License-Identifier: Apache-2.0

#include "./comment.h"

#include "../format-document.h"
#include "../parser/comment-helpers.h"

#include <QtCore/QByteArray>
#include <QtCore/QList>
#include <QtCore/QPair>

/*!
    \class FormatCommentRule
    \internal

    Runs a check over each comment group that a rule may touch: not a
    protected comment (a `TRACE/` record, a license), not the trailing label
    of a closing line or a label inside a line of code, and not a comment of
    a data table that a `format: table` comment marks.
*/

/*!
    Calls runGroup() for each comment group of \a document that a rule is
    allowed to change.
*/
void FormatCommentRule::run(FormatDocument *document) const
{
    const QList<QList<int > > groups = commentGroups(document);
    const QList<QPair<int, int > > tables = tableRanges(document);
    for (int i = 0; i < groups.size(); ++i) {
        const QList<int > & group = groups.at(i);
        bool touchable = true;
        const int offset = document->comments().at(group.first()).offset;
        for (int t = 0; t < tables.size() && touchable; ++t) {
            touchable = offset < tables.at(t).first
                || offset >= tables.at(t).second;
        }
        for (int k = 0; k < group.size() && touchable; ++k) {
            touchable = ! document->isProtectedComment(group.at(k))
                && ! isClosingLabel(document, group.at(k))
                && ! isInlineLabel(document, group.at(k));
        }
        if (touchable) {
            this->runGroup(document, group);
        }
    }
}
