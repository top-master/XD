// Copyright (C) 2015 The XD Company Ltd.
// SPDX-License-Identifier: Apache-2.0

#include "./operator-wrap.h"

#include "../format-document.h"
#include "../parser/code-helpers.h"
#include "../parser/expression-layout.h"
#include "../parser/layout-helpers.h"
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
    existing argument breaks can move those breaks to its outer operators
    when a line exceeds the limit or the target is `any`.

    Only the width decides those breaks, while the rule for a long operator
    chain asks to break between groups of operands that belong together;
    hence, in agent mode (`--ai`), the rule asks the agent to regroup the
    lines it broke (see FormatDocument::request()). A line it cannot break
    stays as it is, and is not asked about.
*/

/*!
    Finds a statement starting on @a line of @a document and gives
    its first token to @a first and the `;` that ends it to @a last, which
    follows the statement past every nested expression it spans. Returns
    false unless the statement ends with its only top-level `;`, which the
    rest of its line does not pass, holds no comment, and is no control
    statement.
*/
bool FormatOperatorWrapRule::wholeStatement(
        const FormatDocument *document,
        int line,
        int *first,
        int *last) const
{ *first = document->firstSymbolOfLine(line); *last = document->lastSymbolOfLine(line);
    if (*first < 0) {
        return false;
    }

    const Token lead = document->token(*first);
    if ((lead == COLON
            || lead == COMMA)
        && *first + 2 <= *last
        && document->token(*first + 1) == IDENTIFIER
    ) {
        const Token openToken = document->token(*first + 2);
        if (openToken == LPAREN
            || (openToken == LBRACE
                && document->braceScope(*first + 2) == FormatDocument::InitScope)
        ) {
            const int close = document->match(*first + 2);
            if (close > *first + 2) {
                if (close > *last) {
                    *last = document->lastSymbolOfLine(
                        document->symbolEndLine(close)
                    );
                }
                const int exprFirst = *first + 1;
                const int from = document->symbols().at(exprFirst).from;
                const int to = document->symbolEnd(close);
                if ( ! document->hasCommentBetween(from, to)
                    && ! document->spansDirective(from, to)
                ) {
                    *first = exprFirst;
                    *last = close + 1;
                    return true;
                }
            }
        }
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
    if (lead == IF
        || lead == ELSE
        || lead == WHILE
        || lead == FOR
        || lead == DO
        || lead == SWITCH
        || lead == CASE
    ) {
        return false;
    }
    bool hasBinaryOperator = false;
    // Follows the statement past the nested expressions it spans, up to the
    // `;` that ends it, so that one whose first line holds a broken call and
    // goes on after that call's `)` still gives its whole range.
    int end = *last;
    for (int i = *first; i < document->symbols().size(); ++i) {
        const Token token = document->token(i);
        if ((token == LPAREN
                || token == LBRACK
                || token == LBRACE)
            && document->match(i) > i
        ) {
            const int close = document->match(i);
            // A body or a block ends the statement at its `}`; only an
            // initializer list lets the statement go on, so that an inline
            // body's `}` never joins the declaration that follows it.
            if (token == LBRACE
                && document->braceScope(close) != FormatDocument::InitScope
            ) {
                return false;
            }
            i = close;
            continue;
        }
        if (token == SEMIC) {
            end = i;
            break;
        }
        if (token == RBRACE) {
            return false;
        }
        hasBinaryOperator |= operatorPrecedence(token) > 0
            && i > *first
            && isOperandEnd(document->token(i - 1));
    }
    *last = end;
    const int from = document->lineStart(line);
    const int to = document->lineEnd(document->symbolEndLine(end));
    return (hasBinaryOperator || document->symbolLine(end) == line)
        && document->token(end) == SEMIC
        && document->lastSymbolOfLine(document->symbolEndLine(end)) == end
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
        const int limit = wrapMaxAt(document, document->lineStart(line));
        int first = -1;
        int last = -1;
        if (document->isDirectiveLine(line)
            || ! this->wholeStatement(document, line, &first, &last)
        ) {
            continue;
        }
        if ( ! codeTargetAnyAt(document, document->lineStart(line))) {
            // Keep existing breaks when each physical line
            // fits its wrap column.
            const int endLine = document->symbolEndLine(last);
            int current = line;
            while (current <= endLine
                && textWidth(document->lineText(current))
                    <= wrapColumn(document, document->lineStart(current))
            ) {
                ++current;
            }
            if (current > endLine) {
                continue;
            }
        }
        const QByteArray indent = document->indentOf(line);
        const int column = document->symbols().at(first).from
            - document->lineStart(line);
        FormatExpressionLayout layout(
            document,
            &conditions,
            textWidth(indent)
        );
        const int lastSymbol = document->token(last) == SEMIC ? last : last - 1;
        if (textWidth(indent + layout.oneLine(first, lastSymbol)) <= limit
            || ! layout.hasOuterChain(first, lastSymbol - 1)
        ) {
            continue;
        }
        const int exprLast = document->token(last) == SEMIC ? last - 1 : last;
        const int trailing = (last < document->symbols().size()
                && document->symbolLine(last)
                    == document->symbolEndLine(exprLast)
            ) ? 1 : 0;
        const QByteArray laidOut = layout.layout(
            first,
            exprLast,
            column,
            trailing
        );
        const int from = document->symbols().at(first).from;
        const int length = document->symbolEnd(exprLast) - from;
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
                    "each line within %1 columns and led by its " "operator")).arg(limit),
                1,
                limit
            );
        }
    }
}
