// Copyright (C) 2015 The XD Company Ltd.
// SPDX-License-Identifier: Apache-2.0

#include "./condition-layout.h"

#include "../format-document.h"
#include "./code-helpers.h"
#include "./expression-layout.h"
#include "./layout-helpers.h"
#include "./scope-conditions.h"

#include <QtCore/QHash>
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
    return this->group(
        open,
        close,
        indent,
        column,
        true,
        this->forcedOperators(open, close),
        0
    );
}

namespace {

/*!
    Names a range of a condition's operands (see `[Structure]
    wrapConditionsPer`): the `(` or `[` that opens it, and the argument of
    that bracket's call it covers, or -1 for all of the bracket.
*/
typedef QPair<int, int > ConditionRange;

/*!
    Tells whether the `(` or `[` at @a open of @a document holds the
    arguments of a call, or an index, rather than a group of operands: a `[`
    always does, and so does a `(` right after a name, a `>`, or a `)`.
*/
static bool opensCall(const FormatDocument *document, int open)
{
    if (document->token(open) == LBRACK) {
        return true;
    }
    switch (document->token(open - 1)) {
    case IDENTIFIER:
    case RPAREN:
    case RBRACK:
    case RANGLE:
    case GTGT:
        return true;
    default:
        return false;
    }
}

} // namespace

/*!
    Builds the `&&` and `||` tokens of the condition from the `(` at
    @a open to the `)` at @a close that break whatever the width, since
    their range holds more operands than `[Structure] wrapConditionsOver`
    allows (see FormatScopeConditions::conditionsLimit()). The range of an
    operator is, by `[Structure] wrapConditionsPer`:

    * `pair`: the closest brackets around it, or its argument of them when
      they are a call's;
    * `pair-bundle`: the closest brackets of a call around it, else the
      condition's own, hence the groups between count along;
    * `pair-root`: the condition's own parentheses.

    Builds none for `line`, which group() asks at each line instead.
*/
QSet<int > FormatConditionLayout::forcedOperators(int open, int close) const
{
    QSet<int > forced;
    const QString per = m_document->configAt(
        m_document->symbols().at(open).from
    ).wrapConditionsPer;
    if (per == QLL("line")) {
        return forced;
    }

    // Finds the range of each operator, and counts the operands of each.
    QList<QPair<int, ConditionRange > > found;
    QHash<ConditionRange, int > counts;
    QList<int > brackets;
    QList<int > arguments;
    for (int k = open + 1; k < close; ++k) {
        const Token token = m_document->token(k);
        if (token == LBRACE && m_document->match(k) > k) {
            k = m_document->match(k);
        } else if ((token == LPAREN || token == LBRACK)
            && m_document->match(k) > k
        ) {
            brackets << k;
            arguments << 0;
        } else if ((token == RPAREN || token == RBRACK)
            && ! brackets.isEmpty()
            && m_document->match(k) == brackets.last()
        ) {
            brackets.removeLast();
            arguments.removeLast();
        } else if (token == COMMA && ! brackets.isEmpty()) {
            ++arguments.last();
        } else if (token == ANDAND || token == OROR) {
            ConditionRange range(open, -1);
            if (per == QLL("pair") && ! brackets.isEmpty()) {
                const int inner = brackets.last();
                range = ConditionRange(
                    inner,
                    opensCall(m_document, inner) ? arguments.last() : -1
                );
            } else if (per == QLL("pair-bundle")) {
                for (int b = brackets.size() - 1; b >= 0; --b) {
                    if (opensCall(m_document, brackets.at(b))) {
                        range = ConditionRange(brackets.at(b), -1);
                        break;
                    }
                }
            }
            found << qMakePair(k, range);
            counts.insert(range, counts.value(range, 1) + 1);
        }
    }

    // Marks the operators of each range that holds more than its limit.
    QSet<ConditionRange > over;
    for (QHash<ConditionRange, int >::const_iterator i = counts.constBegin();
         i != counts.constEnd();
         ++i
    ) {
        const int limit = m_conditions->conditionsLimit(
            i.key().first,
            i.value()
        );
        if (limit > 0 && i.value() > limit) {
            over.insert(i.key());
        }
    }
    for (int i = 0; i < found.size(); ++i) {
        if (over.contains(found.at(i).second)) {
            forced.insert(found.at(i).first);
        }
    }
    return forced;
}

/*!
    Ranks @a token among the operators that break a condition without a `&&`
    or `||` of its own, from the loosest, as C++ binds them; -1 for any other.
*/
static int conditionRank(Token token)
{
    switch (token) {
    case OR:
        return 0;
    case HAT:
        return 1;
    case AND:
        return 2;
    case LTLT:
    case GTGT:
        return 5;
    case PLUS:
    case MINUS:
        return 6;
    case STAR:
    case SLASH:
    case PERCENT:
        return 7;
    default: {
        // Ranks the comparisons right after `&`.
        const int comparison = comparisonRank(token);
        return comparison < 0 ? -1 : 3 + comparison;
    }
    }
}

/*!
    Builds the layout of the one-line condition from the `(` at @a open to
    the `)` at @a close, which holds no `&&` or `||` of its own: broken
    before each of its loosest operators outside of its nested brackets
    (see conditionRank()). An early comparison keeps its right operand's
    tighter operators as the break when those lines fit. Each such line sits
    two steps (8 spaces) past the
    keyword's line, one for the condition's parentheses and one as the
    operand of an operator that does not chain conditions, and the `)` on a
    line of its own, at the keyword's indent. Builds an empty text for a
    condition with a brace in it, or without such an operator.
*/
QByteArray FormatConditionLayout::layoutPlain(int open, int close) const
{
    QList<int > found = loosestOperators(
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
    const int offset = m_document->symbols().at(open).from;
    const FormatFileConfig &fileConfig = m_document->configAt(offset);
    const int indent = m_document->indentOf(line).size();
    const int rank = conditionRank(m_document->token(found.first()));
    const int start = m_document->lineStart(line);
    const int comparisonColumn = textWidth(
        m_document->text().mid(
            start, m_document->symbols().at(found.first()).from - start
        )
    );
    if (found.size() == 1 && (rank == 3 || rank == 4)
        && comparisonColumn < wrapColumn(m_document, offset)
    ) {
        const QList<int > tighter = loosestOperators(
            m_document,
            found.first() + 1,
            close - 1,
            &conditionRank,
            5,
            Q_NULLPTR
        );
        if ( ! tighter.isEmpty()) {
            const int firstWidth = textWidth(
                m_document->text().mid(
                    start,
                    m_document->symbols().at(tighter.first()).from - start
                )
            );
            bool fits = firstWidth <= fileConfig.wrapMax;
            for (int k = 0; k < tighter.size() && fits; ++k) {
                const int end = k + 1 < tighter.size()
                    ? tighter.at(k + 1) - 1 : close - 1;
                fits = indent + 8 + textWidth(sourceText(
                    m_document,
                    tighter.at(k),
                    end
                )) <= fileConfig.wrapMax;
            }
            if (fits) {
                found = tighter;
            }
        }
    }
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
    const int wrap = wrapColumn(m_document, m_document->symbols().at(first).from);
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
    whose `)` closes on a line of its own; @a trailing counts the columns
    that follow the group's `)` on its line, the `)` of the groups it ends.

    An operator of @a forced breaks (see forcedOperators()); any other one
    joins its operand to the line while it fits the wrap column. For
    `[Structure] wrapConditionsPer = line`, a line also takes only the
    operands that `[Structure] wrapConditionsOver` lets share it, asked at
    the line's last operand (see FormatScopeConditions::conditionsLimit()).
*/
QByteArray FormatConditionLayout::group(
        int open, int close,
        int indent, int column, bool isCondition,
        const QSet<int > & forced, int trailing) const
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
    const FormatFileConfig &fileConfig = m_document->configAt(
        m_document->symbols().at(open).from
    );
    const int maximum = qMax(1, fileConfig.wrapMax);
    const bool isPerLine = fileConfig.wrapConditionsPer == QLL("line");
    // Counts the operands that the current line holds; an operand that
    // takes more than one line ends its last line.
    int onLine = 0;
    bool endsLine = false;
    for (int k = 0; k <= ops.size(); ++k) {
        const int first = k == 0 ? open + 1 : ops.at(k - 1) + 1;
        const int last = k == ops.size() ? close - 1 : ops.at(k) - 1;
        // Counts the columns that follow the last operand on its line: the
        // group's `)` and those it ends, and for a condition still on one
        // line, its `) {`, since the condition then closes there.
        int after = 0;
        if (k == ops.size()) {
            after = ! isCondition ? trailing + 1
                                  : (text.contains('\n') ? 0 : 3);
        }
        int operandIndent = continuationIndent(indent);
        if (k > 0) {
            const QByteArray op = m_document->lexem(ops.at(k - 1));
            const int currentWidth = text.size() - lineStart;

            // Asks `wrapConditionsOver` at the operand that may end the line.
            const int limit = isPerLine ? m_conditions->conditionsLimit(
                open,
                ops.size() + 1,
                100.0 * (maximum - currentWidth) / maximum
            ) : 0;
            const bool joins = ! forced.contains(ops.at(k - 1))
                && ! endsLine && (limit == 0 || onLine < limit)
                && ! isClosingLine(text.mid(text.lastIndexOf('\n') + 1))
                && currentWidth + op.size() + 2
                    + textWidth(sourceText(m_document, first, last)) + after
                    <= wrapColumn(
                        m_document,
                        m_document->symbols().at(open).from
                    );
            if (joins) {
                text += ' ' + op + ' ';
            } else {
                text += '\n' + continuationPad(indent) + op + ' ';
                lineStart = text.lastIndexOf('\n') + 1;
                onLine = 0;
            }
        }
        const int lineBefore = lineStart;
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
        const QList<int > nestedOps = m_document->token(start) == LPAREN
            && m_document->match(start) == last
            ? this->operators(start, last) : QList<int >();
        const bool isNested = ! nestedOps.isEmpty();
        const bool fits = operandColumn
            + textWidth(sourceText(m_document, first, last)) + 1 + after
            <= wrapColumn(m_document, m_document->symbols().at(first).from);
        bool hasForced = false;
        for (int n = 0; n < nestedOps.size() && ! hasForced; ++n) {
            hasForced = forced.contains(nestedOps.at(n));
        }
        if (isNested && ( ! fits || hasForced)) {
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
                false,
                forced,
                after
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
        const int newline = text.lastIndexOf('\n');
        if (newline >= 0) {
            lineStart = newline + 1;
        }
        endsLine = lineStart != lineBefore;
        ++onLine;
    }
    // A group that stayed on one line closes there, as it moved nothing.
    if (isCondition || (isComplex && text.contains('\n'))) {
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
    const int offset = m_document->symbols().at(first).from;
    if (column + textWidth(text) <= wrapColumn(m_document, offset)) {
        return text;
    }

    // Keeps a plain operand's comparison whole while it fits `wrapMax`, else
    // breaks it before its `==` or such, one step past the operand's line.
    if ( ! findTernary(m_document, inner, innerLast, &question, &colon)) {
        if (isParenthesized) {
            return text;
        }
        const QList<int > comparisons = loosestOperators(
            m_document, first, last, &conditionRank, 3, Q_NULLPTR
        );
        if (comparisons.size() == 1
            && conditionRank(m_document->token(comparisons.first())) <= 4
            && continuationIndent(indent) + textWidth(sourceText(
                m_document, comparisons.first(), last
            )) > m_document->configAt(offset).wrapMax
        ) {
            const int comparison = comparisons.first();
            const QByteArray head =
                sourceText(m_document, first, comparison) + ' ';
            FormatExpressionLayout value(m_document, m_conditions, indent);
            const QByteArray right = value.layout(
                comparison + 1, last, column + textWidth(head), 0
            );
            const int split = right.indexOf('\n');
            if (split >= 0
                && column + textWidth(head) + textWidth(right.left(split))
                    <= m_document->configAt(offset).wrapMax
            ) {
                *isComplex = true;
                return head + right;
            }
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

    // Tries the conditional's breaks in their shared order; a parenthesized
    // operand then closes on a line of its own.
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
        wrapMaxAt(m_document, offset)
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
    switch (token) {
    case OROR:
        return 0;
    case ANDAND:
        return 1;
    case EQ:
        return 2;
    default:
        return -1;
    }
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
    const int comparisonStep = startsStatement ? 2 * step : step;

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
        if (operators.size() == 1
            && (reaches || comparisonLines(
                m_document,
                first,
                last,
                column,
                indent,
                comparisonStep
            ).size() < 2)
        ) {
            break;
        }
        operators = loosestOperators(
            m_document, first, last, &headRank, found + 1, &found
        );
    }
    if (operators.isEmpty()) {
        return comparisonLines(
            m_document,
            first,
            last,
            column,
            indent,
            comparisonStep
        );
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
