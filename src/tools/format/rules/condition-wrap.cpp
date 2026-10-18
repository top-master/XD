// Copyright (C) 2015 The XD Company Ltd.
// SPDX-License-Identifier: Apache-2.0

#include "./condition-wrap.h"

#include "../format-document.h"
#include "../parser/code-helpers.h"
#include "../parser/layout-helpers.h"

#include <QtCore/QByteArray>

/*!
    \class FormatConditionWrapRule
    \internal

    Breaks the condition of an `if` or a `while` that sits on one line and
    passes the wrap column (see wrapColumn()) before each `&&` and `||`, at
    every nesting level: each operand line starts with its operator, one step
    (4 spaces) past the line that opened its group. A group of plain operands
    closes its `)` at the end of its last line; a group that holds a
    parenthesized operand closes it on a line of its own, at the indent of
    the line that opened it; and the condition's own `)` always does, at the
    keyword's indent, with the body's `{` after it.

    An operand with a conditional expression that still passes the wrap
    column breaks before its `?`, and then before its `:` too (see
    ternaryLines()); a parenthesized one then closes its `)` on a line of its
    own. A condition with a comment, a brace, or a directive in it is left as
    it is, and so is one without a `&&` or `||` of its own.
*/

/*!
    Breaks each long one-line condition of \a document.
*/
void FormatConditionWrapRule::run(FormatDocument *document) const
{
    const int wrap = wrapColumn(document);
    for (int i = 0; i + 1 < document->symbols().size(); ++i) {
        const Token token = document->token(i);
        if ((token != IF
            && token != WHILE)
            || document->token(i + 1) != LPAREN
        ) {
            continue;
        }
        const int open = i + 1;
        const int close = document->match(open);
        const int line = document->symbolLine(i);
        if (close < 0 || document->symbolEndLine(close) != line || document->isDirectiveLine(line)
            || textWidth(document->lineText(line)) <= wrap
            || this->operators(document, open, close).isEmpty()
            || document->hasCommentBetween(document->symbols().at(open).from,
                                           document->symbolEnd(close))
        ) {
            continue;
        }
        bool hasBrace = false;
        for (int k = open + 1; k < close && ! hasBrace; ++k) {
            hasBrace = document->token(k) == LBRACE;
        }
        if (hasBrace) {
            continue;
        }

        // Lays the condition out, and puts the body's `{` right after it.
        const int indent = document->indentOf(line).size();
        const int column = document->symbols().at(open).from
            - document->lineStart(line);
        const QByteArray layout = this->group(
            document,
            open,
            close,
            indent,
            column,
            true
        );
        const int from = document->symbols().at(open).from;
        int to = document->symbolEnd(close);
        QByteArray text = layout;
        if (document->token(close + 1) == LBRACE
            && document->symbolLine(close + 1) == line
        ) {
            to = document->symbolEnd(close + 1);
            text += " {";
        }
        document->replace(from, to - from, text);
        i = close;
    }
}

/*!
    Builds the indexes of the `&&` and `||` tokens of the group that the `(`
    at \a open of \a document opens and the `)` at \a close closes, outside
    of the groups nested in it.
*/
QList<int > FormatConditionWrapRule::operators(const FormatDocument *document, int open,
                                               int close) const
{
    QList<int > found;
    for (int k = open + 1; k < close; ++k) {
        const Token token = document->token(k);
        if ((token == LPAREN
            || token == LBRACK)
            && document->match(k) > k
        ) {
            k = document->match(k);
        } else if (token == ANDAND || token == OROR) {
            found << k;
        }
    }
    return found;
}

/*!
    Builds the layout of the group from the `(` at \a open of \a document to
    the `)` at \a close, whose `(` stands at \a column of a line indented by
    \a indent; \a isCondition tells that the group is a whole condition,
    whose `)` closes on a line of its own.
*/
QByteArray FormatConditionWrapRule::group(const FormatDocument *document, int open, int close,
                                          int indent, int column, bool isCondition) const
{
    const QList<int > ops = this->operators(document, open, close);
    // Keeps the spaces between the `(` and the first operand, such as the
    // one in `( ! ready`.
    const int gapFrom = document->symbolEnd(open);
    QByteArray text = "(" + document->text().mid(
        gapFrom,
        document->symbols().at(open + 1).from - gapFrom
    );
    int lineStart = -column;
    bool isComplex = false;
    for (int k = 0; k <= ops.size(); ++k) {
        const int first = k == 0 ? open + 1 : ops.at(k - 1) + 1;
        const int last = k == ops.size() ? close - 1 : ops.at(k) - 1;
        int operandIndent = indent;
        if (k > 0) {
            text += '\n' + continuationPad(indent)
                + document->lexem(ops.at(k - 1)) + ' ';
            lineStart = text.lastIndexOf('\n') + 1;
            operandIndent = indent + 4;
        }
        const int operandColumn = text.size() - lineStart;

        // Lays out a nested group, after the `!` signs in front of it.
        int start = first;
        while (start < last && document->token(start) == NOT) {
            ++start;
        }
        if (document->token(start) == LPAREN && document->match(start) == last
            && ! this->operators(document, start, last).isEmpty()
        ) {
            const QByteArray signs = start > first
                ? document->text().mid(
                    document->symbols().at(first).from,
                    document->symbols().at(start).from
                        - document->symbols().at(first).from
                )
                : QByteArray();
            text += signs + this->group(
                document,
                start,
                last,
                operandIndent,
                operandColumn + signs.size(),
                false
            );
            isComplex = true;
        } else {
            text += this->atom(
                document,
                first,
                last,
                operandIndent,
                operandColumn,
                &isComplex
            );
        }
        lineStart = text.lastIndexOf('\n') + 1;
    }
    if (isCondition || isComplex) {
        text += '\n' + QByteArray(indent, ' ') + ')';
    } else {
        text += ')';
    }
    return text;
}

/*!
    Builds the text of the operand from the token \a first up to \a last of
    \a document, at \a column of a line indented by \a indent: as the file
    spells it, unless it holds a conditional expression that passes the wrap
    column, which then breaks (see ternaryLines()). \a isComplex is set for a
    parenthesized operand, and for one that takes more than one line.
*/
QByteArray FormatConditionWrapRule::atom(const FormatDocument *document, int first, int last,
                                         int indent, int column, bool *isComplex) const
{
    const QByteArray text = sourceText(document, first, last);
    const bool isParenthesized = document->token(first) == LPAREN
        && document->match(first) == last;
    if (isParenthesized) {
        *isComplex = true;
    }
    const int inner = isParenthesized ? first + 1 : first;
    const int innerLast = isParenthesized ? last - 1 : last;
    int question = -1;
    int colon = -1;
    if (column + textWidth(text) <= wrapColumn(document)
        || ! findTernary(document, inner, innerLast, &question, &colon)
    ) {
        return text;
    }

    // Breaks before the `?`, and before the `:` when that still does not
    // fit; a parenthesized operand then closes on a line of its own.
    const QByteArray head = sourceText(document, first, question - 1);
    const QByteArray middle = sourceText(document, question, colon - 1);
    const QByteArray tail = sourceText(document, colon, innerLast);
    const QList<QByteArray > lines = ternaryLines(
        head,
        middle,
        tail,
        indent,
        column,
        isParenthesized ? 1 : 0,
        wrapColumn(document)
    );
    QByteArray result;
    for (int l = 0; l < lines.size(); ++l) {
        result += (l > 0 ? "\n" : "") + lines.at(l);
    }
    if (lines.size() > 1) {
        *isComplex = true;
        if (isParenthesized) {
            result += '\n' + QByteArray(indent, ' ') + ')';
        }
    } else if (isParenthesized) {
        result += ')';
    }
    return result;
}
