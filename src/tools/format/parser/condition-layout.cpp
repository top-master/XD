// Copyright (C) 2015 The XD Company Ltd.
// SPDX-License-Identifier: Apache-2.0

#include "./condition-layout.h"

#include "../format-document.h"
#include "./code-helpers.h"
#include "./layout-helpers.h"
#include "./scope-conditions.h"

#include <QtCore/QPair>

/*!
    @class FormatConditionLayout
    @internal

    Lays out a condition, for every rule that breaks one: the content of the
    parentheses of an `if` or a `while` (see FormatConditionWrapRule), and
    the head of a conditional expression, before its `?` (see
    FormatTernaryWrapRule). Both break first before the `||` and `&&` of
    their own level, and keep a comparison whole on a line of its own while
    it fits `[Structure] wrapMax` (see comparisonLines()).

    In a condition's parentheses, each operand line starts with its
    operator, one step (4 spaces) past the keyword's line. A nested group
    breaks too only when it does not fit on its line; its operator lines
    then go one step past those of the group around it. A group of plain
    operands closes its `)` at the end of its last line, and a group that
    holds a parenthesized operand closes it on a line of its own, at the
    indent of the line that opened it. An operand with a conditional
    expression that still passes the wrap column breaks before its `?`, and
    then before its `:` too (see ternaryLines()).
*/

/*!
    Builds a layout of the conditions of @a document; @a conditions decides,
    through `[Structure] wrapArgsAt`, whether a value may move below its `=`
    (see reachesWrapArgsAt()).
*/
FormatConditionLayout::FormatConditionLayout(const FormatDocument *document,
                                             FormatScopeConditions *conditions)
    : m_document(document), m_conditions(conditions)
{
}

/*!
    Builds the layout of the whole condition from the `(` at @a open to the
    `)` at @a close, whose `(` stands at @a column of a line indented by
    @a indent (see group()); its `)` closes on a line of its own.
*/
QByteArray FormatConditionLayout::layoutCondition(
        int open, int close,
        int indent, int column) const
{
    return this->group(open, close, indent, column, true);
}

/*!
    Ranks @a token among the operators that break a condition without a `&&`
    or `||` of its own, from the loosest, as C++ binds them; -1 for any other.
*/
static int conditionRank(Token token)
{
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
    for (size_t r = 0; r < sizeof(ranks) / sizeof(ranks[0]); ++r) {
        for (int t = 0; t < 4; ++t) {
            if (ranks[r][t] == token) {
                return int(r);
            }
        }
    }
    return -1;
}

/*!
    Builds the layout of the one-line condition from the `(` at @a open to
    the `)` at @a close, which holds no `&&` or `||` of its own: broken
    before each of its loosest operators outside of its nested brackets
    (see conditionRank()), each such line two steps (8 spaces) past the
    keyword's line, one for the condition's parentheses and one as the
    operand of an operator that does not chain conditions, and the `)` on a
    line of its own, at the keyword's indent. Builds an empty text for a
    condition with a brace in it, or without such an operator.
*/
QByteArray FormatConditionLayout::layoutPlain(int open, int close) const
{
    const QList<int > found = loosestOperators(
        m_document,
        open + 1,
        close - 1,
        &conditionRank,
        0,
        Q_NULLPTR
    );
    if (found.isEmpty()) {
        return QByteArray();
    }
    const int line = m_document->symbolLine(open);
    const int indent = m_document->indentOf(line).size();
    const QByteArray pad(indent + 8, ' ');
    QByteArray text = "("
        + sourceText(m_document, open + 1, found.first() - 1);
    for (int f = 0; f < found.size(); ++f) {
        const int last = f + 1 < found.size()
            ? found.at(f + 1) - 1 : close - 1;
        text += "\n" + pad + sourceText(m_document, found.at(f), last);
    }
    return text + "\n" + QByteArray(indent, ' ') + ")";
}

/*!
    Builds the lines of the head of a conditional expression, the tokens
    @a first up to @a last on a line indented by @a indent, which they
    start (see headLines()): the first line without its indent, and every
    later line with its own.
*/
QList<QByteArray > FormatConditionLayout::layoutHead(
        int first, int last,
        int indent) const
{
    const int wrap = wrapColumn(m_document);
    return this->headLines(first, last, indent, indent, wrap, 0, true);
}

/*!
    Builds the indexes of the `&&` and `||` tokens of the group that the `(`
    at @a open opens and the `)` at @a close closes, outside
    of the groups nested in it.
*/
QList<int > FormatConditionLayout::operators(
        int open,
        int close) const
{
    QList<int > found;
    for (int k = open + 1; k < close; ++k) {
        const Token token = m_document->token(k);
        if ((token == LPAREN
                || token == LBRACK)
            && m_document->match(k) > k
        ) {
            k = m_document->match(k);
        } else if (token == ANDAND || token == OROR) {
            found << k;
        }
    }
    return found;
}

/*!
    Builds the layout of the group from the `(` at @a open to
    the `)` at @a close, whose `(` stands at @a column of a line indented by
    @a indent; @a isCondition tells that the group is a whole condition,
    whose `)` closes on a line of its own.
*/
QByteArray FormatConditionLayout::group(
        int open, int close,
        int indent, int column, bool isCondition) const
{
    const QList<int > ops = this->operators(open, close);
    // Keeps the spaces between the `(` and the first operand, such as the
    // one in `( ! ready`.
    const int gapFrom = m_document->symbolEnd(open);
    QByteArray text = "(" + m_document->text().mid(
        gapFrom,
        m_document->symbols().at(open + 1).from - gapFrom
    );
    int lineStart = -column;
    bool isComplex = false;
    for (int k = 0; k <= ops.size(); ++k) {
        const int first = k == 0 ? open + 1 : ops.at(k - 1) + 1;
        const int last = k == ops.size() ? close - 1 : ops.at(k) - 1;
        int operandIndent = indent;
        if (k > 0) {
            text += '\n' + continuationPad(indent)
                + m_document->lexem(ops.at(k - 1)) + ' ';
            lineStart = text.lastIndexOf('\n') + 1;
            operandIndent = continuationIndent(indent);
        }
        const int operandColumn = text.size() - lineStart;

        // Lays out a nested group, after the `!` signs in front of it.
        int start = first;
        while (start < last && m_document->token(start) == NOT) {
            ++start;
        }
        // Keeps a nested group that fits on one line as it is, hence a
        // condition breaks first at the `&&` and `||` of its own level; a
        // nested group that breaks takes one more step, so that its
        // operators stand apart from those of the group around it.
        const bool isNested = m_document->token(start) == LPAREN
            && m_document->match(start) == last
            && ! this->operators(start, last).isEmpty();
        const bool fits = operandColumn
            + textWidth(sourceText(m_document, first, last)) + 1
            <= wrapColumn(m_document);
        if (isNested && ! fits) {
            const QByteArray signs = start > first
                ? m_document->text().mid(
                    m_document->symbols().at(first).from,
                    m_document->symbols().at(start).from
                        - m_document->symbols().at(first).from
                ) : QByteArray();
            text += signs + this->group(
                start,
                last,
                continuationIndent(indent),
                operandColumn + signs.size(),
                false
            );
            isComplex = true;
        } else {
            text += this->atom(
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
    Builds the text of the operand from the token @a first up to @a last,
    at @a column of a line indented by @a indent: as the file
    spells it, unless it holds a conditional expression that passes the wrap
    column, which then breaks (see ternaryLines()), or a comparison that
    passes `[Structure] wrapMax`, which then breaks before its `==` or such
    (see comparisonLines()). @a isComplex is set for a parenthesized
    operand, and for one that takes more than one line.
*/
QByteArray FormatConditionLayout::atom(
        int first, int last,
        int indent, int column, bool *isComplex) const
{
    const QByteArray text = sourceText(m_document, first, last);
    const bool isParenthesized = m_document->token(first) == LPAREN
        && m_document->match(first) == last;
    if (isParenthesized) {
        *isComplex = true;
    }
    const int inner = isParenthesized ? first + 1 : first;
    const int innerLast = isParenthesized ? last - 1 : last;
    int question = -1;
    int colon = -1;
    if (column + textWidth(text) <= wrapColumn(m_document)) {
        return text;
    }

    // Keeps a plain operand's comparison whole while it fits `wrapMax`, else
    // breaks it before its `==` or such, one step past the operand's line.
    if ( ! findTernary(m_document, inner, innerLast, &question, &colon)) {
        if (isParenthesized) {
            return text;
        }
        const QList<QByteArray > lines = comparisonLines(
            m_document,
            first,
            last,
            column,
            indent,
            continuationIndent(0)
        );
        if (lines.size() > 1) {
            *isComplex = true;
        }
        QByteArray result = lines.first();
        for (int l = 1; l < lines.size(); ++l) {
            result += "\n" + lines.at(l);
        }
        return result;
    }

    // Breaks before the `?`, and before the `:` when that still does not
    // fit; a parenthesized operand then closes on a line of its own.
    const QByteArray head = sourceText(m_document, first, question - 1);
    const QByteArray middle = sourceText(m_document, question, colon - 1);
    const QByteArray tail = sourceText(m_document, colon, innerLast);
    const QList<QByteArray > lines = ternaryLines(
        head,
        middle,
        tail,
        indent,
        column,
        isParenthesized ? 1 : 0,
        wrapColumn(m_document)
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

/*!
    Ranks @a token among the operators that headLines() breaks at, from the
    one it tries first: `||`, then `&&`, then `=`; -1 for any other token.
    Comparisons break last, through comparisonLines().
*/
static int headRank(Token token)
{
    static const Token ranks[][4] = {
        { OROR, OROR, OROR, OROR },
        { ANDAND, ANDAND, ANDAND, ANDAND },
        { EQ, EQ, EQ, EQ },
    };
    for (size_t r = 0; r < sizeof(ranks) / sizeof(ranks[0]); ++r) {
        for (int t = 0; t < 4; ++t) {
            if (ranks[r][t] == token) {
                return int(r);
            }
        }
    }
    return -1;
}

/*!
    Builds the lines of the head of a conditional expression, the tokens
    @a first up to @a last, written from @a column of a line
    indented by @a indent: one line when it fits @a wrap, else broken at the
    loosest operators that headRank() ranks, from @a tier on, outside of its
    brackets (see loosestOperators()), and at its comparisons last (see
    comparisonLines()). It breaks before each `||` or `&&`, and after an
    `=` when the head holds a single one that stands at or past
    `[Structure] wrapArgsAt` (see reachesWrapArgsAt()); an `=` before it
    stays, and its comparison breaks instead, unless that comparison cannot
    break. Each later part goes one step past @a indent and breaks the same
    way, at a later tier. A comparison on the statement's own line
    (@a startsStatement) breaks two steps in, past the one step of the `?`.
*/
QList<QByteArray > FormatConditionLayout::headLines(
        int first, int last, int column, int indent, int wrap, int tier,
        bool startsStatement) const
{
    const QByteArray text = sourceText(m_document, first, last);
    if (column + textWidth(text) <= wrap) {
        return QList<QByteArray >() << text;
    }
    const int step = continuationIndent(0);
    const QList<QByteArray > compared = comparisonLines(
        m_document,
        first,
        last,
        column,
        indent,
        startsStatement ? 2 * step : step
    );

    // Finds the operators to break at; an `=` counts only when it is the
    // head's single one, and stands at or past `wrapArgsAt`, or when the
    // comparison cannot break without it.
    int found = -1;
    QList<int > operators = loosestOperators(
        m_document, first, last, &headRank, tier, &found
    );
    while ( ! operators.isEmpty()
        && m_document->token(operators.first()) == EQ
    ) {
        const bool reaches = reachesWrapArgsAt(
            m_document,
            m_conditions,
            operators.first()
        );
        if (operators.size() == 1 && (reaches || compared.size() < 2)) {
            break;
        }
        operators = loosestOperators(
            m_document, first, last, &headRank, found + 1, &found
        );
    }
    if (operators.isEmpty()) {
        return compared;
    }

    // Splits the head after its `=`, or before each of its operators.
    QList<QPair<int, int > > parts;
    if (m_document->token(operators.first()) == EQ) {
        parts << qMakePair(first, operators.first());
        parts << qMakePair(operators.first() + 1, last);
    } else {
        parts << qMakePair(first, operators.first() - 1);
        for (int k = 0; k < operators.size(); ++k) {
            const int end = k + 1 < operators.size()
                ? operators.at(k + 1) - 1 : last;
            parts << qMakePair(operators.at(k), end);
        }
    }

    // Lays out each part, the later ones a step in, at the later tiers.
    const QByteArray pad = continuationPad(indent);
    const int partIndent = textWidth(pad);
    QList<QByteArray > lines;
    for (int p = 0; p < parts.size(); ++p) {
        const bool isFirst = p == 0;
        QList<QByteArray > partLines = this->headLines(
            parts.at(p).first,
            parts.at(p).second,
            isFirst ? column : partIndent,
            isFirst ? indent : partIndent,
            wrap,
            found + 1,
            isFirst && startsStatement
        );
        if ( ! isFirst) {
            partLines.first().prepend(pad);
        }
        lines << partLines;
    }
    return lines;
}
