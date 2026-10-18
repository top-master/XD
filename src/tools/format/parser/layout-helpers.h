// Copyright (C) 2015 The XD Company Ltd.
// SPDX-License-Identifier: Apache-2.0

#ifndef PARSER_LAYOUT_HELPERS_H
#define PARSER_LAYOUT_HELPERS_H

#include <QtCore/QByteArray>
#include <QtCore/QList>
#include <QtCore/QPair>

class FormatDocument;

int wrapColumn(const FormatDocument *document);
QByteArray continuationPad(int indent);
QByteArray sourceText(const FormatDocument *document, int first, int last);
bool findTernary(const FormatDocument *document, int first, int last,
                 int *question, int *colon);
QList<QPair<int, int > > splitArguments(const FormatDocument *document,
                                        int open, int close);
QList<QByteArray > ternaryLines(const QByteArray & head, const QByteArray & middle,
                                const QByteArray & tail, int indent, int column, int trailing,
                                int wrap);

#endif // PARSER_LAYOUT_HELPERS_H
