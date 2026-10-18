// Copyright (C) 2015 The XD Company Ltd.
// SPDX-License-Identifier: Apache-2.0

#ifndef FORMAT_LINE_SPEC_H
#define FORMAT_LINE_SPEC_H

#include <QtCore/QByteArray>
#include <QtCore/QList>
#include <QtCore/QPair>
#include <QtCore/QString>
#include <QtCore/QStringList>
#include <QtCore/QVector>

class FormatLineSpec
{
public:
    static bool resolve(
            const QString & spec, const QList<QByteArray > & lines, int anchor,
            int *first, int *last, QString *error);
    static bool resolveAll(
            const QStringList & specs, const QList<QByteArray > & lines,
            QVector<QPair<int, int > > *ranges, QString *error);

private:
    static bool parseLine(
            const QString & token, const QList<QByteArray > & lines, int from,
            int *line, QString *error);
    static int splitAt(const QString & spec);
    static bool isFunctionLine(const QByteArray & line);
};

#endif // FORMAT_LINE_SPEC_H
