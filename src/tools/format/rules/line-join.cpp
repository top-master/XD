// Copyright (C) 2015 The XD Company Ltd.
// SPDX-License-Identifier: Apache-2.0

#include "./line-join.h"

#include "../format-document.h"
#include "../format-settings.h"
#include "../parser/code-helpers.h"
#include "../parser/layout-helpers.h"
#include "../parser/scope-conditions.h"

#include <QtCore/QByteArray>

/*!
    Tells whether the member chain ending at @a close starts with a
    parenthesized binary expression. Its member lines can be joined so the
    shared expression layout can reconsider the whole statement together.
*/
static bool followsBinaryGroup(const FormatDocument *document, int close)
{
    while (document->token(close) == RPAREN) {
        const int open = document->match(close);
        if (open < 0) {
            return false;
        }
        if (document->token(open - 1) != IDENTIFIER) {
            for (int i = open + 1; i < close; ++i) {
                const Token token = document->token(i);
                if ((token == LPAREN || token == LBRACK || token == LBRACE)
                    && document->match(i) > i
                ) {
                    i = document->match(i);
                } else if (operatorPrecedence(token) > 0
                    && isOperandEnd(document->token(i - 1))
                ) {
                    return true;
                }
            }
        }
        if (document->token(open - 1) != IDENTIFIER
            || (document->token(open - 2) != DOT
                && document->token(open - 2) != ARROW)
        ) {
            return false;
        }
        close = open - 3;
    }
    return false;
}

/*!
    @class FormatLineJoinRule
    @internal

    Lays the code out anew when `[Structure] target` is `any` (the command
    line's `--any-code`): it joins the lines of each expression, hence the
    layout rules that run after it (condition-wrap, operator-wrap,
    param-wrap, args-wrap, and the rest) see it as if it sat on one line,
    and break again whatever passes the wrap column. With the default,
    `long`, it does nothing.

    It joins a line break inside a `(`, a `[`, or the `{` of an initializer
    list, and, at the level of a statement, one before a line that a binary
    operator leads (see isLeadingOperator()), one in a member chain whose
    head is a parenthesized binary expression, one after an `=`, and one
    before the `:` of a conditional expression. The break turns into one
    space, or into none right after a `(` or a `[` and right before a `)`, a
    `]`, a `,`, or a `;`. It keeps a break next to a comment or a directive,
    one between two string literals (which people split on purpose), the
    closing-only line before `&&` or `||`, and each break between the
    statements of a body.

    The rules run one after the other, each one up to 8 passes; hence this
    rule joins once, before the layout rules, and the line breaks that those
    rules then make stay as they are.
*/

/*!
    Joins each line break of @a document that sits inside an expression.
*/
void FormatLineJoinRule::run(FormatDocument *document) const
{
    if (document->settings().codeTarget != FormatSettings::CodeTargetAny) {
        return;
    }
    QList<int > brackets;
    FormatScopeConditions conditions(document);
    int questions = 0;
    for (int i = 0; i + 1 < document->symbols().size(); ++i) {
        // Tracks the brackets still open, and the `?` of the statement
        // whose `:` has not come yet.
        const Token token = document->token(i);
        if (token == LPAREN || token == LBRACK || token == LBRACE) {
            brackets << i;
        } else if ((token == RPAREN || token == RBRACK || token == RBRACE)
            && ! brackets.isEmpty()
        ) {
            brackets.removeLast();
        }
        if (token == SEMIC || token == LBRACE || token == RBRACE) {
            questions = 0;
        } else if (token == QUESTION) {
            ++questions;
        } else if (token == COLON && questions > 0) {
            --questions;
        }

        // Joins the line break after the token, when there is one.
        if (document->symbolEndLine(i) >= document->symbolLine(i + 1)
            || ! this->mayJoin(document, i, brackets, questions)
        ) {
            continue;
        }
        // A conditional argument policy can retain an existing list rather
        // than having this rule flatten it before the wrapping rules run.
        if ( ! document->settings().wrapArgsOverCondition.isEmpty()
            && ! brackets.isEmpty()
        ) {
            const int open = brackets.last();
            const int close = document->match(open);
            const Token beforeOpen = document->token(open - 1);
            if (document->token(open) == LPAREN && close > open
                && (beforeOpen == IDENTIFIER || beforeOpen == RPAREN
                    || beforeOpen == RANGLE || beforeOpen == GTGT)
                && ! conditions.argsOver(
                    open, splitArguments(document, open, close).size()
                )
            ) {
                continue;
            }
        }
        const Token after = document->token(i + 1);
        const bool isTight = token == LPAREN || token == LBRACK
            || after == RPAREN || after == RBRACK || after == COMMA
            || after == SEMIC || token == DOT || token == ARROW
            || after == DOT || after == ARROW;
        const int from = document->symbolEnd(i);
        document->replace(
            from,
            document->symbols().at(i + 1).from - from,
            isTight ? QByteArray() : QByteArray(" ")
        );
    }
}

/*!
    Tells whether the line break after the token at @a before of
    @a document may be joined: no comment or directive sits next to it, it
    does not part two string literals, and it sits inside an expression,
    which @a brackets (the brackets still open) and @a questions (the `?`
    of the statement still waiting for their `:`) tell.
*/
bool FormatLineJoinRule::mayJoin(
        const FormatDocument *document, int before,
        const QList<int > & brackets,
        int questions) const
{
    const int after = before + 1;
    if (document->isDirectiveLine(document->symbolEndLine(before))
        || document->isDirectiveLine(document->symbolLine(after))
        || document->hasCommentBetween(
            document->symbolEnd(before),
            document->symbols().at(after).from
        )
    ) {
        return false;
    }
    if (document->token(before) == STRING_LITERAL
        && document->token(after) == STRING_LITERAL
    ) {
        return false;
    }
    const Token next = document->token(after);
    if ((next == ANDAND || next == OROR)
        && isClosingLine(document->lineText(document->symbolEndLine(before)))
    ) {
        return false;
    }
    const int nextLine = document->symbolLine(after);
    const int lineEnd = document->lastSymbolOfLine(nextLine);
    if (isClosingLine(document->lineText(nextLine)) && lineEnd >= after
        && (document->token(lineEnd + 1) == ANDAND
            || document->token(lineEnd + 1) == OROR)
        && document->symbolLine(lineEnd + 1) > nextLine
    ) {
        return false;
    }

    // Joins any break inside a `(`, a `[`, or an initializer list.
    if ( ! brackets.isEmpty()) {
        const int open = brackets.last();
        const Token bracket = document->token(open);
        if (bracket != LBRACE
            || document->braceScope(open) == FormatDocument::InitScope
        ) {
            return true;
        }
    }

    // Joins a statement's operator line, the value after its `=`, and the
    // `:` of its conditional expression.
    return isLeadingOperator(next)
        || ((next == DOT || next == ARROW) && followsBinaryGroup(
            document,
            before
        ))
        || document->token(before) == EQ
        || (next == COLON && questions > 0);
}
