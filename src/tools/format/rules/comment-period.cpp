// Copyright (C) 2015 The XD Company Ltd.
// SPDX-License-Identifier: Apache-2.0

#include "./comment-period.h"

#include "../format-document.h"
#include "../lang/comment-line.h"
#include "../parser/comment-helpers.h"

#include <QtCore/QByteArray>
#include <QtCore/QList>
#include <QtCore/QString>

/*!
    \class FormatCommentPeriodRule
    \internal

    Ends each comment with a period; a comment that ends with a comma or a
    semicolon, or with an address, is reported instead.
*/

/*!
    Adds the period after the last word of \a group, unless it already ends
    with `.`, `!`, `?` or `:`, ends in code, is a doc-comment's command, is
    one word (a label), or holds no word at all (such as a rule of dashes).
*/
void FormatCommentPeriodRule::runGroup(FormatDocument *document,
                                       const QList<int > & group) const
{
    const QList<FormatCommentLine > lines = groupLines(document, group);
    if (looksLikeCode(lines) || isSingleWord(lines)) {
        return;
    }
    int last = lines.size() - 1;
    while (last >= 0 && lines.at(last).text.isEmpty()) {
        --last;
    }
    if (last < 0 || fencedLines(lines).at(last)) {
        return;
    }
    const QByteArray & text = lines.at(last).text;
    bool hasWord = false;
    for (int i = 0; i < text.size() && ! hasWord; ++i) {
        hasWord = QChar::isLetterOrNumber(uchar(text.at(i)));
    }
    if ( ! hasWord) {
        return;
    }
    const char end = text.at(text.size() - 1);
    if (end == '.' || end == '!' || end == '?' || end == ':' || text.startsWith('\\')
        || text.startsWith('@')
    ) {
        return;
    }
    const int space = text.lastIndexOf(' ');
    if (end == ',' || end == ';' || text.mid(space + 1).contains("://")) {
        document->report(lines.at(last).offset, "comment-period",
                         QString(QLL("end the comment with a period")));
        return;
    }
    document->replace(lines.at(last).offset + text.size(), 0, ".");
}
