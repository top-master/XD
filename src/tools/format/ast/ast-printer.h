// Copyright (C) 2015 The XD Company Ltd.
// SPDX-License-Identifier: Apache-2.0

#ifndef AST_AST_PRINTER_H
#define AST_AST_PRINTER_H

#include "./ast-node.h"
#include "./ast-transforms.h"
#include "../doc/doc-node.h"
#include "../doc/doc-printer.h"
#include "../format-file-config.h"

#include <QtCore/QByteArray>
#include <QtCore/QPair>
#include <QtCore/QVector>

class FormatDocument;

class AstPrinter
{
public:

    static const DocNode *buildDoc(
            const AstNode *root,
            const QByteArray & source,
            const Symbols & symbols,
            const QVector<int > & lineStarts,
            const FormatFileConfig & config,
            DocArena *arena);

    static QByteArray print(
            const AstNode *root,
            const QByteArray & source,
            const Symbols & symbols,
            const QVector<int > & lineStarts,
            const FormatFileConfig & config,
            const DocPrinter::Options & printerOptions = DocPrinter::Options());

    static const DocNode *buildDoc(
            const AstNode *root,
            const AstTransformState & state,
            const FormatDocument & document,
            DocArena *arena);

    static QByteArray print(
            const AstNode *root,
            const AstTransformState & state,
            const FormatDocument & document,
            const DocPrinter::Options & printerOptions = DocPrinter::Options());
};

#endif // AST_AST_PRINTER_H
