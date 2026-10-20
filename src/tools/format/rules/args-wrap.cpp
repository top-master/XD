// Copyright (C) 2015 The XD Company Ltd.
// SPDX-License-Identifier: Apache-2.0

#include "./args-wrap.h"

#include "../format-document.h"
#include "../parser/code-helpers.h"
#include "../parser/layout-helpers.h"
#include "../parser/scope-conditions.h"

#include <QtCore/QList>
#include <QtCore/QPair>

/*!
    \class FormatArgsWrapRule
    \internal

    Lays out a call of 2 arguments or more that do not fit one line (they
    span lines, or the call's `)` passes the wrap column, see wrapColumn())
    with a break after its `(`: each argument on a line of its own, one step
    (4 spaces) past the line of the `(`, and the `)` back at that line's
    indent:

        const QByteArray input = makeFile(
            "first",
            "second"
        );

    It does that when the call takes more than 3 arguments, or when
    `[Structure] wrapArgsAt` holds at the `(`, whose percents compare the
    column after the `(` with `[Structure] wrapMax`; any other wrapped call
    keeps its layout. The rule leaves alone a call with a brace, a `;` or a
    comment in its arguments, one in the head of a control statement (see
    FormatConditionWrapRule), a `connect()` or `disconnect()` (see
    FormatConnectLayoutRule), and one at file, namespace or class scope,
    where a `(` mostly opens a declaration's parameters (see
    FormatParamWrapRule).
*/

/*!
    Breaks each wrapped call of \a document that asks for it; a call inside
    the arguments of a call it breaks waits for the next pass.
*/
void FormatArgsWrapRule::run(FormatDocument *document) const
{
    FormatScopeConditions conditions(document);
    const int wrap = wrapColumn(document);
    const int wrapMax = qMax(1, document->settings().wrapMax);
    for (int i = 1; i < document->symbols().size(); ++i) {
        const int close = document->match(i);
        if (document->token(i) != LPAREN || close <= i + 1
            || ! this->isCallParen(document, i)
        ) {
            continue;
        }
        const int line = document->symbolLine(i);
        const int lineStart = document->lineStart(line);
        const int from = document->symbols().at(i).from;
        const int closeFrom = document->symbols().at(close).from;
        const int closeEnd = document->symbolEnd(close);

        // Leaves a call alone when its arguments hold a brace, a `;`, a
        // comment, or a preprocessor directive.
        bool isPlain = ! document->hasCommentBetween(from + 1, closeFrom)
            && ! document->spansDirective(from, closeEnd);
        for (int k = i + 1; k < close && isPlain; ++k) {
            const Token token = document->token(k);
            isPlain = token != LBRACE && token != RBRACE && token != SEMIC;
        }
        if ( ! isPlain || this->isInControlHead(document, i)) {
            continue;
        }

        // Leaves a call whose one-line form fits the wrap column up to its
        // `)`, and a call already broken after its `(` within `wrapMax`.
        const QList<QPair<int, int > > arguments = splitArguments(
            document,
            i,
            close
        );
        if (arguments.size() < 2
            || this->oneLineEnd(document, i, close, arguments) <= wrap
            || this->isBrokenAfterOpen(document, i, close, wrapMax)
        ) {
            continue;
        }

        // Tells whether the call asks for the break.
        const QByteArray head = document->text().mid(
            lineStart,
            from + 1 - lineStart
        );
        const qreal percent = textWidth(head) * 100.0 / wrapMax;
        if (arguments.size() <= 3
            && ! conditions.holds(document->settings().wrapArgsAt, i, percent,
                                  FormatSettings::defaultWrapArgsAt())
        ) {
            continue;
        }

        // Writes each argument on a line of its own.
        const QByteArray indent = document->indentOf(line);
        const QByteArray pad = indent + "    ";
        QByteArray text = "(";
        for (int a = 0; a < arguments.size(); ++a) {
            text += "\n" + pad;
            const QPair<int, int > & argument = arguments.at(a);
            text += this->argumentText(
                document,
                argument.first,
                argument.second,
                pad
            );
            if (a + 1 < arguments.size()) {
                text += ",";
            }
        }
        text += "\n" + indent + ")";
        const int length = closeEnd - from;
        if (document->text().mid(from, length) != text
            && document->replace(from, length, text)
        ) {
            // Asks the agent to put the arguments that belong together on one
            // line (see FormatAgentGuides for the prompt it gets).
            document->request(
                from,
                "args-wrap",
                QString(QLL("the tool put each argument on a line of its own; "
                    "put the arguments that belong together on one "
                    "line, each line within %1 columns")).arg(wrapMax),
                document->symbolLine(close) - line + 1,
                wrapMax
            );
        }
        i = close;
    }
}

/*!
    Tells whether the `(` at \a paren of \a document opens the arguments of
    a call inside a function body: a name comes before it, other than
    `connect` and `disconnect`.
*/
bool FormatArgsWrapRule::isCallParen(const FormatDocument *document,
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
    Tells whether the `(` at \a paren of \a document sits in the head of an
    `if`, `while`, `for`, or `switch`, whose layout the condition rules own.
*/
bool FormatArgsWrapRule::isInControlHead(const FormatDocument *document,
                                         int paren) const
{
    for (int k = paren - 1; k > 0; --k) {
        const Token token = document->token(k);
        const int match = document->match(k);
        if ((token == RPAREN || token == RBRACK) && match >= 0) {
            k = match;
            continue;
        }
        if (token == SEMIC || token == LBRACE || token == RBRACE) {
            return false;
        }
        if (token == LPAREN) {
            const Token keyword = document->token(k - 1);
            if (keyword == IF || keyword == WHILE || keyword == FOR
                || keyword == SWITCH
            ) {
                return true;
            }
        }
    }
    return false;
}

/*!
    Builds the column right after the `)` at \a close of \a document, were
    the call whose `(` is at \a open written on the line of that `(`: its
    \a arguments joined by `, `, each with its own line breaks taken out.
    What follows the `)` does not count, since a break inside the call
    cannot make it fit.
*/
int FormatArgsWrapRule::oneLineEnd(const FormatDocument *document, int open, int close,
                                     const QList<QPair<int, int > > & arguments) const
{
    const int lineStart = document->lineStart(document->symbolLine(open));
    const int from = document->symbols().at(open).from;
    QByteArray text = document->text().mid(lineStart, from + 1 - lineStart);
    for (int a = 0; a < arguments.size(); ++a) {
        const QPair<int, int > & argument = arguments.at(a);
        text += (a > 0 ? ", " : "")
            + sourceText(document, argument.first, argument.second).simplified();
    }
    text += ")";
    return textWidth(text);
}

/*!
    Tells whether the call of \a document whose `(` is at \a open and whose
    `)` is at \a close already breaks after its `(`: the `(` ends its line,
    the `)` starts one at the indent of the `(` line, and no line between
    passes \a limit columns. Hence a grouping of related arguments, which an
    agent chose, stays.
*/
bool FormatArgsWrapRule::isBrokenAfterOpen(const FormatDocument *document, int open,
                                           int close, int limit) const
{
    const int line = document->symbolLine(open);
    const int closeLine = document->symbolLine(close);
    if (closeLine <= line + 1
        || document->lastSymbolOfLine(line) != open
        || document->firstSymbolOfLine(closeLine) != close
        || document->indentOf(closeLine) != document->indentOf(line)
    ) {
        return false;
    }
    for (int l = line + 1; l < closeLine; ++l) {
        if (textWidth(document->lineText(l)) > limit) {
            return false;
        }
    }
    return true;
}

/*!
    Builds the text of the argument from the token \a first up to \a last of
    \a document, its later lines indented one step past \a pad.
*/
QByteArray FormatArgsWrapRule::argumentText(const FormatDocument *document, int first,
                                            int last, const QByteArray & pad) const
{
    const QByteArray source = sourceText(document, first, last);
    const QList<QByteArray > lines = source.split('\n');
    QByteArray text = lines.first();
    for (int l = 1; l < lines.size(); ++l) {
        text += "\n" + pad + "    " + lines.at(l).trimmed();
    }
    return text;
}
