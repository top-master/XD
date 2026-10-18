// Copyright (C) 2015 The XD Company Ltd.
// SPDX-License-Identifier: Apache-2.0

#include "./comment-case.h"

#include "../format-document.h"
#include "../lang/comment-line.h"
#include "../parser/comment-helpers.h"

#include <QtCore/QByteArray>
#include <QtCore/QList>
#include <QtCore/QString>

/*!
    @class FormatCommentCaseRule
    @internal

    Starts each comment with an upper-case letter; a comment that starts with
    a code name is reported instead, since the name keeps its spelling.
*/

/*!
    Upper-cases the first letter of @a group, when its first word is a
    plain English word and the group is more than that one word.
*/
void FormatCommentCaseRule::runGroup(
        FormatDocument *document,
        const QList<int > & group) const
{
    const QList<FormatCommentLine > lines = groupLines(document, group);
    if (looksLikeCode(lines) || isSingleWord(lines)) {
        return;
    }
    int first = 0;
    while (first < lines.size() && lines.at(first).text.isEmpty()) {
        ++first;
    }
    if (first >= lines.size()) {
        return;
    }
    const QByteArray & text = lines.at(first).text;
    const char lead = text.at(0);
    if (lead < 'a' || lead > 'z') {
        return;
    }
    int end = 0;
    while (end < text.size()
        && (QChar::isLetterOrNumber(uchar(text.at(end)))
            || text.at(end) == '_')
    ) {
        ++end;
    }
    const QByteArray word = text.left(end);
    const char next = end < text.size() ? text.at(end) : ' ';
    bool isCode = next == '(' || next == ':' || next == '.' || next == '-' || next == '>'
        || next == '=' || next == '/' || next == '*' || next == '[' || next == '<'
        || next == '#' || word.contains('_');
    for (int i = 1; i < word.size() && ! isCode; ++i) {
        isCode = QChar::isUpper(uchar(word.at(i)))
            || QChar::isDigit(uchar(word.at(i)));
    }
    if (isCode) {
        document->report(lines.at(first).offset, "comment-case",
                         QString(QLL("start the comment with an upper-case word")));
        return;
    }
    document->replace(
        lines.at(first).offset,
        1,
        QByteArray(1, char(lead - 'a' + 'A'))
    );
}
