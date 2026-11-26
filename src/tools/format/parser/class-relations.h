// Copyright (C) 2015 The XD Company Ltd.
// SPDX-License-Identifier: Apache-2.0

#ifndef PARSER_CLASS_RELATIONS_H
#define PARSER_CLASS_RELATIONS_H

#include <QtCore/QByteArray>
#include <QtCore/QStringList>

class FormatConfig;
class FormatDocument;
class FormatFileConfig;
class FormatScopeConditions;

namespace FormatRelations {
QByteArray baseName(const FormatDocument *document, int brace, bool *isStruct);
bool hasSuper(const FormatDocument *document, int brace);
bool wantsTypedef(const FormatDocument *document, int brace, const QByteArray &base);
QStringList relatedFiles(const FormatDocument *document, const FormatFileConfig &config);
FormatDocument *readRelated(const QString &path, const FormatConfig &config,
                            bool withSelection = true);
int matchBase(const FormatDocument *document, int first, const QByteArray &base,
              bool allowGlobal);
QByteArray baseIdentifier(const QByteArray &base);
int findQualifiedClass(const FormatDocument *document, const QByteArray &name);
QByteArray ownerOf(const FormatDocument *document, FormatScopeConditions *scopes, int token);
bool isShadowed(const FormatDocument *document, FormatScopeConditions *scopes,
                int token, const QByteArray &base);
bool permitsSuperReference(const FormatDocument *owner,
                           int brace, const QByteArray &base,
                           const FormatDocument *place, int token);
}

#endif // PARSER_CLASS_RELATIONS_H
