// Copyright (C) 2015 The XD Company Ltd.
// SPDX-License-Identifier: Apache-2.0

#include "./param-wrap.h"

#include "../format-document.h"
#include "../parser/code-helpers.h"

#include <QtCore/QByteArray>
#include <QtCore/QList>
#include <QtCore/QString>

/*!
    \class FormatParamWrapRule
    \internal

    Wraps the parameter list of a declaration or definition whose line runs
    past the column limit (`[Structure] wrapMax`, 80 by default): after a
    comma, each line under the first parameter, as many parameters per line
    as fit; when even the first one does not fit there, the list starts on
    the next line, indented two steps past the line it starts on.
*/

/*!
    Rewraps each declaration line past the column limit whose whole
    parameter list sits on that line.
*/
void FormatParamWrapRule::run(FormatDocument *document) const
{
    const int limit = document->settings().wrapMax;
    for (int line = 1; line <= document->lineCount(); ++line) {
        const int first = document->firstSymbolOfLine(line);
        if (first < 0 || document->isDirectiveLine(line)
            || textWidth(document->lineText(line)) <= limit
        ) {
            continue;
        }
        const int last = document->lastSymbolOfLine(line);
        for (int i = first; i < last; ++i) {
            const int close = document->match(i);
            if (document->token(i) != LPAREN || close < 0 || close > last
                || document->token(i + 1) == RPAREN || ! this->isDeclarationParen(document, i)
                || document->hasCommentBetween(document->symbolEnd(i), document->symbols().at(close).from)
            ) {
                continue;
            }
            // Leaves a constructor line with its initializer list to the
            // initializer-list layout.
            if (document->token(close + 1) == COLON) {
                break;
            }

            // Leaves a line that is long for its one-line body, and
            // not for its parameters.
            bool hasBody = false;
            for (int k = close + 1; k <= last && ! hasBody; ++k) {
                hasBody = document->token(k) == LBRACE;
            }
            if (hasBody) {
                break;
            }
            const QList<QByteArray > items = this->parameters(
                document,
                i,
                close
            );
            const int start = document->lineStart(line);
            const int end = contentEnd(document, line);
            const QByteArray prefix = document->text().mid(
                start,
                document->symbolEnd(i) - start
            );
            const QByteArray suffix = document->text().mid(
                document->symbols().at(close).from,
                end - document->symbols().at(close).from
            );

            // Fills under the first parameter, or else from the next line.
            QList<QByteArray > lines;
            const QByteArray head = prefix + items.first()
                + (items.size() == 1 ? suffix : ",");
            if (textWidth(head) <= limit) {
                lines = this->fill(
                    prefix + items.first(),
                    QByteArray(textWidth(prefix), ' '),
                    items.mid(1),
                    suffix,
                    limit
                );
            } else {
                const QByteArray pad = document->indentOf(line) + "        ";
                lines << prefix;
                lines
                    << this->fill(
                        pad + items.first(),
                        pad,
                        items.mid(1),
                        suffix,
                        limit
                    );
            }
            const QByteArray newline = document->text().mid(end, 1) == "\r"
                ? "\r\n" : "\n";
            QByteArray wrapped;
            for (int k = 0; k < lines.size(); ++k) {
                wrapped += (k > 0 ? newline : QByteArray()) + lines.at(k);
            }
            if (wrapped != document->text().mid(start, end - start)) {
                document->replace(start, end - start, wrapped);
            }
            break;
        }
    }
}

/*!
    Tells whether the `(` at \a paren of \a document opens the parameter list
    of a function's declaration or definition, at file, namespace or class
    scope: its name follows a type, or it is a constructor or destructor.
*/
bool FormatParamWrapRule::isDeclarationParen(const FormatDocument *document,
                                             int paren) const
{
    const FormatDocument::Scope scope = document->scopeOf(paren);
    if ((scope != FormatDocument::FileScope && scope != FormatDocument::NamespaceScope
        && scope != FormatDocument::ClassScope)
        || document->token(paren - 1) != IDENTIFIER
    ) {
        return false;
    }
    int name = paren - 1;
    if (document->token(name - 1) == TILDE) {
        return true;
    }
    while (name >= 2
        && document->token(name - 1) == SCOPE
        && document->token(name - 2) == IDENTIFIER
    ) {
        name -= 2;
    }
    const Token typeEnd = document->token(name - 1);
    if (isTypeEnd(typeEnd) || typeEnd == AND) {
        return true;
    }
    if (name < paren - 1) {
        return document->lexem(paren - 1) == document->lexem(paren - 3);
    }
    const Token label = document->token(name - 2);
    const bool afterAccess = typeEnd == COLON
        && (label == PUBLIC || label == PRIVATE || label == PROTECTED || label == SLOTS
            || label == SIGNALS);
    return scope == FormatDocument::ClassScope
        && (typeEnd == SEMIC || typeEnd == LBRACE || typeEnd == RBRACE || afterAccess
            || typeEnd == EXPLICIT || typeEnd == INLINE);
}

/*!
    Builds the text of each parameter between the brackets at \a open and
    \a close of \a document, split at the commas outside of nested brackets
    and template arguments.
*/
QList<QByteArray > FormatParamWrapRule::parameters(const FormatDocument *document, int open,
                                                   int close) const
{
    QList<QByteArray > items;
    int from = open + 1;
    int depth = 0;
    for (int i = open + 1; i <= close; ++i) {
        const Token token = document->token(i);
        if ((token == LPAREN
            || token == LBRACK
            || token == LBRACE)
            && i != close
        ) {
            i = qMax(i, document->match(i));
            continue;
        }
        depth += token == LANGLE
            ? 1 : (token == RANGLE ? -1 : (token == GTGT ? -2 : 0));
        if ((token != COMMA || depth > 0) && i != close) {
            continue;
        }
        const int start = document->symbols().at(from).from;
        items
            << document->text().mid(
                start,
                document->symbolEnd(i - 1) - start
            );
        from = i + 1;
    }
    return items;
}

/*!
    Builds the lines of a wrapped list that starts with \a first, followed by
    \a items, each continuation line starting with \a pad, and the last one
    ending with \a suffix; a line takes the next item for as long as it
    stays within \a limit columns.
*/
QList<QByteArray > FormatParamWrapRule::fill(const QByteArray & first, const QByteArray & pad,
                                             const QList<QByteArray > & items,
                                             const QByteArray & suffix, int limit) const
{
    QList<QByteArray > lines;
    QByteArray current = first;
    for (int i = 0; i < items.size(); ++i) {
        const QByteArray tail = i + 1 == items.size()
            ? suffix : QByteArray(",");
        const QByteArray candidate = current + ", " + items.at(i);
        if (textWidth(candidate + tail) <= limit) {
            current = candidate;
            continue;
        }
        lines << current + ",";
        current = pad + items.at(i);
    }
    lines << current + (items.isEmpty() ? suffix : QByteArray());
    if ( ! items.isEmpty()) {
        lines.last() += suffix;
    }
    return lines;
}
