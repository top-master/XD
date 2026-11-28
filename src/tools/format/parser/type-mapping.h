// Copyright (C) 2015 The XD Company Ltd.
// SPDX-License-Identifier: Apache-2.0

#ifndef PARSER_TYPE_MAPPING_H
#define PARSER_TYPE_MAPPING_H

#include <QtCore/QList>
#include <QtCore/QPair>
#include <QtCore/QString>
#include <QtCore/QStringList>

class FormatDocument;
class FormatConditionContext;

namespace FormatTypeMapping {
bool lastPartMatches(const QString &key, const QString &last);
QString readName(const FormatDocument *document, int from, int *end);
QStringList visibleScopes(const FormatDocument *document, int index);
QStringList candidates(const QString &written, const QStringList &scopes);
bool isCall(const FormatDocument *document, int index);
QStringList callArguments(const FormatDocument *document, int open, int close);
int mappingOf(
        const QList<QPair<QString, QString > > &mappings,
        const QStringList &names,
        const QStringList &arguments,
        QString *capture,
        QStringList *bound);
bool replacement(
        const QList<QPair<QString, QString > > &mappings,
        int index,
        QString capture,
        QStringList arguments,
        QStringList bound,
        const FormatConditionContext *place,
        QString *head,
        QString *call,
        QString *error);
QString shortestName(const QString &name, const QStringList &scopes);
} // Namespace `FormatTypeMapping`.

#endif // PARSER_TYPE_MAPPING_H
