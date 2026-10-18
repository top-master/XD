// Copyright (C) 2015 The XD Company Ltd.
// SPDX-License-Identifier: Apache-2.0

#ifndef FORMAT_PROCESS_RUNNER_H
#define FORMAT_PROCESS_RUNNER_H

#include "./format-runner.h"

#include <QtCore/QByteArray>

#ifndef QT_NO_PROCESS
class FormatProcessRunner : public FormatRunner
{
    typedef FormatRunner super;
public:
    FormatProcessRunner(const Formatter *formatter,
                        const FormatOptions & options);

    QList<FormatFileResult > run(const QStringList & files) Q_DECL_OVERRIDE;

    static QByteArray encode(const FormatFileResult & result);
    static bool decode(const QByteArray & line,
                       QList<FormatFileResult > *results);

private:
    QStringList workerArguments() const;
    static QByteArray encodeIssue(
            const char *kind, const QByteArray & path,
            const FormatIssue & issue);
};
#endif // QT_NO_PROCESS

#endif // FORMAT_PROCESS_RUNNER_H
