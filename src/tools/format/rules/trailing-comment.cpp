// Copyright (C) 2015 The XD Company Ltd.
// SPDX-License-Identifier: Apache-2.0

#include "./trailing-comment.h"

#include "../format-document.h"

#include <QtCore/QByteArray>
#include <QtCore/QList>
#include <QtCore/QString>

/*!
    @class FormatTrailingCommentRule
    @internal

    Moves a comment that trails a line of code onto a line of its own, right
    above that line, at its indent; the label of a closing line (`} //
    namespace`, `#endif // <macro>`) stays.
*/

/*!
    Moves the trailing comment of @a group above its line; a member's
    trailing doc-comment (`///<`) becomes a leading one (`///`).
*/
void FormatTrailingCommentRule::runGroup(
        FormatDocument *document, const QList<int > & group) const
{
    const int index = group.first();
    const FormatComment & comment = document->comments().at(index);
    if ( ! comment.trailing) {
        return;
    }
    const int line = document->commentLine(index);
    const int end = comment.offset + comment.length;
    const int lineEnd = document->lineEnd(line);
    if (document->commentEndLine(index) != line) {
        document->report(
            comment.offset,
            "trailing-comment",
            QString(QLL("move the comment onto the line above its code"))
        );
        return;
    }
    if ( ! document->isOnlyWhitespace(end, lineEnd)) {
        return;
    }
    const QByteArray code = document->lineText(line).trimmed();
    if (code.endsWith('\\')) {
        return;
    }

    // Keeps it in place inside a list that the wrapping rules lay out, where
    // the line it trails is the parameter or argument line it belongs to.
    for (int i = 0; i < document->symbols().size(); ++i) {
        const Token token = document->token(i);
        if (token != LPAREN && token != LBRACK) {
            continue;
        }
        const int close = document->match(i);
        if (close > i
            && document->symbols().at(i).from < comment.offset
            && document->symbols().at(close).from
                >= comment.offset + comment.length
        ) {
            return;
        }
    }

    // Keeps it on a line that continues the one above it, such as the
    // qualifier of a wrapped signature.
    if (line > 0) {
        const QByteArray above = document->lineText(line - 1).trimmed();
        if ( ! above.isEmpty()
            && ! above.endsWith(';')
            && ! above.endsWith('{')
            && ! above.endsWith('}')
            && ! above.endsWith(':')
            && ! above.startsWith('#')
        ) {
            return;
        }
    }

    // Turns a trailing doc marker into a leading one.
    QByteArray text = document->commentText(index);
    if (text.startsWith("///<")
        || text.startsWith("//!<")
        || text.startsWith("/**<")
        || text.startsWith("/*!<")
    ) {
        text.remove(3, 1);
    }

    // Removes the comment with the spaces before it, and puts it above.
    int from = comment.offset;
    while (from > document->lineStart(line)
        && (document->text().at(from - 1) == ' '
            || document->text().at(from - 1) == '\t')
    ) {
        --from;
    }
    const QByteArray newline = document->text().mid(lineEnd - 1, 1) == "\r"
        ? "\r\n" : "\n";
    document->promoteTrailingComment(
        from,
        end - from,
        document->lineStart(line),
        document->indentOf(line) + text + newline
    );
}
