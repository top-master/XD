// Copyright (C) 2015 The XD Company Ltd.
// SPDX-License-Identifier: Apache-2.0

#ifndef FORMAT_RUNNER_H
#define FORMAT_RUNNER_H

#include "./format-file-result.h"
#include "./format-options.h"
#include "./formatter.h"

#include <QtCore/QList>
#include <QtCore/QString>
#include <QtCore/QStringList>

class FormatRunner
{
public:
    FormatRunner(const Formatter *formatter, const FormatOptions & options);
    virtual ~FormatRunner();

    static QStringList kinds();
    static QString defaultKind();
    static FormatRunner *create(const QString & kind, const Formatter *formatter,
                                const FormatOptions & options);

    virtual QList<FormatFileResult > run(const QStringList & files);
    FormatFileResult formatFile(const QString & path) const;

protected:
    int jobCount(int fileCount) const;

    const Formatter *m_formatter;
    FormatOptions m_options;
};

#endif // FORMAT_RUNNER_H
