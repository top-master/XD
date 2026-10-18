// Copyright (C) 2015 The XD Company Ltd.
// SPDX-License-Identifier: Apache-2.0

#include "./init-list.h"

#include "../format-document.h"

#include <QtCore/QByteArray>

/*!
    @class FormatInitListRule
    @internal

    Leads each line of a constructor's member-initializer list with its colon
    or comma, never ending a line with one.
*/

/*!
    Finds each member-initializer list (a `:` right after a function's `)`,
    up to its body's `{`), and moves each colon or comma that ends a line in
    it to the start of the next line.
*/
void FormatInitListRule::run(FormatDocument *document) const
{
    for (int i = 1; i < document->symbols().size(); ++i) {
        if (document->token(i) != COLON
            || document->token(i - 1) != RPAREN
        ) {
            continue;
        }

        // Finds the list's end, the body's brace.
        int brace = -1;
        for (int k = i + 1; k < document->symbols().size(); ++k) {
            const Token token = document->token(k);
            if (token == LPAREN
                || (token == LBRACE
                    && document->braceScope(k) == FormatDocument::InitScope)
            ) {
                k = qMax(k, document->match(k));
                continue;
            }
            if (token == LBRACE) {
                brace = k;
            }
            if (token == LBRACE || token == SEMIC || token == RBRACE) {
                break;
            }
        }
        if (brace < 0
            || document->braceScope(brace) != FormatDocument::FunctionScope
        ) {
            continue;
        }

        // Moves the separators that end a line.
        for (int k = i; k < brace; ++k) {
            const Token token = document->token(k);
            if (token == LPAREN) {
                k = qMax(k, document->match(k));
                continue;
            }
            const int line = document->symbolLine(k);
            if ((token != COLON && token != COMMA) || document->lastSymbolOfLine(line) != k
                || document->symbolEndLine(k - 1) != line
                || document->hasCommentBetween(
                    document->symbolEnd(k),
                    document->symbols().at(k + 1).from
                )
            ) {
                continue;
            }
            const int from = document->symbolEnd(k - 1);
            if (document->spansDirective(
                    from,
                    document->symbols().at(k + 1).from
                )
            ) {
                continue;
            }
            document->replace(from, document->symbolEnd(k) - from, QByteArray());
            document->replace(
                document->symbols().at(k + 1).from,
                0,
                document->lexem(k) + ' '
            );
        }

        // Puts each comma that leads a line under the colon, when the colon
        // leads its own line.
        const int colonLine = document->symbolLine(i);
        if (document->firstSymbolOfLine(colonLine) == i) {
            const QByteArray indent = document->indentOf(colonLine);
            for (int k = i + 1; k < brace; ++k) {
                const int line = document->symbolLine(k);
                if (document->token(k) != COMMA || document->firstSymbolOfLine(line) != k
                    || document->indentOf(line) == indent
                ) {
                    continue;
                }
                document->replace(
                    document->lineStart(line),
                    document->indentOf(line).size(),
                    indent
                );
            }
        }
        i = brace;
    }
}
