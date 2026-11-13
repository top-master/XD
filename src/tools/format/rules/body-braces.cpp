// Copyright (C) 2015 The XD Company Ltd.
// SPDX-License-Identifier: Apache-2.0

#include "./body-braces.h"

#include "../format-document.h"
#include "../parser/layout-helpers.h"
#include "../parser/scope-conditions.h"

#include <QtCore/QByteArray>
#include <QtCore/QList>

/*!
    @class FormatBodyBracesRule
    @internal

    Gives the body of a function, a class, an enum, or a namespace its own
    lines when its closing `}` stands on a line that holds only a chain of
    closing brackets, such as the `} }` of `{ if (ready) { ... } }`, or
    `); }`: the text after the body's `{` moves to a line of its own, one
    step (4 spaces) past the line of the `{`, the body's later lines move in
    until the shallowest of them stands at that step too, and the body's
    `}` closes on a line of its own, at the indent of the line of the `{`.
    A namespace keeps its body at the indent of the line of its `{`.

    It runs after every other rule, on what they leave, unless
    `[Structure] bodyBraces` is `false`. A body whose `}` shares its line
    with other code is compact formatting and stays as it is (such as
    `{ return count(); }`), and so do the body of a control statement and a
    body with a comment, a directive, or a token that spans lines.
*/

/*!
    Tells whether the `}` at @a close of @a document stands on a line that
    holds only a chain of closing brackets, such as `} }`, `) }`, or
    `); }`: another closing bracket before it, and nothing else on its line
    but closing brackets and `;`.
*/
static bool closesChain(const FormatDocument *document, int close)
{
    const int line = document->symbolLine(close);
    const int first = document->firstSymbolOfLine(line);
    const int last = document->lastSymbolOfLine(line);
    if (first < 0 || first >= close) {
        return false;
    }
    for (int k = first; k <= last; ++k) {
        const Token token = document->token(k);
        if (token != RBRACE
            && token != RPAREN
            && token != RBRACK
            && token != SEMIC
        ) {
            return false;
        }
    }
    const Token start = document->token(first);
    return start == RBRACE || start == RPAREN || start == RBRACK;
}

/*!
    Gives each body of @a document that closes in a chain of brackets its
    own lines, unless `[Structure] bodyBraces` is `false`.
*/
void FormatBodyBracesRule::run(FormatDocument *document) const
{
    FormatScopeConditions conditions(document);
    for (int close = 1; close < document->symbols().size(); ++close) {
        const int open = document->match(close);
        if (document->token(close) != RBRACE || open < 0
            || ! closesChain(document, close)
            || ! conditions.holds(
                document->settingsAt(document->symbols().at(open).from).bodyBraces, open
            )
        ) {
            continue;
        }
        const FormatDocument::Scope scope = document->braceScope(open);
        if (scope != FormatDocument::FunctionScope
            && scope != FormatDocument::ClassScope
            && scope != FormatDocument::EnumScope
            && scope != FormatDocument::NamespaceScope
        ) {
            continue;
        }

        // Leaves a body with a comment, a directive, or a token spanning lines.
        const int from = document->symbols().at(open).from;
        const int to = document->symbolEnd(close);
        bool isPlain = ! document->hasCommentBetween(from, to)
            && ! document->spansDirective(from, to);
        for (int k = open; k <= close && isPlain; ++k) {
            isPlain = document->symbolLine(k) == document->symbolEndLine(k);
        }
        if ( ! isPlain) {
            continue;
        }

        // Takes the body's lines, the last one up to the `}` that closes it.
        const int openLine = document->symbolLine(open);
        const QByteArray indent = document->indentOf(openLine);
        const QByteArray bodyIndent = scope == FormatDocument::NamespaceScope
            ? indent : indent + continuationPad(0);
        QList<QByteArray > lines = document->text().mid(
            document->symbolEnd(open),
            document->symbols().at(close).from - document->symbolEnd(open)
        ).split('\n');
        const QByteArray first = lines.takeFirst().trimmed();
        if ( ! lines.isEmpty()) {
            lines.last() = document->indentOf(document->symbolLine(close))
                + lines.last().trimmed();
            if (lines.last().trimmed().isEmpty()) {
                lines.removeLast();
            }
        }

        // Moves the later lines in until the shallowest stands at
        // the body's indent.
        int shallowest = -1;
        for (int l = 0; l < lines.size(); ++l) {
            const QByteArray & line = lines.at(l);
            // Counts the line's leading spaces and tabs.
            int width = 0;
            while (width < line.size()
                && (line.at(width) == ' ' || line.at(width) == '\t')
            ) {
                ++width;
            }
            if (width < line.size()) {
                shallowest = shallowest < 0
                    ? width : qMin(shallowest, width);
            }
        }
        const int shift = shallowest < 0
            ? 0 : qMax(0, bodyIndent.size() - shallowest);
        QByteArray text = "{";
        if ( ! first.isEmpty()) {
            text += "\n" + bodyIndent + first;
        }
        for (int l = 0; l < lines.size(); ++l) {
            const QByteArray & line = lines.at(l);
            const QByteArray pad = line.trimmed().isEmpty()
                ? QByteArray() : QByteArray(shift, ' ');
            text += "\n" + pad + line;
        }

        // Closes the body on a line of its own.
        text += "\n" + indent + "}";
        document->replace(from, to - from, text);
    }
}
