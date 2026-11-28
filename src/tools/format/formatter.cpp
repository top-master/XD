// Copyright (C) 2015 The XD Company Ltd.
// SPDX-License-Identifier: Apache-2.0

#include "./formatter.h"
#include "./ast/ast-parser.h"
#include "./ast/ast-printer.h"
#include "./ast/ast-transforms.h"
#include "./format-document.h"
#include "./format-line-spec.h"
#include "./format-rules.h"
#include "./lexer/format-language.h"

/*!
    @class Formatter
    @internal

    Formats a file's text using the selected checks, parsing code into an AST,
    applying AST/rule transforms, lowering into Doc IR, printing in one pass,
    and then running comment checks in memory.

    Holds no state that a format() call changes; hence several threads may
    format several files with one Formatter at once.
*/

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
        if ( ! found) {
            *error = QString(QLL("unknown rule: %1")).arg(names.at(n));
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
                        == config.checks.at(index)
                ) {
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
            if (config.checks.contains(QString::fromLatin1(m_rules.at(index)->name())))
            {
                activeChecks.append(m_rules.at(index));
            }
        }
    }

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

    if ( ! lineSpecs.isEmpty()) {
        QVector<QPair<int, int > > ranges;
        QList<QByteArray > lines = text.split('\n');
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

    AstRuleOptions options;
    for (int index = 0; index < activeChecks.size(); ++index) {
        const QByteArray & name = activeChecks.at(index)->name();
        options.fixMocMacro |= name == "moc-run-macro";
        options.spaceLogicalNot |= name == "not-space";
        options.checkExcludedFlags |= name == "excludes";
        options.checkPrivateAccess |= name == "q-d-macro";
        options.checkReferenceLocal |= name == "reference-local";
        options.checkLambdas |= name == "no-lambda";
        options.splitStatements |= name == "one-statement";
        options.checkMemberInitializers |= name == "member-init";
        options.spaceTemplateClosers |= name == "template-close-space";
        options.spaceReferences |= name == "reference-space";
        options.closeConditions |= name == "condition-close";
        options.checkStaticVariables |= name == "global-static";
        options.moveOperators |= name == "operator-first";
        options.joinMembers |= name == "chain-members-first";
        options.adjustIncludes |= name == "relative-include";
        options.wrapConditions |= name == "condition-wrap";
        options.wrapArguments |= name == "args-wrap";
        options.wrapParameters |= name == "param-wrap";
        options.wrapTernaries |= name == "ternary-wrap";
        options.wrapOperators |= name == "operator-wrap";
        options.braceControls |= name == "control-braces";
        options.leadInitializers |= name == "init-list-lead";
        options.expandBodies |= name == "body-braces";
        options.indentContinuations |= name == "continuation-indent";
        options.inlineMethods |= name == "inline-keyword";
        options.replaceTypes |= name == "replace-types";
        options.checkConstOverloads |= name == "const-overload-order";
        options.checkGuardedGetters |= name == "qpointer-getter";
        options.layoutConnections |= name == "connect-layout";
        options.checkCodeShapes |= name == "code-shape";
        options.qualifyMembers |= name == "outline-this";
        options.manageSuper |= name == "super-typedef";
        options.formatNamespaces |= name == "namespace-braces";
        options.joinExpressions |= name == "line-join";
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

    bool hasCommentChecks = false;
    for (int index = 0; index < activeChecks.size(); ++index) {
        if (commentChecks.contains(activeChecks.at(index)->name())) {
            hasCommentChecks = true;
            break;
        }
    }

    if (options.anyCodeRule()
        && FormatLanguageInfo::supportsCodeFormatting(document.language())
    ) {
        AstArena arena;
        const AstParseResult ast = document.buildAst(&arena);
        const AstTransformState state = AstTransforms::runDiagnosticsAndTransforms(
            &document,
            ast.root,
            options
        );
        const FormatDocument & printDoc = state.finalDocument
            ? *state.finalDocument : document;
        const AstNode *printRoot = state.finalRoot ? state.finalRoot : ast.root;
        DocPrinter::Options printerOptions;
        printerOptions.printWidth = printDoc.config().wrapMax;
        printerOptions.endOfLine = printDoc.text().contains("\r\n")
            ? QByteArray("\r\n") : QByteArray("\n");
        const QByteArray printed =
            AstPrinter::print(printRoot, state, printDoc, printerOptions);
        document.resetTextAndAnalyze(printed, hasCommentChecks);
    }

    for (int index = 0; index < activeChecks.size(); ++index) {
        const FormatRule *rule = activeChecks.at(index);
        if ( ! commentChecks.contains(rule->name())) {
            continue;
        }
        for (int pass = 0; pass < 8; ++pass) {
            const int issueCount = document.issues().size();
            document.setPass(pass);
            rule->run(&document);
            if ( ! document.takeCommentChanged()) {
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
