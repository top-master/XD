// Copyright (C) 2015 The XD Company Ltd.
// SPDX-License-Identifier: Apache-2.0

#include "./operator-wrap.h"

#include "../format-document.h"
#include "../parser/code-helpers.h"

#include <QtCore/QByteArray>
#include <QtCore/QString>

/*!
    \class FormatOperatorWrapRule
    \internal

    Breaks a statement that sits whole on one line and runs past the column
    limit (`[Structure] wrapMax`, 80 by default) before the operators of its
    outer chain (`<<`, `+`, `-`, `|`, `^`, `&&` or `||`, outside of every
    bracket): each line takes as many operands as fit,
    and each continuation line starts with its operator, one step past the
    line it breaks from. A statement already wrapped, a compact one with
    braces, and a control statement are left to people.

    Only the width decides those breaks, while the rule for a long operator
    chain asks to break between groups of operands that belong together;
    hence, in agent mode (`--ai`), the rule asks the agent to regroup the
    lines it broke (see FormatDocument::request()). A line it cannot break
    (no outer operator, or a first operand that alone passes the limit) stays
    as it is, and is not asked about.
*/

/*!
    Tells whether \a token ends an operand, hence an operator right after it
    is a binary one (not a unary minus, say).
*/
static bool endsOperand(Token token)
{
    return token == IDENTIFIER || token == INTEGER_LITERAL || token == FLOATING_LITERAL
        || token == CHARACTER_LITERAL || token == STRING_LITERAL || token == BOOLEAN_LITERAL
        || token == RPAREN || token == RBRACK || token == THIS;
}

/*!
    Lists the operators of \a line that the line may break before: the binary
    ones outside of every bracket, in order; none unless the line holds one
    whole statement (it starts the line, ends there with its only `;`, holds
    no braces, no comment, and no `?` outside of brackets, and is no control
    statement), since a break inside an argument list or a conditional
    expression reads as a broken operand.
*/
QList<int > FormatOperatorWrapRule::breakableOperators(const FormatDocument *document,
                                                         int line) const
{
    QList<int > operators;
    const int first = document->firstSymbolOfLine(line);
    const int last = document->lastSymbolOfLine(line);
    if (first < 0 || document->token(last) != SEMIC
        || document->hasCommentBetween(document->lineStart(line), document->lineEnd(line))
    ) {
        return operators;
    }

    // Takes only a statement that starts the line and ends on it.
    if (first > 0) {
        const Token before = document->token(first - 1);
        if (before != SEMIC
            && before != LBRACE
            && before != RBRACE
            && before != COLON
        ) {
            return operators;
        }
    }
    const Token lead = document->token(first);
    if (lead == IF || lead == ELSE || lead == WHILE || lead == FOR || lead == DO
        || lead == SWITCH || lead == CASE
    ) {
        return operators;
    }
    QList<int > depths;
    int depth = 0;
    for (int i = first; i <= last; ++i) {
        const Token token = document->token(i);
        if (token == LBRACE || token == RBRACE || (token == SEMIC && i < last)
            || (token == QUESTION && depth == 0)
        ) {
            return QList<int >();
        }
        if (token == RPAREN || token == RBRACK || token == RBRACE) {
            --depth;
        }
        const bool isOperator = token == LTLT || token == PLUS || token == MINUS || token == OR
            || token == HAT || token == ANDAND || token == OROR;
        if (isOperator
            && i > first
            && endsOperand(document->token(i - 1))
        ) {
            operators << i;
            depths << depth;
        }
        if (token == LPAREN || token == LBRACK || token == LBRACE) {
            ++depth;
        }
    }
    QList<int > outer;
    for (int k = 0; k < operators.size(); ++k) {
        if (depths.at(k) == 0) {
            outer << operators.at(k);
        }
    }
    return outer;
}

/*!
    Breaks each line of \a document that runs past the column limit before
    its outer operators, and asks the agent to regroup the lines it made.
*/
void FormatOperatorWrapRule::run(FormatDocument *document) const
{
    const int limit = document->settings().wrapMax;
    for (int line = 1; line <= document->lineCount(); ++line) {
        const QByteArray text = document->lineText(line);
        if (textWidth(text) <= limit || document->isDirectiveLine(line)) {
            continue;
        }
        const QList<int > operators = this->breakableOperators(
            document,
            line
        );
        if (operators.isEmpty()) {
            continue;
        }

        // Fills each line with as many operands as fit, from the line's start.
        const int start = document->lineStart(line);
        const int first = document->firstSymbolOfLine(line);
        const bool ledByOperator = operators.first() == first;
        const QByteArray indent = document->indentOf(line)
            + (ledByOperator ? "" : "    ");
        const int end = start + text.size();
        QList<int > breaks;
        int from = start;
        int prefix = 0;
        while (prefix + textWidth(document->text().mid(from, end - from)) > limit) {
            int chosen = -1;
            for (int k = 0; k < operators.size(); ++k) {
                const int at = operators.at(k);
                const int before = document->symbolEnd(at - 1);
                if (document->symbols().at(at).from <= from) {
                    continue;
                }
                if (prefix + textWidth(document->text().mid(from, before - from)) > limit) {
                    break;
                }
                chosen = at;
            }
            if (chosen < 0) {
                break;
            }
            breaks << chosen;
            from = document->symbols().at(chosen).from;
            prefix = textWidth(indent);
        }

        // Breaks before each chosen operator, and asks about the lines.
        bool broke = false;
        for (int b = 0; b < breaks.size(); ++b) {
            const int at = breaks.at(b);
            const int gapFrom = document->symbolEnd(at - 1);
            broke = document->replace(
                gapFrom,
                document->symbols().at(at).from - gapFrom,
                "\n" + indent
            ) || broke;
        }
        if (broke) {
            document->request(
                start,
                "operator-wrap",
                QString(QLL("the tool broke this expression at its operators "
                    "by width alone; regroup its lines so that the "
                    "operands that belong together (a key and its "
                    "value, a format and its arguments) share one, "
                    "each line within %1 columns and led by its "
                    "operator")).arg(limit),
                1,
                limit
            );
        }
    }
}
