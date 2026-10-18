// Copyright (C) 2015 The XD Company Ltd.
// SPDX-License-Identifier: Apache-2.0

#ifndef FORMAT_FILE_RESULT_H
#define FORMAT_FILE_RESULT_H

#include "./format-issue.h"

#include <QtCore/QList>
#include <QtCore/QString>

/*!
    @internal

    Carries the outcome of one file: whether its formatting changed it (or,
    when only checking, would change it), the issues left for a person to
    fix, the requests for an agent (in agent mode), and the error that kept
    it from being formatted, if any.
*/
class FormatFileResult
{
public:
    inline FormatFileResult() : changed(false) {}

    QString path;
    bool changed;
    QList<FormatIssue > issues;
    QList<FormatIssue > requests;
    QString error;
};

#endif // FORMAT_FILE_RESULT_H
