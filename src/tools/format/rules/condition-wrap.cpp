// Copyright (C) 2015 The XD Company Ltd.
// SPDX-License-Identifier: Apache-2.0

#include "./condition-wrap.h"

#include "../format-document.h"
#include "../parser/code-helpers.h"
#include "../parser/layout-helpers.h"

#include <QtCore/QByteArray>

/*!
    @class FormatConditionWrapRule
    @internal

    Breaks the condition of an `if` or a `while` that sits on one line and
    passes the wrap column (see wrapColumn()), or that spans several lines
    and has one past `[Structure] wrapMax`, before each `&&` and `||` of its
    own level: each operand line starts with its operator, one step
    (4 spaces) past the keyword's line. A nested group breaks too only when it
    does not fit on its line; its operator lines then go one step past
    those of the group around it. A group of plain operands closes its `)`
    at the end of its last line; a group that holds a parenthesized operand
    closes it on a line of its own, at the indent of the line that opened
    it; and the condition's own `)` always does, at the keyword's indent,
    with the body's `{` after it.

    An operand with a conditional expression that still passes the wrap
    column breaks before its `?`, and then before its `:` too (see
    ternaryLines()); a parenthesized one then closes its `)` on a line of its
    own. A one-line condition without a `&&` or `||` of its own breaks
    before its loosest operator instead, once its line passes `wrapMax`
    (see breakPlainCondition()). A condition with a comment, a brace, or a
    directive in it is left as it is, and so is one that spans several lines
    with an operand that does.

    In agent mode (`--ai`), a condition that spans several lines keeps them:
    the rule asks the agent to regroup its lines instead (see
    FormatDocument::request()), since the person who broke them chose where.
*/

/*!
    Breaks each long one-line condition of @a document.
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
        if (close < 0 || document->isDirectiveLine(line)
            || document->hasCommentBetween(
                document->symbols().at(open).from,
                document->symbolEnd(close)
            )
        ) {
            continue;
        }

        // Breaks a one-line condition without a `&&` or `||` of its own
        // before its loosest operator (see `breakPlainCondition()`), only
        // once its line passes `wrapMax`: such a break splits a single
        // comparison, hence it waits for the hard limit.
        if (this->operators(document, open, close).isEmpty()) {
            if (document->symbolEndLine(close) == line
                && textWidth(document->lineText(line))
                    > document->settings().wrapMax
            ) {
                this->breakPlainCondition(document, open, close);
            }
            i = qMax(i, close);
            continue;
        }

        // Takes a one-line condition past the wrap column, or a split one
        // with a line past `wrapMax`.
        const int endLine = document->symbolEndLine(close);
        if (endLine == line) {
            if (textWidth(document->lineText(line)) <= wrap) {
                continue;
            }
        } else if ( ! this->isSplitTooLong(document, open, close)) {
            continue;
        } else if (document->agentMode()) {
            const int limit = document->settings().wrapMax;
            document->request(
                document->symbols().at(i).from,
                "condition-wrap",
                QString(QLL("this condition has a line past %1 columns; "
                    "regroup its lines so that the operands that "
                    "belong together share one, each line within %1 "
                    "columns and led by its && or ||")).arg(limit),
                endLine - line + 1,
                limit
            );
            i = close;
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
            && document->symbolLine(close + 1) == endLine
        ) {
            to = document->symbolEnd(close + 1);
            text += " {";
        }
        document->replace(from, to - from, text);
        i = close;
    }
}

/*!
    Breaks the one-line condition from the `(` at @a open of @a document to
    the `)` at @a close, which holds no `&&` or `||` of its own, before each
    of its loosest operators outside of its nested brackets: each such line
    goes two steps (8 spaces) past the keyword's line, one for the
    condition's parentheses and one as the operand of an operator that does
    not chain conditions. The condition's `)` then closes on a line of its
    own, at the keyword's indent, with the body's `{` after it. A condition
    with a brace in it, or without such an operator, is left as it is.
*/
void FormatConditionWrapRule::breakPlainCondition(
        FormatDocument *document, int open, int close) const
{
    // Ranks the operators from the loosest, as C++ binds them.
    static const Token ranks[][4] = {
        { OR, OR, OR, OR },
        { HAT, HAT, HAT, HAT },
        { AND, AND, AND, AND },
        { EQEQ, NE, NE, NE },
        { LANGLE, RANGLE, LE, GE },
        { LTLT, GTGT, GTGT, GTGT },
        { PLUS, MINUS, MINUS, MINUS },
        { STAR, SLASH, PERCENT, PERCENT },
    };
    QList<int > found;
    for (size_t r = 0; r < sizeof(ranks) / sizeof(ranks[0]) && found.isEmpty(); ++r) {
        for (int k = open + 1; k < close; ++k) {
            const Token token = document->token(k);
            if (token == LBRACE) {
                return;
            }
            if ((token == LPAREN || token == LBRACK)
                && document->match(k) > k
            ) {
                k = document->match(k);
                continue;
            }

            // Leaves out a unary sign: a sign that follows another operator,
            // or that opens the condition right after its `(`.
            const Token before = document->token(k - 1);
            const bool isUnary = k == open + 1 || before == LPAREN || before == COMMA
                || (before >= PLUS && before <= GE) || before == LANGLE
                || before == RANGLE || before == ANDAND || before == OROR
                || before == EQ || before == QUESTION || before == COLON;
            for (int t = 0; t < 4; ++t) {
                if (token == ranks[r][t] && ! isUnary) {
                    found << k;
                    break;
                }
            }
        }
    }
    if (found.isEmpty()) {
        return;
    }

    // Lays the operands out, and puts the body's `{` right after the `)`.
    const int line = document->symbolLine(open);
    const int indent = document->indentOf(line).size();
    const QByteArray pad(indent + 8, ' ');
    QByteArray text = "("
        + sourceText(document, open + 1, found.first() - 1);
    for (int f = 0; f < found.size(); ++f) {
        const int last = f + 1 < found.size()
            ? found.at(f + 1) - 1 : close - 1;
        text += "\n" + pad + sourceText(document, found.at(f), last);
    }
    text += "\n" + QByteArray(indent, ' ') + ")";
    const int from = document->symbols().at(open).from;
    int to = document->symbolEnd(close);
    if (document->token(close + 1) == LBRACE
        && document->symbolLine(close + 1) == line
    ) {
        to = document->symbolEnd(close + 1);
        text += " {";
    }
    document->replace(from, to - from, text);
}

/*!
    Tells whether the condition from the `(` at @a open of @a document to the
    `)` at @a close, which spans several lines, has a line past
    `[Structure] wrapMax`, and each of its operands sits on one line (see
    operators()); people lay out a condition whose operand spans lines.
*/
bool FormatConditionWrapRule::isSplitTooLong(
        const FormatDocument *document, int open,
        int close) const
{
    const int max = document->settings().wrapMax;
    bool tooLong = false;
    for (int l = document->symbolLine(open); l <= document->symbolEndLine(close); ++l) {
        tooLong = tooLong || textWidth(document->lineText(l)) > max;
    }
    if ( ! tooLong) {
        return false;
    }
    const QList<int > ops = this->operators(document, open, close);
    for (int k = 0; k <= ops.size(); ++k) {
        const int first = k == 0 ? open + 1 : ops.at(k - 1) + 1;
        const int last = k == ops.size() ? close - 1 : ops.at(k) - 1;
        if (first > last
            || document->symbolLine(first) != document->symbolEndLine(last)
        ) {
            return false;
        }
    }
    return true;
}

/*!
    Builds the indexes of the `&&` and `||` tokens of the group that the `(`
    at @a open of @a document opens and the `)` at @a close closes, outside
    of the groups nested in it.
*/
QList<int > FormatConditionWrapRule::operators(
        const FormatDocument *document, int open,
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
    Builds the layout of the group from the `(` at @a open of @a document to
    the `)` at @a close, whose `(` stands at @a column of a line indented by
    @a indent; @a isCondition tells that the group is a whole condition,
    whose `)` closes on a line of its own.
*/
QByteArray FormatConditionWrapRule::group(
        const FormatDocument *document, int open, int close,
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
        // Keeps a nested group that fits on one line as it is, hence a
        // condition breaks first at the `&&` and `||` of its own level; a
        // nested group that breaks takes one more step, so that its
        // operators stand apart from those of the group around it.
        const bool isNested = document->token(start) == LPAREN
            && document->match(start) == last
            && ! this->operators(document, start, last).isEmpty();
        const bool fits = operandColumn
            + textWidth(sourceText(document, first, last)) + 1
            <= wrapColumn(document);
        if (isNested && ! fits) {
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
                indent + 4,
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
    Builds the text of the operand from the token @a first up to @a last of
    @a document, at @a column of a line indented by @a indent: as the file
    spells it, unless it holds a conditional expression that passes the wrap
    column, which then breaks (see ternaryLines()). @a isComplex is set for a
    parenthesized operand, and for one that takes more than one line.
*/
QByteArray FormatConditionWrapRule::atom(
        const FormatDocument *document, int first, int last,
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
