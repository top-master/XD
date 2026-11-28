// Copyright (C) 2015 The XD Company Ltd.
// SPDX-License-Identifier: Apache-2.0

#ifndef FORMAT_DOCUMENT_H
#define FORMAT_DOCUMENT_H

#include "./format-comment.h"
#include "./format-issue.h"
#include "./format-file-config.h"

#include "./ast/ast-parser.h"
#include "./lexer/format-language.h"
#include "./lexer/format-token.h"

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

    inline FormatLanguage::Id language() const { return m_language; }
    inline const QVector<int > &lineStarts() const { return m_lineStarts; }
    inline const QVector<bool > &directiveLines() const { return m_directiveLines; }
    inline const QVector<int > &matches() const { return m_matches; }
    inline const QVector<QPair<int, int > > &disabledRanges() const
    { return m_disabledRanges; }
    inline const QVector<QPair<int, int > > &lineRanges() const
    { return m_ranges; }
    AstParseResult buildAst(AstArena *arena) const;

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
            const FormatRule *rule, const QByteArray &key) const
    {
        return m_ruleCaches.value(rule).value(key);
    }
    inline void setRuleCache(
            const FormatRule *rule,
            const QByteArray &key,
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
    FormatSpacing onSetting(const FormatSpacing & setting,
                            int index,
                            QByteArray *replacedLine = Q_NULLPTR);

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
    int commentAfter(int offset) const;
    bool hasCommentBetween(int from, int to) const;
    bool hasProtectedCommentBetween(int from, int to) const;
    bool overlapsComment(int from, int to) const;
    bool isOnlyWhitespace(int from, int to) const;
    bool isCodeWhitespace(int from, int to, bool allowNewlines = true) const;
    QByteArray codeSlice(int from, int to, bool keepWidth = false) const;
    QByteArray codeLineText(int line) const;

    void setLineRanges(const QVector<QPair<int, int > > & ranges);
    bool isLineSelected(int line) const;
    bool isSpanEditable(int offset, int length) const;
    void resetTextAndAnalyze(const QByteArray & text, bool reanalyze = true);

    bool rewriteCommentSpan(int offset, int length, const QByteArray & text);
    bool promoteTrailingComment(
            int removeFrom,
            int removeLength,
            int insertOffset,
            const QByteArray & leadingLine);
    inline bool takeCommentChanged()
    {
        const bool changed = m_commentChanged;
        m_commentChanged = false;
        return changed;
    }

    void report(
            int offset,
            const char *rule,
            const QString & message,
            bool isError = false);
    void request(
            int offset,
            const char *rule,
            const QString & message,
            int lineCount = 1,
            int columnLimit = 0);
    inline const QList<FormatIssue > &requests() const { return m_requests; }
    inline void setIssues(const QList<FormatIssue > & issues) { m_issues = issues; }
    inline void setRequests(const QList<FormatIssue > & requests) { m_requests = requests; }
    void truncateIssues(int count);
    static void shiftIssues(QList<FormatIssue > *issues, int line, int delta);

private:

    bool isOffsetEditable(int offset, int length) const;
    void spliceTextKeepTokens(int offset, int length, const QByteArray & text);
    void analyze();
    void appendComment(const Symbol & symbol, int previousCodeLine);
    void analyzeBrackets();
    void analyzeLines();
    Scope classifyBrace(int index, Scope enclosing) const;

    QByteArray m_text;
    QString m_path;
    FormatLanguage::Id m_language;
    FormatFileConfig m_fileConfig;
    QVector<QPair<int, FormatFileConfig > > m_fileConfigRegions;
    QVector<QPair<int, int > > m_inlineRanges;
    QVector<QPair<int, int > > m_disabledRanges;
    bool m_inlineConfigured;
    QString m_inlineError;
    QHash<const FormatRule *, QHash<QByteArray, QVariant > > m_ruleCaches;
    int m_pass;
    bool m_agentMode;
    bool m_commentChanged;
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
    QList<FormatIssue > m_issues;
};


#endif // FORMAT_DOCUMENT_H
