// Copyright (C) 2015 The XD Company Ltd.
// SPDX-License-Identifier: Apache-2.0

#ifndef QMEMORYSAFETY_BACKTRACE_P_H
#define QMEMORYSAFETY_BACKTRACE_P_H

#include "qglobal.h"

#if defined(QT_PTR_TRACKING) && !defined(QT_BOOTSTRAPPED)
#include "qstringlist.h"

QT_BEGIN_NAMESPACE

/*!
    @internal

    The frames that `%{backtrace}` prints under Fil-C, and how many it may.
*/
struct QFilcBacktrace
{
    QStringList functions;
    int depth;
};

void qFilcBacktraceScan(QFilcBacktrace *trace);

QT_END_NAMESPACE
#endif

#endif
