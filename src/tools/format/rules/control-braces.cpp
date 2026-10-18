// Copyright (C) 2015 The XD Company Ltd.
// SPDX-License-Identifier: Apache-2.0

#include "./control-braces.h"

#include "../format-document.h"
#include "../parser/code-helpers.h"
#include "../parser/layout-helpers.h"

#include <QtCore/QByteArray>
#include <QtCore/QString>

/*!
    \class FormatControlBracesRule
    \internal

    Wraps the body of each `if`, `else`, `for`, `while`, `do`, and `foreach`
    in braces, also when it is one statement; a body that is itself a control
    statement, or that a preprocessor directive splits, is reported instead.
*/

/*!
    Finds each control statement whose body has no braces, and braces it.
*/
void FormatControlBracesRule::run(FormatDocument *document) const
{
    for (int i = 0; i < document->symbols().size(); ++i) {
        if (document->isDirectiveLine(document->symbolLine(i))) {
            continue;
        }
        const int body = this->bodyStart(document, i);
        if (body < 0 || document->token(body) == LBRACE || document->token(body) == SEMIC) {
            continue;
        }
        const Token token = document->token(body);
        if (token == IF || token == FOR || token == WHILE || token == DO || token == SWITCH
            || token == TRY
        ) {
            document->report(document->symbols().at(i).from, "control-braces",
                             QString(QLL("wrap the nested control statement in braces")));
            continue;
        }
        this->brace(document, i, body - 1, body);
    }
}

/*!
    Moves the braced body of the control statement whose keyword is at
    \a keyword of \a document, and whose `{` is at \a brace, off its line,
    when it sits on that line whole (`if (x) { a(); }`): the statements go
    one step (4 spaces) past the keyword's line, and the `}` to the line
    after them, at the keyword's indent. An empty body stays on its line,
    and so does one that a directive splits, or that has a brace or a
    comment of its own.
*/
void FormatControlBracesRule::unfold(FormatDocument *document, int keyword,
                                     int brace) const
{
    const int close = document->match(brace);
    const int line = document->symbolLine(brace);
    if (close <= brace + 1 || document->symbolLine(close) != line
        || document->symbolLine(keyword) != line
        || document->hasCommentBetween(document->symbolEnd(brace),
                                       document->symbols().at(close).from)
    ) {
        return;
    }
    for (int k = brace + 1; k < close; ++k) {
        if (document->token(k) == LBRACE) {
            return;
        }
    }
    const QByteArray indent = document->indentOf(line);
    const int from = document->symbolEnd(brace);
    document->replace(from, document->symbols().at(brace + 1).from - from,
                      "\n" + continuationPad(indent.size()));
    const int last = document->symbolEnd(close - 1);
    document->replace(last, document->symbols().at(close).from - last, "\n" + indent);
}

/*!
    Builds the index of the first token of the body that the control keyword
    at \a keyword of \a document governs, or -1 when that token is no control
    keyword (or the keyword governs no body of its own, as `else if` and the
    `while` of a `do` loop do not).
*/
int FormatControlBracesRule::bodyStart(const FormatDocument *document,
                                       int keyword) const
{
    const Token token = document->token(keyword);
    if (token == ELSE) {
        return document->token(keyword + 1) == IF ? -1 : keyword + 1;
    }
    if (token == DO) {
        return keyword + 1;
    }
    bool isLoopMacro = false;
    if (token == IDENTIFIER) {
        const QByteArray name = document->lexem(keyword);
        isLoopMacro = name == "foreach" || name == "Q_FOREACH";
    }
    if (token != IF && token != FOR && token != WHILE && ! isLoopMacro) {
        return -1;
    }
    if (document->token(keyword + 1) != LPAREN
        || document->match(keyword + 1) < 0
    ) {
        return -1;
    }

    // Leaves out the `while` that ends a `do` loop.
    if (token == WHILE && document->token(keyword - 1) == RBRACE) {
        const int opener = document->match(keyword - 1);
        if (opener > 0 && document->token(opener - 1) == DO) {
            return -1;
        }
    }
    const int body = document->match(keyword + 1) + 1;
    return body < document->symbols().size() ? body : -1;
}

/*!
    Queues the braces around the one-statement body starting at the token
    \a body of \a document, whose control statement starts at \a keyword and
    whose head ends at the token \a head.
*/
void FormatControlBracesRule::brace(FormatDocument *document, int keyword,
                                    int head, int body) const
{
    const int end = statementEnd(document, body);
    if (end < 0) {
        return;
    }
    const int headLine = document->symbolEndLine(head);
    const int endLine = document->symbolEndLine(end);
    for (int line = headLine; line <= endLine; ++line) {
        if (document->isDirectiveLine(line)) {
            document->report(document->symbols().at(keyword).from, "control-braces",
                             QString(QLL("brace the body by hand: a directive splits it")));
            return;
        }
    }

    // Keeps a one-line statement on its line: `if (x) { return; }`.
    const Token after = document->token(end + 1);
    const bool followedOnLine = end + 1 < document->symbols().size()
        && document->symbolLine(end + 1) == endLine;
    if (document->symbolLine(body) == headLine) {
        if (followedOnLine && after != ELSE && after != WHILE) {
            return;
        }
        document->replace(document->symbolEnd(head), 0, " {");
        document->replace(document->symbolEnd(end), 0, " }");
        return;
    }

    // Closes a body on lines of its own with a brace at the keyword's indent;
    // an `else` or the `while` of a `do` joins that brace's line.
    if (followedOnLine) {
        return;
    }
    const QByteArray indent = document->indentOf(document->symbolLine(keyword));
    const bool joins = (after == ELSE || (after == WHILE && document->token(keyword) == DO))
        && ! document->hasCommentBetween(
            document->symbolEnd(end),
            document->symbols().at(end + 1).from
        )
        && ! document->spansDirective(
            document->symbolEnd(end),
            document->symbols().at(end + 1).from
        );
    document->replace(document->symbolEnd(head), 0, " {");
    if (joins) {
        const int from = document->symbolEnd(end);
        const int to = document->symbols().at(end + 1).from;
        document->replace(from, to - from, "\n" + indent + "} ");
        return;
    }
    document->replace(contentEnd(document, endLine), 0, "\n" + indent + "}");
}
