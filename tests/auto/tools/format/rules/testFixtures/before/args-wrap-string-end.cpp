// Copyright (C) 2015 The XD Company Ltd.
// SPDX-License-Identifier: Apache-2.0

void report(Log *log, const QString & prefix, const QString & suffix, int count)
{
    log->line(
        QString::number(count).rightJustified(8, QLatin1Char(' ')).toUpper().trimmed(),
        prefix + "a literal piece of text that is long enough to wrap"
            + suffix
    );
}
