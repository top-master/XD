// Copyright (C) 2015 The XD Company Ltd.
// SPDX-License-Identifier: Apache-2.0

#include "./connect-layout.h"

#include "../format-document.h"
#include "../parser/class-helpers.h"

#include <QtCore/QByteArray>
#include <QtCore/QList>
#include <QtCore/QString>

/*!
    \class FormatConnectLayoutRule
    \internal

    Lays a `connect()` or `disconnect()` call that does not fit one line out
    by pairs: the sender and its signal on the first line, the receiver and
    its slot on the next, and a connection type on a line of its own, each
    line starting under the first argument.
*/

/*!
    Rewrites the argument layout of each four- or five-argument `connect()`
    and `disconnect()` call of \a document whose arguments each fit one line.
*/
void FormatConnectLayoutRule::run(FormatDocument *document) const
{
    for (int i = 0; i + 1 < document->symbols().size(); ++i) {
        if (document->token(i) != IDENTIFIER || document->token(i + 1) != LPAREN
            || document->isDirectiveLine(document->symbolLine(i))
        ) {
            continue;
        }
        const QByteArray name = document->lexem(i);
        if (name != "connect" && name != "disconnect") {
            continue;
        }

        // Leaves out a declaration or a definition of such a function.
        const Token before = document->token(i - 1);
        if ((before == SCOPE && endsType(document->token(i - 3))) || (before != SCOPE
            && before != DOT
            && before != ARROW
            && endsType(before))
        ) {
            continue;
        }
        const int open = i + 1;
        const int close = document->match(open);
        if (close < 0 || document->hasCommentBetween(document->symbols().at(open).from,
                                                     document->symbols().at(close).from)
            || document->spansDirective(document->symbols().at(open).from,
                                        document->symbols().at(close).from)
        ) {
            continue;
        }

        // Splits the arguments at their top-level commas.
        QList<QByteArray > arguments;
        int from = open + 1;
        bool singleLines = true;
        for (int k = open + 1; k <= close; ++k) {
            const Token token = document->token(k);
            if ((token == LPAREN
                || token == LBRACK
                || token == LBRACE)
                && k != close
            ) {
                k = qMax(k, document->match(k));
                continue;
            }
            if (token != COMMA && k != close) {
                continue;
            }
            if (k == from) {
                singleLines = false;
                break;
            }
            const int start = document->symbols().at(from).from;
            const int end = document->symbolEnd(k - 1);
            const QByteArray argument = document->text().mid(
                start,
                end - start
            );
            singleLines = singleLines && ! argument.contains('\n');
            arguments.append(argument);
            from = k + 1;
        }
        const bool hasType = arguments.size() == 5
            && arguments.last().startsWith("Qt::");
        if ( ! singleLines || (arguments.size() != 4 && ! hasType)) {
            continue;
        }

        // Builds the layout: one line when it fits, else one line per pair.
        const int line = document->symbolLine(open);
        const int column = QString::fromUtf8(document->text().mid(
            document->lineStart(line),
            document->symbolEnd(open) - document->lineStart(line)
        )).size();
        const QByteArray pad(column, ' ');
        const QByteArray rest = document->text().mid(
            document->symbolEnd(close),
            contentEndOf(document, close) - document->symbolEnd(close)
        );
        QByteArray layout = arguments.at(0) + ", " + arguments.at(1) + ", " + arguments.at(2) + ", "
            + arguments.at(3);
        const int limit = document->settings().wrapMax;
        if (hasType || column + layout.size() + 1 + rest.size() > limit) {
            // Ends each pair with a comma and the line break that ends the
            // call's first line, `\r\n` in a file of CRLF line endings.
            const QByteArray pairEnd = document->text().mid(
                contentEndOf(document, open),
                1
            ) == "\r"
                ? QByteArray(",\r\n") : QByteArray(",\n");
            layout = arguments.at(0) + ", " + arguments.at(1) + pairEnd + pad + arguments.at(2) + ", "
                + arguments.at(3);
            if (hasType) {
                layout += pairEnd + pad + arguments.at(4);
            }
        }
        const int start = document->symbolEnd(open);
        const int end = document->symbols().at(close).from;
        if (document->text().mid(start, end - start) != layout) {
            document->replace(start, end - start, layout);
        }
        i = close;
    }
}
