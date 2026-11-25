// Copyright (C) 2015 The XD Company Ltd.
// SPDX-License-Identifier: Apache-2.0

#include "./param-wrap.h"

#include "../format-document.h"
#include "../parser/code-helpers.h"
#include "../parser/expression-layout.h"
#include "../parser/layout-helpers.h"
#include "../parser/scope-conditions.h"

#include <QtCore/QByteArray>
#include <QtCore/QList>
#include <QtCore/QPair>
#include <QtCore/QString>

/*!
    @class FormatParamWrapRule
    @internal

    Wraps the parameter list of a declaration or definition whose line runs
    past the column limit (`[Structure] wrapMax`, 80 by default). A list of
    more than `[Structure] wrapArgsOver` parameters (3 by default) starts
    on the next line, indented two steps past the line it starts on, and
    follows `[Structure] joinArgs`. A shorter list that needs to break follows
    `[Structure] joinArgs`, each line under the first parameter; it starts
    on the next line instead when even the first
    parameter does not fit under the `(`, or when `[Structure]
    wrapSignatureAt` holds at the `(` (whose `spaceUsed` is the share of
    `wrapMax` up to after the `(`, as for `wrapArgsAt` of a call, see
    FormatArgsWrapRule). A function, a
    constructor, a destructor, and an operator count alike, at file,
    namespace, or class scope. A list the user already broke over lines
    keeps its line groups (see regroup()).
*/

/*!
    Rewraps each declaration line past the column limit whose whole
    parameter list sits on that line.
*/
void FormatParamWrapRule::run(FormatDocument *document) const
{
    FormatScopeConditions conditions(document);
    // Find declaration lines that exceed their configured width.
    for (int line = 1; line <= document->lineCount(); ++line) {
        const FormatFileConfig &fileConfig = document->configAt(
            document->lineStart(line)
        );
        const int limit = wrapMaxAt(document, document->lineStart(line));
        const int first = document->firstSymbolOfLine(line);
        // Leave empty lines, directives, and lines within the width alone.
        if (first < 0 || document->isDirectiveLine(line)) {
            continue;
        }
        const int last = document->lastSymbolOfLine(line);
        // Find an eligible parameter list on this line.
        for (int i = first; i <= last; ++i) {
            const int close = document->match(i);
            if (document->token(i) != LPAREN
                || close < 0
                || document->token(i + 1) == RPAREN
                || ! isDeclarationParen(document, i)
            ) {
                continue;
            }
            const bool hasComments = document->hasCommentBetween(
                document->symbolEnd(i),
                document->symbols().at(close).from
            );
            if ((close > last
                    && ! hasComments)
                || document->spansDirective(
                    document->symbols().at(i).from,
                    document->symbolEnd(close)
                )
            ) {
                continue;
            }
            bool longLine = false;
            const int lastLine = hasComments ? document->symbolLine(close)
                                             : line;
            for (int l = line; l <= lastLine; ++l) {
                longLine = longLine || textWidth(document->lineText(l)) > limit;
            }
            if ( ! longLine) {
                continue;
            }
            // Leaves a constructor line with its initializer list to the
            // initializer-list layout.
            if (close + 1 < document->symbols().size()
                && document->token(close + 1) == COLON
                && document->symbolLine(close + 1) == line
            ) {
                break;
            }

            // Leaves a line that is long for its one-line body, and
            // not for its parameters.
            bool hasBody = false;
            // Look for a body after the parameter list on the same line.
            for (int k = close + 1; k <= last && ! hasBody; ++k) {
                hasBody = document->token(k) == LBRACE;
            }
            if (hasBody && ! hasComments) {
                break;
            }
            const QList<QPair<int, int > > items =
                splitArguments(document, i, close);
            if (items.isEmpty() || (hasComments && items.size() < 2)) {
                continue;
            }
            const int start = document->lineStart(line);
            int end = contentEnd(document, document->symbolLine(close));
            QByteArray prefix = document->text().mid(
                start,
                document->symbolEnd(i) - start
            );
            QByteArray suffix = document->text().mid(
                document->symbols().at(close).from,
                end - document->symbols().at(close).from
            );
            if (hasComments && items.size() >= 2) {
                int name = i - 1;
                while (document->token(name - 1) == SCOPE
                    && document->token(name - 2) == IDENTIFIER
                ) {
                    name -= 2;
                }
                const int nameFrom = document->symbols().at(name).from;
                const QByteArray indent = document->indentOf(line);
                const QByteArray head =
                    document->text().mid(start, nameFrom - start).trimmed();
                prefix = indent;
                if ( ! head.isEmpty()) {
                    prefix += head + "\n" + indent;
                }
                prefix += document->text().mid(
                    nameFrom,
                    document->symbolEnd(i) - nameFrom
                );
                int brace = close + 1;
                while (brace < document->symbols().size()
                    && document->symbolLine(brace)
                        == document->symbolLine(close)
                    && document->token(brace) != LBRACE
                    && document->token(brace) != SEMIC
                ) {
                    if (document->token(brace) == LPAREN
                        && document->match(brace) > brace
                    ) {
                        brace = document->match(brace);
                    }
                    ++brace;
                }
                if (document->token(brace) == LBRACE
                    && document->symbolLine(brace)
                        == document->symbolLine(close)
                ) {
                    end = document->symbols().at(brace).from;
                }
                const QByteArray after =
                    document->text()
                    .mid(
                        document->symbolEnd(close),
                        end - document->symbolEnd(close)
                    ).trimmed();
                bool lineComment = false;
                for (int c = 0; c < document->comments().size(); ++c) {
                    const FormatComment &comment = document->comments().at(c);
                    if (comment.offset >= document->symbolEnd(i)
                        && comment.offset < document->symbols().at(close).from
                        && comment.isLine()
                        && document->isOnlyWhitespace(
                            comment.offset + comment.length,
                            document->symbols().at(close).from
                        )
                    ) {
                        lineComment = true;
                    }
                }
                suffix = ")";
                if ( ! after.isEmpty()) {
                    if (after.startsWith(';')) {
                        suffix += after;
                    } else if (lineComment) {
                        suffix += " " + after;
                    } else {
                        suffix += "\n" + indent + "    " + after;
                    }
                }
                if (end != contentEnd(
                    document,
                    document->symbolLine(close)
                )) {
                    suffix += "\n" + indent;
                }
            }

            // Starts a long parameter list on the next line; shorter lists
            // may start under the first parameter or on the next line.
            QList<QByteArray > lines;
            FormatExpressionLayout layout(document, &conditions, 0);
            const QByteArray head = prefix + sourceText(
                    document,
                    items.first().first,
                    items.first().second)
                + (items.size() == 1 ? suffix : ",");
            const bool breaksAfterOpen = conditions.holds(
                QString(QLL("structure.wrapSignatureAt")),
                fileConfig.wrapSignatureAt,
                i,
                FormatFileConfig::schema()
                    ->programFor(QString(QLL("structure.wrapSignatureAt")))
            );
            // Choose the layout from the argument count and available width.
            if (hasComments || conditions.argsOver(i, items.size())) {
                const QByteArray pad = document->indentOf(line) + "        ";
                bool fits = false;
                lines = layout.layoutDeclaration(
                    i,
                    prefix,
                    suffix,
                    true,
                    true,
                    pad,
                    limit,
                    &fits
                );
            } else if (textWidth(head) <= limit && ! breaksAfterOpen) {
                bool fits = false;
                lines = layout.layoutDeclaration(
                    i,
                    prefix,
                    suffix,
                    false,
                    false,
                    QByteArray(textWidth(prefix), ' '),
                    limit,
                    &fits
                );
            } else {
                const QByteArray pad = document->indentOf(line) + "        ";
                bool fits = false;
                lines = layout.layoutDeclaration(
                    i,
                    prefix,
                    suffix,
                    true,
                    false,
                    pad,
                    limit,
                    &fits
                );
            }
            const int closeLineEnd = document->lineEnd(
                document->symbolLine(close)
            );
            const QByteArray newline =
                document->text().mid(closeLineEnd - 1, 1) == "\r"
                ? "\r\n" : "\n";
            QByteArray wrapped;
            // Assemble the replacement using the original newline style.
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
    user broke over lines, keeping the parameters that share a line on one line.
    Groups that fit under the first parameter, lined up right, stay as they are.
    Otherwise the list breaks at its `(`, each group on a line of its own
    two steps past the declaration's line, when every group fits that way;
    when one does not, the groups stay under the first parameter, and the
    parameters from the first group that does not fit follow `[Structure] joinArgs`.
    Where `[Structure] wrapSignatureAt` holds at the `(` (as
    @a conditions evaluates it), the list breaks at its `(` in every case.

    It leaves alone a declaration with fewer than 2 parameters, a parameter
    that spans lines, a comment, or a constructor's initializer list.
*/
void FormatParamWrapRule::regroup(
        FormatDocument *document, FormatScopeConditions *conditions) const
{
    // Find declarations whose parameter lists already span lines.
    for (int i = 1; i < document->symbols().size(); ++i) {
        const FormatFileConfig &fileConfig = document->configAt(
            document->symbols().at(i).from
        );
        const int limit = wrapMaxAt(
            document,
            document->symbols().at(i).from
        );
        const int close = document->match(i);
        // Leave ineligible lists and lists containing comments alone.
        if (document->token(i) != LPAREN
            || close <= i + 1
            || document->symbolLine(close) == document->symbolLine(i)
            || (close + 1 < document->symbols().size()
                && document->token(close + 1) == COLON
                && document->symbolLine(close + 1)
                    == document->symbolLine(close)
            )
            || ! isDeclarationParen(document, i)
            || document->hasCommentBetween(
                document->symbolEnd(i),
                document->symbols().at(close).from
            )
        ) {
            continue;
        }

        // Takes the parameters, each on one line, grouped by their lines.
        const QList<QPair<int, int > > items =
            splitArguments(document, i, close);
        bool isWhole = items.size() >= 2;
        // Reject parameters that span lines.
        for (int k = 0; k < items.size() && isWhole; ++k) {
            const QPair<int, int > & item = items.at(k);
            isWhole = document->symbolLine(item.first)
                == document->symbolEndLine(item.second);
        }
        if ( ! isWhole) {
            i = close;
            continue;
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
        const bool mustBreak = conditions->holds(
            QString(QLL("structure.wrapSignatureAt")),
            fileConfig.wrapSignatureAt,
            i,
            FormatFileConfig::schema()
                ->programFor(QString(QLL("structure.wrapSignatureAt")))
        );

        // Keeps the groups under the first parameter when they fit there and
        // stand right; else breaks at the `(` when the groups fit that way;
        // else lays out the parameters from the first group that does not fit
        // using `joinArgs`.
        const QByteArray breakPad = document->indentOf(line) + "        ";
        bool fitsUnder = false;
        bool fitsBroken = false;
        FormatExpressionLayout layout(document, conditions, 0);
        const int firstItemLine = document->symbolLine(items.first().first);
        QList<QByteArray > out;
        // Keep the break after the opening regardless of how
        // many groups follow.
        if (line < firstItemLine) {
            out = layout.layoutDeclaration(
                i,
                prefix,
                suffix,
                true,
                false,
                breakPad,
                limit,
                &fitsBroken
            );
        } else {
            const QList<QByteArray > under = layout.layoutDeclaration(
                i,
                prefix,
                suffix,
                false,
                false,
                QByteArray(textWidth(prefix), ' '),
                limit,
                &fitsUnder
            );
            const QList<QByteArray > broken = layout.layoutDeclaration(
                i,
                prefix,
                suffix,
                true,
                false,
                breakPad,
                limit,
                &fitsBroken
            );
            // Leave an already aligned layout unchanged when it fits.
            if ( ! mustBreak
                && fitsUnder
                && under.join("\n") == document->text().mid(
                    start,
                    end - start
                )
            ) {
                i = close;
                continue;
            }
            out = fitsBroken || mustBreak ? broken : under;
        }
        const QByteArray newline = document->text().mid(end, 1) == "\r" ? "\r\n"
                                                                        : "\n";
        QByteArray wrapped;
        // Assemble the selected layout using the original newline style.
        for (int k = 0; k < out.size(); ++k) {
            wrapped += (k > 0 ? newline : QByteArray()) + out.at(k);
        }
        if (wrapped != document->text().mid(start, end - start)) {
            document->replace(start, end - start, wrapped);
        }
        i = close;
    }
}
