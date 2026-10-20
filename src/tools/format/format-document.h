// Copyright (C) 2015 The XD Company Ltd.
// SPDX-License-Identifier: Apache-2.0

#ifndef FORMAT_DOCUMENT_H
#define FORMAT_DOCUMENT_H

#include "./format-comment.h"
#include "./format-issue.h"
#include "./format-settings.h"

#include "symbols.h"

#include <QtCore/QByteArray>
#include <QtCore/QList>
#include <QtCore/QPair>
#include <QtCore/QSet>
#include <QtCore/QString>
#include <QtCore/QStringList>
#include <QtCore/QVector>

class FormatDocument
{
public:
    enum Scope {
        FileScope,
        NamespaceScope,
        ClassScope,
        EnumScope,
        FunctionScope,
        BlockScope,
        InitScope
    };

    explicit FormatDocument(const QByteArray & text,
                            const QString & path = QString());

    inline const QByteArray &text() const { return m_text; }
    inline const QString &path() const { return m_path; }
    inline const Symbols &symbols() const { return m_symbols; }
    inline const QList<FormatComment > &comments() const { return m_comments; }
    inline const QList<FormatIssue > &issues() const { return m_issues; }
    inline int lineCount() const { return m_lineStarts.size(); }
    inline const FormatSettings &settings() const { return m_settings; }
    inline void setSettings(const FormatSettings & settings) { m_settings = settings; }
    inline int pass() const { return m_pass; }
    inline bool agentMode() const { return m_agentMode; }
    inline const QStringList &includePaths() const { return m_includePaths; }
    inline void setIncludePaths(const QStringList & paths) { m_includePaths = paths; }
    QString findInclude(const QString & name) const;
    bool isHeader() const;
    QStringList quotedIncludes() const;
    inline void setAgentMode(bool agentMode) { m_agentMode = agentMode; }
    void setPass(int pass);
    FormatSpacing onSetting(const FormatSpacing & setting, int index);

    int lineOf(int offset) const;
    int lineStart(int line) const;
    int lineEnd(int line) const;
    QByteArray lineText(int line) const;
    QByteArray indentOf(int line) const;
    int symbolEnd(int index) const;
    int symbolLine(int index) const;
    int symbolEndLine(int index) const;
    QByteArray lexem(int index) const;
    Token token(int index) const;
    int match(int index) const;
    int symbolAfter(int offset) const;
    int headStart(int index) const;
    Scope scopeOf(int index) const;
    Scope braceScope(int index) const;
    int firstSymbolOfLine(int line) const;
    int lastSymbolOfLine(int line) const;
    bool isDirectiveLine(int line) const;
    bool spansDirective(int from, int to) const;
    QByteArray commentText(int index) const;
    int commentLine(int index) const;
    int commentEndLine(int index) const;
    bool isProtectedComment(int index) const;
    bool hasCommentBetween(int from, int to) const;
    bool isOnlyWhitespace(int from, int to) const;

    void setLineRanges(const QVector<QPair<int, int > > & ranges);
    bool isLineSelected(int line) const;

    bool replace(int offset, int length, const QByteArray & text);
    void report(int offset, const char *rule, const QString & message);
    void request(int offset,
                 const char *rule,
                 const QString & message,
                 int lineCount = 1,
                 int columnLimit = 0);
    inline const QList<FormatIssue > &requests() const { return m_requests; }
    void truncateIssues(int count);
    bool applyEdits();

private:
    static void shiftIssues(QList<FormatIssue > *issues, int line, int delta);

    class Edit
    {
    public:
        inline Edit() : offset(0), length(0) {}
        inline Edit(int offset, int length, const QByteArray & text)
            : offset(offset), length(length), text(text)
        {
        }

        int offset;
        int length;
        QByteArray text;
    };

    void analyze();
    void appendComment(const Symbol & symbol, int previousCodeLine);
    void analyzeBrackets();
    void analyzeLines();
    Scope classifyBrace(int index, Scope enclosing) const;

    QByteArray m_text;
    QString m_path;
    FormatSettings m_settings;
    int m_pass;
    bool m_agentMode;
    QStringList m_includePaths;
    QList<FormatIssue > m_requests;
    QSet<int > m_replacedLines;
    Symbols m_symbols;
    QList<FormatComment > m_comments;
    QVector<int > m_lineStarts;
    QVector<int > m_matches;
    QVector<int > m_scopes;
    QVector<int > m_braceScopes;
    QVector<int > m_lineFirstSymbols;
    QVector<int > m_lineLastSymbols;
    QVector<bool > m_directiveLines;
    QVector<QPair<int, int > > m_ranges;
    QList<Edit > m_edits;
    QList<FormatIssue > m_issues;
};

#endif // FORMAT_DOCUMENT_H
