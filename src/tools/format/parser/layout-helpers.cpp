// Copyright (C) 2015 The XD Company Ltd.
// SPDX-License-Identifier: Apache-2.0

#include "./layout-helpers.h"

#include "../format-document.h"
#include "./code-helpers.h"

/*!
    Builds the column the code wrap rules break a line at: the first
    `[Structure] wrapBegin` column, never past `[Structure] wrapMax`; a line
    wider than it does not fit.
*/
int wrapColumn(const FormatDocument *document)
{
    const FormatSettings & settings = document->settings();
    const int begin = settings.wrapBegin.isEmpty()
        ? settings.wrapMax : settings.wrapBegin.first();
    return qMin(begin, settings.wrapMax);
}

/*!
    Builds the indent of a continuation line: one step (4 spaces) past the
    \a indent of the line that opened its group or its statement.
*/
QByteArray continuationPad(int indent)
{
    return QByteArray(indent + 4, ' ');
}

/*!
    Builds the text of \a document from the start of the token \a first up to
    the end of the token \a last, as the file spells it.
*/
QByteArray sourceText(const FormatDocument *document, int first, int last)
{
    const int from = document->symbols().at(first).from;
    return document->text().mid(from, document->symbolEnd(last) - from);
}

/*!
    Finds the one `?` of a conditional expression among the tokens from
    \a first up to \a last of \a document, outside of every bracket pair, and
    the first `:` after it; returns false when there is no such `?`, or more
    than one (a nested conditional expression), or no `:`.
*/
bool findTernary(const FormatDocument *document, int first, int last,
                 int *question, int *colon)
{
    *question = -1;
    *colon = -1;
    for (int k = first; k <= last; ++k) {
        const Token token = document->token(k);
        if ((token == LPAREN
            || token == LBRACK
            || token == LBRACE)
            && document->match(k) > k
        ) {
            k = document->match(k);
        } else if (token == QUESTION) {
            if (*question >= 0) {
                return false;
            }
            *question = k;
        } else if (token == COLON && *question >= 0 && *colon < 0) {
            *colon = k;
        }
    }
    return *question >= 0 && *colon >= 0;
}

/*!
    Builds the index of the `>` that closes the template arguments opened by
    the `<` at \a angle of \a document, before \a end; -1 when a `(`, `&&`,
    `||`, `?` or `;` comes first, hence the `<` compares.
*/
static int templateClose(const FormatDocument *document, int angle, int end)
{
    int depth = 0;
    for (int i = angle; i < end; ++i) {
        const Token token = document->token(i);
        if (token == LPAREN || token == ANDAND || token == OROR || token == QUESTION
            || token == SEMIC
        ) {
            return -1;
        }
        if (token == LANGLE) {
            ++depth;
        } else if (token == RANGLE) {
            --depth;
        } else if (token == GTGT) {
            depth -= 2;
        }
        if (depth <= 0) {
            return i;
        }
    }
    return -1;
}

/*!
    Builds the first and last token of each argument between the brackets at
    \a open and \a close of \a document, split at the commas outside of
    nested brackets and template arguments (a `<` after a name, closed
    before any `(`, `&&`, `||`, `?` or `;`; any other `<` compares); an
    empty list for no argument.
*/
QList<QPair<int, int > > splitArguments(const FormatDocument *document,
                                        int open, int close)
{
    QList<QPair<int, int > > arguments;
    if (close <= open + 1) {
        return arguments;
    }
    int from = open + 1;
    for (int i = open + 1; i < close; ++i) {
        const Token token = document->token(i);
        if ((token == LPAREN || token == LBRACK || token == LBRACE)
            && document->match(i) > i
        ) {
            i = document->match(i);
            continue;
        }
        if (token == LANGLE && document->token(i - 1) == IDENTIFIER) {
            i = qMax(i, templateClose(document, i, close));
            continue;
        }
        if (token == COMMA) {
            arguments << qMakePair(from, i - 1);
            from = i + 1;
        }
    }
    arguments << qMakePair(from, close - 1);
    return arguments;
}

/*!
    Builds the lines of a conditional expression whose part before the `?`
    is \a head, whose `?` part is \a middle, and whose `:` part is \a tail,
    written from the column \a column of a line indented by \a indent, with
    \a trailing characters after it on its last line: one line when that
    fits the \a wrap column; else the head, and the `?` and `:` parts on a
    continuation line (see continuationPad()); else each of those parts on a
    continuation line of its own.
*/
QList<QByteArray > ternaryLines(const QByteArray & head, const QByteArray & middle,
                                const QByteArray & tail, int indent, int column, int trailing,
                                int wrap)
{
    const QByteArray oneLine = head + ' ' + middle + ' ' + tail;
    if (column + textWidth(oneLine) + trailing <= wrap) {
        return QList<QByteArray >() << oneLine;
    }
    const QByteArray pad = continuationPad(indent);
    const QByteArray rest = pad + middle + ' ' + tail;
    if (textWidth(rest) + trailing <= wrap) {
        return QList<QByteArray >() << head << rest;
    }
    return QList<QByteArray >() << head << pad + middle << pad + tail;
}
