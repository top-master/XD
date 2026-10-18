// Copyright (C) 2015 The XD Company Ltd.
// SPDX-License-Identifier: Apache-2.0

#ifndef FORMAT_ISSUE_H
#define FORMAT_ISSUE_H

#include <QtCore/QByteArray>
#include <QtCore/QString>

/*!
    \internal

    Names one thing a rule leaves to a person or an agent: the lines it
    concerns (`line` to `endLine`, the same line for most), the rule, and the
    message. The lines move with the edits that later rules make above them,
    and `endLine` with the ones they make inside the range.
*/
class FormatIssue
{
public:
    inline FormatIssue() : line(0), endLine(0), columnLimit(0) {}
    inline FormatIssue(int line, const QByteArray & rule, const QString & message,
                       int endLine = 0, int columnLimit = 0)
        : line(line), endLine(endLine > line ? endLine : line), columnLimit(columnLimit)
        , rule(rule), message(message)
    {
    }

    int line;
    int endLine;
    int columnLimit;
    QByteArray rule;
    QString message;
};

#endif // FORMAT_ISSUE_H
