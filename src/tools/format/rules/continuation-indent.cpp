// Copyright (C) 2015 The XD Company Ltd.
// SPDX-License-Identifier: Apache-2.0

#include "./continuation-indent.h"

#include "../format-document.h"
#include "../parser/code-helpers.h"

#include <QtCore/QByteArray>

/*!
    @class FormatContinuationIndentRule
    @internal

    Indents each line that an operator leads one step (4 spaces) past the
    line that opened its group: the line of the `(` of a condition or of a
    parenthesized group, or else the statement's first line. A line whose
    operator binds tighter than that of an earlier operator-led line of the
    group (a `!=` below a `||`) continues that line's operand, one step past
    it. A line inside a call's arguments is left as it is, since those lines
    align under the call's first argument.
*/

/*!
    Finds the group that the token at @a index of @a document belongs to:
    the innermost `(` or `[` still open before it, else the start of its
    statement. Returns that bracket's index, or -1 for the statement, and
    gives the line that opened the group to @a base; sets @a keepsIndent for
    a group whose lines align under its first item (the arguments of a call,
    an index, or an initializer list).
*/
static int groupOf(const FormatDocument *document, int index, int *base,
                   bool *keepsIndent)
{
    *base = -1;
    *keepsIndent = false;
    int k = index - 1;
    for (; k >= 0; --k) {
        const Token token = document->token(k);
        if ((token == RPAREN
            || token == RBRACK)
            && document->match(k) >= 0
        ) {
            k = document->match(k);
            continue;
        }
        if (token == LPAREN || token == LBRACK) {
            const Token opener = document->token(k - 1);
            *keepsIndent = token == LBRACK || opener == IDENTIFIER || opener == RANGLE
                || opener == GTGT || opener == RBRACK || opener == RPAREN
                || opener == STATIC_CAST || opener == SIZEOF;
            *base = document->symbolLine(k);
            return k;
        }
        if (token == LBRACE && document->braceScope(k) != FormatDocument::FunctionScope
            && document->braceScope(k) != FormatDocument::BlockScope
        ) {
            *keepsIndent = document->braceScope(k) == FormatDocument::InitScope
                || document->braceScope(k) == FormatDocument::EnumScope;
        }
        if (token == SEMIC || token == LBRACE || token == RBRACE) {
            *base = document->symbolLine(k + 1);
            return -1;
        }
    }
    *base = document->symbolLine(0);
    return -1;
}

/*!
    Builds how tightly the operator @a token that leads a line binds: as
    operatorPrecedence() ranks it, with a `?` loosest of all.
*/
static int leadRank(Token token)
{
    return token == QUESTION ? 0 : operatorPrecedence(token);
}

/*!
    Gives each operator-led continuation line of @a document the indent of
    its group's line plus 4 spaces; a line whose operator binds tighter than
    the one that leads an earlier line of the group continues that line's
    operand, one step past it.
*/
void FormatContinuationIndentRule::run(FormatDocument *document) const
{
    for (int line = 2; line <= document->lineCount(); ++line) {
        const int first = document->firstSymbolOfLine(line);
        if (first < 1 || ! isLeadingOperator(document->token(first))
            || document->isDirectiveLine(line) || document->symbolEndLine(first - 1) >= line
        ) {
            continue;
        }
        // Leaves out a statement's start, and a sign (`-1` after a comma).
        const Token before = document->token(first - 1);
        if (before == SEMIC || before == LBRACE || before == RBRACE || before == COLON
            || before == COMMA || before == EQ || before == LPAREN || before == RETURN
            || isLeadingOperator(before)
        ) {
            continue;
        }

        // Finds the group's line, or the statement's start.
        int base = -1;
        bool keepsIndent = false;
        const int group = groupOf(document, first, &base, &keepsIndent);
        if (keepsIndent || base < 1 || base >= line) {
            continue;
        }
        QByteArray indent = document->indentOf(base) + "    ";

        // Continues the operand of an earlier line of the group whose
        // operator binds looser, or takes the level of one that binds alike.
        const int rank = leadRank(document->token(first));
        for (int earlier = line - 1; earlier > base; --earlier) {
            const int lead = document->firstSymbolOfLine(earlier);
            int earlierBase = -1;
            bool earlierKeeps = false;
            if (lead < 0 || ! isLeadingOperator(document->token(lead))
                || groupOf(document, lead, &earlierBase, &earlierKeeps) != group
                || earlierBase != base
            ) {
                continue;
            }
            const int earlierRank = leadRank(document->token(lead));
            if (earlierRank < rank) {
                indent = document->indentOf(earlier) + "    ";
                break;
            }
            if (earlierRank == rank) {
                indent = document->indentOf(earlier);
                break;
            }
        }
        const QByteArray current = document->indentOf(line);
        if (current != indent) {
            document->replace(document->lineStart(line), current.size(), indent);
        }
    }
}
