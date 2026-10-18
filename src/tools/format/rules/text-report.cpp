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
        if (this->m_pattern.match(QString::fromUtf8(document->lineText(line))).hasMatch()) {
            document->report(document->lineStart(line), this->name().constData(), this->m_message);
        }
    }
}
