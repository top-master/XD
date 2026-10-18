// Copyright (C) 2015 The XD Company Ltd.
// SPDX-License-Identifier: Apache-2.0

#ifndef IGNORE_PARSER_H
#define IGNORE_PARSER_H

#include "./ignore-rule.h"

#include <QtCore/QHash>
#include <QtCore/QList>
#include <QtCore/QSet>
#include <QtCore/QString>
#include <QtCore/QStringList>
#include <QtCore/QVarLengthArray>

class IgnoreParser
{
public:
    IgnoreParser();

    inline const QVarLengthArray<QString, 5> &ignoreFileNames() const { return m_fileNames; }
    void addIgnoreFileName(const QString & name);
    void addExplicitPath(const QString & path);

    static bool parseRule(const QString & line, const QString & base, IgnoreRule *rule);

    bool isIgnored(const QString & path, bool isDirectory);
    QString excludingRule(const QString & path, bool isDirectory);
    QStringList findFiles(const QString & directory, const QStringList & nameFilters);

private:
    bool matchesLast(const QString & root, const QString & path, bool isDirectory,
                     const IgnoreRule **decider = Q_NULLPTR);
    QString rootOf(const QString & folder);
    const QList<IgnoreRule > &rulesOf(const QString & file);
    bool isOnExplicitPath(const QString & path) const;
    bool isInsideExplicitPath(const QString & path) const;
    void collect(const QString & directory, const QStringList & nameFilters,
                 QSet<QString > *visited, QStringList *files);

    QVarLengthArray<QString, 5> m_fileNames;
    QStringList m_explicitPaths;
    QHash<QString, QList<IgnoreRule > > m_rules;
    QHash<QString, QString > m_roots;
};

#endif // IGNORE_PARSER_H
