// Copyright (C) 2015 The XD Company Ltd.
// SPDX-License-Identifier: Apache-2.0

#ifndef FORMAT_RESULT_H
#define FORMAT_RESULT_H

#include "./format-issue.h"

#include <QtCore/QByteArray>
#include <QtCore/QList>
#include <QtCore/QString>

/*!
    \internal

    Carries the outcome of Formatter::format(): the formatted text, whether
    it differs from the input, the issues left for a person to fix, the
    requests for an agent (in agent mode; see FormatDocument::request()),
    and the error that stopped the formatting, if any.
*/
class FormatResult
{
public:
    inline FormatResult() : changed(false) {}

    QByteArray text;
    bool changed;
    QList<FormatIssue > issues;
    QList<FormatIssue > requests;
    QString error;
};

#endif // FORMAT_RESULT_H
