// Copyright (C) 2015 The XD Company Ltd.
// SPDX-License-Identifier: Apache-2.0

#ifndef IGNORE_RULE_H
#define IGNORE_RULE_H

#include <QtCore/QRegularExpression>
#include <QtCore/QString>

/*!
    \internal

    Carries one rule of an ignore file: where it was read (the file, the
    line, and the rule as written, for messages), the folder it was read in
    (the base its anchored patterns start from), its pattern as a regular
    expression over the path relative to that folder, whether it re-includes
    (`!`), and whether it applies to folders only (a trailing `/`).
*/
class IgnoreRule
{
public:
    inline IgnoreRule() : line(0), negated(false), directoryOnly(false) {}

    QString file;
    int line;
    QString text;
    QString base;
    QRegularExpression pattern;
    bool negated;
    bool directoryOnly;
};

#endif // IGNORE_RULE_H
