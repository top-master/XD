// Copyright (C) 2015 The XD Company Ltd.
// SPDX-License-Identifier: Apache-2.0

#ifndef PARSER_LAYOUT_HELPERS_H
#define PARSER_LAYOUT_HELPERS_H

#include "../format-document.h"

#include <QtCore/QByteArray>
#include <QtCore/QList>
#include <QtCore/QPair>

int wrapColumn(const FormatDocument *document, int offset);
int wrapMaxAt(const FormatDocument *document, int offset);
int continuationIndent(int indent, int steps = 1);
QByteArray continuationPad(int indent, int steps = 1);
bool isBinaryOperator(Token token);
bool isControlHead(const FormatDocument *document, int index);
int precedingChainOperator(const FormatDocument *document, int next,
                           bool respectPlacement = true);
bool isClosingLine(const QByteArray & text);
int comparisonRank(Token token);
bool closesEarlierLine(const FormatDocument *document, int bracket);
bool hasMisplacedLines(const FormatDocument *document, int open, int close,
                       int indent, int extraPad = 0);
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
int templateClose(const FormatDocument *document, int angle, int end);
QList<QPair<int, int > > splitArguments(
        const FormatDocument *document,
        int open, int close);
QList<QByteArray > ternaryLines(
        const QByteArray & head, const QByteArray & middle,
        const QByteArray & tail, int indent, int column, int trailing,
        int wrap);

#endif // PARSER_LAYOUT_HELPERS_H
