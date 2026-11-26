// Copyright (C) 2015 The XD Company Ltd.
// SPDX-License-Identifier: Apache-2.0

#include "./format-rules.h"

#include "./rules/as-with.h"
#include "./rules/code-backticks.h"
#include "./rules/comment-case.h"
#include "./rules/comment-period.h"
#include "./rules/comment-width.h"
#include "./rules/doc-placement.h"
#include "./rules/doc-tag-sign.h"
#include "./rules/mark-banner.h"
#include "./rules/pattern-report.h"
#include "./rules/stub-line.h"
#include "./rules/text-report.h"
#include "./rules/trailing-comment.h"
#include "./rules/us-spelling.h"


#include <QtCore/QDir>
#include <QtCore/QFileInfo>
#include <QtCore/QMap>

/*!
    Describes the checks available to the formatter and its command line.
*/
class FormatCheckDescription : public FormatNamedRule
{
public:
    FormatCheckDescription(const char *name, const char *summary, bool readsOtherFiles = false)
        : FormatNamedRule(name, summary), m_readsOtherFiles(readsOtherFiles) {}
    void run(FormatDocument *) const Q_DECL_OVERRIDE {}
    bool readsOtherFiles() const Q_DECL_OVERRIDE { return m_readsOtherFiles; }
private:
    bool m_readsOtherFiles;
};

static QList<FormatRule * > formatCreateCodeRules()
{
    QList<FormatRule * > rules;
    rules << new FormatCheckDescription("moc-run-macro",
            "Spells moc macro Q_MOC_RUN.") << new FormatCheckDescription("replace-types",
            "Writes mapped type names.") << new FormatCheckDescription("relative-include",
            "Writes a relative path for a nearby quoted header.")
        << new FormatCheckDescription("inline-keyword",
            "Spells out inline on in-class definitions.")
        << new FormatCheckDescription("not-space",
            "Puts a space on each side of a logical not (!).") << new FormatCheckDescription("line-join",
            "Joins the lines of each expression when [Structure] target is any, for the layout rules to lay it out anew.")
        << new FormatCheckDescription("control-braces",
            "Wraps control statement bodies in braces.") << new FormatCheckDescription("namespace-braces",
            "Formats namespace braces and labels.")
        << new FormatCheckDescription("operator-wrap",
            "Breaks long expressions before their operators.")
        << new FormatCheckDescription("condition-wrap",
            "Breaks long conditions by nesting level.") << new FormatCheckDescription("ternary-wrap",
            "Breaks long conditional expressions.")
        << new FormatCheckDescription("chain-members-first",
            "Joins member access after closing brackets when it fits.")
        << new FormatCheckDescription("init-list-lead",
            "Leads member-initializer lines with separators.") << new FormatCheckDescription("param-wrap",
            "Wraps the parameters of a declaration.") << new FormatCheckDescription("args-wrap",
            "Breaks wrapped calls and brace lists.")
        << new FormatCheckDescription("operator-first",
            "Breaks a long expression before an operator.")
        << new FormatCheckDescription("continuation-indent",
            "Indents operator-led continuation lines.")
        << new FormatCheckDescription("condition-close",
            "Closes multiline conditions beside the opening brace.")
        << new FormatCheckDescription("template-close-space",
            "Spaces closing template brackets.") << new FormatCheckDescription("reference-space",
            "Writes a named reference declaration as Type & name.")
        << new FormatCheckDescription("one-statement",
            "Puts one statement on each line.");
    rules << new FormatCheckDescription("super-typedef",
        "Adds and uses a configured super typedef.", true);
    rules << new FormatCheckDescription("no-lambda",
            "Reports lambdas in XD source and nested lambdas.") << new FormatCheckDescription("member-init",
            "Reports data members initialized in the class body.") << new FormatCheckDescription("global-static",
            "Reports static objects and local static variables.")
        << new FormatCheckDescription("q-d-macro",
            "Reports d_func() and q_func() calls where Q_D or Q_Q fit.")
        << new FormatCheckDescription("excludes",
            "Reports negated includes checks.") << new FormatCheckDescription("code-shape",
            "Reports code shapes that need a manual rewrite.")
        << new FormatCheckDescription("reference-local",
            "Reports non-const local references.");
    return rules;
}

/*!
    Builds one of each class rule.
*/
static QList<FormatRule * > formatCreateClassRules()
{
    QList<FormatRule * > rules;
    rules << new FormatCheckDescription("outline-this",
            "Qualifies member access where configured.") << new FormatCheckDescription("connect-layout",
            "Lays out connection arguments in pairs.");
    rules << new FormatCheckDescription("const-overload-order",
            "Reports mutable overloads before their const counterparts.") << new FormatCheckDescription("qpointer-getter",
            "Reports raw pointers returned from QPointer fields.");
    return rules;
}

/*!
    Builds one of each comment rule, in the order they run: the layout first
    (a moved trailing comment then gets its wording checked), then the
    wording, then the checks that only report.
*/
static QList<FormatRule * > formatCreateCommentRules()
{
    QList<FormatRule * > rules;
    rules << new FormatMarkBannerRule << new FormatTrailingCommentRule;
    rules << new FormatAsWithRule << new FormatUsSpellingRule;
    rules << new FormatCodeBackticksRule << new FormatCommentCaseRule
        << new FormatCommentPeriodRule << new FormatDocTagSignRule;
    rules << new FormatCommentWidthRule << new FormatStubLineRule
        << new FormatDocPlacementRule;
    rules << new FormatPatternReportRule(
        "so-hence",
        "Reports a bare \"so\" joining two clauses in comments.",
        "(?:[,;]\\s+so\\b(?!\\s+(?:many|much|long|far|few|forth|on|that)\\b)"
            "|(?:^|[.!?]\\s+)so\\b(?![\\s-]+(?:many|much|long|far|called|that)\\b))",
        "join a consequence with hence, or a purpose with so that, not a bare so"
    );
    rules << new FormatPatternReportRule(
        "slash-and-or",
        "Reports a slash between words in comments.",
        "(?:^|\\s)(?!and/or\\b|try/catch\\b|I/O\\b|TCP/IP\\b|N/A\\b)"
            "[A-Za-z]{2,}/[A-Za-z]{2,}(?=[\\s,.;:)]|$)",
        "write and/or (or a list with commas) instead of the slash"
    );
    rules << new FormatPatternReportRule(
        "will-may",
        "Reports \"will\" for what the code does at run time.",
        "\\bwill\\b",
        "say may (or the present tense) for what the code does at run time"
    );
    rules << new FormatPatternReportRule(
        "mapping",
        "Reports wordings the comment rules map to another.",
        "\\bi\\.e\\.|,\\s+that is\\b[^,.;]*,|\\bhowever\\s+(?:small|large|big|long|short|many|much"
            "|few|little|slow|fast|hard|bad|good)\\b",
        "name both terms with and/or, or write no matter how, as the mapping says"
    );
    rules << new FormatPatternReportRule(
        "plain-words",
        "Reports high-register words in comments.",
        "\\b(?:aforementioned|heretofore|notwithstanding|pursuant|hereinafter|inasmuch"
            "|forthwith)\\b",
        "use a plain everyday word"
    );
    rules << new FormatPatternReportRule(
        "module-name",
        "Reports versioned module names in comments.",
        "(?<![/\\w])Qt5[A-Z]\\w*(?!\\w|\\.(?:dll|so|a|lib|prl|dylib|la|pc)\\b)",
        "name the module without its version (QtCore, not Qt5Core)"
    );
    rules << new FormatTextReportRule(
        "em-dash",
        "Reports em-dashes and en-dashes.",
        "\xe2\x80\x94|\xe2\x80\x93",
        "use ; : , ( ) or a plain hyphen instead of the dash"
    );
    rules << new FormatTextReportRule(
        "host-mention",
        "Reports mentions of the machine the work ran on.",
        "(?i)\\b(?:single[- ]core|one[- ]core|\\d+[- ]core\\s+(?:box|machine|host)"
            "|slow\\s+(?:box|machine|host)|low[- ]ram)\\b",
        "state the technical condition, not the machine"
    );
    rules << new FormatTextReportRule(
        "agents-md",
        "Reports mentions of the agent rule file.",
        "AGENTS\\.md",
        "restate the rule itself instead of naming the agent rule file"
    );
    return rules;
}

/*!
    Builds the checks available for selection. Code checks share a traversal;
    comment checks run afterwards on the resulting text.
*/
QList<FormatRule * > formatCreateRules()
{
    QList<FormatRule * > rules;
    rules << formatCreateCodeRules();
    rules << formatCreateClassRules();
    rules << formatCreateCommentRules();
    rules << new FormatCheckDescription("body-braces",
            "Gives body braces their own lines.");
    return rules;
}

/*!
    Tells whether the file at @a path lies in the source tree of XD, the
    tree whose root holds an `XD-mini.pro` file; the rules that only bind
    that tree (such as no lambdas) check it.
*/
bool formatIsInXdTree(const QString & path)
{
    if (path.isEmpty()) {
        return false;
    }
    QDir dir = QFileInfo(path).absoluteDir();
    for (;;) {
        if (QFileInfo(dir.filePath(QString(QLL("XD-mini.pro")))).isFile()) {
            return true;
        }
        if ( ! dir.cdUp()) {
            return false;
        }
    }
}

/*!
    Moves the continuation lines of each statement in @a document along with
    the @a growths queued on its lines: a later line of the statement whose
    indent reaches past a growth's column (such as an argument aligned under
    a call's first one) gets that growth's characters added to its
    indent, or taken from it.
*/
void formatShiftContinuations(
        FormatDocument *document,
        const QList<FormatGrowth > & growths)
{
    QMap<int, QList<FormatGrowth > > byLine;
    for (int i = 0; i < growths.size(); ++i) {
        byLine[document->symbolLine(growths.at(i).index)].append(growths.at(i));
    }
    QMap<int, QList<FormatGrowth > >::const_iterator it = byLine.constBegin();
    for (; it != byLine.constEnd(); ++it) {
        const int line = it.key();
        const QList<FormatGrowth > & onLine = it.value();

        // Finds the statement's last line.
        int endLine = line;
        int depth = 0;
        for (int k = onLine.first().index; k < document->symbols().size(); ++k) {
            const Token token = document->token(k);
            depth += token == LPAREN || token == LBRACK ? 1 : 0;
            depth -= token == RPAREN || token == RBRACK ? 1 : 0;
            const bool ends = token == SEMIC || token == LBRACE
                || token == RBRACE;
            if (depth < 0 || (ends && depth == 0)) {
                endLine = document->symbolLine(k);
                break;
            }
        }

        // Shifts each later line whose indent reaches past a growth.
        for (int other = line + 1; other <= endLine; ++other) {
            if (document->firstSymbolOfLine(other) < 0) {
                continue;
            }
            const QByteArray indent = document->indentOf(other);
            int shift = 0;
            for (int g = 0; g < onLine.size(); ++g) {
                if (onLine.at(g).column < indent.size()) {
                    shift += onLine.at(g).delta;
                }
            }
            if (shift > 0) {
                document->replace(
                    document->lineStart(other),
                    0,
                    QByteArray(shift, ' ')
                );
            } else if (shift < 0 && indent.size() + shift > 0) {
                document->replace(
                    document->lineStart(other),
                    -shift,
                    QByteArray()
                );
            }
        }
    }
}
