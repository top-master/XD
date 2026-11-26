// Copyright (C) 2015 The XD Company Ltd.
// SPDX-License-Identifier: Apache-2.0

#ifndef PARSER_CLASS_HELPERS_H
#define PARSER_CLASS_HELPERS_H

#include "../format-document.h"

#include "../lang/class-members.h"

#include <QtCore/QByteArray>
#include <QtCore/QList>

void addInlineKeyword(FormatDocument *document, int start);
void setOverrideMarker(FormatDocument *document, int overrideToken, const QString &marker);
int functionDeclarationStart(const FormatDocument *document, int body, bool *hasInline,
                             bool *exempt, int *overrideToken);
bool insideUncapturedLambda(const FormatDocument *document, int body, int index);
QSet<QByteArray > declaredLocalNames(const FormatDocument *document, int from, int to);
bool endsType(Token token);
bool isMacroName(const QByteArray & name);
QList<int > classBodyTokens(const FormatDocument *document, int brace);
FormatClassMembers collectMembers(
        const FormatDocument *document, int brace,
        const QByteArray & className);
bool isMacroArgument(const FormatDocument *document, int index);
int findClassBody(const FormatDocument *document,
                  const QByteArray & className);
int contentEndOf(const FormatDocument *document, int index);
int enclosingClassBody(const FormatDocument *document, int index);
QByteArray classNameOf(const FormatDocument *document, int brace);

#endif // PARSER_CLASS_HELPERS_H
