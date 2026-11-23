// Copyright (C) 2015 The XD Company Ltd.
// SPDX-License-Identifier: Apache-2.0

#include "./ternary-wrap.h"

#include "../format-document.h"
#include "../parser/code-helpers.h"
#include "../parser/condition-layout.h"
#include "../parser/layout-helpers.h"
#include "../parser/scope-conditions.h"

#include <QtCore/QByteArray>
#include <QtCore/QList>

/*!
    @class FormatTernaryWrapRule
    @internal

    Breaks a one-line statement that passes `wrapMax` and holds one
    conditional expression outside of every
    bracket pair, such as `Type value = test ? yes : no;`: first before its
    `:`, then before its `?`, then before both when needed (see
    ternaryLines()). When the line before the `?`
    still does not fit, it breaks that line as well, through the
    FormatConditionLayout that every rule breaking a condition shares (see
    FormatConditionLayout::layoutHead()); an `if` condition's own `?` is
    laid out by FormatConditionWrapRule.
    A statement whose conditional branch already spans lines puts the `?`
    and `:` at the same continuation indentation. The rule leaves alone a
    control statement's head, a statement with a comment, and a preprocessor
    line.
*/

/*!
    Breaks long conditional expressions and aligns the operators of a
    conditional with a multiline branch in @a document.
*/
void FormatTernaryWrapRule::run(FormatDocument *document) const
{
    FormatScopeConditions conditions(document);
    const FormatConditionLayout layout(document, &conditions);
    for (int line = 1; line <= document->lineCount(); ++line) {
        const int first = document->firstSymbolOfLine(line);
        const int last = document->lastSymbolOfLine(line);
        // Align a conditional whose branch already spans multiple lines.
        if (first >= 0 && (first == 0
                || document->token(first - 1) == SEMIC
                || document->token(first - 1) == LBRACE
                || document->token(first - 1) == RBRACE
                || document->token(first - 1) == COLON)
            && ! document->isDirectiveLine(line)
            && document->token(first) != IF
            && document->token(first) != WHILE
            && document->token(first) != FOR
            && document->token(first) != SWITCH
            && document->token(first) != DO
            && document->token(first) != ELSE
            && document->token(first) != CASE
        ) {
            const int end = statementEnd(document, first);
            int question = -1;
            int colon = -1;
            if (end > last && findTernary(
                    document, first, end - 1, &question, &colon
                ) && document->symbolLine(question) < document->symbolLine(colon)
                && ! document->hasCommentBetween(
                    document->symbols().at(first).from, document->symbolEnd(end)
                ) && ! document->spansDirective(
                    document->symbols().at(first).from, document->symbolEnd(end)
                )
            ) {
                const QByteArray pad = document->indentOf(line) + "    ";
                const int endOfLine = document->lineEnd(line);
                const QByteArray newline = document->text().mid(
                    endOfLine - 1,
                    1
                ) == "\r"
                    ? "\r\n" : "\n";
                const QByteArray gap = newline + pad;
                const int beforeQuestion = document->symbolEnd(question - 1);
                const int questionFrom = document->symbols().at(question).from;
                const int beforeColon = document->symbolEnd(colon - 1);
                const int colonFrom = document->symbols().at(colon).from;
                if (document->text().mid(
                        beforeQuestion,
                        questionFrom - beforeQuestion
                    ) != gap
                    || document->text().mid(
                        beforeColon,
                        colonFrom - beforeColon
                    ) != gap
                ) {
                    document->replace(
                        beforeQuestion, questionFrom - beforeQuestion, gap,
                        beforeColon, colonFrom - beforeColon, gap
                    );
                }
                continue;
            }
        }
        if (first < 0
            || last <= first
            || document->token(last) != SEMIC
            || document->isDirectiveLine(line)
            || textWidth(document->lineText(line)) <= wrapMaxAt(
                document,
                document->lineStart(line)
            )
        ) {
            continue;
        }
        const Token before = first > 0
            ? document->token(first - 1) : SEMIC;
        if (before != SEMIC
            && before != LBRACE
            && before != RBRACE
            && before != COLON
        ) {
            continue;
        }
        const Token start = document->token(first);
        if (start == IF
            || start == WHILE
            || start == FOR
            || start == SWITCH
            || start == DO
            || start == ELSE
            || start == CASE
            || start == RBRACE
        ) {
            continue;
        }

        // Leave comments in place when rewriting the statement.
        bool isPlain = ! document->hasCommentBetween(
            document->symbols().at(first).from,
            document->symbolEnd(last)
        );
        // Skip nested expressions when looking for another statement.
        for (int k = first; k < last && isPlain; ++k) {
            const Token token = document->token(k);
            if ((token == LPAREN || token == LBRACK || token == LBRACE)
                && document->match(k) > k
            ) {
                k = document->match(k);
                continue;
            }
            isPlain = token != SEMIC && token != RBRACE;
        }
        int question = -1;
        int colon = -1;
        if ( ! isPlain
            || ! findTernary(document, first, last - 1, &question, &colon)
        ) {
            continue;
        }
        const int indent = document->indentOf(line).size();
        const QList<QByteArray > lines = ternaryLines(
            sourceText(document, first, question - 1),
            sourceText(document, question, colon - 1),
            sourceText(document, colon, last - 1),
            indent,
            indent,
            1,
            wrapMaxAt(document, document->lineStart(line))
        );
        if (lines.size() < 2) {
            continue;
        }

        // Breaks the line before the `?` too, when it does not fit.
        const QByteArray originalHead =
            sourceText(document, first, question - 1);
        const QList<QByteArray > head = lines.first() == originalHead
            ? layout.layoutHead(first, question - 1, indent)
            : QList<QByteArray >() << lines.first();
        QByteArray text;
        for (int l = 0; l < head.size(); ++l) {
            text += (l > 0 ? "\n" : "") + head.at(l);
        }
        for (int l = 1; l < lines.size(); ++l) {
            text += "\n" + lines.at(l);
        }
        const int from = document->symbols().at(first).from;
        document->replace(from, document->symbols().at(last).from - from, text);
    }
}
