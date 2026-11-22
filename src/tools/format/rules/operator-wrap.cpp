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

    Breaks a statement that runs past the column
    limit (`[Structure] wrapMax`, 80 by default), and holds a chain of binary
    operators (`<<`, `+`, `-`, `|`, `^`, `&&`, `||`, and the like) outside of
    its brackets, or as the last argument of its call, as
    FormatExpressionLayout lays it out: before the operators of
    the loosest level that does not fit, one operand per line, each
    continuation line led by its operator one step past the line it breaks
    from; a call whose last argument is the chain keeps the chain's head on
    the call's line, and closes on a line of its own. A statement with
    existing argument breaks can move those breaks to its outer operators.

    Only the width decides those breaks, while the rule for a long operator
    chain asks to break between groups of operands that belong together;
    hence, in agent mode (`--ai`), the rule asks the agent to regroup the
    lines it broke (see FormatDocument::request()). A line it cannot break
    stays as it is, and is not asked about.
*/

/*!
    Finds a statement starting on @a line of @a document and gives
    its first token to @a first and its `;` to @a last. Returns false unless
    the statement ends with its only top-level `;` on the same line or
    the closing line of a nested expression,
    holds no comment, and is no control statement.
*/
bool FormatOperatorWrapRule::wholeStatement(
        const FormatDocument *document, int line,
        int *first, int *last) const
{
    *first = document->firstSymbolOfLine(line);
    *last = document->lastSymbolOfLine(line);
    if (*first < 0) {
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
    bool hasBinaryOperator = false;
    // Skip nested expressions while checking the statement boundary.
    for (int i = *first; i <= *last; ++i) {
        const Token token = document->token(i);
        if ((token == LPAREN || token == LBRACK || token == LBRACE)
            && document->match(i) > i
        ) {
            i = document->match(i);
            if (i > *last) {
                if (token == LBRACE
                    && document->braceScope(document->match(i)) != FormatDocument::InitScope
                ) {
                    return false;
                }
                *last = document->lastSymbolOfLine(document->symbolEndLine(i));
            }
            continue;
        }
        hasBinaryOperator |= operatorPrecedence(token) > 0 && i > *first
            && isOperandEnd(document->token(i - 1));
        if ((token == SEMIC && i < *last) || token == RBRACE) {
            return false;
        }
    }
    const int from = document->lineStart(line);
    const int to = document->lineEnd(document->symbolEndLine(*last));
    return (hasBinaryOperator || document->symbolLine(*last) == line)
        && document->token(*last) == SEMIC
        && ! document->hasCommentBetween(from, to)
        && ! document->spansDirective(from, to);
}

/*!
    Lays out each statement of @a document that runs past the column limit
    and holds an operator chain, and asks the agent to regroup the lines it
    made from one line.
*/
void FormatOperatorWrapRule::run(FormatDocument *document) const
{
    FormatScopeConditions conditions(document);
    for (int line = 1; line <= document->lineCount(); ++line) {
        const int limit = document
            ->configAt(document->lineStart(line))
            .wrapMax;
        int first = -1;
        int last = -1;
        if (document->isDirectiveLine(line)
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
        if (textWidth(indent + layout.oneLine(first, last)) <= limit
            || ! layout.hasOuterChain(first, last - 1)) {
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
                QString(QLL("the tool broke this expression at its operators "
                    "by width alone; regroup its lines so that the "
                    "operands that belong together (a key and its "
                    "value, a format and its arguments) share one, "
                    "each line within %1 columns and led by its " "operator"))
                .arg(limit),
                1,
                limit
            );
        }
    }
}
