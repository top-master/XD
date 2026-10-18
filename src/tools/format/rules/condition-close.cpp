// Copyright (C) 2015 The XD Company Ltd.
// SPDX-License-Identifier: Apache-2.0

#include "./condition-close.h"

#include "../format-document.h"
#include "../parser/code-helpers.h"

#include <QtCore/QByteArray>

/*!
    @class FormatConditionCloseRule
    @internal

    Closes a condition that spans lines with its `)` on a line of its own, at
    the column of its keyword, followed by the body's `{` on that line.
*/

/*!
    Moves the `)` of each multi-line `if`, `while`, `for`, or `switch`
    condition to a line of its own, and its `{` next to it.
*/
void FormatConditionCloseRule::run(FormatDocument *document) const
{
    for (int i = 0; i + 1 < document->symbols().size(); ++i) {
        const Token token = document->token(i);
        if ((token != IF && token != WHILE && token != FOR && token != SWITCH)
            || document->token(i + 1) != LPAREN
            || document->isDirectiveLine(document->symbolLine(i))
        ) {
            continue;
        }
        const int close = document->match(i + 1);
        if (close < 0 || document->symbolLine(close) == document->symbolLine(i)
            || document->token(close + 1) != LBRACE
            || document->spansDirective(
                document->symbols().at(i).from,
                document->symbols().at(close + 1).from
            )
        ) {
            continue;
        }
        const QByteArray indent = document->indentOf(document->symbolLine(i));

        // Puts the `)` on a line of its own.
        const int last = close - 1;
        const int closeLine = document->symbolLine(close);
        if (document->symbolEndLine(last) == closeLine) {
            if ( ! isSameLineGap(document, last, close)) {
                continue;
            }
            const int from = document->symbolEnd(last);
            document->replace(
                from,
                document->symbols().at(close).from - from,
                "\n" + indent
            );
        } else if (document->indentOf(closeLine) != indent) {
            const int start = document->lineStart(closeLine);
            document->replace(start, document->indentOf(closeLine).size(), indent);
        }

        // Puts the `{` right after it.
        const int brace = close + 1;
        const int from = document->symbolEnd(close);
        const int to = document->symbols().at(brace).from;
        if (to - from != 1 || document->text().at(from) != ' ') {
            if (document->hasCommentBetween(from, to)
                || document->spansDirective(from, to)
            ) {
                continue;
            }
            document->replace(from, to - from, " ");
        }
    }
}
