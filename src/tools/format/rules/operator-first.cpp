// Copyright (C) 2015 The XD Company Ltd.
// SPDX-License-Identifier: Apache-2.0

#include "./operator-first.h"

#include "../format-document.h"

#include <QtCore/QByteArray>

/*!
    \class FormatOperatorFirstRule
    \internal

    Breaks a long expression before its operator, never after it: each `&&`,
    `||`, `<<`, `+`, `-`, `|`, `^` or `?` that ends a line moves to the
    start of the line after it.
*/

/*!
    Moves each binary operator that ends a line to the start of the next
    line, outside of preprocessor directives and when no comment follows it.
*/
void FormatOperatorFirstRule::run(FormatDocument *document) const
{
    for (int i = 1; i + 1 < document->symbols().size(); ++i) {
        const Token token = document->token(i);
        if (token != ANDAND && token != OROR && token != LTLT && token != PLUS && token != MINUS
            && token != OR && token != HAT && token != QUESTION
        ) {
            continue;
        }
        const int line = document->symbolLine(i);
        if (document->lastSymbolOfLine(line) != i || document->isDirectiveLine(line)
            || document->symbolEndLine(i - 1) != line || document->token(i - 1) == OPERATOR
            || document->hasCommentBetween(document->symbolEnd(i), document->symbols().at(i + 1).from)
        ) {
            continue;
        }
        const int from = document->symbolEnd(i - 1);
        if (document->spansDirective(from, document->symbols().at(i + 1).from)) {
            continue;
        }
        document->replace(from, document->symbolEnd(i) - from, QByteArray());
        document->replace(document->symbols().at(i + 1).from, 0, document->lexem(i) + ' ');
    }
}
