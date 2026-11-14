// Copyright (C) 2015 The XD Company Ltd.
// SPDX-License-Identifier: Apache-2.0

#ifndef PARSER_COMMENT_HELPERS_H
#define PARSER_COMMENT_HELPERS_H

#include "../format-document.h"
#include "../lang/comment-line.h"

#include <QtCore/QList>
#include <QtCore/QPair>
#include <QtCore/QString>
#include <QtCore/QVector>

struct FormatConfigDirective
{
    enum Kind { Add, Begin, End, Off, On, Invalid };
    Kind kind;
    int offset;
    int endOffset;
    Config::Lines lines;
};

QList<FormatConfigDirective > configDirectives(const FormatDocument *document);

QList<FormatCommentLine > commentLines(
        const FormatDocument *document,
        int index);
QList<FormatCommentLine > groupLines(
        const FormatDocument *document,
        const QList<int > & group);
bool isSingleWord(const QList<FormatCommentLine > & lines);
bool looksLikeCode(const QList<FormatCommentLine > & lines);
QVector<bool > fencedLines(const QList<FormatCommentLine > & lines);
QString mapProse(
        const QString & text, QString (*map)(const QString &, const void *),
        const void *context);
QList<QList<int > > commentGroups(
        const FormatDocument *document,
        bool splitProtected = true);
bool isClosingLabel(const FormatDocument *document, int index);
bool isInlineLabel(const FormatDocument *document, int index);
QList<QPair<int, int > > tableRanges(const FormatDocument *document);
QString proseOnly(const QString & text);
bool isCodeWord(const QString & word);
bool pointsAhead(const QString & word);

#endif // PARSER_COMMENT_HELPERS_H
