// Copyright (C) 2015 The XD Company Ltd.
// SPDX-License-Identifier: Apache-2.0

#include "./expression-layout.h"

#include "../format-document.h"
#include "./code-helpers.h"
#include "./layout-helpers.h"
#include "./scope-conditions.h"

/*!
    @class FormatExpressionLayout
    @internal

    Lays out an expression that does not fit its line, the one way the wrap
    rules share (see FormatOperatorWrapRule and FormatArgsWrapRule):

    * A chain of binary operators (`<<`, `+`, `&&`, and the rest that
      operatorPrecedence() ranks) breaks before the operators of its loosest
      level only, each continuation line one step (4 spaces) past the line
      that opened the chain; its leading operators stay on its first line
      while `[Structure] wrapChainAt` does not hold where they start. A
      tighter level inside an operand breaks only when that operand still
      does not fit, one step further in.
    * A chain of one operator whose right operand is a parenthesized group
      stays on its line up to the group's `(`; the group's own chain breaks
      inside it, and its `)` closes on a line of its own.
    * An argument list (and a brace initializer list) keeps its leading
      items, and the head of a last item that is a chain, on the line of its
      opening when they fit; that chain then goes on below, and the closing
      bracket gets a line of its own. Otherwise the list breaks after its
      opening, its items one step in, and the closing bracket goes back to
      the line's indent.
    * Lines that the user grouped stay grouped: items, or operands, that
      shared a line keep sharing one when that line fits. A list or chain
      the user wrote on one line gets an item, or an operand, per line.

    Each nesting level adds one step to the padding of the lines it breaks;
    m_paddingLevel counts those steps past m_indent, the indent of the line
    the expression starts on.
*/

/*!
    Builds a layout of expressions of @p document whose first line has
    @p indent columns of indent; @p conditions decides, through
    `[Structure] wrapArgsAt`, which argument lists may break.
*/
FormatExpressionLayout::FormatExpressionLayout(const FormatDocument *document,
                                               FormatScopeConditions *conditions,
                                               int indent)
    : m_document(document), m_conditions(conditions)
    , m_indent(indent), m_paddingLevel(0)
    , m_wrap(wrapColumn(document)), m_wrapMax(qMax(1, document->settings().wrapMax))
    , m_brokeUngrouped(false)
{
}

/*!
    Builds the text of the tokens @p first up to @p last, written from the
    column @p column, with @p trailing more columns after it on its last
    line: one line when that fits, else broken as the class describes. A
    part with a comment, a `?`, or a comma outside of its brackets stays
    on one line, unbroken.
*/
QByteArray FormatExpressionLayout::layout(
        int first, int last, int column, int trailing)
{
    const QByteArray text = this->oneLine(first, last);
    if (m_document->hasCommentBetween(
            m_document->symbols().at(first).from,
            m_document->symbolEnd(last)
        )
    ) {
        return text;
    }

    // Keeps the line groups of a chain the user broke over lines, even when
    // it would fit one line.
    const QList<int > operators = this->chainOperators(first, last);
    const bool isBroken = m_document->symbolLine(first)
        != m_document->symbolEndLine(last);
    if (this->fits(column, text, trailing)
        && (operators.isEmpty()
            || ! isBroken)
    ) {
        return text;
    }
    if ( ! operators.isEmpty()) {
        return this->layoutChain(first, last, operators, column, trailing);
    }
    if (m_document->token(first) == LPAREN
        && m_document->match(first) == last
    ) {
        return this->layoutGroup(first, last, column, trailing);
    }
    const Token closer = m_document->token(last);
    const int open = m_document->match(last);
    if ((closer == RPAREN
        || closer == RBRACE)
        && open > first
        && this->isListOpen(open)
    ) {
        const QList<QPair<int, int > > items = splitArguments(
            m_document,
            open,
            last
        );
        const bool endsInChain = ! items.isEmpty()
            && this->hasChain(items.last().first, items.last().second);
        if (endsInChain || this->mayBreakList(open, items.size())) {
            return this->layoutList(first, open, last, column);
        }
    }
    return text;
}

/*!
    Builds the text of the list whose head runs from the token @p first up
    to its opening bracket at @p open, and whose closing bracket is at
    @p close, written from the column @p column, broken as the class
    describes, whether or not it fits one line.
*/
QByteArray FormatExpressionLayout::layoutList(
        int first, int open, int close, int column)
{
    const QList<QPair<int, int > > items = splitArguments(
        m_document,
        open,
        close
    );
    const bool isBrace = m_document->token(open) == LBRACE;
    QByteArray head = open > first
        ? this->oneLine(first, open - 1) : QByteArray();
    if (isBrace && ! head.isEmpty() && ! head.endsWith(' ')) {
        head += ' ';
    }
    head += m_document->lexem(open);
    const QByteArray closer = m_document->lexem(close);
    if (items.isEmpty()) {
        return head + closer;
    }

    // Keeps the leading items, and the head of a last item that is a chain,
    // on the line of the opening when they fit.
    const QPair<int, int > & lastItem = items.last();
    const QList<int > lastOperators = this->chainOperators(
        lastItem.first,
        lastItem.second
    );
    if ( ! lastOperators.isEmpty()) {
        QByteArray lead = head;
        bool isPlain = true;
        for (int i = 0; i + 1 < items.size(); ++i) {
            const QPair<int, int > & item = items.at(i);
            isPlain = isPlain
                && m_document->symbolLine(item.first) == m_document->symbolEndLine(item.second);
            lead += this->oneLine(item.first, item.second) + ", ";
        }
        const int chainColumn = column + textWidth(lead);
        lead += this->oneLine(lastItem.first, lastOperators.first() - 1);
        if (isPlain && this->fits(column, lead, 0)) {
            return lead + this->layoutChainRest(
                lastItem.first, lastItem.second, lastOperators,
                chainColumn, 0, true
            )
                + "\n" + this->pad() + closer;
        }
    }

    // Carries on the same way into a last item that is a list ending in such
    // a chain, whose closing bracket then comes right before this one.
    const int lastOpen = m_document->match(lastItem.second);
    const Token lastCloser = m_document->token(lastItem.second);
    if (lastOperators.isEmpty() && (lastCloser == RPAREN || lastCloser == RBRACE)
        && lastOpen > lastItem.first && this->isListOpen(lastOpen)
        && this->hasOuterChain(lastItem.first, lastItem.second)
    ) {
        QByteArray lead = head;
        bool isPlain = true;
        for (int i = 0; i + 1 < items.size(); ++i) {
            const QPair<int, int > & item = items.at(i);
            isPlain = isPlain
                && m_document->symbolLine(item.first) == m_document->symbolEndLine(item.second);
            lead += this->oneLine(item.first, item.second) + ", ";
        }
        if (isPlain) {
            const QByteArray inner = this->layoutList(
                lastItem.first, lastOpen, lastItem.second,
                column + textWidth(lead)
            );
            const int firstBreak = inner.indexOf('\n');
            if (firstBreak > 0 && this->fits(
                    column,
                    lead + inner.left(firstBreak),
                    0
                )
                && inner.endsWith("\n" + this->pad() + m_document->lexem(lastItem.second))
            ) {
                return lead + inner + closer;
            }
        }
    }

    // Breaks after the opening, keeping the user's line groups when every
    // item sits on one line, and the user broke the list at all.
    bool isBroken = m_document->symbolLine(items.first().first)
        != m_document->symbolLine(open);
    bool isWhole = true;
    for (int i = 0; i < items.size(); ++i) {
        const QPair<int, int > & item = items.at(i);
        isWhole = isWhole
            && m_document->symbolLine(item.first) == m_document->symbolEndLine(item.second);
        isBroken = isBroken
            || m_document->symbolLine(item.first) != m_document->symbolLine(items.first().first);
    }
    return head + this->listLines(open, close, items, isBroken && isWhole)
        + "\n" + this->pad() + closer;
}

/*!
    Builds the text of the tokens @p first up to @p last on one line: the
    spaces between tokens as the file has them, a line break turned into one
    space (or none, inside the brackets and before a comma).
*/
QByteArray FormatExpressionLayout::oneLine(int first, int last) const
{
    QByteArray text;
    for (int i = first; i <= last; ++i) {
        const int from = m_document->symbols().at(i).from;
        if (i > first) {
            const int gapFrom = m_document->symbolEnd(i - 1);
            QByteArray gap = m_document->text().mid(gapFrom, from
                - gapFrom
            );
            if (gap.contains('\n')) {
                const Token before = m_document->token(i - 1);
                const Token after = m_document->token(i);
                const bool isTight = before == LPAREN || before == LBRACK || after == RPAREN
                    || after == RBRACK || after == COMMA;
                gap = isTight ? QByteArray() : QByteArray(" ");
            }
            text += gap;
        }
        text += m_document->text().mid(from, m_document->symbolEnd(i)
            - from
        );
    }
    return text;
}

/*!
    Tells whether the tokens @p first up to @p last hold a binary chain
    operator at any depth, an operator that operatorPrecedence() ranks.
*/
bool FormatExpressionLayout::hasChain(int first, int last) const
{
    for (int i = first + 1; i <= last; ++i) {
        if (operatorPrecedence(m_document->token(i)) > 0
            && isOperandEnd(m_document->token(i - 1))
        ) {
            return true;
        }
    }
    return false;
}

/*!
    Tells whether the tokens @p first up to @p last hold a chain that this
    class lays out at their top: binary operators outside of every bracket,
    or a call or brace list whose last item holds such a chain in turn.
*/
bool FormatExpressionLayout::hasOuterChain(int first, int last) const
{
    if ( ! this->chainOperators(first, last).isEmpty()) {
        return true;
    }
    const Token closer = m_document->token(last);
    const int open = m_document->match(last);
    if ((closer != RPAREN
        && closer != RBRACE)
        || open <= first
        || ! this->isListOpen(open)
    ) {
        return false;
    }
    const QList<QPair<int, int > > items = splitArguments(
        m_document,
        open,
        last
    );
    return ! items.isEmpty()
        && this->hasOuterChain(items.last().first, items.last().second);
}

/*!
    Lists the binary operators of the loosest level among the tokens
    @p first up to @p last, outside of every bracket and template argument
    list; none when a `?`, a comma, or a `;` sits at that depth, since a
    break there reads as a broken operand.
*/
QList<int > FormatExpressionLayout::chainOperators(int first, int last) const
{
    QList<int > operators;
    int loosest = 0;
    int angles = 0;
    for (int i = first; i <= last; ++i) {
        const Token token = m_document->token(i);
        const int match = m_document->match(i);
        if ((token == LPAREN
            || token == LBRACK
            || token == LBRACE)
            && match > i
        ) {
            i = match;
            continue;
        }
        if (token == QUESTION || token == COMMA || token == SEMIC) {
            return QList<int >();
        }
        if (token == LANGLE
            && i > first
            && m_document->token(i - 1) == IDENTIFIER
        ) {
            ++angles;
            continue;
        }
        if (angles > 0 && (token == RANGLE || token == GTGT)) {
            angles -= token == GTGT ? 2 : 1;
            continue;
        }
        const int rank = operatorPrecedence(token);
        if (rank == 0
            || i == first
            || ! isOperandEnd(m_document->token(i - 1))
        ) {
            continue;
        }
        if (loosest == 0 || rank < loosest) {
            loosest = rank;
            operators.clear();
        }
        if (rank == loosest) {
            operators << i;
        }
    }
    return operators;
}

/*!
    Builds the text of the chain from the token @p first up to @p last,
    whose loosest operators are @p operators, written from the column
    @p column with @p trailing more columns after it, when it does not fit
    one line (see the class).
*/
QByteArray FormatExpressionLayout::layoutChain(
        int first, int last,
        const QList<int > & operators, int column,
        int trailing)
{
    // Keeps a chain of one operator on its line up to the `(` of its right
    // operand, when that operand is a parenthesized group, and the user did
    // not break the chain.
    const int right = operators.first() + 1;
    const bool isBroken = m_document->symbolLine(first)
        != m_document->symbolEndLine(last);
    if (operators.size() == 1 && ! isBroken
        && m_document->token(right) == LPAREN
        && m_document->match(right) == last
    ) {
        const QByteArray head = this->oneLine(first, operators.first() - 1) + ' '
            + m_document->lexem(operators.first()) + ' ';
        const QList<int > inner = this->chainOperators(right + 1, last - 1);
        const QByteArray groupHead = inner.isEmpty()
            ? this->oneLine(right + 1, last - 1)
            : this->oneLine(right + 1, inner.first() - 1);
        if (this->fits(column, head + '(' + groupHead, 0)) {
            return head
                + this->layoutGroup(
                    right,
                    last,
                    column + textWidth(head),
                    trailing
                );
        }
    }

    // Breaks before the operators, the first operand on this line.
    const QByteArray lead = this->layout(
        first,
        operators.first() - 1,
        column,
        0
    );
    return lead
        + this->layoutChainRest(
            first, last, operators, column, trailing,
            true
        );
}

/*!
    Builds the continuation lines of the chain from the token @p first up to
    @p last, whose loosest operators are @p operators: one line per operator,
    or per group of operators that shared a line and still fit, one padding
    step past the chain's line; the operators the user left on the chain's
    first line stay there, and a group operand that follows the `)` of
    another one joins that line (`) << (`). @p column and @p trailing frame
    the chain as for layoutChain(). With @p keepsLeading set, the leading
    operators of a chain written on one line stay on its first line while
    `[Structure] wrapChainAt` does not hold where they start; the body of
    a broken parenthesized group leaves it unset, hence it breaks before
    each of its operators.
*/
QByteArray FormatExpressionLayout::layoutChainRest(
        int first, int last,
        const QList<int > & operators,
        int column, int trailing, bool keepsLeading)
{
    const bool isBroken = m_document->symbolLine(first)
        != m_document->symbolEndLine(last);
    const QList<QList<int > > groups = isBroken
        ? this->lineGroups(operators)
        : QList<QList<int > >();
    ++m_paddingLevel;
    QByteArray text;
    bool afterGroup = false;
    int at = 0;

    // Keeps the leading operators of a chain written on one line on its
    // first line, while `[Structure] wrapChainAt` does not hold where they
    // start and that line fits.
    if (keepsLeading && ! isBroken) {
        int end = 0;
        while (end < operators.size()) {
            const int start = column
                + textWidth(this->oneLine(first, operators.at(end) - 1)) + 1;
            const qreal percent = start * 100.0 / m_wrapMax;
            const int operandLast = end + 1 < operators.size()
                ? operators.at(end + 1) - 1 : last;
            if (m_conditions->holds(
                    m_document->settings().wrapChainAt, operators.at(end),
                    percent, FormatSettings::defaultWrapChainAt()
                )
                || ! this->fits(
                    column, this->oneLine(first, operandLast),
                    end + 1 < operators.size() ? 0 : trailing
                )
            ) {
                break;
            }
            ++end;
        }
        if (end > 0) {
            const int lastOfLine = end < operators.size()
                ? operators.at(end) - 1 : last;
            text += ' ' + this->oneLine(operators.first(), lastOfLine);
            at = end;
        }
    }

    // Keeps the operators the user left on the chain's first line there,
    // while that line fits.
    if (isBroken) {
        const int firstLine = m_document->symbolLine(first);
        int end = at;
        while (end < operators.size()
            && m_document->symbolLine(operators.at(end)) == firstLine
        ) {
            ++end;
        }
        if (end > 0) {
            const int lastOfLine = end < operators.size()
                ? operators.at(end) - 1 : last;
            const QByteArray more = ' '
                + this->oneLine(operators.first(), lastOfLine);
            if (this->fits(column, this->oneLine(first, operators.first()
                    - 1
                ) + more,
                           end < operators.size() ? 0 : trailing)
            ) {
                text += more;
                at = end;
            }
        }
    }
    while (at < operators.size()) {
        // Takes the operators of the user's line group, or this one alone.
        QList<int > group;
        for (int g = 0; g < groups.size() && group.isEmpty(); ++g) {
            if (groups.at(g).first() == operators.at(at)) {
                group = groups.at(g);
            }
        }
        if (group.isEmpty()) {
            group << operators.at(at);
        }
        const int groupEnd = at + group.size();
        const int groupLast = groupEnd < operators.size()
            ? operators.at(groupEnd) - 1 : last;
        const int tail = groupEnd < operators.size() ? 0 : trailing;

        // Writes a group that fits whole on one line.
        if (group.size() > 1) {
            const QByteArray line = this->oneLine(group.first(), groupLast);
            if (this->fits(textWidth(this->pad()), line, tail)) {
                text += "\n" + this->pad() + line;
                at = groupEnd;
                afterGroup = false;
                continue;
            }
        }

        // Writes each operand of the group on a line of its own.
        for (int k = at; k < groupEnd; ++k) {
            const int op = operators.at(k);
            const int operandLast = k + 1 < operators.size()
                ? operators.at(k + 1) - 1 : last;
            const QByteArray lead = m_document->lexem(op) + ' ';
            const bool isGroup = m_document->token(op + 1) == LPAREN
                && m_document->match(op + 1) == operandLast;
            if (afterGroup && isGroup) {
                const int lineStart = text.lastIndexOf('\n') + 1;
                const int joined = textWidth(text.mid(lineStart)) + 1;
                const QByteArray operand = this->layout(
                    op + 1, operandLast,
                    joined + textWidth(lead),
                    k + 1 < operators.size() ? 0 : trailing
                );
                text += ' ' + lead + operand;
            } else {
                const QByteArray operand = this->layout(
                    op + 1,
                    operandLast,
                    textWidth(this->pad()) + textWidth(lead),
                    k + 1 < operators.size() ? 0 : trailing
                );
                text += "\n" + this->pad() + lead + operand;
            }
            const QByteArray last = text.mid(text.lastIndexOf('\n') + 1).trimmed();
            afterGroup = isGroup && last == ")";
        }
        at = groupEnd;
    }
    --m_paddingLevel;
    if ( ! isBroken) {
        m_brokeUngrouped = true;
    }
    return text;
}

/*!
    Builds the text of the parenthesized group whose `(` is at @p open and
    whose `)` is at @p close, written from the column @p column with
    @p trailing more columns after it: one line when it fits, else its chain
    broken inside it, the first operand right after the `(`, and the `)` on
    a line of its own at the current padding.
*/
QByteArray FormatExpressionLayout::layoutGroup(
        int open, int close, int column, int trailing)
{
    const QByteArray text = this->oneLine(open, close);
    if (this->fits(column, text, trailing) || close <= open + 1) {
        return text;
    }
    const QList<int > operators = this->chainOperators(open + 1, close - 1);
    if (operators.isEmpty()) {
        return '('
            + this->layout(open + 1, close - 1, column + 1, trailing + 1)
            + ')';
    }
    const QByteArray lead = this->layout(
        open + 1,
        operators.first() - 1,
        column + 1,
        0
    );
    return '(' + lead + this->layoutChainRest(
        open + 1,
        close - 1,
        operators,
        column + 1,
        0,
        false
    )
        + "\n" + this->pad() + ')';
}

/*!
    Builds the item lines of the list whose brackets are at @p open and
    @p close and whose items are @p items: the items that shared a line keep
    sharing one, one padding step in, when @p keepsGroups is set and that
    line fits; any other item gets a line of its own. Each line but the last
    ends with its comma.
*/
QByteArray FormatExpressionLayout::listLines(
        int open, int close,
        const QList<QPair<int, int > > & items,
        bool keepsGroups)
{
    Q_UNUSED(open);
    Q_UNUSED(close);
    QList<int > starts;
    for (int i = 0; i < items.size(); ++i) {
        starts << items.at(i).first;
    }
    const QList<QList<int > > groups = keepsGroups
        ? this->lineGroups(starts)
        : QList<QList<int > >();
    ++m_paddingLevel;
    QByteArray text;
    int at = 0;
    while (at < items.size()) {
        int size = 1;
        for (int g = 0; g < groups.size(); ++g) {
            if (groups.at(g).first() == items.at(at).first) {
                size = groups.at(g).size();
            }
        }
        const bool isLastGroup = at + size == items.size();

        // Writes a group that fits whole on one line.
        if (size > 1) {
            QByteArray line;
            for (int k = at; k < at + size; ++k) {
                line += (k > at ? ", " : "") + this->oneLine(
                    items.at(k).first,
                    items.at(k).second
                );
            }
            line += isLastGroup ? "" : ",";
            if (this->fits(textWidth(this->pad()), line, 0)) {
                text += "\n" + this->pad() + line;
                at += size;
                continue;
            }
        }

        // Writes each item of the group on a line of its own.
        for (int k = at; k < at + size; ++k) {
            const bool isLast = k + 1 == items.size();
            text += "\n" + this->pad()
                + this->itemText(
                    items.at(k).first, items.at(k).second,
                    textWidth(this->pad()), isLast ? 0 : 1
                )
                + (isLast ? "" : ",");
        }
        at += size;
    }
    --m_paddingLevel;
    if ( ! keepsGroups) {
        m_brokeUngrouped = true;
    }
    return text;
}

/*!
    Builds the text of the list item from the token @p first up to @p last,
    written from the column @p column with @p trailing more columns after
    it: laid out as layout() does, except that an item the user broke over
    lines keeps its line breaks, with its later lines one step past the
    item's padding, when it holds no chain, group, or list, or when each of
    its line breaks sits between two adjacent string literals (such as
    `QString(QLL("..." "..."))`).
*/
QByteArray FormatExpressionLayout::itemText(
        int first, int last, int column, int trailing)
{
    const bool isPlain = this->chainOperators(first, last).isEmpty()
        && m_document->token(last) != RPAREN && m_document->token(last) != RBRACE;
    if ( ! (isPlain || this->breaksOnlyBetweenLiterals(first, last))
        || m_document->symbolLine(first) == m_document->symbolEndLine(last)
    ) {
        return this->layout(first, last, column, trailing);
    }
    const QList<QByteArray > lines = sourceText(m_document, first, last).split('\n');
    QByteArray text = lines.first().trimmed();
    for (int l = 1; l < lines.size(); ++l) {
        text += "\n" + this->pad() + "    " + lines.at(l).trimmed();
    }
    return text;
}

/*!
    Tells whether each line break between the tokens @p first and @p last
    sits between two adjacent string literals (which spell one string).
*/
bool FormatExpressionLayout::breaksOnlyBetweenLiterals(
        int first, int last) const
{
    for (int i = first + 1; i <= last; ++i) {
        const bool isBreak = m_document->symbolEndLine(i - 1)
            != m_document->symbolLine(i);
        if (isBreak
            && (m_document->token(i - 1) != STRING_LITERAL
                || m_document->token(i) != STRING_LITERAL)
        ) {
            return false;
        }
    }
    return true;
}

/*!
    Tells whether the list opened at @p open, of @p itemCount items, may
    break: a brace initializer list always, an argument list of 2 items or
    more when it has more than 3, or when `[Structure] wrapArgsAt` holds
    at the list's opening.
*/
bool FormatExpressionLayout::mayBreakList(int open, int itemCount) const
{
    if (m_document->token(open) == LBRACE) {
        return true;
    }
    if (itemCount < 2) {
        return false;
    }
    if (itemCount > 3) {
        return true;
    }
    const int line = m_document->symbolLine(open);
    const int lineStart = m_document->lineStart(line);
    const QByteArray head = m_document->text().mid(
        lineStart,
        m_document->symbols().at(open).from + 1 - lineStart
    );
    const qreal percent = textWidth(head) * 100.0 / m_wrapMax;
    return m_conditions->holds(
        m_document->settings().wrapArgsAt, open, percent,
        FormatSettings::defaultWrapChainAt()
    );
}

/*!
    Tells whether the bracket at @p open opens a list this class lays out:
    the arguments of a call (a name, a template's `>`, or a `]` before the
    `(`), or a brace initializer list.
*/
bool FormatExpressionLayout::isListOpen(int open) const
{
    const Token before = m_document->token(open - 1);
    if (m_document->token(open) == LPAREN) {
        return before == IDENTIFIER
            || before == RANGLE
            || before == GTGT
            || before == RBRACK;
    }
    return m_document->token(open) == LBRACE
        && m_document->braceScope(open) == FormatDocument::InitScope;
}

/*!
    Builds the groups of the tokens @p starts that share a source line, in
    order: a run of tokens that start on one line forms one group.
*/
QList<QList<int > > FormatExpressionLayout::lineGroups(
        const QList<int > & starts) const
{
    QList<QList<int > > groups;
    int line = -1;
    for (int i = 0; i < starts.size(); ++i) {
        const int current = m_document->symbolLine(starts.at(i));
        if (groups.isEmpty() || current != line) {
            groups << QList<int >();
            line = current;
        }
        groups.last() << starts.at(i);
    }
    return groups;
}

/*!
    Builds the indent of a line at the current padding: m_indent, plus one
    step of 4 spaces per padding level.
*/
QByteArray FormatExpressionLayout::pad() const
{
    return QByteArray(m_indent + 4 * m_paddingLevel, ' ');
}

/*!
    Tells whether @p text, written from the column @p column with
    @p trailing more columns after it, ends within `[Structure] wrapMax`.
*/
bool FormatExpressionLayout::fits(
        int column, const QByteArray & text, int trailing) const
{
    int width = column;
    const int lastBreak = text.lastIndexOf('\n');
    if (lastBreak >= 0) {
        width = 0;
    }
    return width + textWidth(text.mid(lastBreak + 1)) + trailing <= m_wrapMax
        && ! text.contains('\n');
}
