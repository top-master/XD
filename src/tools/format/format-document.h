// Copyright (C) 2015 The XD Company Ltd.
// SPDX-License-Identifier: Apache-2.0

#ifndef FORMAT_DOCUMENT_H
#define FORMAT_DOCUMENT_H

#include "./format-comment.h"
#include "./format-issue.h"
#include "./format-file-config.h"

#include "symbols.h"

#include <QtCore/QByteArray>
#include <QtCore/QHash>
#include <QtCore/QList>
#include <QtCore/QPair>
#include <QtCore/QSet>
#include <QtCore/QString>
#include <QtCore/QStringList>
#include <QtCore/QVector>
#include <QtCore/QVariant>

class FormatRule;
class CodeEditor;

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
    inline const FormatFileConfig &config() const { return m_fileConfig; }
    const FormatFileConfig &configAt(int offset) const;
    int joinArgsAt(int open, int argument, int column, bool isWrapping) const;
    bool setInlineConfiguration(const FormatConfig &config, QString *error);
    inline bool hasInlineConfiguration() const { return m_inlineConfigured; }
    inline bool hasDisabledFormatting() const { return ! m_disabledRanges.isEmpty(); }
    inline QString inlineConfigurationError() const { return m_inlineError; }
    QList<FormatIssue > inlineWarnings(bool verbose) const;
    inline void setConfig(const FormatFileConfig & fileConfig)
    {
        m_fileConfig = fileConfig;
        m_fileConfigRegions.clear();
        m_inlineRanges.clear();
        m_disabledRanges.clear();
        m_inlineConfigured = false;
        m_inlineError.clear();
        m_ruleCaches.clear();
    }
    inline QVariant ruleCache(
            const FormatRule *rule,
            const QByteArray &key) const
    {
        return m_ruleCaches.value(rule).value(key);
    }
    inline void setRuleCache(
            const FormatRule *rule, const QByteArray &key,
            const QVariant &value)
    {
        m_ruleCaches[rule].insert(key, value);
    }
    inline int pass() const { return m_pass; }
    inline bool agentMode() const { return m_agentMode; }
    inline const QStringList &includePaths() const { return m_includePaths; }
    inline const FormatConfig &formatConfig() const { return m_config; }
    inline void setFormatConfig(const FormatConfig &config) { m_config = config; }
    inline bool hasSelectedRanges() const { return ! m_ranges.isEmpty(); }
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
    bool isAccessLabel(int colon) const;
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
    bool isInlineConfigComment(int index) const;
    bool isFormattingDisabled(int offset) const;
    bool hasCommentBetween(int from, int to) const;
    bool isOnlyWhitespace(int from, int to) const;

    void setLineRanges(const QVector<QPair<int, int > > & ranges);
    bool isLineSelected(int line) const;

    CodeEditor edit(int start, int end);
    bool replace(int offset, int length, const QByteArray & text);
    inline bool canInsert(int offset) const
    { return this->canReplace(offset, 0); }
    bool replace(
            int offset, int length, const QByteArray & text,
            int otherOffset, int otherLength, const QByteArray & otherText);
    void report(int offset, const char *rule, const QString & message,
                bool isError = false);
    void request(int offset,
                 const char *rule,
                 const QString & message,
                 int lineCount = 1,
                 int columnLimit = 0);
    inline const QList<FormatIssue > &requests() const { return m_requests; }
    void truncateIssues(int count);
    bool applyEdits();

private:
    friend class CodeEditor;
    static void shiftIssues(QList<FormatIssue > *issues, int line, int delta);
    bool canReplace(int offset, int length) const;

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

    void moveInlineConfiguration(const QList<Edit > & edits);
    static int movedOffset(int offset, bool isStart,
                           const QList<Edit > & edits);
    void analyze();
    void appendComment(const Symbol & symbol, int previousCodeLine);
    void analyzeBrackets();
    void analyzeLines();
    Scope classifyBrace(int index, Scope enclosing) const;

    bool m_editorChanged = false;
    QByteArray m_text;
    QString m_path;
    FormatFileConfig m_fileConfig;
    QVector<QPair<int, FormatFileConfig > > m_fileConfigRegions;
    QVector<QPair<int, int > > m_inlineRanges;
    QVector<QPair<int, int > > m_disabledRanges;
    bool m_inlineConfigured;
    QString m_inlineError;
    QHash<const FormatRule *, QHash<QByteArray, QVariant > > m_ruleCaches;
    int m_pass;
    bool m_agentMode;
    QStringList m_includePaths;
    FormatConfig m_config;
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


class CodeEditor
{
    FormatDocument *m_target;
    FormatDocument m_backup;
    QByteArray m_original;
    QList<FormatComment > m_comments;
    int m_start;
    int m_end;

public:
    CodeEditor(FormatDocument *document, int start, int end);
    CodeEditor(CodeEditor &&other);
    ~CodeEditor();
    CodeEditor(const CodeEditor &) = delete;
    CodeEditor &operator=(const CodeEditor &) = delete;
    CodeEditor &operator=(CodeEditor &&) = delete;

    bool apply();
    inline FormatDocument *document() { return &m_backup; }
    inline const FormatDocument *document() const { return &m_backup; }
    inline const QByteArray &text() const { return m_backup.text(); }
    inline const Symbols &symbols() const { return m_backup.symbols(); }
    inline const FormatFileConfig &configAt(int offset) const
    { return m_backup.configAt(offset); }
    inline int lineCount() const { return m_backup.lineCount(); }
    inline int lineOf(int offset) const { return m_backup.lineOf(offset); }
    inline int lineStart(int line) const { return m_backup.lineStart(line); }
    inline int lineEnd(int line) const { return m_backup.lineEnd(line); }
    inline QByteArray lineText(int line) const { return m_backup.lineText(line); }
    inline QByteArray indentOf(int line) const { return m_backup.indentOf(line); }
    inline int symbolEnd(int index) const { return m_backup.symbolEnd(index); }
    inline int symbolLine(int index) const { return m_backup.symbolLine(index); }
    inline int symbolEndLine(int index) const
    { return m_backup.symbolEndLine(index); }
    inline QByteArray lexem(int index) const { return m_backup.lexem(index); }
    inline Token token(int index) const { return m_backup.token(index); }
    inline int match(int index) const { return m_backup.match(index); }
    inline int symbolAfter(int offset) const
    { return m_backup.symbolAfter(offset); }
    inline int headStart(int index) const { return m_backup.headStart(index); }
    inline FormatDocument::Scope scopeOf(int index) const
    { return m_backup.scopeOf(index); }
    inline FormatDocument::Scope braceScope(int index) const
    { return m_backup.braceScope(index); }
    inline int firstSymbolOfLine(int line) const
    { return m_backup.firstSymbolOfLine(line); }
    inline int lastSymbolOfLine(int line) const
    { return m_backup.lastSymbolOfLine(line); }
    inline bool isDirectiveLine(int line) const
    { return m_backup.isDirectiveLine(line); }
    inline bool spansDirective(int start, int end) const
    { return m_backup.spansDirective(start, end); }
    inline bool hasCommentBetween(int start, int end) const
    { return m_backup.hasCommentBetween(start, end); }
    inline bool isOnlyWhitespace(int start, int end) const
    { return m_backup.isOnlyWhitespace(start, end); }
    inline QByteArray sourceText(int first, int last) const
    {
        if (last < first) {
            return QByteArray();
        }
        const int start = m_backup.symbols().at(first).from;
        return m_backup.text().mid(start, m_backup.symbolEnd(last) - start);
    }
    inline QByteArray get(int start, int end) const
    { return m_backup.text().mid(start, end - start); }
    inline bool add(int offset, const QByteArray &text)
    { return this->replace(offset, 0, text); }
    inline bool set(int start, int end, const QByteArray &text)
    { return this->replace(start, end - start, text); }
    inline bool replace(int offset, int length, const QByteArray &text)
    {
        return m_target && offset >= m_start && length >= 0
            && offset <= m_end - length
            && m_backup.replace(offset, length, text);
    }
    inline bool replace(int offset, int length, const QByteArray &text,
                        int otherOffset, int otherLength,
                        const QByteArray &otherText)
    {
        return m_target && offset >= m_start && length >= 0
            && offset <= m_end - length
            && otherOffset >= m_start && otherLength >= 0
            && otherOffset <= m_end - otherLength
            && m_backup.replace(offset, length, text,
                                otherOffset, otherLength, otherText);
    }

private:
    QByteArray restore(int offset, int length, const QByteArray &text) const;
};

#endif // FORMAT_DOCUMENT_H
