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

    Breaks a one-line statement that passes the wrap column (see
    wrapColumn()) and holds one conditional expression outside of every
    bracket pair, such as `Type value = test ? yes : no;`: first before its
    `:`, then before its `?`, then before both when needed (see
    ternaryLines()). When the line before the `?`
    still does not fit, it breaks that line as well, through the
    FormatConditionLayout that every rule breaking a condition shares (see
    FormatConditionLayout::layoutHead()); an `if` condition's own `?` is
    laid out by FormatConditionWrapRule.
    The rule leaves alone a control statement's head, a statement with a
    brace or a comment, and a preprocessor line.
*/

/*!
    Breaks the conditional expression of each long one-line statement of
    @a document.
*/
void FormatTernaryWrapRule::run(FormatDocument *document) const
{
    const int wrap = wrapColumn(document);
    FormatScopeConditions conditions(document);
    const FormatConditionLayout layout(document, &conditions);
    for (int line = 1; line <= document->lineCount(); ++line) {
        const int first = document->firstSymbolOfLine(line);
        const int last = document->lastSymbolOfLine(line);
        if (first < 0
            || last <= first
            || document->token(last) != SEMIC
            || document->isDirectiveLine(line)
            || textWidth(document->lineText(line)) <= wrap
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

        // Leaves a line of more than one statement, or with a brace or a
        // comment in it; the line starts its statement (checked above).
        bool isPlain = ! document->hasCommentBetween(
            document->symbols().at(first).from,
            document->symbolEnd(last)
        );
        for (int k = first; k < last && isPlain; ++k) {
            const Token token = document->token(k);
            isPlain = token != SEMIC && token != LBRACE && token != RBRACE;
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
            qMax(1, document->settings().wrapMax)
        );
        if (lines.size() < 2) {
            continue;
        }

        // Breaks the line before the `?` too, when it does not fit.
        const QByteArray originalHead = sourceText(document, first, question - 1);
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
