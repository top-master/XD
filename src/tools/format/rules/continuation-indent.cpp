// Copyright (C) 2015 The XD Company Ltd.
// SPDX-License-Identifier: Apache-2.0

#include "./continuation-indent.h"

#include "../format-document.h"
#include "../parser/code-helpers.h"

#include <QtCore/QByteArray>

/*!
    \class FormatContinuationIndentRule
    \internal

    Indents each line that an operator leads one step (4 spaces) past the
    line that opened its group: the line of the `(` of a condition or of a
    parenthesized group, or else the statement's first line. A line inside a
    call's arguments is left as it is, since those lines align under
    the call's first argument.
*/

/*!
    Gives each operator-led continuation line of \a document the indent of
    its group's line plus 4 spaces.
*/
void FormatContinuationIndentRule::run(FormatDocument *document) const
{
    for (int line = 2; line <= document->lineCount(); ++line) {
        const int first = document->firstSymbolOfLine(line);
        if (first < 1 || ! isLeadingOperator(document->token(first))
            || document->isDirectiveLine(line) || document->symbolEndLine(first - 1) >= line
        ) {
            continue;
        }
        // Leaves out a statement's start, and a sign (`-1` after a comma).
        const Token before = document->token(first - 1);
        if (before == SEMIC || before == LBRACE || before == RBRACE || before == COLON
            || before == COMMA || before == EQ || before == LPAREN || before == RETURN
            || isLeadingOperator(before)
        ) {
            continue;
        }

        // Finds the innermost `(` still open, or the statement's start.
        int base = -1;
        bool keepsIndent = false;
        int k = first - 1;
        for (; k >= 0; --k) {
            const Token token = document->token(k);
            if ((token == RPAREN
                || token == RBRACK)
                && document->match(k) >= 0
            ) {
                k = document->match(k);
                continue;
            }
            if (token == LPAREN || token == LBRACK) {
                const Token opener = document->token(k - 1);
                keepsIndent = token == LBRACK || opener == IDENTIFIER || opener == RANGLE
                    || opener == GTGT || opener == RBRACK || opener == RPAREN
                    || opener == STATIC_CAST || opener == SIZEOF;
                base = document->symbolLine(k);
                break;
            }
            if (token == LBRACE && document->braceScope(k) != FormatDocument::FunctionScope
                && document->braceScope(k) != FormatDocument::BlockScope
            ) {
                keepsIndent = document->braceScope(k) == FormatDocument::InitScope
                    || document->braceScope(k) == FormatDocument::EnumScope;
            }
            if (token == SEMIC || token == LBRACE || token == RBRACE) {
                base = document->symbolLine(k + 1);
                break;
            }
        }
        if (k < 0) {
            base = document->symbolLine(0);
        }
        if (keepsIndent || base < 1 || base >= line) {
            continue;
        }
        const QByteArray indent = document->indentOf(base) + "    ";
        const QByteArray current = document->indentOf(line);
        if (current != indent) {
            document->replace(document->lineStart(line), current.size(), indent);
        }
    }
}
