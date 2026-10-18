// Copyright (C) 2015 The XD Company Ltd.
// SPDX-License-Identifier: Apache-2.0

#ifndef FORMAT_THREAD_RUNNER_H
#define FORMAT_THREAD_RUNNER_H

#include "./format-runner.h"

#ifndef QT_NO_THREAD
class FormatThreadRunner : public FormatRunner
{
    typedef FormatRunner super;
public:
    FormatThreadRunner(const Formatter *formatter,
                       const FormatOptions & options);

    QList<FormatFileResult > run(const QStringList & files) Q_DECL_OVERRIDE;
};
#endif // QT_NO_THREAD

#endif // FORMAT_THREAD_RUNNER_H
