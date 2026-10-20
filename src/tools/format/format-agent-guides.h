// Copyright (C) 2015 The XD Company Ltd.
// SPDX-License-Identifier: Apache-2.0

#ifndef FORMAT_AGENT_GUIDES_H
#define FORMAT_AGENT_GUIDES_H

#include <QtCore/QByteArray>
#include <QtCore/QScopedPointer>
#include <QtCore/QString>
#include <QtCore/QStringList>

class FormatIssue;
QT_BEGIN_NAMESPACE
class QLockFile;
QT_END_NAMESPACE

class FormatAgentGuides
{
public:
    FormatAgentGuides();
    ~FormatAgentGuides();

    static QString folderPrefix();
    static QString usageName();
    inline const QString &folder() const { return m_folder; }
    QString write(const QString & path, const FormatIssue & request);
    static QString templateName(const QByteArray & rule);
    static QString targetOf(const QByteArray & rule);
    static QString keepTarget(const QString & text, const QString & target);
    void remove();
    QString writeUsage();
    void removePrompts();

private:
    Q_DISABLE_COPY(FormatAgentGuides)

    bool makeFolder();
    void removeLeftovers() const;
    static bool isOwnFile(const QString & name);

    QString m_folder;
    int m_count;
    QStringList m_prompts;
    QScopedPointer<QLockFile > m_lock;
};

#endif // FORMAT_AGENT_GUIDES_H
