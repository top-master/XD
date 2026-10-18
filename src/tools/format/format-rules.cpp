// Copyright (C) 2015 The XD Company Ltd.
// SPDX-License-Identifier: Apache-2.0

#include "./format-rules.h"

#include "./rules/as-with.h"
#include "./rules/code-backticks.h"
#include "./rules/code-shape.h"
#include "./rules/comment-case.h"
#include "./rules/comment-period.h"
#include "./rules/comment-width.h"
#include "./rules/condition-close.h"
#include "./rules/condition-wrap.h"
#include "./rules/connect-layout.h"
#include "./rules/const-overload.h"
#include "./rules/continuation-indent.h"
#include "./rules/control-braces.h"
#include "./rules/doc-placement.h"
#include "./rules/excludes.h"
#include "./rules/global-static.h"
#include "./rules/init-list.h"
#include "./rules/inline-keyword.h"
#include "./rules/mark-banner.h"
#include "./rules/member-init.h"
#include "./rules/moc-run.h"
#include "./rules/no-lambda.h"
#include "./rules/not-space.h"
#include "./rules/one-statement.h"
#include "./rules/operator-first.h"
#include "./rules/operator-wrap.h"
#include "./rules/outline-this.h"
#include "./rules/param-wrap.h"
#include "./rules/pattern-report.h"
#include "./rules/private-access.h"
#include "./rules/qll.h"
#include "./rules/qpointer-getter.h"
#include "./rules/reference-space.h"
#include "./rules/relative-include.h"
#include "./rules/stub-line.h"
#include "./rules/super-typedef.h"
#include "./rules/template-close.h"
#include "./rules/ternary-wrap.h"
#include "./rules/text-report.h"
#include "./rules/trailing-comment.h"
#include "./rules/us-spelling.h"

#include <QtCore/QDir>
#include <QtCore/QFileInfo>
#include <QtCore/QMap>

/*!
    Builds one of each code rule, the fixes first, in the order they run:
    the spaces of a `!` before every break, since they count in a line's
    width; the braces before the condition layout, since a brace they add is
    the one the condition closes next to; and the operator breaks of a long
    line before the condition layout too, since a condition they break then
    closes in the same way.
*/
static QList<FormatRule * > formatCreateCodeRules()
{
    QList<FormatRule * > rules;
    rules << new FormatMocRunRule << new FormatQllRule << new FormatRelativeIncludeRule
        << new FormatNotSpaceRule
        << new FormatControlBracesRule << new FormatOperatorWrapRule
        << new FormatConditionWrapRule << new FormatTernaryWrapRule
        << new FormatConditionCloseRule
        << new FormatOperatorFirstRule << new FormatContinuationIndentRule
        << new FormatInitListRule << new FormatParamWrapRule
        << new FormatTemplateCloseRule << new FormatReferenceSpaceRule
        << new FormatOneStatementRule;
    rules << new FormatSuperTypedefRule;
    rules << new FormatNoLambdaRule << new FormatMemberInitRule << new FormatGlobalStaticRule
        << new FormatPrivateAccessRule << new FormatExcludesRule << new FormatCodeShapeRule;
    return rules;
}

/*!
    Builds one of each class rule.
*/
static QList<FormatRule * > formatCreateClassRules()
{
    QList<FormatRule * > rules;
    rules << new FormatOutlineThisRule << new FormatInlineKeywordRule
        << new FormatConnectLayoutRule;
    rules << new FormatConstOverloadRule << new FormatQPointerGetterRule;
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
        << new FormatCommentPeriodRule;
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
            "agents-md", "Reports mentions of the agent rule file.", "AGENTS\\.md",
            "restate the rule itself instead of naming the agent rule file");
    return rules;
}

/*!
    Builds one of each formatting rule, in the order the formatter runs them:
    the code and class rules first, since they may move the code that a
    comment rule looks at afterwards.
*/
QList<FormatRule * > formatCreateRules()
{
    QList<FormatRule * > rules;
    rules << formatCreateCodeRules();
    rules << formatCreateClassRules();
    rules << formatCreateCommentRules();
    return rules;
}

/*!
    Tells whether the file at \a path lies in the source tree of XD, the
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
    Moves the continuation lines of each statement in \a document along with
    the \a growths queued on its lines: a later line of the statement whose
    indent reaches past a growth's column (such as an argument aligned under
    a call's first one) gets that growth's characters added to its
    indent, or taken from it.
*/
void formatShiftContinuations(FormatDocument *document,
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
