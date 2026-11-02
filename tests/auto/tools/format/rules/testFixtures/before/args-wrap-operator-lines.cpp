// Copyright (C) 2015 The XD Company Ltd.
// SPDX-License-Identifier: Apache-2.0

void sum(Log *log, const QList<int > & values, int count, int total)
{
    log->line(
        QString::number(count).rightJustified(8, QLatin1Char(' ')).toUpper().trimmed(),
        values[1] + values[2]
        + count
            + total
    );
}
