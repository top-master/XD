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

    Lays out a call of 2 arguments or more, or a brace initializer list, as
    FormatExpressionLayout does: the leading items, and the head of a last
    item that is an operator chain, stay on the line of the opening when
    they fit, the chain going on below; otherwise a break after the
    opening, the items one step (4 spaces) past the line of the opening, and
    the closing bracket back at that line's indent:

        const QByteArray input = makeFile(
            "first",
            "second"
        );

    Items the user put on one line keep sharing one when it fits; a list
    the user wrote on one line gets an item per line.

    It lays out a list that does not fit one line (the list's `)` passes
    the wrap column, see wrapColumn()) when the call takes more than 3
    arguments, when `[Structure] wrapArgsAt` holds at the `(` (whose
    percents compare the column after the `(` with `[Structure] wrapMax`),
    when its last argument is an operator chain, or when it is a brace list;
    and, on the same terms but whether or not it fits, it lays out a list
    the user broke over lines whose lines are not one step past the line of
    the opening. A list in the head of a control
    statement takes the condition's operand level, one step past the
    keyword's line, as its base. The rule leaves alone a list with a brace
    (other than a nested brace list), a `;` or a comment in it, a
    `connect()` or `disconnect()` (see FormatConnectLayoutRule), and one at
    file, namespace or class scope, where a `(` mostly opens a declaration's
    parameters (see FormatParamWrapRule).
*/

/*!
    Lays out each list of @a document that asks for it; a list inside a list
    it lays out waits for the next pass.
*/
void FormatArgsWrapRule::run(FormatDocument *document) const
{
    FormatScopeConditions conditions(document);
    const int wrap = wrapColumn(document);
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
        const int closeFrom = document->symbols().at(close).from;
        const int closeEnd = document->symbolEnd(close);

        // Leaves a list alone when it holds a brace, a `;`, a comment, or a
        // preprocessor directive.
        bool isPlain = ! document->hasCommentBetween(from + 1, closeFrom)
            && ! document->spansDirective(from, closeEnd);
        for (int k = i + 1; k < close && isPlain; ++k) {
            const Token token = document->token(k);
            isPlain = token != SEMIC
                && (( ! isBrace && token != LBRACE && token != RBRACE)
                    || (isBrace && (token != LBRACE
                        || document->braceScope(k) == FormatDocument::InitScope)));
        }
        const QList<QPair<int, int > > arguments = splitArguments(
            document,
            i,
            close
        );
        if ( ! isPlain || arguments.size() < 2) {
            continue;
        }

        // Finds the indent the list's lines step from: the condition's
        // operand level in the head of a control statement.
        int indent = textWidth(document->indentOf(line));
        const int keyword = this->controlKeyword(document, i);
        if (keyword >= 0 && document->symbolLine(keyword) == line) {
            indent += 4;
        }
        FormatExpressionLayout layout(document, &conditions, indent);
        const int column = textWidth(document->text().mid(lineStart, from
            - lineStart
        ));

        // Tells whether the list asks for a layout: it does not fit, and may
        // break, or its lines are not one step past the opening's line.
        const QByteArray oneLine = layout.oneLine(i, close);
        const bool endsInChain = layout.hasChain(
            arguments.last().first,
            arguments.last().second
        );
        const bool mayBreak = isBrace || endsInChain || arguments.size() > 3
            || this->mayBreakAt(document, &conditions, i);
        const bool needsBreak = mayBreak && column + textWidth(oneLine) > wrap;
        const int closeLine = document->symbolLine(close);
        bool isMisplaced = false;
        for (int l = line + 1; l <= closeLine && ! isMisplaced; ++l) {
            const int lead = document->firstSymbolOfLine(l);
            isMisplaced = lead > i && lead < close
                && textWidth(document->indentOf(l)) != indent + 4;
        }
        if ( ! needsBreak && ! (mayBreak && isMisplaced)) {
            continue;
        }

        // Writes the list as the layout breaks it.
        const QByteArray text = layout.layoutList(i, i, close, column);
        const int length = closeEnd - from;
        if (document->text().mid(from, length) != text
            && document->replace(from, length, text)
            && layout.brokeUngrouped()
        ) {
            // Asks the agent to put the arguments that belong together on one
            // line (see FormatAgentGuides for the prompt it gets).
            const int wrapMax = qMax(1, document->settings().wrapMax);
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
    Tells whether `[Structure] wrapArgsAt`, through @a conditions, holds at
    the `(` at @a open of @a document: its percents compare the column after
    the `(` with `[Structure] wrapMax`.
*/
bool FormatArgsWrapRule::mayBreakAt(
        const FormatDocument *document,
        FormatScopeConditions *conditions, int open) const
{
    const int lineStart = document->lineStart(document->symbolLine(open));
    const QByteArray head = document->text().mid(
        lineStart,
        document->symbols().at(open).from + 1 - lineStart
    );
    const qreal percent = textWidth(head)
        * 100.0
        / qMax(1, document->settings().wrapMax);
    return conditions->holds(
        document->settings().wrapArgsAt, open, percent,
        FormatSettings::defaultWrapChainAt()
    );
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
