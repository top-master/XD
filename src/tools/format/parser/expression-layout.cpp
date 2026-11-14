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
    * An assignment keeps its left side and assignment operator together
      while they stay before `[Structure] wrapBegin`, and lays out its right
      side separately. A longer left side may break at its own operators.
    * A chain of one operator whose right operand is a parenthesized group
      stays on its line up to the group's `(`; the group's own chain breaks
      inside it, and its `)` closes on a line of its own.
    * An argument list (and a brace initializer list) breaks after its
      opening, every item one step in, and its closing bracket goes back
      to the line's indent.
    * Items already below their list's opening, and chain operands, that
      shared a line keep sharing one when that line fits. A list or chain
      the user wrote on one line gets an item, or an operand, per line.

    Each nesting level adds one step to the padding of the lines it breaks;
    m_paddingLevel counts those steps past m_indent, the indent of the line
    the expression starts on.
*/

/*!
    Ranks a member access tighter than every binary operator (see
    operatorPrecedence()).
*/
static const int MemberAccessRank = 11;

/*!
    Tells whether @p token reaches a member: a `.` or a `->`.
*/
static bool isMemberAccess(Token token)
{
    return token == DOT || token == ARROW;
}

static bool isMathOperator(Token token)
{
    return token == PLUS || token == MINUS || token == STAR
        || token == SLASH || token == PERCENT;
}

/*!
    Tells whether the arithmetic chain from @a first to @a last in
    @a document has more than one operator and a numeric operand. Nested
    brackets and operands of other operators form separate chains.
*/
static bool isNumberMathChain(const FormatDocument *document, int first, int last)
{
    int count = 0;
    bool hasNumber = false;
    for (int i = first + 1; i < last; ++i) {
        const Token token = document->token(i);
        if ((token == LPAREN || token == LBRACK || token == LBRACE)
            && document->match(i) > i
        ) {
            i = document->match(i);
        } else if (token == LANGLE && document->token(i - 1) == IDENTIFIER) {
            const int close = templateClose(document, i, last + 1);
            if (close <= i) {
                return false;
            }
            i = close;
        } else if (isOperandEnd(document->token(i - 1))) {
            if (isMathOperator(token)) {
                ++count;
                int next = i + 1;
                if (document->token(next) == PLUS || document->token(next) == MINUS) {
                    ++next;
                }
                hasNumber |= document->token(next) == INTEGER_LITERAL
                    || document->token(next) == FLOATING_LITERAL;
            } else if (isBinaryOperator(token) || isAssignmentOperator(token)) {
                return false;
            }
        }
    }
    return count > 1 && hasNumber;
}

/*!
    Measures the last line of @p text, in columns.
*/
static int lastLineWidth(const QByteArray & text)
{
    return textWidth(text.mid(text.lastIndexOf('\n') + 1));
}

/*!
    Tells whether the last line of @p text holds only closing brackets, such
    as the `)` of a call broken over lines.
*/
static bool endsWithClosingLine(const QByteArray & text)
{
    const int lineStart = text.lastIndexOf('\n') + 1;
    return lineStart > 0 && isClosingLine(text.mid(lineStart));
}

/*!
    Builds a layout of expressions of @p document whose first line has
    @p indent columns of indent; @p conditions decides, through
    `[Structure] wrapArgsAt`, which argument lists may break.
*/
FormatExpressionLayout::FormatExpressionLayout(const FormatDocument *document, FormatScopeConditions *conditions, int indent)
    : m_document(document), m_conditions(conditions)
    , m_indent(indent), m_paddingLevel(0)
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
        int first,
        int last,
        int column,
        int trailing,
        bool forceMath)
{
    const int offset = m_document->symbols().at(first).from;
    const int wrap = wrapColumn(m_document, offset);
    const int wrapMax = wrapMaxAt(m_document, offset);
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
    if ( ! operators.isEmpty()
        && isMathOperator(m_document->token(operators.first()))
        && column + textWidth(text) + trailing > wrap
        && isNumberMathChain(m_document, first, last)
    ) {
        forceMath = true;
    }
    const bool isBroken = m_document->symbolLine(first)
        != m_document->symbolEndLine(last);
    if (this->fits(column, text, trailing, first)
        && ! (forceMath && ! operators.isEmpty()
            && isMathOperator(m_document->token(operators.first())))
        && (operators.isEmpty() || ! isBroken)
    ) {
        return text;
    }
    int question = -1;
    int colon = -1;
    if (findTernary(m_document, first, last, &question, &colon)
        && question > first
        && colon > question + 1
        && colon < last
    ) {
        const QByteArray head = this->oneLine(first, question - 1);
        const QByteArray middle = this->oneLine(question, colon - 1);
        const QByteArray tail = this->oneLine(colon, last);
        const QList<QByteArray > lines = ternaryLines(
            head,
            middle,
            tail,
            textWidth(this->pad()),
            column,
            trailing,
            wrapMax
        );
        if (lines.size() > 1) {
            QByteArray result = lines.first();
            for (int i = 1; i < lines.size(); ++i) {
                result += '\n' + lines.at(i);
            }
            return result;
        }
    }
    if ( ! operators.isEmpty()
        && isAssignmentOperator(m_document->token(operators.first()))
    ) {
        const int assignment = operators.first();
        const QByteArray sign = m_document->lexem(assignment);
        QByteArray head = this->oneLine(first, assignment);
        if (column + textWidth(head) >= wrap) {
            FormatExpressionLayout left(
                m_document,
                m_conditions,
                textWidth(this->pad())
            );
            // Makes the left side and its ` =` end before the wrap column,
            // not only before `wrapMax`.
            const int headTrailing = sign.size() + 1 + wrapMax - wrap;
            head = left.layout(first, assignment - 1, column, headTrailing)
                + ' '
                + sign;
            m_brokeUngrouped |= left.brokeUngrouped();
        }
        const bool isHeadBroken = head.contains('\n');
        const QByteArray lastLine = head.mid(head.lastIndexOf('\n') + 1);
        const int indent = isHeadBroken
            ? textWidth(lastLine) - textWidth(lastLine.trimmed())
            : textWidth(this->pad());
        const int endColumn = isHeadBroken ? lastLineWidth(head)
                                           : column + textWidth(head);
        FormatExpressionLayout right(m_document, m_conditions, indent);
        const QByteArray value = right.layout(
            assignment + 1,
            last,
            endColumn + 1,
            trailing
        );
        m_brokeUngrouped |= right.brokeUngrouped();

        // A member call keeps its head on the assignment line while that
        // head fits `wrapBegin`.
        const QList<int > valueOperators =
            right.chainOperators(assignment + 1, last);
        const int firstBreak = value.indexOf('\n');
        bool keepsChainedCall = false;
        if (m_document->token(assignment) == EQ
            && firstBreak >= 0
            && ! valueOperators.isEmpty()
            && isMemberAccess(m_document->token(valueOperators.first()))
        ) {
            const int member = valueOperators.first();
            const int previousOpen = m_document->match(member - 1);
            const QByteArray firstLine = value.left(firstBreak);
            const bool followsCall = m_document->token(member - 1) == RPAREN
                && (previousOpen == assignment + 1
                    || (previousOpen > assignment + 1
                        && right.isListOpen(previousOpen)));
            keepsChainedCall = (followsCall || firstLine.contains('('))
                && endColumn + 1 + textWidth(firstLine) <= wrap;
        }

        // Moves the value one step below an `=` that stands at or past
        // `wrapArgsAt`, when it does not fit after the `=` and takes fewer
        // lines below it; an `=` before `wrapArgsAt` keeps its value.
        if ( ! isHeadBroken
            && ! keepsChainedCall
            && (value.contains('\n')
                || ! this->fits(endColumn + 1, value, trailing, first))
            && reachesWrapArgsAt(m_document, m_conditions, assignment)
        ) {
            const QByteArray belowPad = continuationPad(textWidth(this->pad()));
            FormatExpressionLayout below(
                m_document, m_conditions, textWidth(belowPad)
            );
            const QByteArray moved = below.layout(
                assignment + 1,
                last,
                textWidth(belowPad),
                trailing
            );
            if (moved.count('\n') < value.count('\n')
                || ( ! value.contains('\n') && ! moved.contains('\n'))
            ) {
                m_brokeUngrouped |= below.brokeUngrouped();
                return head + "\n" + belowPad + moved;
            }
        }
        return head + ' ' + value;
    }
    if ( ! operators.isEmpty()
        && ! isMemberAccess(m_document->token(operators.first()))
    ) {
        return this->layoutChain(first, last, operators, column, trailing, forceMath);
    }
    if (operators.size() > 1
        && isMemberAccess(m_document->token(operators.first()))
    ) {
        const int callEnd = operators.at(1) - 1;
        const int callOpen = m_document->match(callEnd);
        const int nextEnd = operators.size() > 2 ? operators.at(2) - 1 : last;
        if (m_document->token(callEnd) == RPAREN
            && callOpen > operators.first()
            && this->isListOpen(callOpen)
            && (m_document->token(nextEnd) != RPAREN
                || m_conditions->argsOver(
                    callOpen, splitArguments(m_document, callOpen, callEnd).size()
                ))
            && column + textWidth(this->oneLine(first, callOpen)) <= wrap
        ) {
            const QByteArray call = this->layoutList(first, callOpen, callEnd, column);
            if (call.contains('\n')) {
                return call + this->layoutChainRest(
                    first, last, operators.mid(1), column, trailing, false,
                    this->chainStepsIn(first, callEnd, call), false, call
                );
            }
        }
    }
    // Adjacent literals are safe places to split an otherwise indivisible
    // call head, without opening each enclosing single-argument call.
    if (operators.isEmpty()) {
        int end = last;
        while (end > first
            && (m_document->token(end) == RPAREN
                || m_document->token(end) == RBRACK)
        ) {
            --end;
        }
        if (m_document->token(end) == STRING_LITERAL) {
            QByteArray parts;
            int start = first;
            int atColumn = column;
            const QByteArray continuation =
                continuationPad(textWidth(this->pad()));
            for (int i = first + 1; i <= end; ++i) {
                if (m_document->token(i - 1) != STRING_LITERAL
                    || m_document->token(i) != STRING_LITERAL
                ) {
                    continue;
                }
                const QByteArray rest = this->oneLine(start, last);
                if (atColumn + textWidth(rest) + trailing <= wrapMax) {
                    break;
                }
                parts += this->oneLine(start, i - 1) + '\n' + continuation;
                start = i;
                atColumn = textWidth(continuation);
            }
            if (start != first) {
                return parts + this->oneLine(start, last);
            }
        }
    }
    if (m_document->token(first) == LPAREN
        && m_document->match(first) == last
    ) {
        return this->layoutGroup(first, last, column, trailing);
    }
    const Token closer = m_document->token(last);
    const int open = m_document->match(last);
    if ((closer == RPAREN || closer == RBRACE)
        && open > first
        && this->isListOpen(open)
    ) {
        bool chainedCall = false;
        for (int k = first; k < open; ++k) {
            const int previousOpen = m_document->match(k - 1);
            if (isMemberAccess(m_document->token(k))
                && m_document->token(k - 1) == RPAREN
                && previousOpen > first
                && this->isListOpen(previousOpen)
            ) {
                chainedCall = true;
                break;
            }
        }
        const bool startsStatement = m_document->headStart(first) == first;
        const bool followsAssignment =
            isAssignmentOperator(m_document->token(first - 1));
        const int headLimit = startsStatement ? wrapMax : wrap;
        if (closer == RPAREN
            && ! operators.isEmpty()
            && isMemberAccess(m_document->token(operators.first()))
            && ! chainedCall
            && (startsStatement || followsAssignment
                || (operators.size() == 1 && ! isBroken))
            && column + textWidth(this->oneLine(first, open)) <= headLimit
        ) {
            return this->layoutList(first, open, last, column);
        }
        const QList<QPair<int, int > > items =
            splitArguments(m_document, open, last);
        if (operators.isEmpty() && items.size() == 1
            && column + textWidth(this->oneLine(first, open)) < wrap
        ) {
            const QPair<int, int > item = items.first();
            const QList<int > nestedOperators = this->chainOperators(
                item.first, item.second
            );
            const bool isMemberChain = ! nestedOperators.isEmpty()
                && isMemberAccess(m_document->token(nestedOperators.first()));
            const int nestedOpen = m_document->match(item.second);
            const QList<QPair<int, int > > nestedItems = isMemberChain
                && m_document->token(item.second) == RPAREN
                && nestedOpen > item.first
                ? splitArguments(m_document, nestedOpen, item.second)
                : QList<QPair<int, int > >();
            const bool endsWithChain = nestedItems.size() > 1
                && this->hasChain(nestedItems.last().first, nestedItems.last().second)
                && ! this->hasChain(nestedItems.first().first, nestedItems.first().second);
            if (isMemberChain && ! endsWithChain) {
                const QByteArray head = this->oneLine(first, open);
                const QByteArray nested = this->layout(
                    item.first, item.second, column + textWidth(head), 1 + trailing
                );
                if (nested.contains('\n')) {
                    return head + nested + m_document->lexem(last);
                }
            }
        }
        const bool endsInChain = ! items.isEmpty() && this->hasChain(
            items.last().first,
            items.last().second
        );
        const bool chainMayBreak = endsInChain
            && (m_document->symbolLine(open) == m_document->symbolLine(last)
                || m_document->symbolLine(items.first().first) > m_document
                    ->symbolLine(open))
            && (m_document->configAt(offset).wrapArgsOverCondition.isEmpty()
                || m_conditions->argsOver(open, items.size()));
        if (chainMayBreak
            || (this->mayBreakList(open, items.size())
                && (operators.isEmpty() || items.size() > 1)
            )
        ) {
            const QByteArray list = this->layoutList(
                first,
                open,
                last,
                column
            );
            if (operators.isEmpty() || list.contains('\n')) {
                return list;
            }
        }
        // A short call head may keep its arguments on the opening line,
        // while an adjacent-literal expression in its last argument still
        // needs to break. Lay out that argument at its actual column.
        if (operators.isEmpty() && ! items.isEmpty()) {
            const QPair<int, int > item = items.last();
            bool adjacentLiterals = false;
            for (int i = item.first + 1; i <= item.second; ++i) {
                adjacentLiterals |= m_document->token(i - 1) == STRING_LITERAL
                    && m_document->token(i) == STRING_LITERAL;
            }
            if (adjacentLiterals) {
                QByteArray head = this->oneLine(first, item.first - 1);
                if (m_document->token(item.first - 1) == COMMA) {
                    head += ' ';
                }
                ++m_paddingLevel;
                const QByteArray itemText = this->layout(
                    item.first,
                    item.second,
                    column + textWidth(head),
                    trailing + 1
                );
                --m_paddingLevel;
                if (itemText.contains('\n')) {
                    return head + itemText + m_document->lexem(last);
                }
            }
        }
    }
    if ( ! operators.isEmpty()) {
        return this->layoutChain(first, last, operators, column, trailing, forceMath);
    }
    return text;
}

/*!
    Builds the text of a list, broken as the class describes even when it
    fits one line: its head runs from the token @p first up to its opening
    bracket at @p open, and its closing bracket is at @p close.
*/
QByteArray FormatExpressionLayout::layoutList(
        int first,
        int open,
        int close,
        int column)
{
    const int wrap = wrapColumn(m_document, m_document->symbols().at(first).from);
    const QList<QPair<int, int > > items =
        splitArguments(m_document, open, close);
    const bool isBrace = m_document->token(open) == LBRACE;
    QByteArray head = open > first ? this->oneLine(first, open - 1)
                                   : QByteArray();
    if (isBrace && ! head.isEmpty() && ! head.endsWith(' ')) {
        head += ' ';
    }
    head += m_document->lexem(open);
    const QByteArray closer = m_document->lexem(close);
    if (items.isEmpty()) {
        return head + closer;
    }

    int question = -1;
    int colon = -1;
    const QPair<int, int > firstItem = items.first();
    if ( ! isBrace && ! m_conditions->argsOver(
            open,
            items.size()
        ) && findTernary(
            m_document,
            firstItem.first,
            firstItem.second,
            &question,
            &colon
        )
        && question > firstItem.first && column + textWidth(head) + textWidth(
            this->oneLine(
                firstItem.first,
                question - 1
            )
        ) <= wrap
    ) {
        if (items.size() > 1) {
            ++m_paddingLevel;
        }
        const QByteArray item = this->layout(
            firstItem.first,
            firstItem.second,
            column + textWidth(head),
            items.size() == 1 ? 0 : 1
        );
        if (items.size() > 1) {
            --m_paddingLevel;
        }
        if (item.contains('\n')
            || (items.size() > 1
                && this->fits(column + textWidth(head), item, 1, first))
        ) {
            QByteArray text = head + item;
            if (items.size() > 1) {
                text += ',' + this->listLines(
                    open,
                    close,
                    items.mid(1),
                    false,
                    1
                );
            }
            return text + '\n' + this->pad() + closer;
        }
    }

    // Breaks after the opening, keeping the user's line groups when every
    // item sits on one line, and the first item starts below the opening.
    const bool isBroken = m_document->symbolLine(items.first().first)
        != m_document->symbolLine(open);
    bool isWhole = true;
    for (int i = 0; i < items.size(); ++i) {
        const QPair<int, int > & item = items.at(i);
        isWhole = isWhole
            && m_document->symbolLine(item.first) == m_document->symbolEndLine(
                item.second
            );
    }
    return head + this->listLines(
        open,
        close,
        items,
        isBroken && isWhole,
        0
    )
        + "\n"
        + this->pad()
        + closer;
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
            QByteArray gap = m_document->text().mid(
                gapFrom,
                from - gapFrom
            );
            const Token before = m_document->token(i - 1);
            const Token after = m_document->token(i);
            if ((isMemberAccess(before) || isMemberAccess(after))
                && gap.trimmed().isEmpty()
            ) {
                gap.clear();
            } else if (gap.contains('\n')) {
                const bool isTight = before == LPAREN
                    || before == LBRACK
                    || after == RPAREN
                    || after == RBRACK
                    || after == COMMA;
                gap = isTight ? QByteArray() : QByteArray(" ");
            }
            text += gap;
        }
        text += m_document->text().mid(
            from,
            m_document->symbolEnd(i) - from
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
    if ((closer != RPAREN && closer != RBRACE)
        || open <= first
        || ! this->isListOpen(open)
    ) {
        return false;
    }
    const QList<QPair<int, int > > items =
        splitArguments(m_document, open, last);
    return ! items.isEmpty() && this->hasOuterChain(
        items.last().first,
        items.last().second
    );
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
        if ((token == LPAREN || token == LBRACK || token == LBRACE)
            && match > i
        ) {
            i = match;
            continue;
        }
        // Skips a comma of template arguments, which belongs to a type.
        if (token == COMMA && angles > 0) {
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
        if (angles == 0
            && i > first
            && i < last
            && isAssignmentOperator(token)
            && isOperandEnd(m_document->token(i - 1))
        ) {
            operators.clear();
            operators << i;
            return operators;
        }
        const int rank = isMemberAccess(token) ? MemberAccessRank
                                               : operatorPrecedence(token);
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
        int first,
        int last,
        const QList<int > & operators,
        int column,
        int trailing,
        bool forceMath)
{
    const int offset = m_document->symbols().at(first).from;
    const int wrap = wrapColumn(m_document, offset);
    const int wrapMax = wrapMaxAt(m_document, offset);
    // Lay out a parenthesized binary chain before its following member chain.
    const int firstMember = operators.first();
    const int groupClose = firstMember - 1;
    if (isMemberAccess(m_document->token(firstMember))
        && m_document->token(first) == LPAREN
        && m_document->match(first) == groupClose
    ) {
        const QList<int > inner =
            this->chainOperators(first + 1, groupClose - 1);
        const QByteArray suffix = this->oneLine(firstMember, last);
        if ( ! inner.isEmpty()
            && operatorPrecedence(m_document->token(inner.first())) > 0
        ) {
            ++m_paddingLevel;
            const QByteArray group = this->layoutGroup(
                first,
                groupClose,
                column,
                textWidth(suffix) + trailing
            );
            --m_paddingLevel;
            if (group.contains('\n')) {
                return group + this->layoutChainRest(
                    first,
                    last,
                    operators,
                    column,
                    trailing,
                    false,
                    true,
                    false,
                    group
                );
            }
        }
    }

    // Keep an early first operator with the call it introduces. Its argument
    // list may break even when the usual argument thresholds do not ask for
    // it, provided the call head itself fits on the line.
    const int firstOperator = operators.first();
    const int firstOperandLast = operators.size() > 1 ? operators.at(1) - 1
                                                      : last;
    const int callOpen = m_document->match(firstOperandLast);
    if ( ! forceMath && ! isMemberAccess(m_document->token(firstOperator))
        && m_document->symbolLine(firstOperator)
            == m_document->symbolLine(first)
        && column + textWidth(this->oneLine(first, firstOperator - 1)) + 1
            < wrap
        && m_document->token(firstOperandLast) == RPAREN
        && callOpen > firstOperator + 1
        && this->isListOpen(callOpen)
        && callOpen + 1 < firstOperandLast
        && column + textWidth(this->oneLine(first, callOpen)) <= wrapMax
        && ! this->fits(
            column,
            this->oneLine(first, firstOperandLast),
            operators.size() == 1 ? trailing : 0,
            first
        )
    ) {
        const QByteArray head = this->layoutList(
            first,
            callOpen,
            firstOperandLast,
            column
        );
        if (operators.size() == 1) {
            return head;
        }
        return head + this->layoutChainRest(
            first,
            last,
            operators.mid(1),
            column,
            trailing,
            false,
            this->chainStepsIn(first, firstOperandLast, head),
            false,
            head
        );
    }

    // Keeps a chain of one operator on its line up to the `(` of its right
    // operand, when that operand is a parenthesized group, and the user did
    // not break the chain.
    const int right = operators.first() + 1;
    const bool isBroken = m_document->symbolLine(first)
        != m_document->symbolEndLine(last);
    if ( ! forceMath && operators.size() == 1
        && ! isBroken
        && m_document->token(right) == LPAREN
        && m_document->match(right) == last
    ) {
        const QByteArray head = this->oneLine(first, operators.first() - 1)
            + ' '
            + m_document->lexem(operators.first())
            + ' ';
        const QList<int > inner = this->chainOperators(right + 1, last - 1);
        const QByteArray groupHead = inner.isEmpty()
            ? this->oneLine(right + 1, last - 1)
            : this->oneLine(right + 1, inner.first() - 1);
        if (this->fits(column, head + '(' + groupHead, 0, first)) {
            return head + this->layoutGroup(
                right,
                last,
                column + textWidth(head),
                trailing
            );
        }
    }

    if ( ! isMemberAccess(m_document->token(firstOperator))
        && ! (forceMath && isMathOperator(m_document->token(firstOperator)))
        && isNumberMathChain(m_document, firstOperator + 1, firstOperandLast)
    ) {
        const QByteArray head = this->oneLine(first, firstOperator) + ' ';
        const int valueColumn = column + textWidth(head);
        if (valueColumn <= wrap) {
            const QByteArray value = this->layout(
                firstOperator + 1, firstOperandLast, valueColumn,
                operators.size() == 1 ? trailing : 0, forceMath
            );
            const int split = value.indexOf('\n');
            if (split >= 0 && valueColumn + textWidth(value.left(split)) <= wrapMax) {
                const QByteArray prefix = head + value;
                if (operators.size() == 1) {
                    return prefix;
                }
                return prefix + this->layoutChainRest(
                    first, last, operators.mid(1), column, trailing, false,
                    this->chainStepsIn(first, firstOperandLast, prefix), false,
                    prefix, forceMath
                );
            }
        }
    }

    // Breaks before the operators, the first operand on this line.
    const QByteArray lead = this->layout(
        first,
        operators.first() - 1,
        column,
        0,
        forceMath
    );
    return lead + this->layoutChainRest(
        first,
        last,
        operators,
        column,
        trailing,
        ! (forceMath && isMathOperator(m_document->token(operators.first()))),
        this->chainStepsIn(first, operators.first() - 1, lead),
        true,
        lead,
        forceMath
    );
}

/*!
    Builds the continuation lines of the chain from the token @p first up to
    @p last, whose loosest operators are @p operators: one line per operator,
    or per group of operators that shared a line and still fit, one padding
    step past the chain's line; the operators the user left on the chain's
    first line stay there. An operator joins a line of only closing
    brackets when its operand fits there; `&&` and `||` keep their own
    lines. Subsequent member accesses follow `[Structure] wrapSubChain`.
    @p column and @p trailing frame
    the chain as for layoutChain(). With @p keepsLeading set, the leading
    operators of a chain written on one line stay on its first line while
    `[Structure] wrapChainAt` does not hold where they start; the body of
    a broken parenthesized group leaves it unset, hence it breaks before
    each of its operators.

    With @p stepsIn set, the operator lines go one padding step past the
    current one; else they stay at it (see chainStepsIn()). @p head is the
    text before the first operator. With @p byLine
    set, the operands kept on the chain's first line decide it instead, when
    there are any: the step then follows how that line ends. With
    @p forceMath set, a chained arithmetic operand breaks at each of its
    arithmetic operators.
*/
QByteArray FormatExpressionLayout::layoutChainRest(
        int first,
        int last,
        const QList<int > & operators,
        int column,
        int trailing,
        bool keepsLeading,
        bool stepsIn,
        bool byLine,
        const QByteArray & head,
        bool forceMath)
{
    const int offset = m_document->symbols().at(first).from;
    const int wrapMax = wrapMaxAt(m_document, offset);
    const bool isBroken = m_document->symbolLine(first)
        != m_document->symbolEndLine(last);
    const bool forceHere = forceMath
        && isMathOperator(m_document->token(operators.first()));
    const QList<QList<int > > groups = isBroken && ! forceHere ? this->lineGroups(operators)
                                                : QList<QList<int > >();
    const QByteArray space =
        isMemberAccess(m_document->token(operators.first()))
        ? QByteArray() : QByteArray(" ");
    QByteArray text;
    bool afterClosingLine = endsWithClosingLine(head);
    bool subChain = false;
    const QString subChainMode = m_document->configAt(offset).wrapSubChain;
    int at = 0;

    // Keeps the leading operators of a chain written on one line on its
    // first line, while `[Structure] wrapChainAt` does not hold where they
    // start and that line fits.
    if (keepsLeading && ! isBroken && ! afterClosingLine) {
        int end = 0;
        while (end < operators.size()) {
            const int start = column
                + space.size()
                + textWidth(this->oneLine(first, operators.at(end) - 1));
            const qreal percent = start * 100.0 / wrapMax;
            const int operandLast = end + 1 < operators.size()
                ? operators.at(end + 1) - 1 : last;
            if (m_conditions->holds(
                    m_document
                        ->configAt(
                            m_document->symbols().at(operators.at(end)).from
                        )
                        .wrapChainAt,
                    operators.at(end),
                    percent,
                    FormatFileConfig::defaultWrapChainAt()
                )
                || ! this->fits(
                    column,
                    this->oneLine(first, operandLast),
                    end + 1 < operators.size() ? 0 : trailing,
                    first
                )
            ) {
                break;
            }
            ++end;
        }
        if (end > 0) {
            const int lastOfLine = end < operators.size()
                ? operators.at(end) - 1 : last;
            text += space + this->oneLine(operators.first(), lastOfLine);
            at = end;
        }
    }

    // Keeps the operators the user left on the chain's first line there,
    // while that line fits.
    if (isBroken && ! afterClosingLine && ! forceHere) {
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
            const QByteArray more =
                space + this->oneLine(operators.first(), lastOfLine);
            if (this->fits(
                    column,
                    this->oneLine(first, operators.first() - 1) + more,
                    end < operators.size() ? 0 : trailing,
                    first
                )
            ) {
                text += more;
                at = end;
            }
        }
    }
    // A head split between adjacent literals can still carry the member
    // call that originally followed it, if that last line has room.
    bool hasAdjacentLiterals = false;
    for (int i = first + 1; i < operators.first(); ++i) {
        hasAdjacentLiterals |= m_document->token(i - 1) == STRING_LITERAL
            && m_document->token(i) == STRING_LITERAL;
    }
    if (at == 0
        && isMemberAccess(m_document->token(operators.first()))
        && hasAdjacentLiterals
        && head.contains('\n')
        && ! afterClosingLine
        && m_document->symbolLine(operators.first())
            == m_document->symbolEndLine(operators.first() - 1)
    ) {
        const int end = operators.size() > 1 ? operators.at(1) - 1 : last;
        const QByteArray member = this->oneLine(operators.first(), end);
        const int tail = operators.size() > 1 ? 0 : trailing;
        if (lastLineWidth(head) + textWidth(member) + tail <= wrapMax) {
            text += member;
            at = 1;
        }
    }
    // Steps in as the first line ends, once it is known (see `chainStepsIn()`).
    bool steps = stepsIn;
    if (byLine && at > 0 && at < operators.size()) {
        steps = this->chainStepsIn(
            first,
            operators.at(at) - 1,
            QByteArray()
        );
    }
    if (steps) {
        ++m_paddingLevel;
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
        if ((afterClosingLine || subChain)
            && isMemberAccess(m_document->token(group.first()))
            && group.size() > 1
        ) {
            group = QList<int >() << group.first();
        }
        const int groupEnd = at + group.size();
        const int groupLast = groupEnd < operators.size()
            ? operators.at(groupEnd) - 1 : last;
        const int tail = groupEnd < operators.size() ? 0 : trailing;

        // Writes a group that fits whole on one line.
        if (group.size() > 1) {
            const QByteArray line = this->oneLine(group.first(), groupLast);
            if (this->fits(textWidth(this->pad()), line, tail, first)) {
                const int width = lastLineWidth(head + text);
                if (afterClosingLine
                    && m_document->token(group.first()) != ANDAND
                    && m_document->token(group.first()) != OROR
                    && width + space.size() + textWidth(line) + tail <= wrapMax
                ) {
                    text += space + line;
                } else {
                    text += "\n" + this->pad() + line;
                }
                at = groupEnd;
                afterClosingLine = false;
                continue;
            }
        }

        // Writes each operand of the group on a line of its own.
        for (int k = at; k < groupEnd; ++k) {
            const int op = operators.at(k);
            const int operandLast = k + 1 < operators.size()
                ? operators.at(k + 1) - 1 : last;
            const QByteArray lead = m_document->lexem(op) + space;
            const int extra = k + 1 < operators.size() ? 0 : trailing;
            bool joined = false;
            const bool member = isMemberAccess(m_document->token(op));
            if ( ! member) {
                subChain = false;
            }
            const int previousOpen = m_document->match(op - 1);
            const bool isCall = member && (op + 2 <= last)
                && m_document->token(op + 2) == LPAREN;
            const bool followsCall = isCall
                && m_document->token(op - 1) == RPAREN
                && previousOpen >= operators.first()
                && this->isListOpen(previousOpen);
            if (((afterClosingLine && ! followsCall) || (subChain && member
                    && subChainMode != QLL("always")))
                && m_document->token(op) != ANDAND
                && m_document->token(op) != OROR
            ) {
                const int operatorColumn =
                    lastLineWidth(head + text) + space.size();
                const QByteArray operand = this->layout(
                    op + 1,
                    operandLast,
                    operatorColumn + textWidth(lead),
                    extra,
                    forceMath
                );
                const int firstBreak = operand.indexOf('\n');
                const int width = textWidth(firstBreak < 0
                    ? operand : operand.left(firstBreak)
                );
                int limit = wrapMax;
                if (subChain && ! afterClosingLine && subChainMode == QLL("wrap-begin")) {
                    limit = wrapColumn(m_document, offset);
                }
                if ((subChain && ! afterClosingLine && subChainMode == QLL("never"))
                    || operatorColumn + textWidth(lead) + width
                        + (firstBreak < 0 ? extra : 0) <= limit
                ) {
                    text += space + lead + operand;
                    joined = true;
                    subChain = member;
                }
            }
            if ( ! joined) {
                const QByteArray operatorPad = subChain && member && ! steps
                    ? continuationPad(textWidth(this->pad())) : this->pad();
                const int operandColumn =
                    textWidth(operatorPad) + textWidth(lead);
                const QByteArray oneLine = this->oneLine(
                    op + 1,
                    operandLast
                );
                QByteArray operand;
                if (member
                    && ! isBroken
                    && operandColumn + textWidth(oneLine) + extra <= wrapMax
                ) {
                    operand = oneLine;
                } else {
                    operand = this->layout(
                        op + 1,
                        operandLast,
                        operandColumn,
                        extra,
                        forceMath
                    );
                }
                text += "\n" + operatorPad + lead + operand;
            }
            afterClosingLine = endsWithClosingLine(text);
        }
        at = groupEnd;
    }
    if (steps) {
        --m_paddingLevel;
    }
    if ( ! isBroken) {
        m_brokeUngrouped = true;
    }
    return text;
}

/*!
    Tells whether the operator lines of a chain go one padding step past the
    current one, where the chain's first operand runs from the token
    @p headFirst to @p headLast, laid out as @p head.

    A chain at the statement's level steps in. Inside a bracket, which adds
    its own step already, the chain is an argument that goes on past its
    line, and takes one more step (see hasMisplacedLines()), unless the line
    closes a bracket opened on an earlier line, ends with a string literal,
    or starts with a string literal. After a line of
    only a closing bracket, it never steps in. @p headLast is the last token
    of that line, and @p headFirst its first one.
*/
bool FormatExpressionLayout::chainStepsIn(
        int headFirst, int headLast, const QByteArray & head) const
{
    if (endsWithClosingLine(head)) {
        return false;
    }
    if (m_paddingLevel == 0) {
        return true;
    }
    const Token last = m_document->token(headLast);
    return ! closesEarlierLine(m_document, headLast)
        && last != STRING_LITERAL
        && ! startsWithStringLiteral(m_document, headFirst);
}

/*!
    Builds the text of the parenthesized group whose `(` is at @p open and
    whose `)` is at @p close, written from the column @p column with
    @p trailing more columns after it: one line when it fits, else its chain
    broken inside it, the first operand right after the `(`, and the `)` on
    a line of its own at the current padding.
*/
QByteArray FormatExpressionLayout::layoutGroup(
        int open,
        int close,
        int column,
        int trailing)
{
    const int wrapMax = wrapMaxAt(
        m_document,
        m_document->symbols().at(open).from
    );
    const QByteArray text = this->oneLine(open, close);
    if (this->fits(column, text, trailing, open) || close <= open + 1) {
        return text;
    }
    const QList<int > operators = this->chainOperators(open + 1, close - 1);
    if (operators.isEmpty()) {
        return '(' + this->layout(
            open + 1,
            close - 1,
            column + 1,
            trailing + 1
        )
            + ')';
    }
    const QByteArray lead = this->layout(
        open + 1,
        operators.first() - 1,
        column + 1,
        0
    );
    const QByteArray group = '('
        + lead
        + this->layoutChainRest(
            open + 1,
            close - 1,
            operators,
            column + 1,
            0,
            false,
            ! endsWithClosingLine(lead),
            false,
            lead
        );
    const Token firstOperator = m_document->token(operators.first());
    const Token afterClose = m_document->token(close + 1);
    if ((firstOperator == ANDAND || firstOperator == OROR)
        && (afterClose == ANDAND
            || afterClose == OROR
            || afterClose == SEMIC)
        && m_document->symbolEndLine(close - 1) == m_document->symbolLine(close)
        && lastLineWidth(group) + 1 + trailing <= wrapMax
    ) {
        return group + ')';
    }
    return group + "\n" + this->pad() + ')';
}

/*!
    Builds the item lines of the list whose brackets are at @p open and
    @p close and whose items are @p items, the first of them being the
    list's item @p firstItem (zero-based): the items that shared a line keep
    sharing one, one padding step in, when @p keepsGroups is set and that
    line fits; any other item gets a line of its own. Each line but the last
    ends with its comma.

    Each line then takes the items after it that `[Structure] joinArgs`
    lets share it (see FormatDocument::joinArgsAt()), asked at the line's
    last item, while the line fits; an item the user grouped with others
    stays with its group.
*/
QByteArray FormatExpressionLayout::listLines(
        int open,
        int close,
        const QList<QPair<int, int > > & items,
        bool keepsGroups,
        int firstItem)
{
    const bool hasTrailingComma = m_document->token(close - 1) == COMMA;
    QList<int > starts;
    for (int i = 0; i < items.size(); ++i) {
        starts << items.at(i).first;
    }
    const QList<QList<int > > groups = keepsGroups ? this->lineGroups(starts)
                                                   : QList<QList<int > >();
    ++m_paddingLevel;
    const int column = textWidth(this->pad());
    QByteArray text;
    int at = 0;
    // The items before this index belong to a group too long for one line,
    // hence each gets a line of its own.
    int singlesUntil = 0;
    while (at < items.size()) {
        int size = 1;
        for (int g = 0; g < groups.size() && at >= singlesUntil; ++g) {
            if (groups.at(g).first() == items.at(at).first) {
                size = groups.at(g).size();
            }
        }
        bool isWrapping = ! keepsGroups || at < singlesUntil;

        // Writes a group that fits whole on one line.
        QByteArray line;
        if (size > 1) {
            for (int k = at; k < at + size; ++k) {
                line += (k > at ? ", " : "") + this->oneLine(
                    items.at(k).first,
                    items.at(k).second
                );
            }
            const bool isLastGroup = at + size == items.size();
            const QByteArray comma = isLastGroup && ! hasTrailingComma
                ? QByteArray() : QByteArray(",");
            if ( ! this->fits(column, line + comma, 0, open)) {
                singlesUntil = at + size;
                size = 1;
                isWrapping = true;
            }
        }

        // Writes an item on a line of its own.
        if (size == 1) {
            const bool isLast = at + 1 == items.size() && ! hasTrailingComma;
            line = this->itemText(
                items.at(at).first,
                items.at(at).second,
                column,
                isLast ? 0 : 1
            );
        }

        // Joins the items that `joinArgs` lets share the line.
        int last = at + size - 1;
        while (last + 1 < items.size() && ! line.contains('\n')) {
            int nextSize = 1;
            for (int g = 0; g < groups.size(); ++g) {
                if (last + 1 >= singlesUntil
                    && groups.at(g).first() == items.at(last + 1).first
                ) {
                    nextSize = groups.at(g).size();
                }
            }
            if (nextSize > 1) {
                break;
            }
            const int allowed = m_document->joinArgsAt(
                open,
                firstItem + last,
                column + textWidth(line),
                isWrapping
            );
            if (last + 2 - at > allowed) {
                break;
            }
            const bool nextIsLast = last + 2 == items.size()
                && ! hasTrailingComma;
            const QByteArray joined = line + ", " + this->oneLine(
                items.at(last + 1).first,
                items.at(last + 1).second
            );
            if ( ! this->fits(column, joined, nextIsLast ? 0 : 1, open)) {
                break;
            }
            line = joined;
            ++last;
        }
        const bool isLast = last + 1 == items.size() && ! hasTrailingComma;
        text += "\n" + this->pad() + line + (isLast ? "" : ",");
        at = last + 1;
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
    lines keeps its line breaks when it holds no chain, group, or list, or
    when each of its line breaks sits between two adjacent string literals
    (such as `QString(QLL("..." "..."))`). Each later line then goes one
    step past the item's padding when adjacent string literals share it, or
    when the line before it ends without a string literal or an earlier
    bracket's close and does not start with a string literal (see
    hasMisplacedLines()).
*/
QByteArray FormatExpressionLayout::itemText(
        int first,
        int last,
        int column,
        int trailing)
{
    int question = -1;
    int colon = -1;
    const bool isPlain = ! findTernary(
        m_document,
        first,
        last,
        &question,
        &colon
    )
        && this->chainOperators(first, last).isEmpty() && m_document->token(last) != RPAREN && m_document->token(last) != RBRACE;
    if ( ! (isPlain || this->breaksOnlyBetweenLiterals(first, last))
        || m_document->symbolLine(first) == m_document->symbolEndLine(last)
    ) {
        return this->layout(first, last, column, trailing);
    }
    // Finds which later lines take the step (see `hasMisplacedLines()`).
    QList<bool > steps;
    int lineFirst = first;
    for (int i = first + 1; i <= last; ++i) {
        if (m_document->symbolEndLine(i - 1) == m_document->symbolLine(i)) {
            continue;
        }
        const Token end = m_document->token(i - 1);
        const bool adjacentStrings =
            end == STRING_LITERAL && startsWithStringLiteral(m_document, i);
        const bool keepsPadding = ! adjacentStrings && (end == STRING_LITERAL
            || closesEarlierLine(m_document, i - 1)
            || startsWithStringLiteral(m_document, lineFirst));
        steps << ! keepsPadding;
        lineFirst = i;
    }
    const QList<QByteArray > lines = sourceText(m_document, first, last)
        .split('\n');
    QByteArray text = lines.first().trimmed();
    for (int l = 1; l < lines.size(); ++l) {
        const bool stepsIn = l - 1 >= steps.size() || steps.at(l - 1);
        text += "\n" + this->pad()
            + (stepsIn ? "    " : "")
            + lines.at(l).trimmed();
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
                || ! startsWithStringLiteral(m_document, i))
        ) {
            return false;
        }
    }
    return true;
}

/*!
    Tells whether the list opened at @p open, of @p itemCount items, may
    break: a brace initializer list always, an argument list when it has
    more than `[Structure] wrapArgsOver`, or when `[Structure] wrapArgsAt`
    holds at the list's opening.
*/
bool FormatExpressionLayout::mayBreakList(int open, int itemCount) const
{
    const FormatFileConfig &fileConfig =
        m_document->configAt(m_document->symbols().at(open).from);
    if ( ! fileConfig.wrapArgsOverCondition.isEmpty()) {
        return m_conditions->argsOver(open, itemCount);
    }
    if (m_document->token(open) == LBRACE) {
        return true;
    }
    if (itemCount < 1) {
        return false;
    }
    if (m_conditions->argsOver(open, itemCount)) {
        return true;
    }
    const int line = m_document->symbolLine(open);
    const int lineStart = m_document->lineStart(line);
    const QByteArray head = m_document->text().mid(
        lineStart,
        m_document->symbols().at(open).from + 1 - lineStart
    );
    const qreal percent = textWidth(head) * 100.0 / qMax(
        1,
        fileConfig.wrapMax
    );
    return m_conditions->holds(
        fileConfig.wrapArgsAt,
        open,
        percent,
        FormatFileConfig::defaultWrapChainAt()
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
    return continuationPad(m_indent, m_paddingLevel);
}

/*!
    Tells whether @p text, written from the column @p column with
    @p trailing more columns after it, ends within `[Structure] wrapMax`.
*/
bool FormatExpressionLayout::fits(
        int column,
        const QByteArray & text,
        int trailing,
        int token) const
{
    int width = column;
    const int lastBreak = text.lastIndexOf('\n');
    if (lastBreak >= 0) {
        width = 0;
    }
    const int wrapMax = wrapMaxAt(
        m_document,
        m_document->symbols().at(token).from
    );
    return width + textWidth(text.mid(lastBreak + 1)) + trailing <= wrapMax
        && ! text.contains('\n');
}
