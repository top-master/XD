// Copyright (C) 2015 The XD Company Ltd.
// SPDX-License-Identifier: Apache-2.0

#include "./continuation-indent.h"

#include "../format-document.h"
#include "../parser/code-helpers.h"
#include "../parser/layout-helpers.h"

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

    In an `if`, `while`, or the condition clause of a `for`, a non-logical
    operator leads its line two steps past the keyword's line: one step for
    the parentheses, and one step as an operand of the condition. In any
    nested group, each bracket opened on that group's line adds a step (see
    conditionSteps()). A preprocessor directive does not end a group whose
    matching close comes later, so the same padding applies across branches.
*/

static bool groupKeepsIndent(const FormatDocument *document, int group)
{
    const Token token = document->token(group);
    if (token == LBRACE) {
        const FormatDocument::Scope scope = document->braceScope(group);
        return scope == FormatDocument::InitScope
            || scope == FormatDocument::EnumScope;
    }
    if (token == LBRACK) {
        return true;
    }
    const Token opener = document->token(group - 1);
    return opener == IDENTIFIER || opener == RANGLE || opener == GTGT
        || opener == RBRACK || opener == RPAREN || opener == STATIC_CAST
        || opener == SIZEOF;
}

/*!
    Finds the group that the token at @a index of @a document belongs to:
    the innermost `(` or `[` still open before it, or a braced initializer,
    else the start of its statement. A preprocessor line ends the search
    unless a matching open bracket spans it. Returns that bracket's index,
    or -1 for the statement, and gives the line that opened the group to
    @a base; sets @a keepsIndent for a group whose lines align under its
    first item.
*/
static int groupOf(const FormatDocument *document, int index, int *base,
                   bool *keepsIndent)
{
    *base = -1;
    *keepsIndent = false;
    int k = index - 1;
    for (; k >= 0; --k) {
        const Token token = document->token(k);
        if (document->isDirectiveLine(document->symbolLine(k))) {
            for (int group = k - 1; group >= 1; --group) {
                if (document->isDirectiveLine(document->symbolLine(group))) {
                    continue;
                }
                const Token groupToken = document->token(group);
                if ((groupToken == LPAREN
                        || groupToken == LBRACK
                        || groupToken == LBRACE)
                    && document->match(group) >= index
                ) {
                    *keepsIndent = groupKeepsIndent(document, group);
                    if (groupToken != LBRACE || *keepsIndent) {
                        *base = document->symbolLine(group);
                        return group;
                    }
                }
            }
            *base = document->symbolLine(k + 1);
            return -1;
        }
        if ((token == RPAREN
                || token == RBRACK)
            && document->match(k) >= 0
        ) {
            k = document->match(k);
            continue;
        }
        if (token == LPAREN || token == LBRACK) {
            *keepsIndent = groupKeepsIndent(document, k);
            *base = document->symbolLine(k);
            return k;
        }
        if (token == LBRACE
            && document->braceScope(k) != FormatDocument::FunctionScope
            && document->braceScope(k) != FormatDocument::BlockScope
        ) {
            *keepsIndent = groupKeepsIndent(document, k);
        }
        if (token == SEMIC || token == LBRACE || token == RBRACE) {
            *base = document->symbolLine(k + 1);
            return -1;
        }
    }
    *base = document->symbolLine(0);
    return -1;
}

static bool isForCondition(const FormatDocument *document, int group, int index)
{
    if (document->token(group - 1) != FOR) {
        return false;
    }
    int semicolons = 0;
    for (int k = group + 1; k < index; ++k) {
        if (document->isDirectiveLine(document->symbolLine(k))) {
            continue;
        }
        const Token token = document->token(k);
        if ((token == LPAREN || token == LBRACK || token == LBRACE)
            && document->match(k) > k
        ) {
            k = document->match(k);
        } else if (token == SEMIC) {
            ++semicolons;
        }
    }
    return semicolons == 1;
}

static int conditionSteps(const FormatDocument *document, int group, int base,
                          int index)
{
    int steps = 0;
    int bracket = group;
    while (bracket >= 1 && document->token(bracket) == LPAREN) {
        if (document->symbolLine(bracket) == base) {
            ++steps;
        }
        const Token opener = document->token(bracket - 1);
        if (opener == IF || opener == WHILE
            || (opener == FOR && isForCondition(document, bracket, index))
        ) {
            if (bracket != group
                && document->symbolLine(bracket) != base
                && steps > 0
            ) {
                --steps;
            }
            return bracket == group ? 0 : steps;
        }
        int outerBase = -1;
        bool keepsIndent = false;
        bracket = groupOf(document, bracket, &outerBase, &keepsIndent);
        if (keepsIndent) {
            return 0;
        }
    }
    return 0;
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
        if (first < 1
            || ! isLeadingOperator(document->token(first))
            || document->isDirectiveLine(line)
            || document->symbolEndLine(first - 1) >= line
        ) {
            continue;
        }
        // Leaves out a statement's start, and a sign (`-1` after a comma).
        const Token before = document->token(first - 1);
        if (before == SEMIC
            || before == LBRACE
            || before == RBRACE
            || before == COLON
            || before == COMMA
            || before == EQ
            || before == LPAREN
            || before == RETURN
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
        const Token opener = group >= 1
            ? document->token(group - 1) : NOTOKEN;
        const Token leading = document->token(first);
        const bool isCondition = opener == IF || opener == WHILE
            || (opener == FOR && isForCondition(document, group, first));
        const bool isOperand = isCondition
            && leading != ANDAND
            && leading != OROR;
        QByteArray indent = document->indentOf(base)
            + continuationPad(0, isOperand ? 2 : 1);

        // Gives a group nested in a condition one step for each of its
        // brackets that the group's line opened.
        const int steps = conditionSteps(document, group, base, first);
        if (steps > 0) {
            indent = document->indentOf(base) + continuationPad(0, steps);
        }

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
                indent = document->indentOf(earlier) + continuationPad(0);
                break;
            }
            if (earlierRank == rank) {
                indent = document->indentOf(earlier);
                break;
            }
        }
        const QByteArray current = document->indentOf(line);
        if (current != indent) {
            document
                ->replace(document->lineStart(line), current.size(), indent);
        }
    }
}
