// Copyright (C) 2015 The XD Company Ltd.
// SPDX-License-Identifier: Apache-2.0

void collect()
{
    const QStringList documentLines = QString::fromUtf8(payload.readAll()).split(QLatin1Char('\n'));
}
