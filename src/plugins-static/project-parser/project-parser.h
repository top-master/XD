// Copyright (C) 2015 The XD Company Ltd.
// SPDX-License-Identifier: Apache-2.0

#ifndef PROJECT_PARSER_H
#define PROJECT_PARSER_H

#include <QtCore/QByteArray>
#include <QtCore/QHash>
#include <QtCore/QString>
#include <QtCore/QStringList>

namespace Project {

class Parser
{
public:
    inline Parser() {}

    bool run(const QString & projectFile, const QString & qmake,
             const QStringList & qmakeArguments, QString *error);
    QHash<QString, QStringList > parse(const QByteArray & output) const;

    inline const QStringList &files() const { return m_files; }
    inline const QStringList &includePaths() const { return m_includePaths; }
    inline const QStringList &projects() const { return m_projects; }

    static QStringList words(const QString & line);

private:
    bool runOne(const QString & projectFile, const QString & qmake,
                const QStringList & qmakeArguments, QString *error, int depth);
    static QString absolute(const QString & folder, const QString & path);

    QStringList m_files;
    QStringList m_includePaths;
    QStringList m_projects;
};

} // namespace Project

#endif // PROJECT_PARSER_H
