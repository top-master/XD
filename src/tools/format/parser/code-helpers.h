// Copyright (C) 2015 The XD Company Ltd.
// SPDX-License-Identifier: Apache-2.0

#ifndef PARSER_CODE_HELPERS_H
#define PARSER_CODE_HELPERS_H

#include "../format-document.h"

#include <QtCore/QByteArray>

bool isTypeEnd(Token token);
bool isBuiltinType(Token token);
bool isSameLineGap(const FormatDocument *document, int before, int after);
int contentEnd(const FormatDocument *document, int line);
int statementEnd(const FormatDocument *document, int index);
int lambdaBody(const FormatDocument *document, int bracket);
bool isLeadingOperator(Token token);
bool isOperandEnd(Token token);
int operatorPrecedence(Token token);
int textWidth(const QByteArray & text);

#endif // PARSER_CODE_HELPERS_H
