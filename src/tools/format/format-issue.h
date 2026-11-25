// Copyright (C) 2015 The XD Company Ltd.
// SPDX-License-Identifier: Apache-2.0

#ifndef FORMAT_ISSUE_H
#define FORMAT_ISSUE_H

#include <QtCore/QByteArray>
#include <QtCore/QString>

/*!
    @internal

    Names one thing a rule leaves to a person or an agent: the lines it
    concerns (`line` to `endLine`, the same line for most), the rule, and the
    message. The lines move with the edits that later rules make above them,
    and `endLine` with the ones they make inside the range.

    A request may also name the column its lines must keep within
    (`columnLimit`, 0 for none), which the agent's prompt file states (see
    FormatAgentGuides). An issue that `isError` marks prints to the error
    output, as a setting such as `[Lint] byRefVariable = error` asks.
*/
class FormatIssue
{
public:
    inline FormatIssue() : line(0), endLine(0), columnLimit(0), isError(false) {}
    inline FormatIssue(
            int line,
            const QByteArray & rule,
            const QString & message,
            int endLine = 0,
            int columnLimit = 0)
        : line(line), endLine(endLine > line ? endLine : line), columnLimit(columnLimit)
        , isError(false), rule(rule), message(message)
    {
    }

    int line;
    int endLine;
    int columnLimit;
    bool isError;
    QByteArray rule;
    QString message;
};

#endif // FORMAT_ISSUE_H
