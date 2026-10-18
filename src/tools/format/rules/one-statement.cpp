// Copyright (C) 2015 The XD Company Ltd.
// SPDX-License-Identifier: Apache-2.0

#include "./one-statement.h"

#include "../format-document.h"
#include "../parser/code-helpers.h"

#include <QtCore/QByteArray>

/*!
    @class FormatOneStatementRule
    @internal

    Puts one statement per line: a statement that follows another on its
    line moves to a line of its own, at the same indent.
*/

/*!
    Splits each line that holds two statements, when the line has no braces
    of its own (such as the one-line body `{ return; }`), no `case` label, and
    no preprocessor directive; the semicolons of a `for` head do not count.
*/
void FormatOneStatementRule::run(FormatDocument *document) const
{
    for (int line = 1; line <= document->lineCount(); ++line) {
        const int first = document->firstSymbolOfLine(line);
        const int last = document->lastSymbolOfLine(line);
        if (first < 0 || last < first || document->isDirectiveLine(line)) {
            continue;
        }
        const Token lead = document->token(first);
        if (lead == CASE || lead == DEFAULT) {
            continue;
        }
        const FormatDocument::Scope scope = document->scopeOf(first);
        if (scope == FormatDocument::EnumScope
            || scope == FormatDocument::InitScope
        ) {
            continue;
        }
        bool hasBrace = false;
        for (int i = first; i <= last; ++i) {
            hasBrace = hasBrace || document->token(i) == LBRACE
                || document->token(i) == RBRACE;
        }
        if (hasBrace) {
            continue;
        }
        const QByteArray indent = document->indentOf(line);
        for (int i = first; i < last; ++i) {
            const Token token = document->token(i);
            if (token == LPAREN) {
                i = qMax(i, document->match(i));
                continue;
            }
            if (token != SEMIC || ! isSameLineGap(document, i, i + 1)) {
                continue;
            }
            const int from = document->symbolEnd(i);
            document->replace(
                from,
                document->symbols().at(i + 1).from - from,
                "\n" + indent
            );
        }
    }
}
