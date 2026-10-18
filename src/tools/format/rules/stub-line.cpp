// Copyright (C) 2015 The XD Company Ltd.
// SPDX-License-Identifier: Apache-2.0

#include "./stub-line.h"

#include "../format-document.h"
#include "../lang/comment-line.h"
#include "../parser/comment-helpers.h"

#include <QtCore/QByteArray>
#include <QtCore/QList>
#include <QtCore/QString>
#include <QtCore/QVector>

/*!
    @class FormatStubLineRule
    @internal

    Reports a wrapped comment line that carries 17 characters or fewer, the
    runt of a wrap that the lines around it should take up.
*/

/*!
    Reports each line of @a group that continues the line above it (in the
    same paragraph) with 17 characters or fewer; a list item, a doc command,
    and code are left out.
*/
void FormatStubLineRule::runGroup(
        FormatDocument *document,
        const QList<int > & group) const
{
    const QList<FormatCommentLine > lines = groupLines(document, group);
    if (lines.size() < 2 || looksLikeCode(lines)) {
        return;
    }
    const QVector<bool > fenced = fencedLines(lines);
    for (int i = 1; i < lines.size(); ++i) {
        const QByteArray & text = lines.at(i).text;
        const QByteArray & above = lines.at(i - 1).text;
        if (fenced.at(i) || text.isEmpty() || above.isEmpty() || above.startsWith('\\')
            || text.startsWith('*') || text.startsWith('-') || text.startsWith('\\')
            || text.startsWith('@') || above.endsWith(':')
            || (text.size() > 1 && QChar::isDigit(uchar(text.at(0))) && text.at(1) == '.')
        ) {
            continue;
        }
        const int length = QString::fromUtf8(text).size();
        if (length <= 17) {
            document->report(lines.at(i).offset, "stub-line",
                             QString(QLL("rewrap the comment: this line has only %1 characters"))
                             .arg(length));
        }
    }
}
