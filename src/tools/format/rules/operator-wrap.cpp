// Copyright (C) 2015 The XD Company Ltd.
// SPDX-License-Identifier: Apache-2.0

#include "./operator-wrap.h"

#include "../format-document.h"
#include "../parser/code-helpers.h"
#include "../parser/expression-layout.h"
#include "../parser/scope-conditions.h"

#include <QtCore/QByteArray>
#include <QtCore/QString>

/*!
    @class FormatOperatorWrapRule
    @internal

    Breaks a statement that sits whole on one line, runs past the column
    limit (`[Structure] wrapMax`, 80 by default), and holds a chain of binary
    operators (`<<`, `+`, `-`, `|`, `^`, `&&`, `||`, and the like) outside of
    its brackets, or as the last argument of its call, as
    FormatExpressionLayout lays it out: before the operators of
    the loosest level that does not fit, one operand per line, each
    continuation line led by its operator one step past the line it breaks
    from; a call whose last argument is the chain keeps the chain's head on
    the call's line, and closes on a line of its own. A statement already
    wrapped, a compact one with braces, and a control statement are left to
    the other rules and to people.

    Only the width decides those breaks, while the rule for a long operator
    chain asks to break between groups of operands that belong together;
    hence, in agent mode (`--ai`), the rule asks the agent to regroup the
    lines it broke (see FormatDocument::request()). A line it cannot break
    stays as it is, and is not asked about.
*/

/*!
    Finds the statement that @a line of @a document holds whole, and gives
    its first token to @a first and its `;` to @a last. Returns false unless
    the statement starts the line, ends there with its only `;`, holds
    neither braces, a comment, nor a `?` outside of brackets, and is no
    control statement.
*/
bool FormatOperatorWrapRule::wholeStatement(
        const FormatDocument *document, int line,
        int *first, int *last) const
{
    *first = document->firstSymbolOfLine(line);
    *last = document->lastSymbolOfLine(line);
    if (*first < 0 || document->token(*last) != SEMIC
        || document->hasCommentBetween(
            document->lineStart(line),
            document->lineEnd(line)
        )
    ) {
        return false;
    }

    // Takes only a statement that starts the line and ends on it.
    if (*first > 0) {
        const Token before = document->token(*first - 1);
        if (before != SEMIC
            && before != LBRACE
            && before != RBRACE
            && before != COLON
        ) {
            return false;
        }
    }
    const Token lead = document->token(*first);
    if (lead == IF || lead == ELSE || lead == WHILE || lead == FOR || lead == DO
        || lead == SWITCH || lead == CASE
    ) {
        return false;
    }
    int depth = 0;
    for (int i = *first; i <= *last; ++i) {
        const Token token = document->token(i);
        if (token == LBRACE || token == RBRACE || (token == SEMIC && i < *last)
            || (token == QUESTION && depth == 0)
        ) {
            return false;
        }
        if (token == RPAREN || token == RBRACK) {
            --depth;
        }
        if (token == LPAREN || token == LBRACK) {
            ++depth;
        }
    }
    return true;
}

/*!
    Lays out each statement of @a document that runs past the column limit
    and holds an operator chain, and asks the agent to regroup the lines it
    made from one line.
*/
void FormatOperatorWrapRule::run(FormatDocument *document) const
{
    FormatScopeConditions conditions(document);
    const int limit = document->settings().wrapMax;
    for (int line = 1; line <= document->lineCount(); ++line) {
        const QByteArray text = document->lineText(line);
        int first = -1;
        int last = -1;
        if (textWidth(text) <= limit || document->isDirectiveLine(line)
            || ! this->wholeStatement(document, line, &first, &last)
        ) {
            continue;
        }
        const QByteArray indent = document->indentOf(line);
        FormatExpressionLayout layout(
            document,
            &conditions,
            textWidth(indent)
        );
        if ( ! layout.hasOuterChain(first, last - 1)) {
            continue;
        }
        const QByteArray laidOut = layout.layout(
            first,
            last - 1,
            textWidth(indent),
            1
        );
        const int from = document->symbols().at(first).from;
        const int length = document->symbolEnd(last - 1) - from;
        if (laidOut == document->text().mid(from, length)
            || ! document->replace(from, length, laidOut)
        ) {
            continue;
        }
        if (layout.brokeUngrouped()) {
            document->request(
                from,
                "operator-wrap",
                QString(QLL("the tool broke this expression at its operators " "by width alone; regroup its lines so that the " "operands that belong together (a key and its " "value, a format and its arguments) share one, " "each line within %1 columns and led by its " "operator")).arg(limit),
                1,
                limit
            );
        }
    }
}
