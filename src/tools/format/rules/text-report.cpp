// Copyright (C) 2015 The XD Company Ltd.
// SPDX-License-Identifier: Apache-2.0

#include "./text-report.h"

#include "../format-document.h"

#include <QtCore/QString>

/*!
    @class FormatTextReportRule
    @internal

    Reports each line of the whole text (code, strings and comments) that
    matches one regular expression, with one message.
*/

/*!
    Reports the lines of @a document that match the pattern.
*/
void FormatTextReportRule::run(FormatDocument *document) const
{
    for (int line = 1; line <= document->lineCount(); ++line) {
        const QString text = QString::fromUtf8(document->lineText(line));
        if ( ! document->hasDisabledFormatting()) {
            if (this->m_pattern.match(text).hasMatch()) {
                document->report(
                    document->lineStart(line),
                    this->name().constData(),
                    this->m_message
                );
            }
            continue;
        }
        QRegularExpressionMatchIterator matches =
            this->m_pattern.globalMatch(text);
        while (matches.hasNext()) {
            const QRegularExpressionMatch match = matches.next();
            const int offset = document->lineStart(line)
                + text.left(match.capturedStart()).toUtf8().size();
            if ( ! document->isFormattingDisabled(offset)) {
                document->report(
                    offset,
                    this->name().constData(),
                    this->m_message
                );
                break;
            }
        }
    }
}
