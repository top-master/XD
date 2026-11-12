// Copyright (C) 2015 The XD Company Ltd.
// SPDX-License-Identifier: Apache-2.0

#include "./condition-wrap.h"

#include "../format-document.h"
#include "../parser/code-helpers.h"
#include "../parser/condition-layout.h"
#include "../parser/layout-helpers.h"
#include "../parser/scope-conditions.h"

#include <QtCore/QByteArray>

/*!
    @class FormatConditionWrapRule
    @internal

    Breaks the condition of an `if` or a `while` that sits on one line and
    passes the wrap column (see wrapColumn()), or that spans several lines
    and has one past `[Structure] wrapMax`, through FormatConditionLayout,
    which other rules share: before each `&&` and `||` of its own level (see
    FormatConditionLayout::layoutCondition()), the condition's `)` then
    closing on a line of its own, at the keyword's indent, with the body's
    `{` after it. A one-line condition without a `&&` or `||` of its own
    breaks before its loosest operator instead, once its line passes
    `wrapMax` (see FormatConditionLayout::layoutPlain()). A condition with a
    comment, a brace, or a directive in it is left as it is, and so is one
    that spans several lines with an operand that does.

    In agent mode (`--ai`), a condition that spans several lines keeps them:
    the rule asks the agent to regroup its lines instead (see
    FormatDocument::request()), since the person who broke them chose where.
*/

/*!
    Puts @a text, the layout of the condition from the `(` at @a open of
    @a document to the `)` at @a close, in its place, with the body's `{`
    right after it when that `{` shares the line of the `)`.
*/
static void replaceCondition(FormatDocument *document, int open, int close,
                             QByteArray text)
{
    const int from = document->symbols().at(open).from;
    int to = document->symbolEnd(close);
    if (document->token(close + 1) == LBRACE
        && document->symbolLine(close + 1) == document->symbolEndLine(close)
    ) {
        to = document->symbolEnd(close + 1);
        text += " {";
    }
    document->replace(from, to - from, text);
}

/*!
    Breaks each long one-line condition of @a document.
*/
void FormatConditionWrapRule::run(FormatDocument *document) const
{
    const int wrap = wrapColumn(document);
    FormatScopeConditions conditions(document);
    const FormatConditionLayout layout(document, &conditions);
    for (int i = 0; i + 1 < document->symbols().size(); ++i) {
        const Token token = document->token(i);
        if ((token != IF
                && token != WHILE)
            || document->token(i + 1) != LPAREN
        ) {
            continue;
        }
        const int open = i + 1;
        const int close = document->match(open);
        const int line = document->symbolLine(i);
        if (close < 0 || document->isDirectiveLine(line)
            || document->hasCommentBetween(
                document->symbols().at(open).from,
                document->symbolEnd(close)
            )
        ) {
            continue;
        }

        // Breaks a one-line condition without a `&&` or `||` of its own
        // before its loosest operator (see `layoutPlain()`), only
        // once its line passes `wrapMax`: such a break splits a single
        // comparison, hence it waits for the hard limit.
        if (layout.operators(open, close).isEmpty()) {
            const bool isLong = document->symbolEndLine(close) == line
                && textWidth(document->lineText(line)) > document->settings().wrapMax;
            const QByteArray text = isLong
                ? layout.layoutPlain(open, close) : QByteArray();
            if ( ! text.isEmpty()) {
                replaceCondition(document, open, close, text);
            }
            i = qMax(i, close);
            continue;
        }

        // Takes a one-line condition past the wrap column, or a split one
        // with a line past `wrapMax`.
        const int endLine = document->symbolEndLine(close);
        if (endLine == line) {
            if (textWidth(document->lineText(line)) <= wrap) {
                continue;
            }
        } else if ( ! this->isSplitTooLong(document, layout, open, close)) {
            continue;
        } else if (document->agentMode()) {
            const int limit = document->settings().wrapMax;
            document->request(
                document->symbols().at(i).from,
                "condition-wrap",
                QString(QLL("this condition has a line past %1 columns; "
                    "regroup its lines so that the operands that "
                    "belong together share one, each line within %1 "
                    "columns and led by its && or ||")).arg(limit),
                endLine - line + 1,
                limit
            );
            i = close;
            continue;
        }
        bool hasBrace = false;
        for (int k = open + 1; k < close && ! hasBrace; ++k) {
            hasBrace = document->token(k) == LBRACE;
        }
        if (hasBrace) {
            continue;
        }

        // Lays the condition out, and puts the body's `{` right after it.
        const int indent = document->indentOf(line).size();
        const int column = document->symbols().at(open).from
            - document->lineStart(line);
        replaceCondition(
            document,
            open,
            close,
            layout.layoutCondition(open, close, indent, column)
        );
        i = close;
    }
}

/*!
    Tells whether the condition from the `(` at @a open of @a document to the
    `)` at @a close, which spans several lines, has a line past
    `[Structure] wrapMax`, and each of its operands sits on one line (see
    operators()); people lay out a condition whose operand spans lines.
*/
bool FormatConditionWrapRule::isSplitTooLong(
        const FormatDocument *document, const FormatConditionLayout & layout,
        int open, int close) const
{
    const int max = document->settings().wrapMax;
    bool tooLong = false;
    for (int l = document->symbolLine(open); l <= document->symbolEndLine(close); ++l) {
        tooLong = tooLong || textWidth(document->lineText(l)) > max;
    }
    if ( ! tooLong) {
        return false;
    }
    const QList<int > ops = layout.operators(open, close);
    for (int k = 0; k <= ops.size(); ++k) {
        const int first = k == 0 ? open + 1 : ops.at(k - 1) + 1;
        const int last = k == ops.size() ? close - 1 : ops.at(k) - 1;
        if (first > last
            || document->symbolLine(first) != document->symbolEndLine(last)
        ) {
            return false;
        }
    }
    return true;
}
