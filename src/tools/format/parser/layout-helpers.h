// Copyright (C) 2015 The XD Company Ltd.
// SPDX-License-Identifier: Apache-2.0

#ifndef PARSER_LAYOUT_HELPERS_H
#define PARSER_LAYOUT_HELPERS_H

#include "../format-document.h"

#include <QtCore/QByteArray>
#include <QtCore/QList>
#include <QtCore/QPair>

int wrapColumn(const FormatDocument *document);
int continuationIndent(int indent, int steps = 1);
QByteArray continuationPad(int indent, int steps = 1);
bool logicalBreakBeforeCall(const FormatDocument *document, int open,
                            int *operatorIndex, QByteArray *indent);
bool isBinaryOperator(Token token);
bool isClosingLine(const QByteArray & text);
bool closesEarlierLine(const FormatDocument *document, int bracket);
bool hasMisplacedLines(const FormatDocument *document, int open, int close,
                       int indent);
bool startsWithStringLiteral(const FormatDocument *document, int token);
QByteArray sourceText(const FormatDocument *document, int first, int last);
bool findTernary(const FormatDocument *document, int first, int last,
                 int *question, int *colon);
bool isDeclarationParen(const FormatDocument *document, int paren);
class FormatScopeConditions;

bool reachesWrapArgsAt(const FormatDocument *document,
                       FormatScopeConditions *conditions, int token);
QList<QByteArray > comparisonLines(
        const FormatDocument *document, int first,
        int last, int column, int indent, int step);
QList<int > loosestOperators(
        const FormatDocument *document, int first, int last,
        int (*rankOf)(Token), int fromRank, int *rank);
QList<QPair<int, int > > splitArguments(
        const FormatDocument *document,
        int open, int close);
QList<QByteArray > ternaryLines(
        const QByteArray & head, const QByteArray & middle,
        const QByteArray & tail, int indent, int column, int trailing,
        int wrap);

#endif // PARSER_LAYOUT_HELPERS_H
