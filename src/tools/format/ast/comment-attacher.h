// Copyright (C) 2015 The XD Company Ltd.
// SPDX-License-Identifier: Apache-2.0

#ifndef AST_COMMENT_ATTACHER_H
#define AST_COMMENT_ATTACHER_H

#include "./ast-node.h"

#include <QtCore/QByteArray>
#include <QtCore/QVector>

class AstCommentAttacher
{
public:

    static void attach(AstNode *root,
                       QVector<AstComment * > *comments,
                       const QByteArray & source);
};

#endif // AST_COMMENT_ATTACHER_H
