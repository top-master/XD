// Copyright (C) 2015 The XD Company Ltd.
// SPDX-License-Identifier: Apache-2.0

#include "./operator-first.h"

#include "../format-document.h"
#include "../parser/code-helpers.h"
#include "../parser/layout-helpers.h"

#include <QtCore/QByteArray>

/*!
    @class FormatOperatorFirstRule
    @internal

    Breaks a long expression before its operator, never after it: each `&&`,
    `||`, `<<`, `+`, `-`, `|`, `^` or `?` that ends a line moves to the
    start of the line after it. After a line of only closing brackets, an
    operator and its operand join that line when they fit, except for `&&`
    and `||`, which keep their own lines.
*/

/*!
    Tells whether the `:` at @a index of @a document belongs to a `?` of its
    own statement, rather than starting an initializer list or a label.
*/
static bool isTernaryColon(const FormatDocument *document, int index)
{
    for (int k = index - 1; k >= 0; --k) {
        const Token token = document->token(k);
        if ((token == RPAREN || token == RBRACK)
            && document->match(k) >= 0
        ) {
            k = document->match(k);
            continue;
        }
        if (token == QUESTION) {
            return true;
        }
        if (token == SEMIC || token == LBRACE || token == RBRACE) {
            return false;
        }
    }
    return false;
}

/*!
    Joins the line that the operator at @a index of @a document leads to the
    line before it, when that one holds only closing brackets and the joined
    line fits `wrapMax`. The `&&` and `||` operators, a prefix `++` or
    `--`, a `:` without its `?`, a block's `}`, and the head of a control
    statement (whose body follows it) keep their lines. Returns whether the
    edit was queued.
*/
static bool joinsClosingLine(FormatDocument *document, int index)
{
    const Token token = document->token(index);
    if (token == ANDAND || token == OROR) {
        return false;
    }
    const Token next = document->token(index + 1);
    const bool isPostfix = (token == INCR || token == DECR)
        && (next == SEMIC || next == RPAREN || next == RBRACK || next == COMMA);
    const bool isTight = isPostfix || token == DOT || token == ARROW
        || token == DOTSTAR || token == ARROW_STAR;
    if ( ! isTight && ! isAssignmentOperator(token)
        && ( ! isBinaryOperator(token)
            || (token == COLON && ! isTernaryColon(document, index)))
    ) {
        return false;
    }
    const int line = document->symbolLine(index);
    const int before = index - 1;
    if (line < 2
        || document->firstSymbolOfLine(line) != index
        || document->lastSymbolOfLine(line) == index
        || document->symbolEndLine(before) != line - 1
        || document->isDirectiveLine(line)
        || document->isDirectiveLine(line - 1)
        || ! isClosingLine(document->lineText(line - 1))
        || document->hasCommentBetween(
            document->symbolEnd(before),
            document->lineEnd(line)
        )
    ) {
        return false;
    }
    const int open = document->match(before);
    const Token closer = document->token(before);
    if (closer == RBRACE
        && (open < 0 || document->braceScope(open) != FormatDocument::InitScope)
    ) {
        return false;
    }
    if (closer == RPAREN && open > 0) {
        const Token keyword = document->token(open - 1);
        if (keyword == IF
            || keyword == WHILE
            || keyword == FOR
            || keyword == SWITCH
            || keyword == CATCH
        ) {
            return false;
        }
    }

    // Moves the operator line's text up behind the closing brackets.
    const QByteArray space = isTight ? QByteArray() : QByteArray(" ");
    const QByteArray text = document->lineText(line).trimmed();
    const int previousStart = document->lineStart(line - 1);
    const int previousEnd = document->symbolEnd(before);
    const QByteArray joined = document->text().mid(
        previousStart,
        previousEnd - previousStart
    ) + space + text;
    if (textWidth(joined) > document->settings().wrapMax) {
        return false;
    }
    const int from = document->lineStart(line);
    const int lineEnd = document->lineEnd(line);
    const bool hasNewline = lineEnd < document->text().size();
    return document->replace(
        previousEnd,
        from - previousEnd,
        space + text + (hasNewline ? "\n" : ""),
        from, lineEnd - from + (hasNewline ? 1 : 0), QByteArray()
    );
}

/*!
    Moves each binary operator that ends a line to the start of the next
    line, outside of preprocessor directives and when no comment follows it.
    Joins an operator-led line to a closing-only line when both are selected
    and their combined width fits, except for `&&` and `||`.
*/
void FormatOperatorFirstRule::run(FormatDocument *document) const
{
    for (int i = 1; i + 1 < document->symbols().size(); ++i) {
        const Token token = document->token(i);
        if (joinsClosingLine(document, i)) {
            continue;
        }
        if (token != ANDAND
            && token != OROR
            && token != LTLT
            && token != PLUS
            && token != MINUS
            && token != OR
            && token != HAT
            && token != QUESTION
        ) {
            continue;
        }
        const int line = document->symbolLine(i);
        if (document->lastSymbolOfLine(line) != i || document->isDirectiveLine(line)
            || document->symbolEndLine(i - 1) != line || document->token(i - 1) == OPERATOR
            || document->hasCommentBetween(
                document->symbolEnd(i),
                document->symbols().at(i + 1).from
            )
        ) {
            continue;
        }
        const int from = document->symbolEnd(i - 1);
        if (document->spansDirective(
                from,
                document->symbols().at(i + 1).from
            )
        ) {
            continue;
        }
        document->replace(
            from, document->symbolEnd(i) - from, QByteArray(),
            document->symbols().at(i + 1).from, 0, document->lexem(i) + ' '
        );
    }
}
