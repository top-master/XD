// Copyright (C) 2015 The XD Company Ltd.
// SPDX-License-Identifier: Apache-2.0

#include "./formatter.h"
#include "./format-document.h"
#include "./format-line-spec.h"
#include "./format-rules.h"
#include "./parser/code-helpers.h"
#include "./parser/comment-helpers.h"
#include "./parser/class-helpers.h"
#include "./parser/class-relations.h"
#include "./parser/condition-context.h"
#include "./parser/type-mapping.h"
#include "./parser/condition-layout.h"
#include "./parser/expression-layout.h"
#include "./parser/layout-helpers.h"
#include "./parser/scope-conditions.h"

#include "preprocessor.h"
#include "config-condition.h"

#include <QtCore/QDir>
#include <QtCore/QFileInfo>
#include <QtCore/QRegularExpression>
#include <QtCore/QSharedPointer>

/*!
    @class Formatter
    @internal

    Formats a file's text in one traversal using the selected checks, the
    way the `format` tool and its tests use it.

    Holds no state that a format() call changes; hence several threads may
    format several files with one Formatter at once.
*/

/*!
    Returns the token where the name of the declaration whose `(` is at
    @a paren starts: its `operator` keyword, or its first name past any
    `::`; -1 when @a paren opens no declaration.
*/
static int declarationName(int paren, const FormatDocument * document)
{
    int name = paren - 1;
    bool isOperator = false;
    for (int k = paren - 1; k >= qMax(1, paren - 16); --k) {
        const Token token = document->token(k);
        if (token == OPERATOR) {
            name = k;
            isOperator = true;
            break;
        }
        if (token == SEMIC
            || token == LBRACE
            || token == RBRACE
            || token == COLON
            || ((token == LPAREN || token == RPAREN) && k < paren - 2)
        ) {
            break;
        }
    }
    if ( ! isOperator && document->token(name) != IDENTIFIER) {
        return -1;
    }
    if ( ! isOperator && document->token(name - 1) == TILDE) {
        return name - 1;
    }
    while (name >= 2
        && document->token(name - 1) == SCOPE
        && document->token(name - 2) == IDENTIFIER
    ) {
        name -= 2;
    }
    return name;
}

/*!
    Lays out the lines of a declaration whose parameter list holds comments:
    the name starts on the line after the return type, the closing `)` joins
    the last parameter unless a line comment trails it, and a qualifier or a
    `{` that no longer fits takes a line of its own.
*/
static QList<QByteArray > commentedSignatureLines(
        const FormatDocument * document, int paren, int close,
        const QList<QByteArray > & lines, const QByteArray & prefix,
        const QByteArray & suffix, const QByteArray & indent, int limit)
{
    QList<QByteArray > result = lines;
    if (result.size() < 2) {
        return result;
    }

    // Starts the name on the line after the return type.
    const int name = declarationName(paren, document);
    if (name > 0 && document->symbolLine(name) == document->symbolLine(paren)) {
        const int column = document->symbols().at(name).from
            - document->lineStart(document->symbolLine(paren));
        if (column > 0 && column < result.first().size()
            && result.first().startsWith(prefix)
        ) {
            const QByteArray head = result.first();
            result[0] = head.left(column).trimmed();
            result.insert(1, indent + head.mid(column));
        }
    }

    // Ends the last parameter line at the closing `)`.
    if (suffix.contains('\n') || ! result.last().endsWith(suffix)) {
        return result;
    }
    QByteArray last = result.last();
    last.chop(suffix.size());
    QByteArray rest = suffix.mid(1);
    while (rest.startsWith(' ') || rest.startsWith('\t')) {
        rest.remove(0, 1);
    }
    while (rest.endsWith(' ') || rest.endsWith('\t')) {
        rest.chop(1);
    }
    QByteArray brace;
    if (rest.endsWith('{')) {
        brace = "{";
        rest.chop(1);
        while (rest.endsWith(' ') || rest.endsWith('\t')) {
            rest.chop(1);
        }
    }
    bool trailsLast = false;
    for (int index = 0; index < document->comments().size() && ! trailsLast; ++index) {
        const FormatComment & comment = document->comments().at(index);
        trailsLast = comment.isLine()
            && comment.offset >= document->symbolEnd(close - 1)
            && comment.offset < document->symbols().at(close).from
            && document->commentLine(index) == document->symbolEndLine(close - 1);
    }
    const QByteArray pad = continuationPad(indent.size());
    result.removeLast();
    QByteArray parameterLine = last;
    if ( ! trailsLast) {
        parameterLine += ')';
        const bool joins = rest.startsWith(';') || rest.startsWith(',');
        if ( ! rest.isEmpty() && brace.isEmpty()
            && textWidth(parameterLine) + (joins ? 0 : 1) + textWidth(rest) <= limit
        ) {
            parameterLine += joins ? rest : ' ' + rest;
            rest.clear();
        }
        result << parameterLine;
    } else {
        result << parameterLine;
        QByteArray closeLine = pad + ')';
        const bool joins = rest.startsWith(';') || rest.startsWith(',');
        if ( ! rest.isEmpty()
            && textWidth(closeLine) + (joins ? 0 : 1) + textWidth(rest) <= limit
        ) {
            closeLine += joins ? rest : ' ' + rest;
            rest.clear();
        }
        result << closeLine;
    }
    if ( ! rest.isEmpty()) {
        result << pad + rest;
    }
    if ( ! brace.isEmpty()) {
        result << indent + brace;
    }
    return result;
}

/*!
    Builds the formatter with every rule selected.

*/
Formatter::Formatter()
    : m_rules(formatCreateRules())
{
    for (int i = 0; i < this->m_rules.size(); ++i) {
        this->m_selected.append(this->m_rules.at(i));
    }
}

/*!
    Destroys the rules.
*/
Formatter::~Formatter()
{
    qDeleteAll(this->m_rules);
}

/*!
    Lists every available check.
*/
QList<const FormatRule * > Formatter::rules() const
{
    QList<const FormatRule * > rules;
    for (int i = 0; i < this->m_rules.size(); ++i) {
        rules.append(this->m_rules.at(i));
    }
    return rules;
}

/*!
    Selects only the rules that @a names lists (in the formatter's own
    order), or every rule for an empty list; returns false, with @a error
    naming it, when a name matches no rule.
*/
bool Formatter::selectRules(const QStringList & names, QString *error)
{
    QList<const FormatRule * > selected;
    for (int n = 0; n < names.size(); ++n) {
        bool found = false;
        for (int i = 0; i < this->m_rules.size(); ++i) {
            found = found
                || QString::fromLatin1(this->m_rules.at(i)->name())
                    == names.at(n);
        }
        if ( ! found) { *error = QString(QLL("unknown rule: %1")).arg(names.at(n));
            return false;
        }
    }
    for (int i = 0; i < this->m_rules.size(); ++i) {
        if (names.isEmpty()
            || names.contains(QString::fromLatin1(this->m_rules.at(i)->name()))
        ) {
            selected.append(this->m_rules.at(i));
        }
    }
    this->m_selected = selected;
    return true;
}

/*!
    Formats @a text, the content of the file at @a path, with the selected
    rules, on the lines of the git-blame style @a lineSpecs (the whole text
    when it is empty).

    Applies code checks during one traversal. Runs each comment check until
    it queues no more edits, at most eight times.
*/
FormatResult Formatter::format(
        const QByteArray & text,
        const QString & path,
        const QStringList & lineSpecs,
        const FormatConfig & config,
        bool agentMode) const
{
    FormatResult result;
    FormatDocument document(text, path);
    FormatConfig activeConfig = config;
    if (activeConfig.scheduledFiles.isEmpty() && ! path.isEmpty()) {
        activeConfig.scheduledFiles << path;
    }
    document.setFormatConfig(activeConfig);
    document.setAgentMode(agentMode);
    document.setIncludePaths(config.includePaths);

    QList<const FormatRule * > activeChecks = m_selected;
    if ( ! config.checks.isEmpty()) {
        activeChecks.clear();
        for (int index = 0; index < config.checks.size(); ++index) {
            bool found = false;
            for (int rule = 0; rule < m_rules.size(); ++rule) {
                if (QString::fromLatin1(m_rules.at(rule)->name())
                    == config.checks.at(index))
                {
                    found = true;
                    break;
                }
            }
            if ( ! found) {
                result.error = QString(QLL("unknown rule: %1"))
                    .arg(config.checks.at(index));
                return result;
            }
        }
        for (int index = 0; index < m_rules.size(); ++index) {
            if (config.checks.contains(QString::fromLatin1(m_rules.at(index)->name()))) {
                activeChecks.append(m_rules.at(index));
            }
        }
    }

    // Reads the project's choices for the file.
    FormatFileConfig fileConfig;
    if ( ! FormatFileConfig::load(path, config, &fileConfig, &result.error)) {
        return result;
    }
    document.setConfig(fileConfig);
    if ((text.contains("Format rule")
            || text.contains('@'))
        && ! document.setInlineConfiguration(config, &result.error)
    ) {
        return result;
    }

    // Resolves the lines to format.
    if ( ! lineSpecs.isEmpty()) {
        QVector<QPair<int, int > > ranges;
        QList<QByteArray > lines = text.split('\n');

        // Drops the empty part after a last newline, which starts no line.
        if (text.endsWith('\n')) {
            lines.removeLast();
        }
        if ( ! FormatLineSpec::resolveAll(
            lineSpecs,
            lines,
            &ranges,
            &result.error
        )) {
            return result;
        }
        document.setLineRanges(ranges);
    }

    bool fixMocMacro = false;
    bool spaceLogicalNot = false;
    bool checkExcludedFlags = false;
    bool checkPrivateAccess = false;
    bool checkReferenceLocal = false;
    bool checkLambdas = false;
    bool splitStatements = false;
    bool checkMemberInitializers = false;
    bool spaceTemplateClosers = false;
    bool spaceReferences = false;
    bool closeConditions = false;
    bool checkStaticVariables = false;
    bool moveOperators = false;
    bool joinMembers = false;
    bool adjustIncludes = false;
    bool wrapConditions = false;
    bool wrapArguments = false;
    bool wrapParameters = false;
    bool wrapTernaries = false;
    bool wrapOperators = false;
    bool braceControls = false;
    bool leadInitializers = false;
    bool expandBodies = false;
    bool indentContinuations = false;
    bool inlineMethods = false;
    bool replaceTypes = false;
    bool checkConstOverloads = false;
    bool checkGuardedGetters = false;
    bool layoutConnections = false;
    bool checkCodeShapes = false;
    bool qualifyMembers = false;
    bool manageSuper = false;
    bool formatNamespaces = false;
    bool joinExpressions = false;
    for (int index = 0; index < activeChecks.size(); ++index) {
        fixMocMacro |= activeChecks.at(index)->name() == "moc-run-macro";
        spaceLogicalNot |= activeChecks.at(index)->name() == "not-space";
        checkExcludedFlags |= activeChecks.at(index)->name() == "excludes";
        checkPrivateAccess |= activeChecks.at(index)->name() == "q-d-macro";
        checkReferenceLocal |= activeChecks.at(index)->name() == "reference-local";
        checkLambdas |= activeChecks.at(index)->name() == "no-lambda";
        splitStatements |= activeChecks.at(index)->name() == "one-statement";
        checkMemberInitializers |= activeChecks.at(index)->name() == "member-init";
        spaceTemplateClosers |= activeChecks.at(index)->name() == "template-close-space";
        spaceReferences |= activeChecks.at(index)->name() == "reference-space";
        closeConditions |= activeChecks.at(index)->name() == "condition-close";
        checkStaticVariables |= activeChecks.at(index)->name() == "global-static";
        moveOperators |= activeChecks.at(index)->name() == "operator-first";
        joinMembers |= activeChecks.at(index)->name() == "chain-members-first";
        adjustIncludes |= activeChecks.at(index)->name() == "relative-include";
        wrapConditions |= activeChecks.at(index)->name() == "condition-wrap";
        wrapArguments |= activeChecks.at(index)->name() == "args-wrap";
        wrapParameters |= activeChecks.at(index)->name() == "param-wrap";
        wrapTernaries |= activeChecks.at(index)->name() == "ternary-wrap";
        wrapOperators |= activeChecks.at(index)->name() == "operator-wrap";
        braceControls |= activeChecks.at(index)->name() == "control-braces";
        leadInitializers |= activeChecks.at(index)->name() == "init-list-lead";
        expandBodies |= activeChecks.at(index)->name() == "body-braces";
        indentContinuations |= activeChecks.at(index)->name() == "continuation-indent";
        inlineMethods |= activeChecks.at(index)->name() == "inline-keyword";
        replaceTypes |= activeChecks.at(index)->name() == "replace-types";
        checkConstOverloads |= activeChecks.at(index)->name() == "const-overload-order";
        checkGuardedGetters |= activeChecks.at(index)->name() == "qpointer-getter";
        layoutConnections |= activeChecks.at(index)->name() == "connect-layout";
        checkCodeShapes |= activeChecks.at(index)->name() == "code-shape";
        qualifyMembers |= activeChecks.at(index)->name() == "outline-this";
        manageSuper |= activeChecks.at(index)->name() == "super-typedef";
        formatNamespaces |= activeChecks.at(index)->name() == "namespace-braces";
        joinExpressions |= activeChecks.at(index)->name() == "line-join";
    }

    {
        FormatDocument *source = &document;
        QList<int > lambdaBodies;
        int declarationStart = 0;
        int declarationSkipUntil = 0;
        int conditionSkipUntil = 0;
        int parameterSkipUntil = 0;
        int initializerSkipUntil = 0;
        int mappingSkipUntil = 0;
        int connectSkipUntil = 0;
        int superSkipUntil = 0;
        int joinedUntil = 0;
        QByteArray relationsText = source->text();
        QHash<QString, QStringList > pathsByExtensions;
        QHash<QString, QSharedPointer<FormatDocument > > owners;
        QHash<QString, int > classBraces;
        QHash<QString, QStringList > programs;
        const bool inXd = checkLambdas && formatIsInXdTree(source->path());
        const QDir directory = source->path().isEmpty()
            ? QDir() : QFileInfo(source->path()).absoluteDir();
        const QRegularExpression include(
            QString(QLL("^\\s*#\\s*include\\s*\"([^\"]+)\""))
        );
        for (int index = 0; index < source->symbols().size(); ++index) {
            if (joinExpressions && index >= joinedUntil) {
                QList<int > brackets;
                int questions = 0;
                int limit = index;
                for (; limit + 1 < source->symbols().size(); ++limit) {
                    const Token token = source->token(limit);
                    if (token == LPAREN || token == LBRACK
                        || (token == LBRACE
                            && ( ! brackets.isEmpty()
                                || source->braceScope(limit) == FormatDocument::InitScope)))
                    {
                        brackets.append(limit);
                    } else if ((token == RPAREN || token == RBRACK || token == RBRACE)
                        && ! brackets.isEmpty())
                    {
                        brackets.removeLast();
                    } else if (brackets.isEmpty()
                        && (token == SEMIC || token == LBRACE || token == RBRACE))
                    {
                        break;
                    }
                }
                joinedUntil = qMax(index + 1, limit + 1);
                brackets.clear();
                FormatScopeConditions conditions(source);
                CodeEditor editor = document.edit(source->symbols().at(index).from,
                    limit + 1 < source->symbols().size()
                        ? source->symbols().at(limit + 1).from : source->text().size());
                int templateEnd = -1;
                for (int cursor = index; cursor < limit
                    && cursor + 1 < source->symbols().size(); ++cursor)
                {
                    const Token token = source->token(cursor);
                    if (token == LANGLE && templateEnd < cursor
                        && ! brackets.isEmpty()
                        && isDeclarationParen(source, brackets.last()))
                    {
                        templateEnd = templateClose(source, cursor, limit + 1);
                    }
                    if (token == LPAREN || token == LBRACK || token == LBRACE) {
                        brackets.append(cursor);
                    } else if ((token == RPAREN || token == RBRACK || token == RBRACE)
                        && ! brackets.isEmpty())
                    {
                        brackets.removeLast();
                    }
                    if (token == SEMIC || token == LBRACE || token == RBRACE) {
                        questions = 0;
                    } else if (token == QUESTION) {
                        ++questions;
                    } else if (token == COLON && questions > 0) {
                        --questions;
                    }
                    const int next = cursor + 1;
                    if ( ! codeTargetAnyAt(source, source->symbols().at(cursor).from)
                        || cursor < templateEnd
                        || source->symbolEndLine(cursor) >= source->symbolLine(next)
                        || source->isDirectiveLine(source->symbolEndLine(cursor))
                        || source->isDirectiveLine(source->symbolLine(next))
                        || source->hasCommentBetween(source->symbolEnd(cursor),
                                                   source->symbols().at(next).from)
                        || (token == STRING_LITERAL && source->token(next) == STRING_LITERAL))
                    {
                        continue;
                    }
                    const Token after = source->token(next);
                    const int nextLine = source->symbolLine(next);
                    const int lineEnd = source->lastSymbolOfLine(nextLine);
                    if ((after == DOT || after == ARROW)
                        && textWidth(source->lineText(source->symbolEndLine(cursor)))
                            > wrapMaxAt(source, source->symbols().at(cursor).from))
                    {
                        continue;
                    }
                    if (((after == ANDAND || after == OROR)
                            && isClosingLine(editor.lineText(source->symbolEndLine(cursor))))
                        || (isClosingLine(editor.lineText(nextLine)) && lineEnd >= next
                            && (source->token(lineEnd + 1) == ANDAND
                                || source->token(lineEnd + 1) == OROR)
                            && source->symbolLine(lineEnd + 1) > nextLine))
                    {
                        continue;
                    }
                    bool insideExpression = false;
                    if ( ! brackets.isEmpty()) {
                        const int open = brackets.last();
                        insideExpression = source->token(open) != LBRACE
                            || source->braceScope(open) == FormatDocument::InitScope;
                    }
                    if ( ! insideExpression && ! isLeadingOperator(after)
                        && after != DOT && after != ARROW && token != EQ
                        && ! (after == COLON && questions > 0))
                    {
                        continue;
                    }
                    const FormatFileConfig &fileConfig = source->configAt(
                        source->symbols().at(cursor).from);
                    if ( ! brackets.isEmpty()) {
                        const int open = brackets.last();
                        const int close = source->match(open);
                        const Token beforeOpen = source->token(open - 1);
                        if ( ! fileConfig.wrapArgsOverCondition.isEmpty()
                            && source->token(open) == LPAREN && close > open
                            && (beforeOpen == IDENTIFIER || beforeOpen == RPAREN
                                || beforeOpen == RANGLE || beforeOpen == GTGT)
                            && ! conditions.argsOver(open, splitArguments(source, open, close).size()))
                        {
                            continue;
                        }
                        if ( ! fileConfig.wrapConditionsOverCondition.isEmpty()
                            && source->token(open) == LPAREN
                            && (beforeOpen == IF || beforeOpen == WHILE)
                            && ! conditions.conditionsOver(open,
                                conditions.conditionArgumentCount(open, 0)))
                        {
                            continue;
                        }
                    }
                    const bool tight = token == LPAREN || token == LBRACK
                        || after == RPAREN || after == RBRACK || after == COMMA
                        || after == SEMIC || token == DOT || token == ARROW
                        || after == DOT || after == ARROW;
                    const int from = source->symbolEnd(cursor);
                    editor.replace(from, source->symbols().at(next).from - from,
                                   tight ? QByteArray() : QByteArray(" "));
                }
                editor.apply();
            }
            const int start = source->symbols().at(index).from;
            const int end = source->symbolEnd(index);
            if (adjustIncludes && ! source->path().isEmpty()) {
                const int line = source->symbolLine(index);
                if (source->firstSymbolOfLine(line) == index
                    && source->isDirectiveLine(line))
                {
                    const QString text = QString::fromUtf8(source->lineText(line));
                    const QRegularExpressionMatch found = include.match(text);
                    if (found.hasMatch()) {
                        const QString name = found.captured(1);
                        if ( ! name.startsWith(QLL("./"))
                            && ! name.startsWith(QLL("../"))
                            && ! QDir::isAbsolutePath(name)
                            && QFileInfo(directory.filePath(name)).isFile())
                        {
                            const int offset = source->lineStart(line)
                                + text.left(found.capturedStart(1)).toUtf8().size();
                            CodeEditor editor = document.edit(offset, offset);
                            editor.add(offset, "./");
                            editor.apply();
                        }
                    }
                }
            }
            if ((moveOperators || joinMembers) && index > 0
                && index + 1 < source->symbols().size())
            {
                const Token kind = source->token(index);
                const bool member = kind == DOT || kind == ARROW
                    || kind == DOTSTAR || kind == ARROW_STAR;
                if ((member && joinMembers) || ( ! member && moveOperators)) {
                    const int head = source->headStart(index);
                    const int rangeStart = source->lineStart(source->symbolLine(head));
                    const int followingLine = source->symbolEndLine(index + 1);
                    const int rangeEnd = qMin(source->text().size(),
                        source->lineEnd(followingLine) + 1);
                    const int oldCount = source->symbols().size();
                    CodeEditor editor = document.edit(rangeStart, rangeEnd);
                    FormatDocument *copy = editor.document();
                    FormatScopeConditions conditions(copy);
                    bool handled = layoutChainOperand(copy, &conditions, index)
                        || joinsClosingLine(copy, index);
                    if ( ! handled
                        && (kind == ANDAND || kind == OROR || kind == LTLT
                            || kind == PLUS || kind == MINUS || kind == OR
                            || kind == HAT || kind == EQEQ || kind == NE
                            || kind == QUESTION))
                    {
                        const int line = copy->symbolLine(index);
                        const int from = copy->symbolEnd(index - 1);
                        const int next = copy->symbols().at(index + 1).from;
                        if (copy->lastSymbolOfLine(line) == index
                            && ! copy->isDirectiveLine(line)
                            && copy->symbolEndLine(index - 1) == line
                            && copy->token(index - 1) != OPERATOR
                            && ! copy->hasCommentBetween(copy->symbolEnd(index), next)
                            && ! copy->spansDirective(from, next))
                        {
                            handled = editor.replace(
                                from, copy->symbolEnd(index) - from, QByteArray(),
                                next, 0, copy->lexem(index) + ' '
                            );
                        }
                    }
                    if (handled && editor.apply()) {
                        const int shift = source->symbols().size() - oldCount;
                        if (joinedUntil > index) {
                            joinedUntil += shift;
                        }
                        if (superSkipUntil > index) {
                            superSkipUntil += shift;
                        }
                        if (connectSkipUntil > index) {
                            connectSkipUntil += shift;
                        }
                        if (mappingSkipUntil > index) {
                            mappingSkipUntil += shift;
                        }
                        if (initializerSkipUntil > index) {
                            initializerSkipUntil += shift;
                        }
                        if (parameterSkipUntil > index) {
                            parameterSkipUntil += shift;
                        }
                        if (conditionSkipUntil > index) {
                            conditionSkipUntil += shift;
                        }
                        if (declarationSkipUntil > index) {
                            declarationSkipUntil += shift;
                        }
                        --index;
                        continue;
                    }
                }
            }
            if (fixMocMacro && source->token(index) == IDENTIFIER
                && source->lexem(index) == "QT_MOC_RUN")
            {
                CodeEditor editor = document.edit(start, end);
                editor.replace(start, end - start, "Q_MOC_RUN");
                editor.apply();
            }
            if (replaceTypes && index >= mappingSkipUntil
                && source->token(index) == IDENTIFIER
                && source->token(index + 1) == LPAREN)
            {
                const int closing = source->match(index + 1);
                if (closing > index + 1) {
                    const int head = source->headStart(index);
                    const int rangeStart = source->lineStart(source->symbolLine(head));
                    const int oldCount = source->symbols().size();
                    int rescanFrom = start;
                    CodeEditor editor = document.edit(rangeStart, source->symbolEnd(closing));
                    FormatDocument *document = editor.document();
                    const int i = index;
                    do {
                        if (document->token(i) != IDENTIFIER
                            || document->token(i + 1) != LPAREN
                            || document->match(i + 1) <= i + 1
                            || document->isDirectiveLine(document->symbolLine(i))
                        ) {
                            break;
                        }
                        const int close = document->match(i + 1);
                        const FormatFileConfig &config = document->configAt(
                            document->symbols().at(i).from
                        );
                        const QString last = QString::fromLatin1(document->lexem(i));
                        bool mayMatch = false;
                        for (int m = 0; m < config.replaceTypes.size() && ! mayMatch; ++m) {
                            mayMatch = FormatTypeMapping::lastPartMatches(
                                config.replaceTypes.at(m).first,
                                last
                            );
                        }
                        if ( ! mayMatch) {
                            break;
                        }

                        // Finds the start of the name as written, `A::B::Name` or `::Name`.
                        int start = i;
                        while (start >= 2 && document->token(start - 1) == SCOPE
                            && document->token(start - 2) == IDENTIFIER
                        ) {
                            start -= 2;
                        }
                        if (start >= 1 && document->token(start - 1) == SCOPE) {
                            --start;
                        }
                        if ( ! FormatTypeMapping::isCall(document, start)) {
                            break;
                        }
                        int end = i;
                        const QString written = FormatTypeMapping::readName(document, start, &end);
                        const QStringList scopes = FormatTypeMapping::visibleScopes(document, start);
                        QString capture;
                        const QStringList arguments = FormatTypeMapping::callArguments(source, i + 1, close);
                        QStringList bound;
                        const int mapping = FormatTypeMapping::mappingOf(
                            config.replaceTypes, FormatTypeMapping::candidates(written, scopes), arguments,
                            &capture, &bound
                        );
                        if (mapping < 0) {
                            break;
                        }

                        // Checks `target` for this line.
                        const FormatReplaceTypesContext context(
                            document,
                            i,
                            &config
                        );
                        QStringList *program = &(programs[config.replaceTypesTarget]);
                        if (program->isEmpty()) {
                            Config::Evaluator::MismatchScope scope(
                                &context, Config::NameMismatchIgnore
                            );
                            *program = Config::Condition::compile(
                                config.replaceTypesTarget,
                                &context
                            );
                            scope.close();
                            if ( ! context.lastCompiled()) {
                                break;
                            }
                        }
                        const QVariant wanted = FormatFileConfig::resolve(
                            QString(QLL("replace.types.target")), *program, &context
                        );
                        if ( ! context.lastResolvePassed()
                            || ! wanted.toBool()
                        ) {
                            break;
                        }

                        const int from = document->symbols().at(start).from;
                        rescanFrom = from;
                        QString head;
                        QString call;
                        QString error;
                        if ( ! FormatTypeMapping::replacement(
                            config.replaceTypes,
                            mapping,
                            capture,
                            arguments,
                            bound,
                            &context,
                            &head,
                            &call,
                            &error
                        )) {
                            document->report(from, "replace-types",
                                             QString(QLL("%1: %2")).arg(written, error));
                            break;
                        }
                        if (head.isEmpty()) {
                            break;
                        }
                        const QString shortest = FormatTypeMapping::shortestName(head, scopes);
                        const int to = document->symbolEnd(close);
                        const QString actionKey = QString(QLL("replace.types.action"));
                        const QVariant action = config.hasExpression(actionKey)
                            ? config.resolveSetting(actionKey, &context) : QVariant();
                        const QString mode = action.isValid() ? action.toString() : config.replaceTypesAction;
                        if (mode == QLL("replace")
                            && document->replace(
                                from,
                                to - from,
                                (shortest + call).toLatin1()
                            )
                        ) {
                            mappingSkipUntil = close + 1;
                            break;
                        }
                        document->report(from, "replace-types",
                                         QString(QLL("write %1%2")).arg(shortest, call));
                    } while (false);
                    const bool changed = editor.apply();
                    const int shift = source->symbols().size() - oldCount;
                    if (joinedUntil > index) {
                        joinedUntil += shift;
                    }
                    if (superSkipUntil > index) {
                        superSkipUntil += shift;
                    }
                    if (connectSkipUntil > index) {
                        connectSkipUntil += shift;
                    }
                    if (mappingSkipUntil > index) {
                        mappingSkipUntil += shift;
                    }
                    if (initializerSkipUntil > index) {
                        initializerSkipUntil += shift;
                    }
                    if (parameterSkipUntil > index) {
                        parameterSkipUntil += shift;
                    }
                    if (conditionSkipUntil > index) {
                        conditionSkipUntil += shift;
                    }
                    if (declarationSkipUntil > index) {
                        declarationSkipUntil += shift;
                    }
                    if (changed) {
                        index = source->symbolAfter(rescanFrom) - 1;
                        continue;
                    }
                }
            }
            if (spaceTemplateClosers && index > 0
                && source->token(index) == LANGLE
                && ! source->isDirectiveLine(source->symbolLine(index)))
            {
                const Token previous = source->token(index - 1);
                const bool isCast = previous == STATIC_CAST || previous == DYNAMIC_CAST
                    || previous == REINTERPRET_CAST || previous == CONST_CAST;
                if (isCast || (previous == IDENTIFIER
                    && source->token(index - 2) != OPERATOR))
                {
                    QList<int > closers;
                    int depth = 0;
                    int cursor = index;
                    bool valid = true;
                    for (; cursor < source->symbols().size(); ++cursor) {
                        const Token kind = source->token(cursor);
                        if (kind == LANGLE) {
                            ++depth;
                        } else if (kind == RANGLE || kind == GTGT) {
                            depth -= kind == GTGT ? 2 : 1;
                            if (depth < 0) {
                                valid = false;
                                break;
                            }
                            closers.append(cursor);
                            if (depth == 0) {
                                break;
                            }
                        } else if (kind == LPAREN || kind == LBRACK) {
                            const int close = source->match(cursor);
                            if (close < 0) {
                                valid = false;
                                break;
                            }
                            cursor = close;
                        } else if (kind != IDENTIFIER && kind != SCOPE
                            && kind != STAR && kind != AND && kind != CONST
                            && kind != VOLATILE && kind != COMMA
                            && kind != INTEGER_LITERAL && ! isBuiltinType(kind))
                        {
                            valid = false;
                            break;
                        }
                    }
                    const Token after = source->token(cursor + 1);
                    valid = valid && depth == 0
                        && (after == IDENTIFIER || after == SCOPE
                            || after == LPAREN || after == RPAREN
                            || after == COMMA || after == SEMIC || after == AND
                            || after == STAR || after == ANDAND || after == LBRACE
                            || after == RANGLE || after == GTGT || after == COLON
                            || after == EQ);
                    if (valid && ! closers.isEmpty()) {
                        const int last = source->symbolEnd(closers.last());
                        const int oldCount = source->symbols().size();
                        CodeEditor editor = document.edit(start, last);
                        for (int closer = 0; closer < closers.size(); ++closer) {
                            const int token = closers.at(closer);
                            const int offset = source->symbols().at(token).from;
                            const bool spaced = offset > 0
                                && source->text().at(offset - 1) == ' ';
                            if (source->token(token) == GTGT) {
                                editor.replace(offset, 2, spaced ? "> >" : " > >");
                            } else if ( ! spaced) {
                                editor.add(offset, " ");
                            }
                        }
                        editor.apply();
                        const int shift = source->symbols().size() - oldCount;
                        if (joinedUntil > index) {
                            joinedUntil += shift;
                        }
                        if (superSkipUntil > index) {
                            superSkipUntil += shift;
                        }
                        if (connectSkipUntil > index) {
                            connectSkipUntil += shift;
                        }
                        if (mappingSkipUntil > index) {
                            mappingSkipUntil += shift;
                        }
                        if (initializerSkipUntil > index) {
                            initializerSkipUntil += shift;
                        }
                        if (parameterSkipUntil > index) {
                            parameterSkipUntil += shift;
                        }
                        if (conditionSkipUntil > index) {
                            conditionSkipUntil += shift;
                        }
                        if (declarationSkipUntil > index) {
                            declarationSkipUntil += shift;
                        }
                    }
                }
            }
            if (checkMemberInitializers && index >= declarationSkipUntil
                && source->scopeOf(index) == FormatDocument::ClassScope)
            {
                const Token kind = source->token(index);
                const Token previous = source->token(index - 1);
                if (kind == SEMIC || kind == LBRACE || kind == RBRACE
                    || (kind == COLON && (previous == PUBLIC
                        || previous == PRIVATE || previous == PROTECTED
                        || previous == SLOTS || previous == SIGNALS)))
                {
                    declarationStart = index + 1;
                } else if (kind == LPAREN) {
                    declarationSkipUntil = qMax(index, source->match(index)) + 1;
                } else if (kind == EQ) {
                    bool skip = previous == RPAREN || previous == CONST
                        || previous == OPERATOR;
                    for (int token = declarationStart; token < index && ! skip; ++token) {
                        const Token head = source->token(token);
                        skip = head == STATIC || head == TYPEDEF || head == USING
                            || head == ENUM || head == FRIEND || head == TEMPLATE
                            || (head == IDENTIFIER
                                && source->lexem(token) == "constexpr");
                    }
                    if ( ! skip) {
                        source->report(start, "member-init", QString(QLL(
                            "initialize the member in the constructor's "
                            "initializer list instead")));
                    }
                    const int statement = statementEnd(source, index);
                    if (statement >= 0) {
                        declarationSkipUntil = statement;
                    }
                }
            }
            if (checkLambdas) {
                while ( ! lambdaBodies.isEmpty()
                    && index > source->match(lambdaBodies.last()))
                {
                    lambdaBodies.removeLast();
                }
                const int body = lambdaBody(source, index);
                if (body >= 0) {
                    if ( ! lambdaBodies.isEmpty()) {
                        source->report(start, "no-lambda", QString(QLL(
                            "move the outer lambda into a named function: "
                            "a lambda never encloses another one")));
                    } else if (inXd) {
                        source->report(start, "no-lambda", QString(QLL(
                            "replace the lambda with a named member or "
                            "static function")));
                    }
                    lambdaBodies.append(body);
                }
            }
            if (checkConstOverloads && source->token(index) == LBRACE
                && source->braceScope(index) == FormatDocument::ClassScope)
            {
                FormatDocument *document = source;
                const int brace = index;
            QHash<QByteArray, int > firstMutable;
            QSet<QByteArray > reported;
            const QList<int > tokens = classBodyTokens(document, brace);
            for (int t = 0; t < tokens.size(); ++t) {
                const int i = tokens.at(t);
                if (document->token(i) != IDENTIFIER
                    || document->token(i + 1) != LPAREN
                    || ! endsType(document->token(i - 1))
                    || document->match(i + 1) < 0
                ) {
                    continue;
                }
                const int close = document->match(i + 1);
                int commas = 0;
                for (int k = i + 2; k < close; ++k) {
                    if (document->token(k) == LPAREN
                        || document->token(k) == LANGLE
                    ) {
                        k = qMax(k, document->match(k));
                    } else if (document->token(k) == COMMA) {
                        ++commas;
                    }
                }
                const QByteArray key = document->lexem(i) + '/'
                    + QByteArray::number(commas);
                const bool isConst = document->token(close + 1) == CONST;
                if ( ! isConst && ! firstMutable.contains(key)) {
                    firstMutable.insert(key, i);
                } else if (isConst
                    && firstMutable.contains(key)
                    && ! reported.contains(key)
                ) {
                    reported.insert(key);
                    document->report(
                        document->symbols().at(firstMutable.value(key)).from,
                        "const-overload-order",
                        QString(QLL("declare the const overload of %1() first"))
                            .arg(QString::fromLatin1(document->lexem(i)))
                    );
                }
            }
            }
            if (checkGuardedGetters && source->token(index) == RETURN
                && index + 5 < source->symbols().size())
            {
                int value = index + 1;
                if (source->token(value) == THIS && source->token(value + 1) == ARROW) {
                    value += 2;
                }
                if (source->token(value) == IDENTIFIER
                    && source->token(value + 1) == DOT
                    && source->token(value + 2) == IDENTIFIER
                    && source->lexem(value + 2) == "data"
                    && source->token(value + 3) == LPAREN
                    && source->token(value + 4) == RPAREN)
                {
                    const QByteArray name = source->lexem(value);
                    bool guarded = false;
                    for (int candidate = 0;
                         candidate + 1 < source->symbols().size() && ! guarded;
                         ++candidate)
                    {
                        if (source->token(candidate) != IDENTIFIER
                            || source->lexem(candidate) != "QPointer"
                            || source->token(candidate + 1) != LANGLE)
                        {
                            continue;
                        }
                        int closing = candidate + 1;
                        int depth = 0;
                        for (; closing < source->symbols().size(); ++closing) {
                            const Token token = source->token(closing);
                            depth += token == LANGLE ? 1
                                : (token == RANGLE ? -1 : (token == GTGT ? -2 : 0));
                            if (depth <= 0) {
                                break;
                            }
                        }
                        guarded = source->token(closing + 1) == IDENTIFIER
                            && source->token(closing + 2) == SEMIC
                            && source->lexem(closing + 1) == name;
                    }
                    if (guarded) {
                        source->report(start, "qpointer-getter", QString(QLL(
                            "return the QPointer itself, not its data()")));
                    }
                }
            }
            if (checkCodeShapes && ! source->isDirectiveLine(source->symbolLine(index))) {
                const Token previous = source->token(index - 1);
                if (index == 0 || previous == SEMIC || previous == LBRACE
                    || previous == RBRACE)
                {
                    reportStatementShapes(source, index);
                }
                if (source->token(index) == LBRACE
                    && source->braceScope(index) == FormatDocument::FunctionScope)
                {
                    reportLongInlineBody(source, index);
                }
                const int line = source->symbolLine(index);
                if (source->firstSymbolOfLine(line) == index
                    && QString::fromUtf8(source->lineText(line)).size() > 70)
                {
                    const int last = source->lastSymbolOfLine(line);
                    for (int token = index; token < last; ++token) {
                        if (source->token(token) == RPAREN
                            && source->token(token + 1) == COLON
                            && source->match(token) >= index
                            && source->scopeOf(token) != FormatDocument::FunctionScope
                            && source->scopeOf(token) != FormatDocument::BlockScope
                            && source->symbolLine(token + 1) == line
                            && token + 2 <= last)
                        {
                            source->report(source->lineStart(line), "code-shape", QString(QLL(
                                "break the member-initializer list onto lines of its own")));
                            break;
                        }
                    }
                }
            }
            if (manageSuper && source->token(index) == LBRACE
                && source->braceScope(index) == FormatDocument::ClassScope)
            {
                const FormatConfig &runConfig = source->formatConfig();
                const FormatFileConfig &fileConfig = source->config();
                const bool useDefaults = ! source->hasInlineConfiguration()
                    && ! source->hasSelectedRanges()
                    && runConfig.fileLineSpecs.isEmpty() && runConfig.lineSpecs.isEmpty()
                    && fileConfig.typedefSuper == FormatFileConfig::defaultTypedefSuper()
                    && fileConfig.replaceSuperSites == FormatFileConfig::defaultReplaceSuperSites();
                const int closing = source->match(index);
                if ( ! useDefaults && closing > index) {
                    bool isStruct = false;
                    const QByteArray base = FormatRelations::baseName(source, index, &isStruct);
                    if ( ! base.isEmpty() && ! FormatRelations::hasSuper(source, index)
                        && FormatRelations::wantsTypedef(source, index, base))
                    {
                        if (isStruct) {
                            source->report(start, "super-typedef", QString(QLL(
                                "give the struct a private typedef %1 super;"))
                                .arg(QString::fromUtf8(base)));
                        } else {
                            const int oldCount = source->symbols().size();
                            CodeEditor editor = document.edit(start, source->symbolEnd(closing));
                            FormatDocument *copy = editor.document();
                            const int first = index + 1;
                            const int firstLine = copy->symbolLine(first);
                            const QByteArray text = "typedef " + base + " super;";
                            if (firstLine == copy->symbolLine(index)) {
                                const int offset = copy->symbolEnd(index);
                                const bool hasSpace = offset < copy->text().size()
                                    && (copy->text().at(offset) == ' '
                                        || copy->text().at(offset) == '\t');
                                editor.add(offset, " " + text + (hasSpace ? "" : " "));
                            } else if (copy->token(first) == Q_OBJECT_TOKEN
                                || copy->token(first) == Q_GADGET_TOKEN)
                            {
                                editor.add(contentEnd(copy, firstLine),
                                           "\n" + copy->indentOf(firstLine) + text);
                            } else {
                                const Token token = copy->token(first);
                                const bool access = token == PUBLIC || token == PRIVATE
                                    || token == PROTECTED || token == RBRACE;
                                const QByteArray indent = access
                                    ? copy->indentOf(copy->symbolLine(index)) + "    "
                                    : copy->indentOf(firstLine);
                                editor.add(copy->lineStart(firstLine), indent + text + "\n");
                            }
                            editor.apply();
                            const int shift = source->symbols().size() - oldCount;
                            if (joinedUntil > index) { joinedUntil += shift; }
                            if (superSkipUntil > index) { superSkipUntil += shift; }
                            if (connectSkipUntil > index) { connectSkipUntil += shift; }
                            if (mappingSkipUntil > index) { mappingSkipUntil += shift; }
                            if (initializerSkipUntil > index) { initializerSkipUntil += shift; }

                            if (parameterSkipUntil > index) { parameterSkipUntil += shift; }

                            if (conditionSkipUntil > index) { conditionSkipUntil += shift; }
                            if (declarationSkipUntil > index) { declarationSkipUntil += shift; }
                        }
                    }
                }
            }
            if (manageSuper && index >= superSkipUntil
                && (source->token(index) == IDENTIFIER || source->token(index) == SCOPE)
                && ! source->isDirectiveLine(source->symbolLine(index)))
            {
                const FormatConfig &runConfig = source->formatConfig();
                const FormatFileConfig &defaultConfig = source->config();
                const bool useDefaults = ! source->hasInlineConfiguration()
                    && ! source->hasSelectedRanges()
                    && runConfig.fileLineSpecs.isEmpty() && runConfig.lineSpecs.isEmpty()
                    && defaultConfig.typedefSuper == FormatFileConfig::defaultTypedefSuper()
                    && defaultConfig.replaceSuperSites == FormatFileConfig::defaultReplaceSuperSites();
                if ( ! useDefaults && ((source->token(index) == IDENTIFIER
                        && (source->token(index + 1) == SCOPE
                            || source->token(index + 1) == LANGLE))
                    || (source->token(index) == SCOPE
                        && source->token(index + 1) == IDENTIFIER)))
                {
                    if (relationsText != source->text()) {
                        relationsText = source->text();
                        pathsByExtensions.clear();
                        owners.clear();
                        classBraces.clear();
                    }
                    FormatScopeConditions scopes(source);
                    const QByteArray className = FormatRelations::ownerOf(source, &scopes, index);
                    if ( ! className.isEmpty()) {
                        const FormatFileConfig &fileConfig = source->configAt(start);
                        QString extensions = fileConfig.extensions;
                        const QString extensionsKey = QString(QLL("general.extensions"));
                        if (fileConfig.hasExpression(extensionsKey)) {
                            const QVariant resolved = scopes.settingAt(extensionsKey, index);
                            if (resolved.isValid()) {
                                extensions = resolved.toString();
                            }
                        }
                        if ( ! pathsByExtensions.contains(extensions)) {
                            FormatFileConfig effectiveConfig = fileConfig;
                            effectiveConfig.extensions = extensions;
                            pathsByExtensions.insert(extensions,
                                FormatRelations::relatedFiles(source, effectiveConfig));
                        }
                        const QString key = extensions + QLatin1Char('\n')
                            + QString::fromUtf8(className);
                        if ( ! classBraces.contains(key)) {
                            int found = FormatRelations::findQualifiedClass(source, className);
                            const QStringList paths = pathsByExtensions.value(extensions);
                            for (int path = 0; found < 0 && path < paths.size(); ++path) {
                                QSharedPointer<FormatDocument > related(
                                    FormatRelations::readRelated(paths.at(path), runConfig));
                                if (related) {
                                    found = FormatRelations::findQualifiedClass(related.data(), className);
                                    if (found >= 0) {
                                        owners.insert(key, related);
                                    }
                                }
                            }
                            classBraces.insert(key, found);
                        }
                        const int brace = classBraces.value(key);
                        if (brace >= 0) {
                            const FormatDocument *owner = owners.contains(key)
                                ? owners.value(key).data() : source;
                            bool isStruct = false;
                            const QByteArray base = FormatRelations::baseName(owner, brace, &isStruct);
                            const int scopeToken = base.isEmpty() ? -1
                                : FormatRelations::matchBase(source, index, base,
                                    ! className.contains("::"));
                            if (scopeToken >= 0
                                && ! FormatRelations::isShadowed(source, &scopes, index, base))
                            {
                                if (FormatRelations::permitsSuperReference(
                                    owner, brace, base, source, index))
                                {
                                    const bool constructor = index > 0
                                        && source->token(index - 1) == USING
                                        && scopeToken + 1 < source->symbols().size()
                                        && source->lexem(scopeToken + 1)
                                            == FormatRelations::baseIdentifier(base);
                                    const int to = constructor
                                        ? source->symbolEnd(scopeToken + 1)
                                        : source->symbols().at(scopeToken).from;
                                    const int oldCount = source->symbols().size();
                                    CodeEditor editor = document.edit(start, to);
                                    editor.set(start, to, constructor ? "super::super" : "super");
                                    const bool changed = editor.apply();
                                    const int shift = source->symbols().size() - oldCount;
                                    if (joinedUntil > index) { joinedUntil += shift; }
                                    if (superSkipUntil > index) { superSkipUntil += shift; }
                                    if (connectSkipUntil > index) { connectSkipUntil += shift; }
                                    if (mappingSkipUntil > index) { mappingSkipUntil += shift; }
                                    if (initializerSkipUntil > index) { initializerSkipUntil += shift; }

                                    if (parameterSkipUntil > index) { parameterSkipUntil += shift; }

                                    if (conditionSkipUntil > index) { conditionSkipUntil += shift; }
                                    if (declarationSkipUntil > index) { declarationSkipUntil += shift; }
                                    if (changed) {
                                        const int replacementEnd = start
                                            + (constructor ? int(sizeof("super::super") - 1)
                                                           : int(sizeof("super") - 1));
                                        superSkipUntil = source->symbolAfter(replacementEnd);
                                        index = source->symbolAfter(start) - 1;
                                        continue;
                                    }
                                }
                                superSkipUntil = scopeToken + 1;
                            }
                        }
                    }
                }
            }
            if (formatNamespaces && source->token(index) == NAMESPACE
                && (index == 0 || source->token(index - 1) != USING))
            {
                int open = index + 1;
                while (open < source->symbols().size()
                    && source->token(open) != LBRACE
                    && source->token(open) != EQ && source->token(open) != SEMIC)
                {
                    ++open;
                }
                const int close = source->match(open);
                if (source->token(open) == LBRACE && close > open) {
                    const int rangeEnd = source->lineEnd(source->symbolLine(close));
                    const int oldCount = source->symbols().size();
                    CodeEditor editor = document.edit(
                        source->lineStart(source->symbolLine(index)), rangeEnd
                    );
                    const FormatDocument *document = source;
                    const int i = index;
                    do {
                        if (document->token(i) != NAMESPACE
                            || (i > 0 && document->token(i - 1) == USING)
                        ) {
                            break;
                        }

                        // Finds the `{`, after the name; a `=` or a `;` first makes an alias.
                        int open = i + 1;
                        while (open < document->symbols().size()
                            && document->token(open) != LBRACE
                            && document->token(open) != EQ
                            && document->token(open) != SEMIC
                        ) {
                            ++open;
                        }
                        if (open >= document->symbols().size()
                            || document->token(open) != LBRACE
                        ) {
                            break;
                        }
                        const int close = document->match(open);
                        if (close < 0 || document->spansDirective(
                            document->symbols().at(open).from,
                            document->symbolEnd(close)
                        )) {
                            break;
                        }
                        const int openLine = document->symbolLine(open);
                        const QByteArray indent = document->indentOf(openLine);
                        const int closeLine = document->symbolLine(close);

                        // Ends the opening line at the `{`.
                        if (open + 1 <= close && document->symbolLine(open + 1) == document->symbolLine(open)
                            && ! editor.document()->hasCommentBetween(
                                document->symbolEnd(open),
                                document->symbols().at(open + 1).from
                            )
                        ) {
                            const int from = document->symbolEnd(open);
                            editor.replace(
                                from,
                                document->symbols().at(open + 1).from - from,
                                "\n" + indent
                            );
                        }

                        // Puts the `}` on a line of its own.
                        if (document->firstSymbolOfLine(closeLine) != close
                            && ! editor.document()->hasCommentBetween(
                                document->symbolEnd(close - 1),
                                document->symbols().at(close).from
                            )
                        ) {
                            const int from = document->symbolEnd(close - 1);
                            editor.replace(
                                from,
                                document->symbols().at(close).from - from,
                                "\n" + indent
                            );
                        }

                        if (close + 1 < document->symbols().size()
                            && document->token(close + 1) != SEMIC
                            && document->symbolLine(close + 1) == closeLine)
                        {
                            const int from = document->symbolEnd(close);
                            const int to = document->symbols().at(close + 1).from;
                            if (editor.isOnlyWhitespace(from, to)) {
                                editor.set(from, to, "\n" + indent);
                            }
                        }

                    } while (false);
                    editor.apply();
                    const int shift = source->symbols().size() - oldCount;
                    if (joinedUntil > index) { joinedUntil += shift; }
                    if (superSkipUntil > index) { superSkipUntil += shift; }
                    if (connectSkipUntil > index) { connectSkipUntil += shift; }
                    if (mappingSkipUntil > index) { mappingSkipUntil += shift; }
                    if (initializerSkipUntil > index) { initializerSkipUntil += shift; }

                    if (parameterSkipUntil > index) { parameterSkipUntil += shift; }

                    if (conditionSkipUntil > index) { conditionSkipUntil += shift; }
                    if (declarationSkipUntil > index) { declarationSkipUntil += shift; }
                }
            }
            if (inlineMethods && source->token(index) == LBRACE
                && source->braceScope(index) == FormatDocument::ClassScope)
            {
                const int closing = source->match(index);
                if (closing > index) {
                    const int oldCount = source->symbols().size();
                    CodeEditor editor = document.edit(start, source->symbolEnd(closing));
                    FormatDocument *document = editor.document();
                    const int brace = index;
                    do {
                        if (document->token(brace) != LBRACE
                            || document->braceScope(brace) != FormatDocument::ClassScope
                        ) {
                            break;
                        }
                        QList<int > lacking;
                        bool anyInline = false;
                        // Lists the overrides that lack `inline`, with their markers.
                        QList<QPair<int, int > > lackingOverrides;
                        const QList<int > tokens = classBodyTokens(document, brace);
                        for (int t = 0; t < tokens.size(); ++t) {
                            const int body = tokens.at(t);
                            if (document->token(body) != LBRACE
                                || document->braceScope(body) != FormatDocument::FunctionScope
                            ) {
                                continue;
                            }
                            bool hasInline = false;
                            bool exempt = false;
                            int overrideToken = -1;
                            const int start = functionDeclarationStart(
                                document,
                                body,
                                &hasInline,
                                &exempt,
                                &overrideToken
                            );

                            // Gives an inline override the marker of `[Lint] inlineOverride`.
                            if (overrideToken >= 0) {
                                const int offset = document->symbols().at(overrideToken).from;
                                const QString marker = settingValueAt(
                                    document, offset, QString(QLL("lint.inlineOverride")),
                                    document->configAt(offset).inlineOverride
                                ).toString();
                                const bool keepsMarker = marker == QLL("override")
                                    || marker == QLL("Q_DECL_OVERRIDE");
                                if ( ! keepsMarker) {
                                    if (hasInline) {
                                        setOverrideMarker(document, overrideToken, marker);
                                        anyInline = true;
                                    } else if ( ! marker.isEmpty() && ! exempt && start >= 0) {
                                        lackingOverrides.append(qMakePair(start, overrideToken));
                                    }
                                    continue;
                                }
                            }
                            anyInline = anyInline || hasInline;
                            if ( ! hasInline && ! exempt && start >= 0) {
                                lacking.append(start);
                            }
                        }
                        if ( ! anyInline) {
                            break;
                        }
                        for (int i = 0; i < lacking.size(); ++i) {
                            addInlineKeyword(document, lacking.at(i));
                        }
                        for (int i = 0; i < lackingOverrides.size(); ++i) {
                            const QPair<int, int > item = lackingOverrides.at(i);
                            const int offset = document->symbols().at(item.second).from;
                            const QString marker = settingValueAt(
                                document, offset, QString(QLL("lint.inlineOverride")),
                                document->configAt(offset).inlineOverride
                            ).toString();
                            addInlineKeyword(document, item.first);
                            setOverrideMarker(document, item.second, marker);
                        }
                    } while (false);
                    editor.apply();
                    const int shift = source->symbols().size() - oldCount;
                    if (joinedUntil > index) {
                        joinedUntil += shift;
                    }
                    if (superSkipUntil > index) {
                        superSkipUntil += shift;
                    }
                    if (connectSkipUntil > index) {
                        connectSkipUntil += shift;
                    }
                    if (mappingSkipUntil > index) {
                        mappingSkipUntil += shift;
                    }
                    if (initializerSkipUntil > index) {
                        initializerSkipUntil += shift;
                    }
                    if (parameterSkipUntil > index) {
                        parameterSkipUntil += shift;
                    }
                    if (conditionSkipUntil > index) {
                        conditionSkipUntil += shift;
                    }
                    if (declarationSkipUntil > index) {
                        declarationSkipUntil += shift;
                    }
                }
            }
            if (leadInitializers && index > 0 && index >= initializerSkipUntil
                && source->token(index) == COLON && source->token(index - 1) == RPAREN)
            {
                int brace = index + 1;
                while (brace < source->symbols().size()) {
                    const Token kind = source->token(brace);
                    if (kind == LPAREN || (kind == LBRACE
                        && source->braceScope(brace) == FormatDocument::InitScope))
                    {
                        brace = qMax(brace, source->match(brace)) + 1;
                        continue;
                    }
                    if (kind == LBRACE || kind == SEMIC || kind == RBRACE) {
                        break;
                    }
                    ++brace;
                }
                if (brace < source->symbols().size() && source->token(brace) == LBRACE) {
                    const int from = source->lineStart(source->symbolLine(index - 1));
                    const int to = source->symbolEnd(brace);
                    const int oldCount = source->symbols().size();
                    CodeEditor editor = document.edit(from, to);
                    FormatDocument *document = editor.document();
                    const int i = index;
                    do {
                        if (document->token(i) != COLON
                            || document->token(i - 1) != RPAREN
                        ) {
                            break;
                        }

                        // Finds the list's end, the body's brace.
                        int brace = -1;
                        for (int k = i + 1; k < document->symbols().size(); ++k) {
                            const Token token = document->token(k);
                            if (token == LPAREN
                                || (token == LBRACE
                                    && document->braceScope(k) == FormatDocument::InitScope)
                            ) {
                                k = qMax(k, document->match(k));
                                continue;
                            }
                            if (token == LBRACE) {
                                brace = k;
                            }
                            if (token == LBRACE || token == SEMIC || token == RBRACE) {
                                break;
                            }
                        }
                        if (brace < 0
                            || document->braceScope(brace) != FormatDocument::FunctionScope
                        ) {
                            break;
                        }

                        // Moves the separators that end a line.
                        for (int k = i; k < brace; ++k) {
                            const Token token = document->token(k);
                            if (token == LPAREN) {
                                k = qMax(k, document->match(k));
                                continue;
                            }
                            const int line = document->symbolLine(k);
                            if ((token != COLON && token != COMMA) || document->lastSymbolOfLine(line) != k
                                || document->symbolEndLine(k - 1) != line
                                || document->hasCommentBetween(
                                    document->symbolEnd(k),
                                    document->symbols().at(k + 1).from
                                )
                            ) {
                                continue;
                            }
                            const int from = document->symbolEnd(k - 1);
                            if (document->spansDirective(
                                from,
                                document->symbols().at(k + 1).from
                            )) {
                                continue;
                            }
                            document->replace(
                                from, document->symbolEnd(k) - from, QByteArray(),
                                document->symbols().at(k + 1).from, 0, document->lexem(k) + ' '
                            );
                        }

                        // Puts each comma that leads a line under the colon, when the colon
                        // leads its own line.
                        const int colonLine = document->symbolLine(i);
                        if (document->firstSymbolOfLine(colonLine) == i) {
                            const QByteArray indent = document->indentOf(colonLine);
                            for (int k = i + 1; k < brace; ++k) {
                                const int line = document->symbolLine(k);
                                if (document->token(k) != COMMA
                                    || document->firstSymbolOfLine(line) != k
                                    || document->indentOf(line) == indent
                                ) {
                                    continue;
                                }
                                document->replace(
                                    document->lineStart(line),
                                    document->indentOf(line).size(),
                                    indent
                                );
                            }
                        }
                        initializerSkipUntil = brace + 1;
                    } while (false);
                    const bool changed = editor.apply();
                    const int shift = source->symbols().size() - oldCount;
                    if (joinedUntil > index) {
                        joinedUntil += shift;
                    }
                    if (superSkipUntil > index) {
                        superSkipUntil += shift;
                    }
                    if (connectSkipUntil > index) {
                        connectSkipUntil += shift;
                    }
                    if (mappingSkipUntil > index) {
                        mappingSkipUntil += shift;
                    }
                    if (initializerSkipUntil > index) {
                        initializerSkipUntil += shift;
                    }
                    if (parameterSkipUntil > index) {
                        parameterSkipUntil += shift;
                    }
                    if (conditionSkipUntil > index) {
                        conditionSkipUntil += shift;
                    }
                    if (declarationSkipUntil > index) {
                        declarationSkipUntil += shift;
                    }
                    if (changed) {
                        initializerSkipUntil = index;
                        --index;
                        continue;
                    }
                }
            }
            if (braceControls
                && (source->token(index) == IF || source->token(index) == ELSE
                    || source->token(index) == DO || source->token(index) == FOR
                    || source->token(index) == WHILE
                    || (source->token(index) == IDENTIFIER
                        && (source->lexem(index) == "foreach"
                            || source->lexem(index) == "Q_FOREACH"))))
            {
                const int body = controlBodyStart(source, index);
                int last = body;
                if (body >= 0 && source->token(body) == LBRACE) {
                    last = source->match(body);
                } else if (body >= 0) {
                    last = statementEnd(source, body);
                }
                const int rangeStart = index > 0 && source->token(index) == ELSE
                    ? qMin(source->lineStart(source->symbolLine(index)),
                           source->symbolEnd(index - 1))
                    : source->lineStart(source->symbolLine(index));
                int rangeEnd = last >= body && body >= 0
                    ? source->lineEnd(source->symbolEndLine(last))
                    : source->lineEnd(source->symbolLine(index));
                if (last >= body && body >= 0 && source->token(body) != LBRACE
                    && (source->token(last + 1) == ELSE
                        || (source->token(last + 1) == WHILE && source->token(index) == DO)))
                {
                    rangeEnd = qMax(rangeEnd, source->symbols().at(last + 1).from);
                }
                const int oldCount = source->symbols().size();
                CodeEditor editor = document.edit(rangeStart, rangeEnd);
                FormatDocument *document = editor.document();
                const int i = index;
                do {
                        if (document->isDirectiveLine(document->symbolLine(i))) {
                            break;
                        }
                        if (document->token(i) == ELSE && document->token(i - 1) == RBRACE
                            && document->symbolEndLine(i - 1) < document->symbolLine(i)
                            && ! document->hasCommentBetween(
                                document->symbolEnd(i - 1), document->symbols().at(i).from
                            ) && ! document->spansDirective(
                                document->symbolEnd(i - 1), document->symbols().at(i).from
                            )
                        ) {
                            const int from = document->symbolEnd(i - 1);
                            document->replace(from, document->symbols().at(i).from - from, " ");
                        }
                        const int body = controlBodyStart(document, i);
                        if (body >= 0
                            && document->token(body) == LBRACE
                            && ! document->isHeader()
                        ) {
                            expandInlineBody(document, i, body);
                            break;
                        }
                        if (body < 0
                            || document->token(body) == LBRACE
                            || document->token(body) == SEMIC
                        ) {
                            break;
                        }
                        const Token token = document->token(body);
                        if (token == IF
                            || token == FOR
                            || token == WHILE
                            || token == DO
                            || token == SWITCH
                            || token == TRY
                        ) {
                            document->report(document->symbols().at(i).from, "control-braces",
                                             QString(QLL("wrap the nested control statement in braces")));
                            break;
                        }
                        braceStatementBody(document, i, body - 1, body);
                } while (false);
                editor.apply();
                const int shift = source->symbols().size() - oldCount;
                if (joinedUntil > index) {
                    joinedUntil += shift;
                }
                if (superSkipUntil > index) {
                    superSkipUntil += shift;
                }
                if (connectSkipUntil > index) {
                    connectSkipUntil += shift;
                }
                if (mappingSkipUntil > index) {
                    mappingSkipUntil += shift;
                }
                if (initializerSkipUntil > index) {
                    initializerSkipUntil += shift;
                }
                if (parameterSkipUntil > index) {
                    parameterSkipUntil += shift;
                }
                if (conditionSkipUntil > index) {
                    conditionSkipUntil += shift;
                }
                if (declarationSkipUntil > index) {
                    declarationSkipUntil += shift;
                }
            }
            if (wrapOperators && source->firstSymbolOfLine(source->symbolLine(index)) == index) {
                const int line = source->symbolLine(index);
                const int statement = statementEnd(source, index);
                const int rangeEnd = statement >= index
                    ? qMax(source->lineEnd(line), source->symbolEnd(statement))
                    : source->lineEnd(line);
                const int oldCount = source->symbols().size();
                CodeEditor editor = document.edit(source->lineStart(line), rangeEnd);
                FormatDocument *document = editor.document();
                FormatScopeConditions conditions(document);
                do {
                        const int limit = wrapMaxAt(document, document->lineStart(line));
                        int first = -1;
                        int last = -1;
                        if (document->isDirectiveLine(line)
                            || ! findWholeStatement(document, line, &first, &last)
                        ) {
                            break;
                        }
                        if ( ! codeTargetAnyAt(document, document->lineStart(line))) {
                            // Keep existing breaks when each physical line
                            // fits its wrap column.
                            const int endLine = document->symbolEndLine(last);
                            int current = line;
                            while (current <= endLine
                                && textWidth(document->lineText(current)) <= wrapColumn(
                                    document, document->lineStart(current)
                                )
                            ) {
                                ++current;
                            }
                            if (current > endLine) {
                                break;
                            }
                        }
                        const QByteArray indent = document->indentOf(line);
                        FormatExpressionLayout layout(
                            document,
                            &conditions,
                            textWidth(indent)
                        );
                        if (textWidth(indent + layout.oneLine(first, last)) <= limit
                            || ! layout.hasOuterChain(first, last - 1)
                        ) {
                            break;
                        }
                        const QByteArray laidOut = layout.layout(
                            first,
                            last - 1,
                            textWidth(indent),
                            1
                        );
                        const int from = document->symbols().at(first).from;
                        const int length = document->symbolEnd(last - 1) - from;
                        if (laidOut == document->text().mid(from, length)
                            || ! document->replace(from, length, laidOut)
                        ) {
                            break;
                        }
                        if (layout.brokeUngrouped()) {
                            document->request(
                                from,
                                "operator-wrap",
                                QString(QLL("the tool broke this expression at its operators "
                                    "by width alone; regroup its lines so that the "
                                    "operands that belong together (a key and its "
                                    "value, a format and its arguments) share one, "
                                    "each line within %1 columns and led by its " "operator"))
                                .arg(limit),
                                1,
                                limit
                            );
                        }
                } while (false);
                editor.apply();
                const int shift = source->symbols().size() - oldCount;
                if (joinedUntil > index) {
                    joinedUntil += shift;
                }
                if (superSkipUntil > index) {
                    superSkipUntil += shift;
                }
                if (connectSkipUntil > index) {
                    connectSkipUntil += shift;
                }
                if (mappingSkipUntil > index) {
                    mappingSkipUntil += shift;
                }
                if (initializerSkipUntil > index) {
                    initializerSkipUntil += shift;
                }
                if (parameterSkipUntil > index) {
                    parameterSkipUntil += shift;
                }
                if (conditionSkipUntil > index) {
                    conditionSkipUntil += shift;
                }
                if (declarationSkipUntil > index) {
                    declarationSkipUntil += shift;
                }
            }
            if (wrapTernaries && source->firstSymbolOfLine(source->symbolLine(index)) == index) {
                const int line = source->symbolLine(index);
                const int statement = statementEnd(source, index);
                const int rangeEnd = statement >= index
                    ? qMax(source->lineEnd(line), source->symbolEnd(statement))
                    : source->lineEnd(line);
                const int oldCount = source->symbols().size();
                CodeEditor editor = document.edit(source->lineStart(line), rangeEnd);
                FormatDocument *document = editor.document();
                FormatScopeConditions conditions(document);
                const FormatConditionLayout layout(document, &conditions);
                do {
                        const int first = document->firstSymbolOfLine(line);
                        const int last = document->lastSymbolOfLine(line);
                        // Align a conditional whose branch already spans multiple lines.
                        if (first >= 0 && (first == 0
                                || document->token(first - 1) == SEMIC
                                || document->token(first - 1) == LBRACE
                                || document->token(first - 1) == RBRACE
                                || document->token(first - 1) == COLON)
                            && ! document->isDirectiveLine(line)
                            && document->token(first) != IF
                            && document->token(first) != WHILE
                            && document->token(first) != FOR
                            && document->token(first) != SWITCH
                            && document->token(first) != DO
                            && document->token(first) != ELSE
                            && document->token(first) != CASE
                        ) {
                            const int end = statementEnd(document, first);
                            int question = -1;
                            int colon = -1;
                            if (end > last && findTernary(
                                    document, first, end - 1, &question, &colon
                                ) && document->symbolLine(question) > line
                                && document->symbolLine(question) < document->symbolLine(colon)
                                && ! document->hasCommentBetween(
                                    document->symbols().at(first).from, document->symbolEnd(end)
                                ) && ! document->spansDirective(
                                    document->symbols().at(first).from, document->symbolEnd(end)
                                )
                            ) {
                                const QByteArray pad = document->indentOf(line) + "    ";
                                const int endOfLine = document->lineEnd(line);
                                const QByteArray newline = document->text().mid(
                                    endOfLine - 1,
                                    1
                                ) == "\r"
                                    ? "\r\n" : "\n";
                                const QByteArray gap = newline + pad;
                                const int beforeQuestion = document->symbolEnd(question - 1);
                                const int questionFrom = document->symbols().at(question).from;
                                const int beforeColon = document->symbolEnd(colon - 1);
                                const int colonFrom = document->symbols().at(colon).from;
                                if (document->text().mid(
                                        beforeQuestion,
                                        questionFrom - beforeQuestion
                                    ) != gap
                                    || document->text().mid(
                                        beforeColon,
                                        colonFrom - beforeColon
                                    ) != gap
                                ) {
                                    document->replace(
                                        beforeQuestion, questionFrom - beforeQuestion, gap,
                                        beforeColon, colonFrom - beforeColon, gap
                                    );
                                }
                                break;
                            }
                        }
                        if (first < 0
                            || last <= first
                            || document->token(last) != SEMIC
                            || document->isDirectiveLine(line)
                            || textWidth(document->lineText(line)) <= wrapMaxAt(
                                document,
                                document->lineStart(line)
                            )
                        ) {
                            break;
                        }
                        const Token before = first > 0
                            ? document->token(first - 1) : SEMIC;
                        if (before != SEMIC
                            && before != LBRACE
                            && before != RBRACE
                            && before != COLON
                        ) {
                            break;
                        }
                        const Token start = document->token(first);
                        if (start == IF
                            || start == WHILE
                            || start == FOR
                            || start == SWITCH
                            || start == DO
                            || start == ELSE
                            || start == CASE
                            || start == RBRACE
                        ) {
                            break;
                        }

                        // Leave comments in place when rewriting the statement.
                        bool isPlain = ! document->hasCommentBetween(
                            document->symbols().at(first).from,
                            document->symbolEnd(last)
                        );
                        // Skip nested expressions when looking for another statement.
                        for (int k = first; k < last && isPlain; ++k) {
                            const Token token = document->token(k);
                            if ((token == LPAREN || token == LBRACK || token == LBRACE)
                                && document->match(k) > k
                            ) {
                                k = document->match(k);
                                continue;
                            }
                            isPlain = token != SEMIC && token != RBRACE;
                        }
                        int question = -1;
                        int colon = -1;
                        if ( ! isPlain
                            || ! findTernary(document, first, last - 1, &question, &colon)
                        ) {
                            break;
                        }
                        const int indent = document->indentOf(line).size();
                        const QList<QByteArray > lines = ternaryLines(
                            sourceText(document, first, question - 1),
                            sourceText(document, question, colon - 1),
                            sourceText(document, colon, last - 1),
                            indent,
                            indent,
                            1,
                            wrapMaxAt(document, document->lineStart(line))
                        );
                        if (lines.size() < 2) {
                            break;
                        }

                        // Breaks the line before the `?` too, when it does not fit.
                        const QByteArray originalHead =
                            sourceText(document, first, question - 1);
                        const QList<QByteArray > head = lines.first() == originalHead
                            ? layout.layoutHead(first, question - 1, indent)
                            : QList<QByteArray >() << lines.first();
                        QByteArray text;
                        for (int l = 0; l < head.size(); ++l) {
                            text += (l > 0 ? "\n" : "") + head.at(l);
                        }
                        for (int l = 1; l < lines.size(); ++l) {
                            int nestedQuestion = -1;
                            int nestedColon = -1;
                            if (l == lines.size() - 1
                                && findTernary(document, colon + 1, last - 1,
                                               &nestedQuestion, &nestedColon))
                            {
                                const QByteArray pad = continuationPad(indent);
                                const bool sharesBranch = lines.at(l).trimmed().startsWith('?');
                                const int nestedIndent = indent
                                    + (sharesBranch ? continuationIndent(0) : 0);
                                const QList<QByteArray > nestedHead = layout.layoutHead(
                                    colon + 1, nestedQuestion - 1, nestedIndent);
                                QByteArray prefix = pad;
                                if (sharesBranch) {
                                    prefix += sourceText(document, question, colon - 1) + " ";
                                }
                                prefix += ": ";
                                text += "\n" + prefix + nestedHead.first();
                                for (int part = 1; part < nestedHead.size(); ++part) {
                                    text += "\n" + nestedHead.at(part);
                                }
                                const QByteArray middle = sourceText(document, nestedQuestion, nestedColon - 1);
                                const QByteArray tail = sourceText(document, nestedColon, last - 1);
                                if (nestedHead.size() > 1) {
                                    const QByteArray nestedPad = continuationPad(indent, 2);
                                    text += "\n" + nestedPad + middle + "\n" + nestedPad + tail;
                                } else {
                                    text += " " + middle + " " + tail;
                                }
                            } else {
                                text += "\n" + lines.at(l);
                            }
                        }
                        const int from = document->symbols().at(first).from;
                        document->replace(from, document->symbols().at(last).from - from, text);
                } while (false);
                editor.apply();
                const int shift = source->symbols().size() - oldCount;
                if (joinedUntil > index) {
                    joinedUntil += shift;
                }
                if (superSkipUntil > index) {
                    superSkipUntil += shift;
                }
                if (connectSkipUntil > index) {
                    connectSkipUntil += shift;
                }
                if (mappingSkipUntil > index) {
                    mappingSkipUntil += shift;
                }
                if (initializerSkipUntil > index) {
                    initializerSkipUntil += shift;
                }
                if (parameterSkipUntil > index) {
                    parameterSkipUntil += shift;
                }
                if (conditionSkipUntil > index) {
                    conditionSkipUntil += shift;
                }
                if (declarationSkipUntil > index) {
                    declarationSkipUntil += shift;
                }
            }
            if (layoutConnections && index >= connectSkipUntil
                && source->token(index) == IDENTIFIER
                && (source->lexem(index) == "connect" || source->lexem(index) == "disconnect")
                && source->token(index + 1) == LPAREN)
            {
                const int closing = source->match(index + 1);
                if (closing > index) {
                    const int rangeStart = source->lineStart(source->symbolLine(index));
                    const int rangeEnd = source->lineEnd(source->symbolEndLine(closing));
                    const int oldCount = source->symbols().size();
                    CodeEditor editor = document.edit(rangeStart, rangeEnd);
                    FormatDocument *document = editor.document();
                    const int i = index;
                    do {
                        if (document->token(i) != IDENTIFIER || document->token(i + 1) != LPAREN
                            || document->isDirectiveLine(document->symbolLine(i))
                        ) {
                            break;
                        }
                        const QByteArray name = document->lexem(i);
                        if (name != "connect" && name != "disconnect") {
                            break;
                        }

                        // Leaves out a declaration or a definition of such a function.
                        const Token before = document->token(i - 1);
                        if ((before == SCOPE && endsType(document->token(i - 3))) || (before != SCOPE
                                && before != DOT
                                && before != ARROW
                                && endsType(before))
                        ) {
                            break;
                        }
                        const int open = i + 1;
                        const int close = document->match(open);
                        if (close < 0 || document->hasCommentBetween(
                                document->symbols().at(open).from,
                                document->symbols().at(close).from
                            )
                            || document->spansDirective(
                                document->symbols().at(open).from,
                                document->symbols().at(close).from
                            )
                        ) {
                            break;
                        }

                        // Splits the arguments at their top-level commas.
                        QList<QByteArray > arguments;
                        int from = open + 1;
                        bool singleLines = true;
                        for (int k = open + 1; k <= close; ++k) {
                            const Token token = document->token(k);
                            if ((token == LPAREN
                                    || token == LBRACK
                                    || token == LBRACE)
                                && k != close
                            ) {
                                k = qMax(k, document->match(k));
                                continue;
                            }
                            if (token != COMMA && k != close) {
                                continue;
                            }
                            if (k == from) {
                                singleLines = false;
                                break;
                            }
                            const int start = document->symbols().at(from).from;
                            const int end = document->symbolEnd(k - 1);
                            const QByteArray argument = document->text().mid(
                                start,
                                end
                                    - start
                            );
                            singleLines = singleLines && ! argument.contains('\n');
                            arguments.append(argument);
                            from = k + 1;
                        }
                        const bool hasType = arguments.size() == 5
                            && arguments.last().startsWith("Qt::");
                        if ( ! singleLines || (arguments.size() != 4 && ! hasType)) {
                            break;
                        }

                        // Builds the layout: one line when it fits, else one line per pair.
                        const int line = document->symbolLine(open);
                        const int column = QString::fromUtf8(
                            document->text().mid(
                                document->lineStart(line),
                                document->symbolEnd(open) - document->lineStart(line)
                            )
                        ).size();
                        const QByteArray pad(column, ' ');
                        const QByteArray rest = document->text().mid(
                            document->symbolEnd(close),
                            contentEndOf(document, close) - document->symbolEnd(close)
                        );
                        QByteArray layout = arguments.at(0) + ", " + arguments.at(1) + ", " + arguments.at(2) + ", "
                            + arguments.at(3);
                        const int limit = wrapMaxAt(document, document->symbols().at(open).from);
                        if (hasType || column + layout.size() + 1 + rest.size() > limit) {
                            // Ends each pair with a comma and the line break that ends the
                            // call's first line, `\r\n` in a file of CRLF line endings.
                            const QByteArray pairEnd = document->text().mid(
                                contentEndOf(document, open),
                                1
                            ) == "\r"
                                ? QByteArray(",\r\n") : QByteArray(",\n");
                            layout = arguments.at(0) + ", " + arguments.at(1) + pairEnd + pad + arguments.at(2) + ", "
                                + arguments.at(3);
                            if (hasType) {
                                layout += pairEnd + pad + arguments.at(4);
                            }
                        }
                        const int start = document->symbolEnd(open);
                        const int end = document->symbols().at(close).from;
                        if (document->text().mid(start, end - start) != layout) {
                            document->replace(start, end - start, layout);
                        }
                        connectSkipUntil = close + 1;
                    } while (false);
                    editor.apply();
                    const int shift = source->symbols().size() - oldCount;
                    if (joinedUntil > index) {
                        joinedUntil += shift;
                    }
                    if (superSkipUntil > index) {
                        superSkipUntil += shift;
                    }
                    if (connectSkipUntil > index) {
                        connectSkipUntil += shift;
                    }
                    if (mappingSkipUntil > index) {
                        mappingSkipUntil += shift;
                    }
                    if (initializerSkipUntil > index) {
                        initializerSkipUntil += shift;
                    }
                    if (parameterSkipUntil > index) {
                        parameterSkipUntil += shift;
                    }
                    if (conditionSkipUntil > index) {
                        conditionSkipUntil += shift;
                    }
                    if (declarationSkipUntil > index) {
                        declarationSkipUntil += shift;
                    }
                }
            }
            if (wrapParameters && index > 0 && index >= parameterSkipUntil
                && source->token(index) == LPAREN)
            {
                const int closing = source->match(index);
                if (closing > index + 1 && isDeclarationParen(source, index)
                    && source->token(closing + 1) != COLON)
                {
                    const int line = source->symbolLine(index);
                    const int closeLine = source->symbolLine(closing);
                    const int rangeStart = source->lineStart(line);
                    const int rangeEnd = contentEnd(source, closeLine);
                    const int oldCount = source->symbols().size();
                    CodeEditor editor = document.edit(rangeStart, rangeEnd);
                    FormatDocument *copy = editor.document();
                    FormatScopeConditions conditions(copy);
                    const FormatFileConfig &fileConfig = copy->configAt(start);
                    const int limit = wrapMaxAt(copy, start);
                    const QList<QPair<int, int > > items = splitArguments(copy, index, closing);
                    const bool commented = source->hasCommentBetween(
                        source->symbolEnd(index),
                        source->symbols().at(closing).from
                    );
                    bool eligible = ! items.isEmpty()
                        && ! copy->isDirectiveLine(line)
                        && ! copy->hasCommentBetween(copy->symbolEnd(index),
                            copy->symbols().at(closing).from);
                    const bool alreadyBroken = closeLine != line;
                    if (alreadyBroken) {
                        eligible = eligible && items.size() >= 2;
                        // A commented list may hold an item the user broke over
                        // lines; laying it out again keeps those lines.
                        for (int item = 0; item < items.size() && eligible
                            && ! commented; ++item) {
                            eligible = copy->symbolLine(items.at(item).first)
                                == copy->symbolEndLine(items.at(item).second);
                        }
                    } else {
                        eligible = eligible && textWidth(copy->lineText(line)) > limit;
                        const int last = copy->lastSymbolOfLine(line);
                        for (int token = closing + 1; token <= last && eligible; ++token) {
                            if (copy->token(token) == LBRACE) {
                                eligible = textWidth(copy->text().mid(
                                    copy->lineStart(line),
                                    copy->symbols().at(token).from - copy->lineStart(line)
                                )) > limit;
                            }
                        }
                    }
                    if (eligible) {
                        const int from = copy->lineStart(line);
                        const int to = contentEnd(copy, closeLine);
                        const QByteArray prefix = copy->text().mid(
                            from, copy->symbolEnd(index) - from
                        );
                        const QByteArray suffix = copy->text().mid(
                            copy->symbols().at(closing).from,
                            to - copy->symbols().at(closing).from
                        );
                        const bool breaksAfterOpen = conditions.holds(
                            QString(QLL("structure.wrapSignatureAt")),
                            fileConfig.wrapSignatureAt, index,
                            FormatFileConfig::schema()->programFor(
                                QString(QLL("structure.wrapSignatureAt")))
                        );
                        const QByteArray pad = copy->indentOf(line) + "        ";
                        FormatExpressionLayout layout(copy, &conditions, 0);
                        QList<QByteArray > lines;
                        bool fits = false;
                        if (alreadyBroken) {
                            if (line < copy->symbolLine(items.first().first)) {
                                lines = layout.layoutDeclaration(
                                    index, prefix, suffix, true, commented, pad, limit,
                                    &fits
                                );
                            } else {
                                bool fitsUnder = false;
                                bool fitsBroken = false;
                                const QList<QByteArray > under = layout.layoutDeclaration(
                                    index, prefix, suffix, false, false,
                                    QByteArray(textWidth(prefix), ' '), limit, &fitsUnder
                                );
                                const QList<QByteArray > broken = layout.layoutDeclaration(
                                    index, prefix, suffix, true, commented,
                                    pad, limit, &fitsBroken
                                );
                                // A commented list keeps one item per line, since
                                // an item's line comment would otherwise read
                                // over the item after it.
                                if ( ! commented && ! breaksAfterOpen && fitsUnder
                                    && under.join("\n") == copy->text().mid(from, to - from))
                                {
                                    lines = under;
                                } else {
                                    lines = fitsBroken || breaksAfterOpen || commented
                                        ? broken : under;
                                }
                            }
                        } else {
                            const QByteArray head = prefix + sourceText(
                                copy, items.first().first, items.first().second
                            ) + (items.size() == 1 ? suffix : ",");
                            if (conditions.argsOver(index, items.size())) {
                                lines = layout.layoutDeclaration(
                                    index, prefix, suffix, true, true, pad, limit, &fits
                                );
                            } else if (textWidth(head) <= limit && ! breaksAfterOpen
                                && ! commented
                            ) {
                                lines = layout.layoutDeclaration(
                                    index, prefix, suffix, false, false,
                                    QByteArray(textWidth(prefix), ' '), limit, &fits
                                );
                            } else {
                                // A commented list gives every item its own line,
                                // so a comment never reads over the next item.
                                lines = layout.layoutDeclaration(
                                    index, prefix, suffix, true, commented, pad, limit,
                                    &fits
                                );
                            }
                        }
                        if (commented) {
                            lines = commentedSignatureLines(
                                source, index, closing, lines, prefix, suffix,
                                copy->indentOf(line), limit
                            );
                        }
                        const QByteArray newline = copy->text().mid(to, 1) == "\r"
                            ? "\r\n" : "\n";
                        const QByteArray wrapped = lines.join(newline);
                        if (wrapped != copy->text().mid(from, to - from)) {
                            editor.set(from, to, wrapped);
                        }
                        parameterSkipUntil = closing + 1;
                    }
                    editor.apply();
                    const int shift = source->symbols().size() - oldCount;
                    if (joinedUntil > index) {
                        joinedUntil += shift;
                    }
                    if (superSkipUntil > index) {
                        superSkipUntil += shift;
                    }
                    if (connectSkipUntil > index) {
                        connectSkipUntil += shift;
                    }
                    if (mappingSkipUntil > index) {
                        mappingSkipUntil += shift;
                    }
                    if (initializerSkipUntil > index) {
                        initializerSkipUntil += shift;
                    }
                    if (parameterSkipUntil > index) {
                        parameterSkipUntil += shift;
                    }
                    if (conditionSkipUntil > index) {
                        conditionSkipUntil += shift;
                    }
                    if (declarationSkipUntil > index) {
                        declarationSkipUntil += shift;
                    }
                }
            }
            if (wrapArguments && index > 0
                && (source->token(index) == LPAREN || source->token(index) == LBRACE))
            {
                const int closing = source->match(index);
                if (closing > index) {
                    const int firstLine = source->symbolLine(index);
                    const int lastLine = source->symbolEndLine(closing);
                    const int oldCount = source->symbols().size();
                    int rangeEnd = source->lineEnd(lastLine);
                    const int keyword = controlKeywordFor(source, index);
                    if (keyword >= 0 && source->match(keyword + 1) == closing + 1) {
                        rangeEnd = qMax(rangeEnd, source->symbols().at(closing + 1).from);
                    }
                    CodeEditor editor = document.edit(
                        source->lineStart(firstLine), rangeEnd
                    );
                    // Comments are blanked out in the editor's document, so
                    // the list has to be asked here whether it holds any.
                    const bool commentedList = source->hasCommentBetween(
                        source->symbols().at(index).from + 1,
                        source->symbols().at(closing).from
                    );
                    FormatDocument *document = editor.document();
                    FormatScopeConditions conditions(document);
                    const int i = index;
                    bool retryArguments = false;
                    do {
                        const int close = document->match(i);
                        const bool isCall = document->token(i) == LPAREN
                            && isFunctionCallParen(document, i);
                        const bool isBrace = document->token(i) == LBRACE
                            && document->braceScope(i) == FormatDocument::InitScope;
                        if (close <= i + 1 || ( ! isCall && ! isBrace)) {
                            break;
                        }
                        const int line = document->symbolLine(i);
                        const int lineStart = document->lineStart(line);
                        const int from = document->symbols().at(i).from;
                        const FormatFileConfig &fileConfig = document->configAt(from);
                        const int closeFrom = document->symbols().at(close).from;
                        const int closeEnd = document->symbolEnd(close);

                        // Leaves protected comments and directives in place while laying out the list.
                        bool isPlain = ! document->hasCommentBetween(from + 1, closeFrom)
                            && ! document->spansDirective(from, closeEnd);
                        // Skip nested expressions when looking for a statement boundary.
                        for (int k = i + 1; k < close && isPlain; ++k) {
                            const Token token = document->token(k);
                            if ((token == LPAREN || token == LBRACK || token == LBRACE)
                                && document->match(k) > k
                            ) {
                                k = document->match(k);
                                continue;
                            }
                            isPlain = token != SEMIC;
                        }
                        const QList<QPair<int, int > > arguments = splitArguments(
                            document,
                            i,
                            close
                        );
                        // Takes a list of one argument only when it spans lines and its
                        // argument is an operator chain (see below), whose padding may be off.
                        const bool isSplit = document->symbolLine(i)
                            != document->symbolLine(close);
                        if ( ! isPlain || arguments.isEmpty()) {
                            break;
                        }

                        // Finds the indent the list's lines step from: the condition's
                        // operand level in the head of a control statement.
                        int indent = textWidth(document->indentOf(line));
                        const int keyword = controlKeywordFor(document, i);
                        const bool isHeadLine = keyword >= 0
                            && document->symbolLine(keyword) == line;

                        // Lets a call that ends the condition of an `if` or a `while`, its
                        // `(` on the keyword's line, hug the condition's `)`: its lines then
                        // step from the keyword's line, and the condition closes right after
                        // it, as `)) {`.
                        const int headClose = keyword >= 0
                            ? document->match(keyword + 1) : -1;
                        const bool hugsHead = compactAlwaysAt(document, document->symbols().at(i).from)
                            && isCall
                            && isHeadLine
                            && (document->token(keyword) == IF
                                || document->token(keyword) == WHILE)
                            && close + 1 == headClose
                            && ! document->hasCommentBetween(
                                closeEnd,
                                document->symbols().at(headClose).from
                            );
                        if (isHeadLine && ! hugsHead) {
                            indent += 4;
                        }
                        const int previousOperator =
                            precedingChainOperator(document, close + 1, false);
                        const bool inlineOperator = previousOperator >= 0
                            && document->token(previousOperator) != QUESTION
                            && document->firstSymbolOfLine(document->symbolLine(previousOperator))
                                != previousOperator;
                        const int extraPad = inlineOperator
                            ? conditions.operatorPadding(previousOperator)
                            : 0;
                        FormatExpressionLayout layout(document, &conditions, indent);
                        layout.setKeepsCommentedLines(commentedList);
                        const int column = textWidth(
                            document->text().mid(lineStart, from - lineStart)
                        );

                        // Tells whether the list asks for a layout: it does not fit, and may
                        // break, or its lines sit at other indents than its brackets give
                        // them (see `hasMisplacedLines()`).
                        const QByteArray oneLine = layout.oneLine(i, close);
                        const bool endsInChain = layout.hasChain(
                            arguments.last().first,
                            arguments.last().second
                        );
                        if (arguments.size() < 2 && ! endsInChain) {
                            break;
                        }
                        const bool argumentsBelow =
                            document->symbolLine(arguments.first().first) > line;
                        const bool over = conditions.argsOver(i, arguments.size());
                        const bool mayBreak = fileConfig.wrapArgsOverCondition.isEmpty()
                            ? isBrace || (endsInChain && ( ! isSplit || argumentsBelow))
                                || over || reachesWrapArgsAt(document, &conditions, i)
                            : over;
                        const bool needsBreak = mayBreak
                            && column + textWidth(oneLine) > wrapColumn(document, from);
                        const int closeLine = document->symbolLine(close);
                        const bool isMisplaced = hasMisplacedLines(
                            document,
                            i,
                            close,
                            indent,
                            extraPad
                        );
                        if ( ! needsBreak && ! (mayBreak && isMisplaced)) {
                            break;
                        }

                        // Leaves a statement's list whose meant lines stand right under its
                        // first item as it is, unless `[Structure] wrapArgsAt` asks for a
                        // break after its opening; a list in a control head breaks there
                        // still, as its condition then closes on its own line.
                        int joinCount = 0;
                        const bool keepsLines = keepsUserLines(
                            document,
                            i,
                            close,
                            arguments,
                            &conditions,
                            fileConfig,
                            &joinCount
                        );
                        if (keepsLines
                            && keyword < 0
                            && ! reachesWrapArgsAt(document, &conditions, i)
                            && isAlignedUnderFirst(document, i, close, arguments)
                        ) {
                            break;
                        }

                        // Moves a one-line function body that the list's line ends with to a
                        // line of its own, when that alone makes it fit.
                        const int body = needsBreak ? inlineFunctionBody(document, i) : -1;
                        if (body >= 0) {
                            const int start = document->symbolEnd(body - 1);
                            const QByteArray indent = document->indentOf(line);
                            if (document->replace(
                                start,
                                document->symbols().at(body).from - start,
                                "\n" + indent
                            )) {
                                retryArguments = true;
                                break;
                            }
                        }

                        int logicalOperator = -1;
                        QByteArray logicalIndent;
                        if (isCall && needsBreak && logicalBreakBeforeCall(
                            document, i, &logicalOperator, &logicalIndent
                        )) {
                            const int start = document->symbolEnd(logicalOperator - 1);
                            if (document->replace(
                                start, document->symbols().at(logicalOperator).from - start,
                                "\n" + logicalIndent
                            )) {
                                retryArguments = true;
                                break;
                            }
                        }

                        // Writes the list as the layout breaks it, keeping the items the
                        // user put on one line together where the lines look meant.
                        const QByteArray text = layout.layoutList(
                            i,
                            i,
                            close,
                            column,
                            keepsLines,
                            joinCount,
                            extraPad
                        );
                        const int length = (hugsHead
                            ? document->symbols().at(headClose).from
                            : closeEnd) - from;
                        if (document->text().mid(from, length) != text
                            && document->replace(from, length, text)
                            && layout.brokeUngrouped()
                        ) {
                            // Asks the agent to put the arguments that belong together on one
                            // line (see FormatAgentGuides for the prompt it gets).
                            const int wrapMax = wrapMaxAt(document, from);
                            document->request(
                                from,
                                "args-wrap",
                                QString(QLL("the tool put each argument on a line of its own; "
                                    "put the arguments that belong together on one "
                                    "line, each line within %1 columns")).arg(wrapMax),
                                closeLine - line + 1,
                                wrapMax
                            );
                        }
                    } while (false);
                    const bool changed = editor.apply();
                    const int shift = source->symbols().size() - oldCount;
                    if (joinedUntil > index) {
                        joinedUntil += shift;
                    }
                    if (superSkipUntil > index) {
                        superSkipUntil += shift;
                    }
                    if (connectSkipUntil > index) {
                        connectSkipUntil += shift;
                    }
                    if (mappingSkipUntil > index) {
                        mappingSkipUntil += shift;
                    }
                    if (initializerSkipUntil > index) {
                        initializerSkipUntil += shift;
                    }
                    if (parameterSkipUntil > index) {
                        parameterSkipUntil += shift;
                    }
                    if (conditionSkipUntil > index) {
                        conditionSkipUntil += shift;
                    }
                    if (declarationSkipUntil > index) {
                        declarationSkipUntil += shift;
                    }
                    if (changed && (retryArguments || (keyword >= 0 && closeConditions))) {
                        if (keyword >= 0 && closeConditions) {
                            conditionSkipUntil = keyword;
                            index = keyword - 1;
                        } else {
                            --index;
                        }
                        continue;
                    }
                }
            }
            if (wrapConditions && index >= conditionSkipUntil
                && (source->token(index) == IF || source->token(index) == WHILE)
                && source->token(index + 1) == LPAREN)
            {
                const int closing = source->match(index + 1);
                if (closing > index) {
                    const int limit = source->token(closing + 1) == LBRACE
                        ? source->symbolEnd(closing + 1) : source->symbolEnd(closing);
                    const int oldCount = source->symbols().size();
                    CodeEditor editor = document.edit(
                        source->lineStart(source->symbolLine(index)), limit
                    );
                    FormatDocument *document = editor.document();
                    FormatScopeConditions conditions(document);
                    const FormatConditionLayout layout(document, &conditions);
                    const int i = index;
                    do {
                        const Token token = document->token(i);
                        if ((token != IF
                                && token != WHILE)
                            || document->token(i + 1) != LPAREN
                        ) {
                            break;
                        }
                        const int open = i + 1;
                        const int close = document->match(open);
                        const int line = document->symbolLine(i);
                        const FormatFileConfig &fileConfig =
                            document->configAt(document->symbols().at(i).from);
                        if (close < 0 || document->isDirectiveLine(line)
                            || document->hasCommentBetween(
                                document->symbols().at(open).from,
                                document->symbolEnd(close)
                            )
                        ) {
                            break;
                        }

                        // Breaks a one-line condition without a `&&` or `||` of its own
                        // before its loosest operator (see `layoutPlain()`), only
                        // once its line passes `wrapMax`: such a break splits a single
                        // comparison, hence it waits for the hard limit.
                        const QList<int > operators = layout.operators(open, close);
                        if (operators.isEmpty()) {
                            const bool isLong = document->symbolEndLine(close) == line
                                && textWidth(document->lineText(line)) > wrapMaxAt(document, document->lineStart(line));
                            const QByteArray text = isLong
                                ? layout.layoutPlain(open, close) : QByteArray();
                            if ( ! text.isEmpty()) {
                                replaceCondition(document, open, close, text);
                            }
                            conditionSkipUntil = close + 1;
                            break;
                        }
                        if ( ! fileConfig.wrapConditionsOverCondition.isEmpty()
                            && ! conditions.conditionsOver(open, operators.size() + 1)
                        ) {
                            conditionSkipUntil = close + 1;
                            break;
                        }

                        // Takes a one-line condition past the wrap column, or a split one
                        // with a line past `wrapMax`.
                        const int endLine = document->symbolEndLine(close);
                        if (endLine == line) {
                            if (textWidth(document->lineText(line))
                                    <= wrapColumn(document, document->symbols().at(i).from)
                            ) {
                                break;
                            }
                        } else if ( ! isConditionSplitTooLong(document, layout, open, close)
                            && ! hasMisplacedLines(document, open, close,
                                                   document->indentOf(line).size()))
                        {
                            break;
                        } else if (document->agentMode()) {
                            const int limit = wrapMaxAt(document, document->lineStart(line));
                            document->request(
                                document->symbols().at(i).from,
                                "condition-wrap",
                                QString(QLL("this condition has a line past %1 columns; "
                                    "regroup its lines so that the operands that "
                                    "belong together share one, each line within %1 "
                                    "columns and led by its && or ||")).arg(limit),
                                endLine - line + 1,
                                limit
                            );
                            conditionSkipUntil = close + 1;
                            break;
                        }
                        bool hasBrace = false;
                        for (int k = open + 1; k < close && ! hasBrace; ++k) {
                            hasBrace = document->token(k) == LBRACE;
                        }
                        if (hasBrace) {
                            break;
                        }

                        // Lays the condition out, and puts the body's `{` right after it.
                        const int indent = document->indentOf(line).size();
                        const int column = document->symbols().at(open).from
                            - document->lineStart(line);
                        replaceCondition(
                            document,
                            open,
                            close,
                            layout.layoutCondition(open, close, indent, column)
                        );
                        conditionSkipUntil = close + 1;
                    } while (false);
                    editor.apply();
                    const int shift = source->symbols().size() - oldCount;
                    if (joinedUntil > index) {
                        joinedUntil += shift;
                    }
                    if (superSkipUntil > index) {
                        superSkipUntil += shift;
                    }
                    if (connectSkipUntil > index) {
                        connectSkipUntil += shift;
                    }
                    if (mappingSkipUntil > index) {
                        mappingSkipUntil += shift;
                    }
                    if (initializerSkipUntil > index) {
                        initializerSkipUntil += shift;
                    }
                    if (parameterSkipUntil > index) {
                        parameterSkipUntil += shift;
                    }
                    if (conditionSkipUntil > index) {
                        conditionSkipUntil += shift;
                    }
                    if (declarationSkipUntil > index) {
                        declarationSkipUntil += shift;
                    }
                }
            }
            if (closeConditions && (source->token(index) == IF
                    || source->token(index) == WHILE
                    || source->token(index) == FOR
                    || source->token(index) == SWITCH)
                && source->token(index + 1) == LPAREN
                && ! source->isDirectiveLine(source->symbolLine(index)))
            {
                const int close = source->match(index + 1);
                if (close > index && source->token(close + 1) == LBRACE
                    && source->symbolLine(close) != source->symbolLine(index)
                    && ! source->spansDirective(start, source->symbolEnd(close + 1)))
                {
                    const int rangeStart = source->lineStart(source->symbolLine(index));
                    const int rangeEnd = source->symbolEnd(close + 1);
                    CodeEditor editor = document.edit(rangeStart, rangeEnd);
                    FormatDocument *copy = editor.document();
                    const QByteArray indent = copy->indentOf(copy->symbolLine(index));
                    const int last = close - 1;
                    const int closeLine = copy->symbolLine(close);
                    const Token lastToken = copy->token(last);
                    const bool hugsGroup = compactAlwaysAt(copy, start)
                        && (lastToken == RPAREN || lastToken == RBRACK)
                        && copy->match(last) >= 0
                        && copy->symbolLine(copy->match(last)) == copy->symbolLine(index)
                        && copy->firstSymbolOfLine(closeLine) == last
                        && copy->indentOf(closeLine) == indent;
                    bool eligible = true;
                    if (hugsGroup || copy->symbolEndLine(last) == closeLine) {
                        eligible = isSameLineGap(copy, last, close);
                        if (eligible) {
                            const int from = copy->symbolEnd(last);
                            const int to = copy->symbols().at(close).from;
                            const QByteArray gap = hugsGroup ? QByteArray() : "\n" + indent;
                            if (copy->text().mid(from, to - from) != gap) {
                                editor.set(from, to, gap);
                            }
                        }
                    } else if (copy->indentOf(closeLine) != indent) {
                        const int from = copy->lineStart(closeLine);
                        editor.set(from, from + copy->indentOf(closeLine).size(), indent);
                    }
                    if (eligible) {
                        const int from = copy->symbolEnd(close);
                        const int to = copy->symbols().at(close + 1).from;
                        if ((to - from != 1 || copy->text().at(from) != ' ')
                            && ! copy->hasCommentBetween(from, to)
                            && ! copy->spansDirective(from, to))
                        {
                            editor.set(from, to, " ");
                        }
                    }
                    editor.apply();
                }
            }
            if (spaceReferences && index > 0
                && index + 2 < source->symbols().size())
            {
                const Token kind = source->token(index);
                const Token after = source->token(index + 2);
                if ((kind == AND || kind == ANDAND)
                    && source->token(index + 1) == IDENTIFIER
                    && isTypeEnd(source->token(index - 1))
                    && ! source->isDirectiveLine(source->symbolLine(index))
                    && (after == COMMA || after == RPAREN || after == EQ
                        || after == SEMIC || after == LBRACK || after == COLON)
                    && isReferenceDeclaration(source, index))
                {
                    const int line = source->symbolLine(index);
                    CodeEditor editor = document.edit(
                        source->lineStart(line), source->lineEnd(line)
                    );
                    FormatDocument *copy = editor.document();
                    if (isSameLineGap(copy, index - 1, index)
                        && isSameLineGap(copy, index, index + 1))
                    {
                        const FormatFileConfig &fileConfig = copy->configAt(start);
                        FormatSpacing setting = kind == AND
                            ? fileConfig.refSuffix : fileConfig.moveSuffix;
                        const QVariant resolved = settingValueAt(
                            copy, start,
                            QString(kind == AND ? QLL("types.refSuffix") : QLL("types.moveSuffix")),
                            QVariant()
                        );
                        if (resolved.isValid()) {
                            setting = FormatFileConfig::toSpacing(
                                resolved.toString(),
                                QString(kind == AND ? QLL(" ") : QLL("keep-as-is"))
                            );
                        }
                        const FormatSpacing spacing = copy->onSetting(setting, index);
                        if ( ! spacing.isNull()) {
                            const int from = copy->symbolEnd(index - 1);
                            const int to = copy->symbols().at(index + 1).from;
                            const QByteArray suffix = spacing.keepAsIs
                                ? copy->text().mid(end, to - end) : spacing.text;
                            const QByteArray wanted = " " + copy->lexem(index) + suffix;
                            if (copy->text().mid(from, to - from) != wanted) {
                                editor.set(from, to, wanted);
                            }
                        }
                    }
                    editor.apply();
                }
            }
            if (checkStaticVariables && index + 2 < source->symbols().size()) {
                const int i = index;
                do {
                    if (source->token(i) != STATIC
                        || source->scopeOf(i) == FormatDocument::ClassScope
                        || source->isDirectiveLine(source->symbolLine(i))
                    ) {
                        break;
                    }
                    const bool inFunction = source->scopeOf(i) == FormatDocument::FunctionScope
                        || source->scopeOf(i) == FormatDocument::BlockScope;
                    int k = i + 1;
                    while (source->token(k) == CONST
                        || source->token(k) == VOLATILE
                    ) {
                        ++k;
                    }
                    if (source->token(k) != IDENTIFIER
                        || ! source->lexem(k).startsWith('Q')
                        || source->lexem(k).size() < 2
                        || ! QChar::isUpper(source->lexem(k).at(1))
                    ) {
                        if (inFunction && isStaticVariable(source, i)) {
                            const QString message = QString(QLL(
                                "move the static mapping out of the function, to file scope "
                                "(an unnamed namespace); a short list to loop task(s) over "
                                "each entry can stay"));
                            const int offset = source->symbols().at(i).from;
                            if (source->agentMode()) {
                                source->request(
                                    offset,
                                    "global-static",
                                    message,
                                    1,
                                    0
                                );
                            } else {
                                source->report(offset, "global-static", message);
                            }
                        }
                        break;
                    }
                    ++k;
                    if (source->token(k) == LANGLE) {
                        int depth = 0;
                        for (; k < source->symbols().size(); ++k) {
                            const Token token = source->token(k);
                            depth += token == LANGLE
                                ? 1
                                : (token == RANGLE ? -1 : (token == GTGT ? -2 : 0));
                            if (depth <= 0 || token == SEMIC) {
                                break;
                            }
                        }
                        ++k;
                    }
                    if (source->token(k) != IDENTIFIER) {
                        break;
                    }
                    const Token after = source->token(k + 1);
                    if (after == SEMIC
                        || after == EQ
                        || (inFunction
                            && after == LPAREN)
                    ) {
                        source->report(source->symbols().at(i).from, "global-static",
                                         QString(QLL("hold the static object in a Q_GLOBAL_STATIC")));
                    }
                } while (false);
            }
            if (checkReferenceLocal && index > 0
                && index + 2 < source->symbols().size())
            {
                const int i = index;
                do {
                    if (source->token(i) != AND
                        || source->token(i + 1) != IDENTIFIER
                        || source->token(i + 2) != EQ
                        || source->isDirectiveLine(source->symbolLine(i))
                    ) {
                        break;
                    }
                    const int offset = source->symbols().at(i).from;
                    const FormatFileConfig &config = source->configAt(offset);
                    const QString fallback = config.byRefVariable == FormatFileConfig::LintIgnore
                        ? QString(QLL("ignore")) : config.byRefVariable == FormatFileConfig::LintError
                        ? QString(QLL("error")) : QString(QLL("warn"));
                    const QString value = settingValueAt(
                        source, offset, QString(QLL("lint.byRefVariable")), fallback
                    ).toString();
                    const FormatFileConfig::LintLevel level = FormatFileConfig::lintLevel(value, config);
                    if (level == FormatFileConfig::LintIgnore) {
                        break;
                    }
                    const FormatDocument::Scope scope = source->scopeOf(i);
                    if (scope != FormatDocument::FunctionScope
                        && scope != FormatDocument::BlockScope
                    ) {
                        break;
                    }

                    // Walks back over the type, and its template arguments, to the
                    // start of the statement.
                    int start = i - 1;
                    bool isConst = false;
                    while (start >= 0) {
                        const Token token = source->token(start);
                        if (token == RANGLE || token == GTGT) {
                            int depth = 0;
                            for (; start >= 0; --start) {
                                const Token inner = source->token(start);
                                depth += inner == RANGLE
                                    ? 1 : (inner == GTGT ? 2 : (inner == LANGLE ? -1 : 0));
                                if (depth <= 0) {
                                    break;
                                }
                            }
                            --start;
                            continue;
                        }
                        if (token == CONST) {
                            isConst = true;
                        } else if (token != IDENTIFIER && token != SCOPE && token != STAR
                            && token != VOLATILE && token != AUTO && token != STATIC
                            && ! isBuiltinType(token)
                        ) {
                            break;
                        }
                        --start;
                    }
                    const Token before = start >= 0 ? source->token(start) : SEMIC;
                    if (isConst || start == i - 1
                        || (before != SEMIC && before != LBRACE && before != RBRACE)
                    ) {
                        break;
                    }

                    // Names the pointer that replaces the reference.
                    const int typeFrom = source->symbols().at(start + 1).from;
                    const QByteArray type = source->text()
                        .mid(typeFrom, source->symbolEnd(i - 1) - typeFrom).simplified();
                    const QString message = QString(QLL(
                        "name the local through a pointer, `%1 *%2 = &(...);`, then use "
                        "`*%2`, in place of a non-const reference"
                    )).arg(
                        QString::fromUtf8(type),
                        QString::fromUtf8(source->lexem(i + 1))
                    );
                    if (source->agentMode()) {
                        source->request(offset, "reference-local", message, 1, 0);
                    } else {
                        source->report(offset, "reference-local", message,
                                         level == FormatFileConfig::LintError);
                    }
                } while (false);
            }
            if (checkPrivateAccess && source->token(index) == IDENTIFIER
                && source->token(index + 1) == LPAREN
                && ! source->isDirectiveLine(source->symbolLine(index)))
            {
                const QByteArray name = source->lexem(index);
                const Token previous = source->token(index - 1);
                if ((name == "d_func" || name == "q_func")
                    && ! isTypeEnd(previous) && previous != AND)
                {
                    source->report(
                        start,
                        "q-d-macro",
                        QString(QLL("write Q_%1(<Class>) instead of %2()"))
                            .arg(QLatin1Char(name.at(0) == 'd' ? 'D' : 'Q'))
                            .arg(QString::fromLatin1(name))
                    );
                }
            }
            if (checkExcludedFlags && source->token(index) == NOT) {
                int member = index + 1;
                while (source->token(member) == IDENTIFIER
                    && (source->token(member + 1) == DOT
                        || source->token(member + 1) == ARROW))
                {
                    member += 2;
                }
                if (member > index + 1 && source->token(member) == IDENTIFIER
                    && source->token(member + 1) == LPAREN)
                {
                    const QByteArray name = source->lexem(member);
                    if (name == "includes" || name == "includesLax") {
                        source->report(
                            start,
                            "excludes",
                            QString(QLL("check a cleared flag with %1(), not !%2()"))
                                .arg(name == "includes" ? QLL("excludes") : QLL("excludesLax"))
                                .arg(QString::fromLatin1(name))
                        );
                    }
                }
            }
            if (splitStatements && source->token(index) == SEMIC
                && index + 1 < source->symbols().size())
            {
                const int line = source->symbolLine(index);
                const int first = source->firstSymbolOfLine(line);
                const int last = source->lastSymbolOfLine(line);
                bool eligible = first >= 0 && last > index
                    && ! source->isDirectiveLine(line)
                    && source->token(first) != CASE
                    && source->token(first) != DEFAULT;
                if (eligible) {
                    const FormatDocument::Scope scope = source->scopeOf(first);
                    eligible = scope != FormatDocument::EnumScope
                        && scope != FormatDocument::InitScope;
                }
                for (int token = first; token <= last && eligible; ++token) {
                    const Token kind = source->token(token);
                    if (kind == LBRACE || kind == RBRACE) {
                        eligible = false;
                    }
                }
                for (int token = first; token < index && eligible; ++token) {
                    if (source->token(token) == LPAREN) {
                        const int close = source->match(token);
                        if (close >= index) {
                            eligible = false;
                        } else if (close > token) {
                            token = close;
                        }
                    }
                }
                if (eligible) {
                    const int next = source->symbols().at(index + 1).from;
                    CodeEditor editor = document.edit(end, next);
                    if (isSameLineGap(editor.document(), index, index + 1)) {
                        editor.set(end, next, "\n" + source->indentOf(line));
                    }
                    editor.apply();
                }
            }
            if (qualifyMembers && source->token(index) == IDENTIFIER
                && ! source->isDirectiveLine(source->symbolLine(index)))
            {
                FormatScopeConditions conditions(source);
                const FormatMemberScope scope = conditions.scopeOf(index);
                if (scope.body >= 0 && scope.params >= 0 && scope.memberToken >= 0
                    && ! scope.className.isEmpty() && ! scope.isStatic
                    && ! scope.members.statics.contains(source->lexem(scope.memberToken))
                    && (scope.members.names.contains(source->lexem(index))
                        || scope.baseMembers.contains(source->lexem(index)))
                    && ! scope.members.statics.contains(source->lexem(index))
                    && ! insideUncapturedLambda(source, scope.body, index))
                {
                    const int closing = source->match(scope.body);
                    QSet<QByteArray > shadowed = declaredLocalNames(
                        source, scope.params + 1, source->match(scope.params)
                    );
                    shadowed.unite(declaredLocalNames(source, scope.body + 1, closing));
                    const Token before = source->token(index - 1);
                    const Token after = source->token(index + 1);
                    const bool hasThis = before == ARROW && source->token(index - 2) == THIS;
                    const bool accessible = ! shadowed.contains(source->lexem(index))
                        && (hasThis || (before != DOT && before != ARROW
                            && before != SCOPE && before != TILDE && before != GOTO))
                        && after != SCOPE && ! (after == COLON && before == SEMIC)
                        && ! isMacroArgument(source, index);
                    if (closing > index && accessible) {
                        const bool wanted = conditions.holds(
                            QString(QLL("relations.thisMode")),
                            source->configAt(start).thisMode, index
                        );
                        if (wanted != hasThis) {
                            const int from = source->symbols().at(hasThis ? index - 2 : index).from;
                            const int line = source->symbolLine(index);
                            const int column = from - source->lineStart(line);
                            const int rangeStart = source->lineStart(line);
                            const int rangeEnd = source->symbolEnd(closing);
                            const int oldCount = source->symbols().size();
                            CodeEditor editor = document.edit(rangeStart, rangeEnd);
                            FormatDocument *copy = editor.document();
                            const int length = hasThis ? start - from : 0;
                            const QByteArray inserted = wanted ? QByteArray("this->") : QByteArray();
                            if (editor.replace(from, length, inserted)) {
                                QList<FormatGrowth > growths;
                                growths << FormatGrowth(index, column, inserted.size() - length);
                                formatShiftContinuations(copy, growths);
                            }
                            const bool changed = editor.apply();
                            const int shift = source->symbols().size() - oldCount;
                            if (joinedUntil > index) {
                                joinedUntil += shift;
                            }
                            if (superSkipUntil > index) {
                                superSkipUntil += shift;
                            }
                            if (connectSkipUntil > index) {
                                connectSkipUntil += shift;
                            }
                            if (mappingSkipUntil > index) {
                                mappingSkipUntil += shift;
                            }
                            if (initializerSkipUntil > index) {
                                initializerSkipUntil += shift;
                            }
                            if (parameterSkipUntil > index) {
                                parameterSkipUntil += shift;
                            }
                            if (conditionSkipUntil > index) {
                                conditionSkipUntil += shift;
                            }
                            if (declarationSkipUntil > index) {
                                declarationSkipUntil += shift;
                            }
                            if (changed) {
                                index = source->symbolAfter(from) + (wanted ? 2 : 0);
                                continue;
                            }
                        }
                    }
                }
            }
            if (indentContinuations && source->symbolLine(index) > 1
                && source->firstSymbolOfLine(source->symbolLine(index)) == index
                && isLeadingOperator(source->token(index)))
            {
                const int line = source->symbolLine(index);
                const int statement = statementEnd(source, index);
                const int rangeEnd = statement >= index
                    ? source->symbolEnd(statement) : start;
                CodeEditor editor = document.edit(source->lineStart(line), rangeEnd);
                FormatDocument *document = editor.document();
                do {
                        const int first = document->firstSymbolOfLine(line);
                        if (first < 1
                            || ! isLeadingOperator(document->token(first))
                            || document->isDirectiveLine(line)
                            || document->symbolEndLine(first - 1) >= line
                        ) {
                            break;
                        }
                        // Leaves out a statement's start, and a sign (`-1` after a comma).
                        const Token before = document->token(first - 1);
                        if (before == SEMIC
                            || before == LBRACE
                            || before == RBRACE
                            || before == COLON
                            || before == COMMA
                            || before == EQ
                            || before == LPAREN
                            || before == RETURN
                            || isLeadingOperator(before)
                        ) {
                            break;
                        }

                        // Finds the group's line, or the statement's start.
                        int base = -1;
                        bool keepsIndent = false;
                        const int group = continuationGroup(document, first, &base, &keepsIndent);
                        if (keepsIndent || base < 1 || base >= line) {
                            break;
                        }
                        const Token opener = group >= 1
                            ? document->token(group - 1) : NOTOKEN;
                        const Token leading = document->token(first);
                        const bool isCondition = opener == IF || opener == WHILE
                            || (opener == FOR && isForCondition(document, group, first));
                        const bool isOperand = isCondition
                            && leading != ANDAND
                            && leading != OROR;
                        QByteArray indent = document->indentOf(base)
                            + continuationPad(0, isOperand ? 2 : 1);

                        // A grouped operand followed by a member access has one step for
                        // its parentheses and another for the binary continuation.
                        const int close = group >= 0 ? document->match(group) : -1;
                        const Token afterGroup = close >= 0
                            ? document->token(close + 1)
                            : NOTOKEN;
                        if (group >= 0 && document->token(group) == LPAREN
                            && ! isCondition && document->symbolLine(group) == base
                            && (afterGroup == DOT || afterGroup == ARROW)
                        ) {
                            indent = document->indentOf(base) + continuationPad(0, 2);
                        }

                        // Gives a group nested in a condition one step for each of its
                        // brackets that the group's line opened.
                        const int steps = conditionSteps(document, group, base, first);
                        if (steps > 0) {
                            indent = document->indentOf(base) + continuationPad(0, steps);
                        }

                        // Continues the operand of an earlier line of the group whose
                        // operator binds looser, or takes the level of one that binds alike.
                        const int rank = leadRank(document->token(first));
                        for (int earlier = line - 1; earlier > base; --earlier) {
                            const int lead = document->firstSymbolOfLine(earlier);
                            int earlierBase = -1;
                            bool earlierKeeps = false;
                            if (lead < 0 || ! isLeadingOperator(document->token(lead))
                                || continuationGroup(document, lead, &earlierBase, &earlierKeeps) != group
                                || earlierBase != base
                            ) {
                                continue;
                            }
                            const int earlierRank = leadRank(document->token(lead));
                            if (earlierRank < rank) {
                                indent = document->indentOf(earlier) + continuationPad(0);
                                break;
                            }
                            if (earlierRank == rank) {
                                indent = document->indentOf(earlier);
                                break;
                            }
                        }
                        if (leading == QUESTION) {
                            const int head = document->headStart(first);
                            const int end = statementEnd(document, first);
                            int outerQuestion = -1;
                            int outerColon = -1;
                            if (end > first && findTernary(document, head, end - 1,
                                                          &outerQuestion, &outerColon)
                                && outerQuestion < first)
                            {
                                indent = document->indentOf(document->symbolLine(head))
                                    + continuationPad(0, 2);
                            }
                        }
                        const QByteArray current = document->indentOf(line);
                        if (current != indent) {
                            document
                                ->replace(document->lineStart(line), current.size(), indent);
                            QList<FormatGrowth > growths;
                            growths << FormatGrowth(first, current.size() - 1,
                                                    indent.size() - current.size());
                            formatShiftContinuations(document, growths);
                        }
                } while (false);
                editor.apply();
            }
            if (expandBodies && index > 0 && source->token(index) == RBRACE) {
                const int opening = source->match(index);
                if (opening >= 0) {
                    const int oldCount = source->symbols().size();
                    CodeEditor editor = document.edit(
                        source->symbols().at(opening).from, source->symbolEnd(index)
                    );
                    FormatDocument *document = editor.document();
                    FormatScopeConditions conditions(document);
                    const int close = index;
                    do {
                        const int open = document->match(close);
                        if (document->token(close) != RBRACE || open < 0
                            || ! closesBracketChain(document, close)
                            || ! conditions.holds(
                                QString(QLL("structure.bodyBraces")),
                                document->configAt(
                                    document->symbols().at(open).from
                                ).bodyBraces,
                                open
                            )
                        ) {
                            break;
                        }
                        const FormatDocument::Scope scope = document->braceScope(open);
                        if (scope != FormatDocument::FunctionScope
                            && scope != FormatDocument::ClassScope
                            && scope != FormatDocument::EnumScope
                            && scope != FormatDocument::NamespaceScope
                        ) {
                            break;
                        }

                        // Leaves a body with a comment, a directive, or a token spanning lines.
                        const int from = document->symbols().at(open).from;
                        const int to = document->symbolEnd(close);
                        bool isPlain = ! document->hasCommentBetween(from, to)
                            && ! document->spansDirective(from, to);
                        for (int k = open; k <= close && isPlain; ++k) {
                            isPlain = document->symbolLine(k) == document->symbolEndLine(k);
                        }
                        if ( ! isPlain) {
                            break;
                        }

                        // Takes the body's lines, the last one up to the `}` that closes it.
                        const int openLine = document->symbolLine(open);
                        const QByteArray indent = document->indentOf(openLine);
                        const QByteArray bodyIndent = scope == FormatDocument::NamespaceScope
                            ? indent : indent + continuationPad(0);
                        QList<QByteArray > lines = document->text().mid(
                            document->symbolEnd(open),
                            document->symbols().at(close).from - document->symbolEnd(open)
                        ).split('\n');
                        const QByteArray first = lines.takeFirst().trimmed();
                        if ( ! lines.isEmpty()) {
                            lines.last() = document->indentOf(document->symbolLine(close))
                                + lines.last().trimmed();
                            if (lines.last().trimmed().isEmpty()) {
                                lines.removeLast();
                            }
                        }

                        // Moves the later lines in until the shallowest stands at
                        // the body's indent.
                        int shallowest = -1;
                        for (int l = 0; l < lines.size(); ++l) {
                            const QByteArray & line = lines.at(l);
                            // Counts the line's leading spaces and tabs.
                            int width = 0;
                            while (width < line.size()
                                && (line.at(width) == ' ' || line.at(width) == '\t')
                            ) {
                                ++width;
                            }
                            if (width < line.size()) {
                                shallowest = shallowest < 0
                                    ? width : qMin(shallowest, width);
                            }
                        }
                        const int shift = shallowest < 0
                            ? 0 : qMax(0, bodyIndent.size() - shallowest);
                        QByteArray text = "{";
                        if ( ! first.isEmpty()) {
                            text += "\n" + bodyIndent + first;
                        }
                        for (int l = 0; l < lines.size(); ++l) {
                            const QByteArray & line = lines.at(l);
                            const QByteArray pad = line.trimmed().isEmpty()
                                ? QByteArray() : QByteArray(shift, ' ');
                            text += "\n" + pad + line;
                        }

                        // Closes the body on a line of its own.
                        text += "\n" + indent + "}";
                        document->replace(from, to - from, text);
                    } while (false);
                    editor.apply();
                    const int shift = source->symbols().size() - oldCount;
                    if (joinedUntil > index) {
                        joinedUntil += shift;
                    }
                    if (superSkipUntil > index) {
                        superSkipUntil += shift;
                    }
                    if (connectSkipUntil > index) {
                        connectSkipUntil += shift;
                    }
                    if (mappingSkipUntil > index) {
                        mappingSkipUntil += shift;
                    }
                    if (initializerSkipUntil > index) {
                        initializerSkipUntil += shift;
                    }
                    if (parameterSkipUntil > index) {
                        parameterSkipUntil += shift;
                    }
                    if (conditionSkipUntil > index) {
                        conditionSkipUntil += shift;
                    }
                    if (declarationSkipUntil > index) {
                        declarationSkipUntil += shift;
                    }
                }
            }
            if ( ! spaceLogicalNot || source->token(index) != NOT
                || source->isDirectiveLine(source->symbolLine(index))
                || (index > 0 && source->token(index - 1) == OPERATOR))
            {
                continue;
            }
            const int notStart = source->symbols().at(index).from;
            const int notEnd = source->symbolEnd(index);
            const int before = index > 0 ? source->symbolEnd(index - 1) : 0;
            const int after = index + 1 < source->symbols().size()
                ? source->symbols().at(index + 1).from : notEnd;
            CodeEditor editor = document.edit(before, after);
            if (index == 0 || source->token(index - 1) != NOT) {
                if (notStart > 0 && editor.get(before, notStart).isEmpty()) {
                    editor.set(before, notStart, " ");
                }
            }
            if (index + 1 < source->symbols().size()
                && source->token(index + 1) != NOT)
            {
                if (editor.get(notEnd, after).isEmpty()) {
                    editor.set(notEnd, after, " ");
                }
            }
            editor.apply();
        }
        document.applyEdits();
    }

    if (formatNamespaces) {
        for (int index = 0; index < document.symbols().size(); ++index) {
            if (document.token(index) != NAMESPACE
                || (index > 0 && document.token(index - 1) == USING))
            {
                continue;
            }
            int open = index + 1;
            while (open < document.symbols().size()
                && document.token(open) != LBRACE
                && document.token(open) != EQ && document.token(open) != SEMIC)
            {
                ++open;
            }
            const int close = document.match(open);
            if (document.token(open) != LBRACE || close <= open
                || document.spansDirective(document.symbols().at(open).from,
                                           document.symbolEnd(close)))
            {
                continue;
            }
            const QByteArray name = open > index + 1
                ? sourceText(&document, index + 1, open - 1).simplified() : QByteArray();
            const int closeLine = document.symbolLine(close);

            // Labels the `}` of a named namespace.
            const bool hasNext = close + 1 < document.symbols().size()
                && document.symbolLine(close + 1) == closeLine;
            if (hasNext) {
                continue;
            }
            const int from = document.symbolEnd(close);
            const int to = document.lineEnd(closeLine);
            const QByteArray rest = document.text().mid(from, to - from);
            const QByteArray comment = rest.trimmed();
            if ( ! comment.isEmpty() && ! comment.startsWith("//")) {
                continue;
            }

            // Keeps a comment of its own that already names the namespace, after
            // the `}` or on the line above it.
            const int scope = name.lastIndexOf("::");
            const QByteArray lastName = scope < 0 ? name : name.mid(scope + 2);
            const QByteArray above = closeLine > 1
                ? document.lineText(closeLine - 1).trimmed() : QByteArray();
            const bool namesAfter = ! comment.isEmpty() && ! isNamespaceLabel(comment)
                && commentNamesNamespace(comment, lastName);
            const bool namesAbove = comment.isEmpty()
                && document.firstSymbolOfLine(closeLine) == close
                && above.startsWith("//") && commentNamesNamespace(above, lastName);
            if ( ! name.isEmpty() && (namesAfter || namesAbove)) {
                continue;
            }
            QByteArray wanted;
            if ( ! name.isEmpty()) {
                wanted = " // Namespace `" + name + "`.";
            } else if ( ! comment.isEmpty()) {
                wanted = " " + comment;
            }
            if (rest != wanted) {
                document.replace(from, to - from, wanted);
                document.applyEdits();
            }
        }
    }

    const QSet<QByteArray > commentChecks = QSet<QByteArray >()
        << "mark-banner"
        << "trailing-comment"
        << "as-with"
        << "us-spelling"
        << "code-backticks"
        << "comment-case"
        << "comment-period"
        << "doc-tag-sign"
        << "comment-width"
        << "stub-line"
        << "doc-placement"
        << "so-hence"
        << "slash-and-or"
        << "will-may"
        << "mapping"
        << "plain-words"
        << "module-name"
        << "em-dash"
        << "host-mention"
        << "agents-md";
    for (int index = 0; index < activeChecks.size(); ++index) {
        const FormatRule *rule = activeChecks.at(index);
        if ( ! commentChecks.contains(rule->name())) {
            continue;
        }
        for (int pass = 0; pass < 8; ++pass) {
            const int issueCount = document.issues().size();
            document.setPass(pass);
            rule->run(&document);
            if ( ! document.applyEdits()) {
                break;
            }
            if ( ! document.inlineConfigurationError().isEmpty()) {
                result.error = document.inlineConfigurationError();
                return result;
            }
            if (pass < 7) {
                document.truncateIssues(issueCount);
            }
        }
    }

    if ( ! document.inlineConfigurationError().isEmpty()) {
        result.error = document.inlineConfigurationError();
        return result;
    }

    result.text = document.text();
    result.changed = result.text != text;
    result.issues = document.issues();
    result.requests = document.requests();
    if (document.hasInlineConfiguration()) {
        result.warnings = document.inlineWarnings(config.verbose);
    }
    return result;
}
