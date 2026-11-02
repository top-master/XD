// Copyright (C) 2015 The XD Company Ltd.
// SPDX-License-Identifier: Apache-2.0

void report(Log *log, const QString & suffix, const QList<int > & values, int index)
{
    log->line(
        QString::fromLatin1("the first part of a rather long message to the user and ")
        + suffix,
        values[index + 1] + values[index + 2]
        + values[index + 3]
        + values[index + 4]
        + values[index + 5],
        u8"another long piece of text that keeps on going for quite a while longer"
        + suffix,
        "a plain literal piece of text that is also long enough to wrap right here"
        + suffix
    );
}
