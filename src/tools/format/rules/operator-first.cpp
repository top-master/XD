// Copyright (C) 2015 The XD Company Ltd.
// SPDX-License-Identifier: Apache-2.0

#include "./operator-first.h"

#include "../format-document.h"
#include "../parser/code-helpers.h"
#include "../parser/layout-helpers.h"
#include "../parser/expression-layout.h"
#include "../parser/scope-conditions.h"

#include <QtCore/QByteArray>

/*!
    @class FormatOperatorFirstRule
    @internal

    Breaks a long expression before its operator, never after it: each `&&`,
    `||`, `<<`, `+`, `-`, `|`, `^`, `==`, `!=` or `?` that ends a line
    moves to the start of the line after it. After a line of only closing
    brackets, an operator and its operand join that line when they fit,
    except for `&&` and `||`, which keep their own lines.
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
    Places the operators on either side of a multiline operand at the same
    indentation. A call closes on its last argument. A parenthesized
    operand keeps its own closing layout.
*/
static bool layoutChainOperand(
        FormatDocument *document, FormatScopeConditions *conditions, int next)
{
    const int close = next - 1;
    if ( ! closesEarlierLine(document, close)) {
        return false;
    }
    const int previous = precedingChainOperator(document, next);
    if (previous < 1) {
        return false;
    }
    const int first = previous + 1;
    const int open = document->match(close);
    const int from = document->symbolEnd(previous - 1);
    const int end = document->symbols().at(next).from;
    if (document->hasCommentBetween(from, end)
        || document->spansDirective(from, end)
    ) {
        return false;
    }
    const int line = document->symbolLine(previous);
    const bool startsLine = document->firstSymbolOfLine(line) == previous;
    const bool conditional = document->token(previous) == QUESTION;
    const QByteArray pad = startsLine
        ? document->indentOf(line)
        : continuationPad(textWidth(document->indentOf(line)),
                          conditional ? 1 : conditions->operatorPadding(previous));
    const QByteArray newline = document->text().mid(
        document->lineEnd(line) - 1,
        1
    ) == "\r"
        ? "\r\n" : "\n";
    const QByteArray gap = newline + pad;
    const int closeEnd = document->symbolEnd(close);
    if (first < open) {
        const bool keepInline = ! startsLine && ! conditional;
        const int layoutIndent = keepInline
            ? textWidth(document->indentOf(line))
            : textWidth(pad);
        FormatExpressionLayout layout(document, conditions, layoutIndent);
        const int lineStart = document->lineStart(line);
        const int operatorColumn = keepInline
            ? textWidth(document->text().mid(lineStart, from - lineStart)) + 1
            : textWidth(pad);
        const int column = operatorColumn
            + document->lexem(previous).size()
            + 1;
        const QByteArray flat = layout.oneLine(first, close);
        QByteArray operand = flat;
        const int limit = conditional
            ? wrapMaxAt(document, from)
            : wrapColumn(document, from);
        if (column + textWidth(flat) > limit) {
            const int extraPad = keepInline
                ? conditions->operatorPadding(previous) : 0;
            operand = layout.layoutList(
                first,
                open,
                close,
                column,
                true,
                0,
                extraPad
            );
            if (newline == "\r\n") {
                operand.replace("\r\n", "\n");
                operand.replace("\n", newline);
            }
        }
        const QByteArray leadGap = keepInline ? QByteArray(" ") : gap;
        const QByteArray replacement = leadGap
            + document->lexem(previous)
            + ' '
            + operand
            + gap;
        if (replacement != document->text().mid(from, end - from)) {
            document->replace(from, end - from, replacement);
        }
    } else if (document->text().mid(closeEnd, end - closeEnd) != gap) {
        document->replace(closeEnd, end - closeEnd, gap);
    }
    return true;
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
    // Keeps logical operators on their own continuation lines.
    if (token == ANDAND || token == OROR) {
        return false;
    }
    const Token next = document->token(index + 1);
    const bool isPostfix = (token == INCR || token == DECR)
        && (next == SEMIC || next == RPAREN || next == RBRACK || next == COMMA);
    const bool isTight = isPostfix || token == DOT || token == ARROW
        || token == DOTSTAR || token == ARROW_STAR;
    // Joins expression operators, leaving labels and prefix increments alone.
    if ( ! isTight && ! isAssignmentOperator(token)
        && ( ! isBinaryOperator(token)
            || (token == COLON && ! isTernaryColon(document, index)))
    ) {
        return false;
    }
    const int line = document->symbolLine(index);
    const int before = index - 1;
    // Requires adjacent lines with only closing brackets on the first one,
    // and leaves comments and directives alone.
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
    // Joins initializer braces, leaving block braces on their own lines.
    if (closer == RBRACE
        && (open < 0 || document->braceScope(open) != FormatDocument::InitScope)
    ) {
        return false;
    }
    // Keeps a control statement's body separate from its closing parenthesis.
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
    // Keeps the break when the joined line would exceed `wrapMax`.
    if (textWidth(joined)
            > wrapMaxAt(document, document->lineStart(line))
    ) {
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
    FormatScopeConditions conditions(document);
    const bool members = this->forMembers();
    for (int i = 1; i + 1 < document->symbols().size(); ++i) {
        const Token token = document->token(i);
        const bool memberAccess = token == DOT || token == ARROW
            || token == DOTSTAR || token == ARROW_STAR;
        if (memberAccess != members) {
            continue;
        }
        if (layoutChainOperand(document, &conditions, i)
            || joinsClosingLine(document, i)
        ) {
            continue;
        }
        if (token != ANDAND
            && token != OROR
            && token != LTLT
            && token != PLUS
            && token != MINUS
            && token != OR
            && token != HAT
            && token != EQEQ
            && token != NE
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
        )) {
            continue;
        }
        document->replace(
            from, document->symbolEnd(i) - from, QByteArray(),
            document->symbols().at(i + 1).from, 0, document->lexem(i) + ' '
        );
    }
}
