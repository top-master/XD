// Copyright (C) 2015 The XD Company Ltd.
// SPDX-License-Identifier: Apache-2.0

#include "./args-wrap.h"

#include "../format-document.h"
#include "../parser/code-helpers.h"
#include "../parser/expression-layout.h"
#include "../parser/layout-helpers.h"
#include "../parser/scope-conditions.h"

#include <QtCore/QList>
#include <QtCore/QPair>

/*!
    @class FormatArgsWrapRule
    @internal

    Lays out call arguments or a brace initializer list as
    FormatExpressionLayout does: every item moves below the opening, one
    step (4 spaces) in, and the closing bracket goes back to that line's
    indent:

        const QByteArray input = makeFile(
            "first",
            "second"
        );

    A list already broken after its opening keeps the items that share a
    line together when they fit, and so does any list the user broke between
    its items over lines that look meant (see keepsUserLines()); any other
    list gets an item per line, as `[Structure] joinArgs` allows, and so do
    the items from a group too long for its line to the list's end. Outside
    a control head, a list with such lines, each later one right under its
    first item (see isAlignedUnderFirst()), stays as it is where
    `[Structure] wrapArgsAt` does not hold at its opening. A
    call with one argument is considered when its list already spans lines
    and that argument holds an operator chain.

    It lays out a list that does not fit one line (the list's `)` passes
    the wrap column, see wrapColumn()) when the call takes more than
    `[Structure] wrapArgsOver` arguments (3 by default), when
    `[Structure] wrapArgsAt` holds at the `(` (whose percents compare the
    column after the `(` with `[Structure] wrapMax`), when a list ending in
    an operator chain starts its arguments below the opening or has not yet
    broken, or when it is a brace list;
    and, on the same terms but whether or not it fits, it lays out a list
    the user broke over lines whose lines are not one step past the line of
    the opening. A list in the head of a control
    statement takes the condition's operand level, one step past the
    keyword's line, as its base; under `[Structure] compact = always`, a call
    that ends the condition of an `if` or a `while`, its `(` on the
    keyword's line, takes the keyword's line instead, and the condition's `)`
    hugs the call's, as `)) {`. The rule leaves alone a list with a top-level
    `;` or a comment in it, a
    `connect()` or `disconnect()` (see FormatConnectLayoutRule), and one at
    file, namespace or class scope, where a `(` mostly opens a declaration's
    parameters (see FormatParamWrapRule).
*/

/*!
    Tells whether the layout keeps the lines the user broke the list over,
    from the token @a open of @a document to its closing token @a close,
    whose items are @a arguments: the settings @a fileConfig take
    `[Structure] target` as `long`, and the lines look meant:

    * each item sits whole on one line;
    * none of them passes the wrap column (see wrapColumn());
    * no item of a later line starts past the end of the line above, which
      leaves room the user did not use;
    * each line holds the average count of items per line, rounded, or the
      last line fewer, as no more items may follow; unless
      `[Structure] wrapMistakes`, asked at the list through @a conditions,
      picks `never`.

    For a line that strays from that average, @a joinCount gets the count of
    items per line `wrapMistakes` lays the list out with: the average for
    `average`, else the number it gives; 0 leaves them to `joinArgs`.
*/
static bool keepsUserLines(
        const FormatDocument *document,
        int open,
        int close,
        const QList<QPair<int, int > > & arguments,
        FormatScopeConditions *conditions,
        const FormatFileConfig & fileConfig,
        int *joinCount)
{
    *joinCount = 0;
    const int firstLine = document->symbolLine(open);
    const int lastLine = document->symbolLine(close);
    if (codeTargetAnyAt(document, document->symbols().at(open).from)) {
        return false;
    }

    // Takes a line past the wrap column as lines that do not fit.
    for (int l = firstLine; l <= lastLine; ++l) {
        if (textWidth(document->lineText(l))
            > wrapColumn(document, document->lineStart(l))
        ) {
            return false;
        }
    }

    // Counts the items of each line, and finds one past the line above.
    QList<int > counts;
    int line = -1;
    for (int i = 0; i < arguments.size(); ++i) {
        const QPair<int, int > & item = arguments.at(i);
        const int from = document->symbols().at(item.first).from;
        const int itemLine = document->lineOf(from);
        if (itemLine != document->symbolEndLine(item.second)) {
            return false;
        }
        const int column = textWidth(
            document->text().mid(
                document->lineStart(itemLine),
                from - document->lineStart(itemLine)
            )
        );
        if (itemLine > firstLine
            && column > textWidth(document->lineText(itemLine - 1).trimmed())
                + textWidth(document->indentOf(itemLine - 1))
        ) {
            return false;
        }
        if (itemLine == line) {
            ++counts.last();
        } else {
            counts << 1;
            line = itemLine;
        }
    }

    // Takes a line whose count strays from the average as a mistake, which
    // `wrapMistakes` lays out anew with its count per line.
    const QVariant mistakes = conditions->resolveAt(
        QString(QLL("structure.wrapMistakes")),
        fileConfig.wrapMistakes,
        open
    );
    const bool isNumber = mistakes.type() == QVariant::Double;
    if ( ! isNumber && mistakes.toString() != QLL("average")) {
        return true;
    }
    const int average = qRound(qreal(arguments.size()) / counts.size());
    for (int i = 0; i < counts.size(); ++i) {
        const bool isShortLast = i + 1 == counts.size()
            && counts.at(i) < average;
        if (counts.at(i) != average && ! isShortLast) {
            *joinCount = isNumber
                ? qMax(1, FormatFileConfig::countOf(mistakes))
                : average;
            return false;
        }
    }
    return true;
}

/*!
    Tells whether the user broke the list from the token @a open of
    @a document to its closing token @a close, whose items are
    @a arguments, the way the C++ rules wrap a parameter list: its first
    item on the line of the opening, each later line starting right under
    that item, and the closing bracket on the line of the last item.
*/
static bool isAlignedUnderFirst(
        const FormatDocument *document,
        int open,
        int close,
        const QList<QPair<int, int > > & arguments)
{
    const int line = document->symbolLine(open);
    const int first = arguments.first().first;
    if (document->symbolLine(first) != line
        || document->symbolLine(arguments.last().second)
            != document->symbolLine(close)
    ) {
        return false;
    }
    const int column = textWidth(
        document->text().mid(
            document->lineStart(line),
            document->symbols().at(first).from - document->lineStart(line)
        )
    );
    for (int i = 1; i < arguments.size(); ++i) {
        const int itemLine = document->symbolLine(arguments.at(i).first);
        const bool startsLine = itemLine
            != document->symbolLine(arguments.at(i - 1).second);
        if (startsLine
            && textWidth(document->indentOf(itemLine)) != column
        ) {
            return false;
        }
    }
    return true;
}

/*!
    Finds a logical operator before the call at @a open whose break keeps
    both resulting lines within wrapMax. Gives its token and the indentation
    of its continuation line; leaves comments, directives and already split
    calls to their own layouts.
*/
static bool logicalBreakBeforeCall(
        const FormatDocument *document, int open,
        int *operatorIndex, QByteArray *indent)
{
    *operatorIndex = -1;
    indent->clear();
    const int close = document->match(open);
    const int line = document->symbolLine(open);
    if (close <= open || document->symbolEndLine(close) != line
        || document->isDirectiveLine(line)
    ) {
        return false;
    }
    const int lineFirst = document->firstSymbolOfLine(line);
    int op = open - 1;
    while (op > lineFirst) {
        const Token token = document->token(op);
        if (token == ANDAND || token == OROR) {
            break;
        }
        if (token == RPAREN || token == RBRACK) {
            const int match = document->match(op);
            if (match < lineFirst) {
                return false;
            }
            op = match - 1;
            continue;
        }
        if (token == COMMA || token == SEMIC || token == QUESTION
            || token == COLON || token == EQ || token == LBRACE
            || token == RBRACE || operatorPrecedence(token) > 0
        ) {
            return false;
        }
        --op;
    }
    if (op <= lineFirst || (document->token(op) != ANDAND
            && document->token(op) != OROR)
    ) {
        return false;
    }
    const int from = document->symbolEnd(op - 1);
    const int at = document->symbols().at(op).from;
    const int end = contentEnd(document, line);
    if (document->hasCommentBetween(from, end)
        || document->spansDirective(from, end)
    ) {
        return false;
    }
    const QByteArray prefix = document->text().mid(
        document->lineStart(line), from - document->lineStart(line)
    );
    const QByteArray tail = document->text().mid(at, end - at);
    const QByteArray padding = continuationPad(
        textWidth(document->indentOf(line))
    );
    const int limit = wrapMaxAt(document, at);
    if (textWidth(prefix) > limit
        || textWidth(padding) + textWidth(tail) > limit
    ) {
        return false;
    }
    *operatorIndex = op;
    *indent = padding;
    return true;
}

/*!
    Lays out each list of @a document that asks for it; a list inside a list
    it lays out waits for the next pass.
*/
void FormatArgsWrapRule::run(FormatDocument *document) const
{
    FormatScopeConditions conditions(document);
    for (int i = 1; i < document->symbols().size(); ++i) {
        const int close = document->match(i);
        const bool isCall = document->token(i) == LPAREN
            && this->isCallParen(document, i);
        const bool isBrace = document->token(i) == LBRACE
            && document->braceScope(i) == FormatDocument::InitScope;
        if (close <= i + 1 || ( ! isCall && ! isBrace)) {
            continue;
        }
        const int line = document->symbolLine(i);
        const int lineStart = document->lineStart(line);
        const int from = document->symbols().at(i).from;
        const FormatFileConfig &fileConfig = document->configAt(from);
        const int closeFrom = document->symbols().at(close).from;
        const int closeEnd = document->symbolEnd(close);

        // Leave comments and directives in place while laying out the list.
        bool isPlain = ! document->hasCommentBetween(from + 1, closeFrom)
            && ! document->spansDirective(from, closeEnd);
        // Skip nested expressions when looking for a statement boundary.
        for (int k = i + 1; k < close && isPlain; ++k) {
            const Token token = document->token(k);
            if ((token == LPAREN || token == LBRACK || token == LBRACE)
                && document->match(k) > k
            ) {
                k = document->match(k);
                continue;
            }
            isPlain = token != SEMIC;
        }
        const QList<QPair<int, int > > arguments = splitArguments(
            document,
            i,
            close
        );
        // Takes a list of one argument only when it spans lines and its
        // argument is an operator chain (see below), whose padding may be off.
        const bool isSplit = document->symbolLine(i)
            != document->symbolLine(close);
        if ( ! isPlain
            || arguments.isEmpty()
            || (arguments.size() < 2
                && ! isSplit)
        ) {
            continue;
        }

        // Finds the indent the list's lines step from: the condition's
        // operand level in the head of a control statement.
        int indent = textWidth(document->indentOf(line));
        const int keyword = this->controlKeyword(document, i);
        const bool isHeadLine = keyword >= 0
            && document->symbolLine(keyword) == line;

        // Lets a call that ends the condition of an `if` or a `while`, its
        // `(` on the keyword's line, hug the condition's `)`: its lines then
        // step from the keyword's line, and the condition closes right after
        // it, as `)) {` (see FormatConditionCloseRule).
        const int headClose = keyword >= 0
            ? document->match(keyword + 1) : -1;
        const bool hugsHead = compactAlwaysAt(document, document->symbols().at(i).from)
            && isCall
            && isHeadLine
            && (document->token(keyword) == IF
                || document->token(keyword) == WHILE)
            && close + 1 == headClose
            && ! document->hasCommentBetween(
                closeEnd,
                document->symbols().at(headClose).from
            );
        if (isHeadLine && ! hugsHead) {
            indent += 4;
        }
        const int previousOperator =
            precedingChainOperator(document, close + 1, false);
        const bool inlineOperator = previousOperator >= 0
            && document->token(previousOperator) != QUESTION
            && document->firstSymbolOfLine(document->symbolLine(previousOperator))
                != previousOperator;
        const int extraPad = inlineOperator
            ? conditions.operatorPadding(previousOperator)
            : 0;
        FormatExpressionLayout layout(document, &conditions, indent);
        const int column = textWidth(
            document->text().mid(lineStart, from - lineStart)
        );

        // Tells whether the list asks for a layout: it does not fit, and may
        // break, or its lines sit at other indents than its brackets give
        // them (see `hasMisplacedLines()`).
        const QByteArray oneLine = layout.oneLine(i, close);
        const bool endsInChain = layout.hasChain(
            arguments.last().first,
            arguments.last().second
        );
        if (arguments.size() < 2 && ! endsInChain) {
            continue;
        }
        const bool argumentsBelow =
            document->symbolLine(arguments.first().first) > line;
        const bool over = conditions.argsOver(i, arguments.size());
        const bool mayBreak = fileConfig.wrapArgsOverCondition.isEmpty()
            ? isBrace || (endsInChain && ( ! isSplit || argumentsBelow))
                || over || reachesWrapArgsAt(document, &conditions, i)
            : over;
        const bool needsBreak = mayBreak
            && column + textWidth(oneLine) > wrapColumn(document, from);
        const int closeLine = document->symbolLine(close);
        const bool isMisplaced = hasMisplacedLines(
            document,
            i,
            close,
            indent,
            extraPad
        );
        if ( ! needsBreak && ! (mayBreak && isMisplaced)) {
            continue;
        }

        // Leaves a statement's list whose meant lines stand right under its
        // first item as it is, unless `[Structure] wrapArgsAt` asks for a
        // break after its opening; a list in a control head breaks there
        // still, as its condition then closes on its own line.
        int joinCount = 0;
        const bool keepsLines = keepsUserLines(
            document,
            i,
            close,
            arguments,
            &conditions,
            fileConfig,
            &joinCount
        );
        if (keepsLines
            && keyword < 0
            && ! reachesWrapArgsAt(document, &conditions, i)
            && isAlignedUnderFirst(document, i, close, arguments)
        ) {
            continue;
        }

        // Moves a one-line function body that the list's line ends with to a
        // line of its own, when that alone makes it fit.
        const int body = needsBreak ? this->inlineBody(document, i) : -1;
        if (body >= 0) {
            const int start = document->symbolEnd(body - 1);
            const QByteArray indent = document->indentOf(line);
            if (document->replace(
                start,
                document->symbols().at(body).from - start,
                "\n" + indent
            )) {
                i = document->match(body);
                continue;
            }
        }

        int logicalOperator = -1;
        QByteArray logicalIndent;
        if (isCall && needsBreak && logicalBreakBeforeCall(
            document, i, &logicalOperator, &logicalIndent
        )) {
            const int start = document->symbolEnd(logicalOperator - 1);
            if (document->replace(
                start, document->symbols().at(logicalOperator).from - start,
                "\n" + logicalIndent
            )) {
                continue;
            }
        }

        // Writes the list as the layout breaks it, keeping the items the
        // user put on one line together where the lines look meant.
        const QByteArray text = layout.layoutList(
            i,
            i,
            close,
            column,
            keepsLines,
            joinCount,
            extraPad
        );
        const int length = (hugsHead
            ? document->symbols().at(headClose).from
            : closeEnd) - from;
        if (document->text().mid(from, length) != text
            && document->replace(from, length, text)
            && layout.brokeUngrouped()
        ) {
            // Asks the agent to put the arguments that belong together on one
            // line (see FormatAgentGuides for the prompt it gets).
            const int wrapMax = wrapMaxAt(document, from);
            document->request(
                from,
                "args-wrap",
                QString(QLL("the tool put each argument on a line of its own; "
                    "put the arguments that belong together on one "
                    "line, each line within %1 columns")).arg(wrapMax),
                closeLine - line + 1,
                wrapMax
            );
        }
        i = close;
    }
}

/*!
    Builds the `{` of the one-line function body that holds the list at
    @a paren of @a document, when that body fits the wrap column (see
    wrapColumn()) on a line of its own, at the indent of its head's line;
    -1 for any other list. Such a body opens and closes on the list's line,
    right after the function's head.
*/
int FormatArgsWrapRule::inlineBody(
        const FormatDocument *document,
        int paren) const
{
    // Finds the brace that holds the list.
    int brace = -1;
    for (int k = paren - 1; k > 0 && brace < 0; --k) {
        const Token token = document->token(k);
        if (token == RBRACE && document->match(k) >= 0) {
            k = document->match(k);
        } else if (token == LBRACE && document->match(k) > paren) {
            brace = k;
        }
    }
    const int line = document->symbolLine(paren);
    if (brace < 1
        || document->braceScope(brace) != FormatDocument::FunctionScope
        || document->symbolLine(brace) != line
        || document->symbolLine(brace - 1) != line
        || document->symbolLine(document->match(brace)) != line
    ) {
        return -1;
    }

    // Measures the body on a line of its own.
    const int from = document->symbols().at(brace).from;
    const QByteArray text = document->text().mid(
        from, document->symbolEnd(document->match(brace)) - from
    );
    const int width = textWidth(document->indentOf(line)) + textWidth(text);
    if (width > wrapColumn(document, from)) {
        return -1;
    }
    return brace;
}

/*!
    Tells whether the `(` at @a paren of @a document opens the arguments of
    a call inside a function body: a name comes before it, other than
    `connect` and `disconnect`.
*/
bool FormatArgsWrapRule::isCallParen(
        const FormatDocument *document,
        int paren) const
{
    if (document->token(paren - 1) != IDENTIFIER) {
        return false;
    }
    const FormatDocument::Scope scope = document->scopeOf(paren);
    if (scope != FormatDocument::FunctionScope
        && scope != FormatDocument::BlockScope
        && scope != FormatDocument::InitScope
    ) {
        return false;
    }
    const QByteArray name = document->lexem(paren - 1);
    return name != "connect" && name != "disconnect";
}

/*!
    Builds the index of the `if`, `while`, `for`, or `switch` whose head holds
    the `(` at @a paren of @a document, or -1 when it sits in no such head.
*/
int FormatArgsWrapRule::controlKeyword(
        const FormatDocument *document, int paren) const
{
    for (int k = paren - 1; k > 0; --k) {
        const Token token = document->token(k);
        const int match = document->match(k);
        if ((token == RPAREN || token == RBRACK) && match >= 0) {
            k = match;
            continue;
        }
        if (token == SEMIC || token == LBRACE || token == RBRACE) {
            return -1;
        }
        if (token == LPAREN) {
            const Token keyword = document->token(k - 1);
            if (keyword == IF || keyword == WHILE || keyword == FOR
                || keyword == SWITCH
            ) {
                return k - 1;
            }
        }
    }
    return -1;
}
