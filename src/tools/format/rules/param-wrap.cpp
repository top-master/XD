// Copyright (C) 2015 The XD Company Ltd.
// SPDX-License-Identifier: Apache-2.0

#include "./param-wrap.h"

#include "../format-document.h"
#include "../parser/code-helpers.h"

#include <QtCore/QByteArray>
#include <QtCore/QList>
#include <QtCore/QString>

/*!
    @class FormatParamWrapRule
    @internal

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
                || document->token(i + 1) == RPAREN || ! this->isDeclarationParen(
                    document,
                    i
                )
                || document->hasCommentBetween(
                    document->symbolEnd(i),
                    document->symbols().at(close).from
                )
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
    this->regroup(document, &conditions);
}

/*!
    Lays out again each declaration of @a document whose parameter list the
    user broke over lines, keeping the parameters that share a line on one
    line. Groups that fit under the first parameter, lined up right, stay as
    they are. Otherwise the list breaks at its `(`, each group on a line of
    its own two steps past the declaration's line, when every group fits
    that way; when one does not, the groups stay under the first parameter,
    and the last parameters of a group that does not fit go on to further
    lines. Where `[Structure] wrapArgsAt` holds at the `(` (as @a conditions
    evaluates it), the list breaks at its `(` in every case.

    It leaves alone a declaration with fewer than 2 parameters, a parameter
    that spans lines, a comment, or a constructor's initializer list.
*/
void FormatParamWrapRule::regroup(
        FormatDocument *document,
        FormatScopeConditions *conditions) const
{
    const int limit = document->settings().wrapMax;
    for (int i = 1; i < document->symbols().size(); ++i) {
        const int close = document->match(i);
        if (document->token(i) != LPAREN || close <= i + 1
            || document->symbolLine(close) == document->symbolLine(i)
            || document->token(close + 1) == COLON
            || ! this->isDeclarationParen(document, i)
            || document->hasCommentBetween(
                document->symbolEnd(i),
                document->symbols().at(close).from
            )
        ) {
            continue;
        }

        // Takes the parameters, each on one line, grouped by their lines.
        const QList<QPair<int, int > > items = splitArguments(
            document,
            i,
            close
        );
        bool isWhole = items.size() >= 2;
        QList<QByteArray > texts;
        QList<int > lines;
        for (int k = 0; k < items.size() && isWhole; ++k) {
            const QPair<int, int > & item = items.at(k);
            isWhole = document->symbolLine(item.first)
                == document->symbolEndLine(item.second);
            texts << sourceText(document, item.first, item.second);
            lines << document->symbolLine(item.first);
        }
        if ( ! isWhole) {
            i = close;
            continue;
        }
        QList<QList<QByteArray > > groups;
        for (int k = 0; k < texts.size(); ++k) {
            if (k == 0 || lines.at(k) != lines.at(k - 1)) {
                groups << QList<QByteArray >();
            }
            groups.last() << texts.at(k);
        }

        // Finds the declaration's head and tail around its parameters.
        const int line = document->symbolLine(i);
        const int start = document->lineStart(line);
        const int closeLine = document->symbolLine(close);
        const int end = contentEnd(document, closeLine);
        const QByteArray prefix = document->text().mid(
            start,
            document->symbolEnd(i) - start
        );
        const QByteArray suffix = document->text().mid(
            document->symbols().at(close).from,
            end - document->symbols().at(close).from
        );
        const qreal percent = textWidth(prefix) * 100.0 / qMax(1, limit);
        const bool mustBreak = conditions->holds(
            document->settings().wrapArgsAt, i,
            percent,
            FormatSettings::defaultWrapChainAt()
        );

        // Keeps the groups under the first parameter when they fit there and
        // stand right; else breaks at the `(` when the groups fit that way;
        // else moves the last parameters of a group that does not fit to a
        // line of their own, under the first parameter (or, where `wrapArgsAt`
        // holds, from the next line).
        const QByteArray breakPad = document->indentOf(line) + "        ";
        bool fitsUnder = false;
        bool fitsBroken = false;
        const QList<QByteArray > under = this->groupedLines(
            prefix, groups, suffix, false,
            breakPad, limit, &fitsUnder
        );
        const QList<QByteArray > broken = this->groupedLines(
            prefix, groups, suffix, true,
            breakPad, limit, &fitsBroken
        );
        if ( ! mustBreak && fitsUnder
            && under.join("\n") == document->text().mid(start, end - start)
        ) {
            i = close;
            continue;
        }
        const QList<QByteArray > out = fitsBroken || mustBreak
            ? broken : under;
        const QByteArray newline = document->text().mid(end, 1) == "\r"
            ? "\r\n" : "\n";
        QByteArray wrapped;
        for (int k = 0; k < out.size(); ++k) {
            wrapped += (k > 0 ? newline : QByteArray()) + out.at(k);
        }
        if (wrapped != document->text().mid(start, end - start)) {
            document->replace(start, end - start, wrapped);
        }
        i = close;
    }
}

/*!
    Builds the lines of a parameter list whose line groups are @p groups,
    between the declaration's @p prefix (up to its `(`) and @p suffix (from
    its `)`): with @p breaksAfterOpen, the `(` ends the first line and each
    group takes a line of its own at @p breakPad; else the first group
    follows the `(`, and the others line up under it. A group whose line
    passes @p limit keeps as many parameters as fit, and its last ones go on
    to further lines at the same padding. Sets @p fits when every group fits
    on one line of its own.
*/
QList<QByteArray > FormatParamWrapRule::groupedLines(
        const QByteArray & prefix, const QList<QList<QByteArray > > & groups,
        const QByteArray & suffix, bool breaksAfterOpen,
        const QByteArray & breakPad,
        int limit, bool *fits) const
{
    const QByteArray pad = breaksAfterOpen
        ? breakPad : QByteArray(textWidth(prefix), ' ');
    QList<QByteArray > lines;
    if (breaksAfterOpen) {
        lines << prefix;
    }
    *fits = true;
    for (int g = 0; g < groups.size(); ++g) {
        const bool isLastGroup = g + 1 == groups.size();
        const QByteArray lead = (g == 0 && ! breaksAfterOpen)
            ? prefix : pad;
        const QList<QByteArray > filled = this->fill(
            lead + groups.at(g).first(),
            pad,
            groups.at(g).mid(1),
            isLastGroup ? suffix : QByteArray(","),
            limit
        );
        *fits = *fits
            && filled.size() == 1
            && textWidth(filled.first()) <= limit;
        lines << filled;
    }
    return lines;
}

/*!
    Tells whether the `(` at @a paren of @a document opens the parameter list
    of a function's declaration or definition, at file, namespace or class
    scope: its name follows a type, or it is a constructor or destructor.
*/
bool FormatParamWrapRule::isDeclarationParen(
        const FormatDocument *document,
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
    Builds the text of each parameter between the brackets at @a open and
    @a close of @a document, split at the commas outside of nested brackets
    and template arguments.
*/
QList<QByteArray > FormatParamWrapRule::parameters(
        const FormatDocument *document, int open,
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
            << document->text().mid(start, document->symbolEnd(i - 1)
                - start
            );
        from = i + 1;
    }
    return items;
}

/*!
    Builds the lines of a wrapped list that starts with @a first, followed by
    @a items, each continuation line starting with @a pad, and the last one
    ending with @a suffix; a line takes the next item for as long as it
    stays within @a limit columns.
*/
QList<QByteArray > FormatParamWrapRule::fill(
        const QByteArray & first, const QByteArray & pad,
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
