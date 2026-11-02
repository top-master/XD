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
    @a indent of the line that opened its group or its statement.
*/
QByteArray continuationPad(int indent)
{
    return QByteArray(indent + 4, ' ');
}

/*!
    Tells whether @a token is a binary operator that may start a line.
*/
static bool isBinaryOperator(Token token)
{
    switch (token) {
    case PLUS: case MINUS: case STAR: case SLASH: case PERCENT:
    case LTLT: case GTGT: case AND: case OR: case HAT:
    case ANDAND: case OROR: case EQEQ: case NE:
    case LANGLE: case RANGLE: case LE: case GE:
    case QUESTION: case COLON:
        return true;
    default:
        return false;
    }
}

/*!
    Tells whether a line of the bracket group from the token @a open of
    @a document to its closing token @a close, which opens on a line indented
    by @a indent, sits at another indent than its brackets give it.

    The padding of a line counts one step (4 spaces)
    per bracket that a line above it left open, and taken away by the
    bracket's closing one on a later line, from that closing line on.
    Inside such a bracket, an argument that goes on past the end of its
    line adds one more step to its next lines, until the `,` that ends it,
    unless that line opened a bracket of its own, ends with a closing bracket
    (`)`, `]` or `}`) or a string literal, or starts with a string literal
    (see startsWithStringLiteral()); a bracket opened while an argument had
    that step keeps it for its own lines. A line that starts with a
    binary operator, right below another one that does, sits at that
    line's indent; hence the operator lines of one chain line up.
*/
bool hasMisplacedLines(const FormatDocument *document, int open, int close,
                       int indent)
{
    const int firstLine = document->symbolLine(open);
    const int lastLine = document->symbolLine(close);

    // Lists the brackets open at the current line: each one's closing token,
    // and whether the argument its content is at has the extra step.
    QList<QPair<int, bool > > ranges;
    int k = open;
    bool aboveLeadsWithOperator = false;
    int aboveIndent = -1;
    for (int l = firstLine; l <= lastLine; ++l) {
        // Closes the ranges whose closing brackets lead the line.
        while (k <= close && document->symbolLine(k) == l
            && ! ranges.isEmpty() && ranges.last().first == k
        ) {
            ranges.removeLast();
            ++k;
        }

        // Checks the line's indent: an operator line below another one
        // takes its indent, any other line the one its brackets give it.
        const int lead = document->firstSymbolOfLine(l);
        const bool leadsWithOperator = lead >= 0
            && isBinaryOperator(document->token(lead));
        if (l > firstLine && lead > open && lead <= close) {
            int wanted = aboveIndent;
            if ( ! leadsWithOperator
                || ! aboveLeadsWithOperator
                || aboveIndent < 0
            ) {
                int steps = 0;
                for (int r = 0; r < ranges.size(); ++r) {
                    steps += ranges.at(r).second ? 2 : 1;
                }
                wanted = indent + 4 * steps;
            }
            if (textWidth(document->indentOf(l)) != wanted) {
                return true;
            }
            aboveIndent = wanted;
        }
        aboveLeadsWithOperator = leadsWithOperator;

        // Opens and closes the ranges of the rest of the line.
        bool opened = false;
        int last = -1;
        for (; k <= close && document->symbolLine(k) == l; ++k) {
            const Token token = document->token(k);
            const int other = document->match(k);
            last = k;
            if ((token == LPAREN || token == LBRACK || token == LBRACE)
                && other > k && document->symbolLine(other) > l
            ) {
                ranges.append(qMakePair(other, false));
                opened = true;
            } else if ( ! ranges.isEmpty() && ranges.last().first == k) {
                ranges.removeLast();
            }
        }

        // Gives the argument that goes on past the line its extra step, or
        // takes it away at its `,`.
        if (last < 0 || ranges.isEmpty()) {
            continue;
        }
        const Token end = document->token(last);
        if (end == COMMA) {
            ranges.last().second = false;
        } else if ( ! opened && end != RPAREN && end != RBRACK && end != RBRACE
            && end != STRING_LITERAL
            && lead >= 0 && ! startsWithStringLiteral(document, lead)
        ) {
            ranges.last().second = true;
        }
    }
    return false;
}

/*!
    Tells whether the token @a token of @a document starts a string literal:
    a `"` string, or one with an encoding or raw prefix (`u8"`, `u"`, `U"`,
    `L"`, `R"`, and the like), which the tokenizer reads as a name right
    before the string.
*/
bool startsWithStringLiteral(const FormatDocument *document, int token)
{
    if (token < 0 || token >= document->symbols().size()) {
        return false;
    }
    if (document->token(token) == STRING_LITERAL) {
        return true;
    }
    static const char *const prefixes[] = {
        "u8", "u", "U", "L", "R", "u8R", "uR", "UR", "LR"
    };
    if (document->token(token) != IDENTIFIER
        || token + 1 >= document->symbols().size()
        || document->token(token + 1) != STRING_LITERAL
        || document->symbols().at(token + 1).from != document->symbolEnd(token)
    ) {
        return false;
    }
    const QByteArray name = document->lexem(token);
    for (size_t i = 0; i < sizeof(prefixes) / sizeof(prefixes[0]); ++i) {
        if (name == prefixes[i]) {
            return true;
        }
    }
    return false;
}

/*!
    Builds the text of @a document from the start of the token @a first up to
    the end of the token @a last, as the file spells it.
*/
QByteArray sourceText(const FormatDocument *document, int first, int last)
{
    if (last < first) {
        return QByteArray();
    }
    const int from = document->symbols().at(first).from;
    return document->text().mid(from, document->symbolEnd(last) - from);
}

/*!
    Finds the one `?` of a conditional expression among the tokens from
    @a first up to @a last of @a document, outside of every bracket pair, and
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
    the `<` at @a angle of @a document, before @a end; -1 when a `(`, `&&`,
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
    @a open and @a close of @a document, split at the commas outside of
    nested brackets and template arguments (a `<` after a name, closed
    before any `(`, `&&`, `||`, `?` or `;`; any other `<` compares); an
    empty list for no argument. A trailing comma (`{ a, b, }`) ends the last
    argument, and adds no empty one.
*/
QList<QPair<int, int > > splitArguments(
        const FormatDocument *document,
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

    // Leaves out the empty item after a trailing comma (`{ a, b, }`).
    if (from <= close - 1) {
        arguments << qMakePair(from, close - 1);
    }
    return arguments;
}

/*!
    Builds the lines of a conditional expression whose part before the `?`
    is @a head, whose `?` part is @a middle, and whose `:` part is @a tail,
    written from the column @a column of a line indented by @a indent, with
    @a trailing characters after it on its last line: one line when that
    fits the @a wrap column; else the head, and the `?` and `:` parts on a
    continuation line (see continuationPad()); else each of those parts on a
    continuation line of its own.
*/
QList<QByteArray > ternaryLines(
        const QByteArray & head, const QByteArray & middle,
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
