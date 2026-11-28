// Copyright (C) 2015 The XD Company Ltd.
// SPDX-License-Identifier: Apache-2.0

#ifndef AST_AST_PARSER_H
#define AST_AST_PARSER_H

#include "./ast-node.h"
#include "../lexer/format-language.h"

#include <QtCore/QByteArray>
#include <QtCore/QList>
#include <QtCore/QVector>

struct AstParseResult
{
    inline AstParseResult() : root(Q_NULLPTR) {}

    AstNode *root;
    QVector<AstComment * > comments;
};

class AstParser
{
public:

    static AstParseResult parse(
            const QByteArray & text,
            const Symbols & symbols,
            const QList<FormatComment > & comments,
            const QVector<int > & lineStarts,
            const QVector<bool > & directiveLines,
            const QVector<int > & matches,
            FormatLanguage::Id language,
            AstArena *arena);
};

#endif // AST_AST_PARSER_H
