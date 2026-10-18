// Copyright (C) 2015 The XD Company Ltd.
// SPDX-License-Identifier: Apache-2.0

#include "./prose.h"

#include "../format-document.h"
#include "../lang/comment-line.h"
#include "../parser/comment-helpers.h"

#include <QtCore/QList>
#include <QtCore/QString>
#include <QtCore/QVector>

/*!
    @class FormatProseRule
    @internal

    Rewrites the prose of each comment line with one mapping, outside of
    backticks and fenced code.
*/

/*!
    Replaces each line of @a group whose prose the mapping changes.
*/
void FormatProseRule::runGroup(FormatDocument *document,
                               const QList<int > & group) const
{
    const QList<FormatCommentLine > lines = groupLines(document, group);
    const QVector<bool > fenced = fencedLines(lines);
    for (int i = 0; i < lines.size(); ++i) {
        if (fenced.at(i)) {
            continue;
        }
        const QString text = QString::fromUtf8(lines.at(i).text);
        const QString mapped = mapProse(text, this->m_map, Q_NULLPTR);
        if (mapped != text) {
            document->replace(lines.at(i).offset, lines.at(i).text.size(), mapped.toUtf8());
        }
    }
}
