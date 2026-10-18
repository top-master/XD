// Copyright (C) 2015 The XD Company Ltd.
// SPDX-License-Identifier: Apache-2.0

#include "./pattern-report.h"

#include "../format-document.h"
#include "../lang/comment-line.h"
#include "../parser/comment-helpers.h"

#include <QtCore/QList>
#include <QtCore/QString>
#include <QtCore/QVector>

/*!
    \class FormatPatternReportRule
    \internal

    Reports each match of one regular expression in the prose of a comment
    (outside of backticks and fenced code), with one message.
*/

/*!
    Reports the first match on each prose line of \a group.
*/
void FormatPatternReportRule::runGroup(FormatDocument *document,
                                       const QList<int > & group) const
{
    if (this->m_docOnly
        && ! document->comments().at(group.first()).isDoc()
    ) {
        return;
    }
    const QList<FormatCommentLine > lines = groupLines(document, group);
    const QVector<bool > fenced = fencedLines(lines);
    for (int i = 0; i < lines.size(); ++i) {
        if (fenced.at(i)) {
            continue;
        }
        const QString prose = proseOnly(QString::fromUtf8(lines.at(i).text));
        const QRegularExpressionMatch found = this->m_pattern.match(prose);
        if (found.hasMatch()) {
            document->report(lines.at(i).offset, this->name().constData(), this->m_message);
        }
    }
}
