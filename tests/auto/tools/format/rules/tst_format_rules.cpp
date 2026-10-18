// Copyright (C) 2015 The XD Company Ltd.
// SPDX-License-Identifier: Apache-2.0

#include "format-document.h"
#include "format-line-spec.h"
#include "format-process-runner.h"
#include "format-runner.h"
#include "format-settings.h"
#include "formatter.h"
#include "preprocessor.h"

#include <QtCore/QLockFile>
#include <QtCore/QTemporaryDir>
#include <QtTest/QtTest>

typedef QVector<QPair<int, int > > LineRanges;
Q_DECLARE_METATYPE(LineRanges)
typedef QList<int > IssueLines;
Q_DECLARE_METATYPE(IssueLines)

/// Tests the formatter's logic, linked in directly: moc's comment tokens,
/// the document under formatting, the git-blame style line ranges, the
/// worker results, and each formatting rule.
class tst_FormatRules : public QObject
{
    Q_OBJECT
    typedef QObject super;
private slots:
    /// Moc's tokenizer.
    void testTokenize_shouldDropCommentsByDefault();
    void testTokenize_shouldEmitCommentsWhenKept();

    /// Document.
    void testComments_shouldTellEachKind_data();
    void testComments_shouldTellEachKind();
    void testComments_shouldTellTrailingFromOwnLine();
    void testSymbols_shouldLeaveOutCommentsAndWhitespace();
    void testReplace_shouldDropEditOutsideRanges();
    void testReplace_shouldDropOverlappingEdit();
    void testApplyEdits_shouldMoveRangesAndIssues();

    /// Line ranges.
    void testResolve_shouldFollowGitBlame_data();
    void testResolve_shouldFollowGitBlame();

    /// Formatter and runners.
    void testFormat_shouldKeepTextWithoutRules();
    void testFormat_shouldRejectBadRange();
    void testEncode_shouldRoundTripThroughDecode();

    /// Rules.
    void testFix_shouldRewrite_data();
    void testFix_shouldRewrite();
    void testReport_shouldFlagLines_data();
    void testReport_shouldFlagLines();
    void testRelativeInclude_shouldPinSiblingHeader();
    void testDocPlacement_shouldMoveHeaderDocsToSource();
    void testNoLambda_shouldFlagEveryLambdaInXdTree();
    void testAllRules_shouldKeepCleanCode();
    void testSettings_shouldReadFromRootDown();
    void testSettings_shouldReadWrapColumns();
    void testCommentWidth_shouldWrap_data();
    void testAgentMode_shouldAskForOperatorGroups();
    void testAgentMode_shouldAskForDocAccessChains();
    void testAgentMode_shouldAskNothingWhenOff();
    void testSettings_shouldInheritStructureInComments();
    void testCommentWidth_shouldWrap();
    void testSettings_shouldRejectUnknownValue();
    void testReferenceSpace_shouldFollowSettings_data();
    void testReferenceSpace_shouldFollowSettings();
    void testAllRules_shouldSurviveEmptyComments();
    void testRules_shouldRewriteFile_data();
    void testRules_shouldRewriteFile();

private:
    Symbols tokenize(const QByteArray & text, bool keepComments) const;
    FormatResult formatWith(const QString & rule, const QByteArray & text,
                            const QString & path = QString()) const;
    IssueLines issueLines(const FormatResult & result,
                          const QString & rule) const;
};

/// Tokenizes `text` the way the formatter does, with comment tokens only
/// when `keepComments` is set, and restores moc's switches afterwards.
Symbols tst_FormatRules::tokenize(const QByteArray & text,
                                  bool keepComments) const
{
    const bool preprocessOnly = Preprocessor::preprocessOnly;
    const bool keep = Preprocessor::keepComments;
    Preprocessor::preprocessOnly = true;
    Preprocessor::keepComments = keepComments;
    const Symbols symbols = qMove(Preprocessor::tokenize(text));
    Preprocessor::preprocessOnly = preprocessOnly;
    Preprocessor::keepComments = keep;
    return symbols;
}

/// Formats `text`, the file at `path`, with the rules that `rule` names,
/// joined by commas (every rule for an empty name).
FormatResult tst_FormatRules::formatWith(const QString & rule, const QByteArray & text,
                                         const QString & path) const
{
    Formatter formatter;
    QString error;
    const QStringList names = rule.split(QLatin1Char(','), QString::SkipEmptyParts);
    if (!formatter.selectRules(names, &error)) {
        FormatResult result;
        result.error = error;
        return result;
    }
    return formatter.format(text, path, QStringList());
}

/// Lists the lines of the issues of `result` that the rule `rule` reported.
IssueLines tst_FormatRules::issueLines(const FormatResult & result,
                                       const QString & rule) const
{
    IssueLines lines;
    for (int i = 0; i < result.issues.size(); ++i) {
        if (QString::fromLatin1(result.issues.at(i).rule) == rule) {
            lines << result.issues.at(i).line;
        }
    }
    return lines;
}

void tst_FormatRules::testTokenize_shouldDropCommentsByDefault()
{
    const Symbols symbols = qMove(this->tokenize("int a; // one\n/* two */ int b;\n"
                                                 "#define X 1 // three\n", false));
    for (int i = 0; i < symbols.size(); ++i) {
        qExpect(symbols.at(i).token != CPP_COMMENT && symbols.at(i).token != C_COMMENT)
                ->toBeTruthy();
    }
}

void tst_FormatRules::testTokenize_shouldEmitCommentsWhenKept()
{
    const QByteArray text = "int a; // one\n/* two\n */ int b;\n#include \"x.h\" // three\n";
    const Symbols symbols = qMove(this->tokenize(text, true));
    QList<QByteArray > comments;
    for (int i = 0; i < symbols.size(); ++i) {
        const Symbol & symbol = symbols.at(i);
        if (symbol.token == CPP_COMMENT || symbol.token == C_COMMENT) {
            comments << text.mid(symbol.from, symbol.len);
        }
    }
    const QList<QByteArray > expected = QList<QByteArray >() << "// one" << "/* two\n */"
        << "// three";
    QCOMPARE(comments, expected);
}

void tst_FormatRules::testComments_shouldTellEachKind_data()
{
    QTest::addColumn<QByteArray >("text");
    QTest::addColumn<int >("kind");
    QTest::newRow("line") << QByteArray("// a\n") << int(FormatComment::Line);
    QTest::newRow("triple slash") << QByteArray("/// a\n") << int(FormatComment::LineDoc);
    QTest::newRow("slash bang") << QByteArray("//! a\n") << int(FormatComment::LineDoc);
    QTest::newRow("four slashes") << QByteArray("//// a\n") << int(FormatComment::Line);
    QTest::newRow("block") << QByteArray("/* a */\n") << int(FormatComment::Block);
    QTest::newRow("star star") << QByteArray("/** a */\n") << int(FormatComment::BlockDoc);
    QTest::newRow("star bang") << QByteArray("/*! a */\n") << int(FormatComment::BlockDoc);
    QTest::newRow("empty block") << QByteArray("/**/\n") << int(FormatComment::Block);
}

void tst_FormatRules::testComments_shouldTellEachKind()
{
    QFETCH(QByteArray, text);
    QFETCH(int, kind);
    Formatter formatter;
    const FormatDocument document(text);
    qExpect(document.comments().size())->toEqual(1);
    qExpect(int(document.comments().at(0).kind))->toEqual(kind);
    qExpect(document.comments().at(0).offset)->toEqual(0);
    qExpect(document.comments().at(0).length)->toEqual(text.size() - 1);
}

void tst_FormatRules::testComments_shouldTellTrailingFromOwnLine()
{
    Formatter formatter;
    const FormatDocument document("// own\nint a; // trailing\n    /* own */ int b;\n");
    qExpect(document.comments().size())->toEqual(3);
    qExpect(document.comments().at(0).trailing)->toBeFalsy();
    qExpect(document.comments().at(1).trailing)->toBeTruthy();
    qExpect(document.comments().at(2).trailing)->toBeFalsy();
}

void tst_FormatRules::testSymbols_shouldLeaveOutCommentsAndWhitespace()
{
    Formatter formatter;
    const QByteArray text = "int /* x */ a; // y\n";
    const FormatDocument document(text);
    QList<QByteArray > lexems;
    for (int i = 0; i < document.symbols().size(); ++i) {
        const Symbol & symbol = document.symbols().at(i);
        lexems << text.mid(symbol.from, symbol.len);
    }
    QCOMPARE(lexems, QList<QByteArray >() << "int" << "a" << ";");
}

void tst_FormatRules::testReplace_shouldDropEditOutsideRanges()
{
    Formatter formatter;
    FormatDocument document("a\nb\nc\n");
    document.setLineRanges(LineRanges() << qMakePair(2, 2));
    qExpect(document.replace(0, 1, "x"))->toBeFalsy();
    qExpect(document.replace(2, 1, "y"))->toBeTruthy();
    qExpect(document.applyEdits())->toBeTruthy();
    qExpect(document.text())->toEqual(QByteArray("a\ny\nc\n"));
}

void tst_FormatRules::testReplace_shouldDropOverlappingEdit()
{
    Formatter formatter;
    FormatDocument document("abcdef\n");
    qExpect(document.replace(1, 3, "X"))->toBeTruthy();
    qExpect(document.replace(2, 1, "Y"))->toBeFalsy();
    qExpect(document.replace(4, 0, "Z"))->toBeTruthy();
    qExpect(document.replace(4, 0, "W"))->toBeFalsy();
    document.applyEdits();
    qExpect(document.text())->toEqual(QByteArray("aXZef\n"));
}

void tst_FormatRules::testApplyEdits_shouldMoveRangesAndIssues()
{
    Formatter formatter;
    FormatDocument document("a\nb\nc\nd\n");
    document.setLineRanges(LineRanges() << qMakePair(1, 1) << qMakePair(4, 4));
    document.report(6, "test-rule", QString(QLL("on d")));
    qExpect(document.replace(1, 0, "\nnew"))->toBeTruthy();
    document.applyEdits();
    qExpect(document.text())->toEqual(QByteArray("a\nnew\nb\nc\nd\n"));
    qExpect(document.issues().at(0).line)->toEqual(5);
    qExpect(document.isLineSelected(5))->toBeTruthy();
    qExpect(document.isLineSelected(4))->toBeFalsy();
}

void tst_FormatRules::testResolve_shouldFollowGitBlame_data()
{
    QTest::addColumn<QStringList >("specs");
    QTest::addColumn<LineRanges >("ranges");
    QTest::newRow("start,end") << (QStringList() << QString(QLL("2,3")))
        << (LineRanges() << qMakePair(2, 3));
    QTest::newRow("start,") << (QStringList() << QString(QLL("7,")))
        << (LineRanges() << qMakePair(7, 9));
    QTest::newRow(",end") << (QStringList() << QString(QLL(",3")))
        << (LineRanges() << qMakePair(1, 3));
    QTest::newRow("start") << (QStringList() << QString(QLL("8")))
        << (LineRanges() << qMakePair(8, 9));
    QTest::newRow("start,+count") << (QStringList() << QString(QLL("2,+2")))
        << (LineRanges() << qMakePair(2, 3));
    QTest::newRow("start,-count") << (QStringList() << QString(QLL("4,-2")))
        << (LineRanges() << qMakePair(3, 4));
    QTest::newRow("end before start") << (QStringList() << QString(QLL("4,2")))
        << (LineRanges() << qMakePair(2, 4));
    QTest::newRow("regex,+count") << (QStringList() << QString(QLL("/return/,+1")))
        << (LineRanges() << qMakePair(3, 3));
    QTest::newRow("regex from previous range")
        << (QStringList() << QString(QLL("1,4")) << QString(QLL("/return/")))
        << (LineRanges() << qMakePair(1, 4) << qMakePair(8, 9));
    QTest::newRow("anchored regex")
        << (QStringList() << QString(QLL("6,7")) << QString(QLL("^/return/,+1")))
        << (LineRanges() << qMakePair(6, 7) << qMakePair(3, 3));
    QTest::newRow("comma in regex") << (QStringList() << QString(QLL("/a,b/,+1")))
        << (LineRanges() << qMakePair(5, 5));
    QTest::newRow("funcname") << (QStringList() << QString(QLL(":other")))
        << (LineRanges() << qMakePair(6, 9));
    QTest::newRow("funcname to next") << (QStringList() << QString(QLL(":main")))
        << (LineRanges() << qMakePair(1, 5));
}

void tst_FormatRules::testResolve_shouldFollowGitBlame()
{
    QFETCH(QStringList, specs);
    QFETCH(LineRanges, ranges);
    const QList<QByteArray > lines = QByteArray("int main()\n{\n    return foo;\n}\n// a,b\n"
                                                "int other()\n{\n    return 0;\n}").split('\n');
    LineRanges resolved;
    QString error;
    qExpect(FormatLineSpec::resolveAll(specs, lines, &resolved, &error))->toBeTruthy();
    qExpect(error)->toBeEmpty();
    QCOMPARE(resolved, ranges);
}

void tst_FormatRules::testFormat_shouldKeepTextWithoutRules()
{
    Formatter formatter;
    const QByteArray text = "int  a ;\n";
    const FormatResult result = qMove(formatter.format(text, QString(), QStringList()));
    qExpect(result.error)->toBeEmpty();
    qExpect(result.changed)->toBeFalsy();
    qExpect(result.text)->toEqual(text);
}

void tst_FormatRules::testFormat_shouldRejectBadRange()
{
    Formatter formatter;
    const FormatResult result = qMove(formatter.format("a\nb\n", QString(),
                                                       QStringList() << QString(QLL("3"))));
    qExpect(result.error)->toEqualString(QLL("-L 3: file has only 2 lines"));
}

void tst_FormatRules::testEncode_shouldRoundTripThroughDecode()
{
#ifdef QT_NO_PROCESS
    QSKIP("This build has no process runner.");
#else
    FormatFileResult result;
    result.path = QString(QLL("dir with\ttab/a%b.cpp"));
    result.changed = true;
    result.issues << FormatIssue(3, "some-rule", QString(QLL("two\nlines\tand tab")));
    result.issues << FormatIssue(9, "other-rule", QString(QLL("plain")));
    result.requests
        << FormatIssue(4, "ask-rule", QString(QLL("regroup")), 6, 70);
    const QList<QByteArray > lines = FormatProcessRunner::encode(result).split('\n');
    QList<FormatFileResult > decoded;
    for (int i = 0; i < lines.size(); ++i) {
        FormatProcessRunner::decode(lines.at(i), &decoded);
    }
    qExpect(decoded.size())->toEqual(1);
    qExpect(decoded.at(0).path)->toEqual(result.path);
    qExpect(decoded.at(0).changed)->toBeTruthy();
    qExpect(decoded.at(0).issues.size())->toEqual(2);
    qExpect(decoded.at(0).issues.at(0).line)->toEqual(3);
    qExpect(decoded.at(0).issues.at(0).rule)->toEqual(QByteArray("some-rule"));
    qExpect(decoded.at(0).issues.at(0).message)->toEqual(result.issues.at(0).message);
    qExpect(decoded.at(0).requests.size())->toEqual(1);
    qExpect(decoded.at(0).requests.at(0).line)->toEqual(4);
    qExpect(decoded.at(0).requests.at(0).endLine)->toEqual(6);
    qExpect(decoded.at(0).requests.at(0).columnLimit)->toEqual(70);
    qExpect(decoded.at(0).requests.at(0).rule)->toEqual(QByteArray("ask-rule"));
#endif
}

void tst_FormatRules::testFix_shouldRewrite_data()
{
    QTest::addColumn<QString >("rule");
    QTest::addColumn<QByteArray >("input");
    QTest::addColumn<QByteArray >("output");

    // Macro spelling.
    QTest::newRow("moc-run-macro")
        << QString(QLL("moc-run-macro"))
        << QByteArray("#ifndef QT_MOC_RUN\nint a;\n#endif\n")
        << QByteArray("#ifndef Q_MOC_RUN\nint a;\n#endif\n");
    // Control statements.
    QTest::newRow("control-braces one line")
        << QString(QLL("control-braces"))
        << QByteArray("void f()\n{\n    if (x) return;\n}\n")
        << QByteArray("void f()\n{\n    if (x) { return; }\n}\n");
    QTest::newRow("control-braces body below")
        << QString(QLL("control-braces"))
        << QByteArray("void f()\n{\n    if (x)\n        a();\n}\n")
        << QByteArray("void f()\n{\n    if (x) {\n        a();\n    }\n}\n");
    QTest::newRow("control-braces else")
        << QString(QLL("control-braces"))
        << QByteArray("void f()\n{\n    if (x)\n        a();\n    else\n        b();\n}\n")
        << QByteArray("void f()\n{\n    if (x) {\n        a();\n    } else {\n        b();\n    }\n}\n");
    QTest::newRow("control-braces else if")
        << QString(QLL("control-braces"))
        << QByteArray("void f()\n{\n    if (x)\n        a();\n    else if (y)\n        b();\n}\n")
        << QByteArray("void f()\n{\n    if (x) {\n        a();\n    } else if (y) {\n        b();\n    }\n}\n");
    QTest::newRow("control-braces for")
        << QString(QLL("control-braces"))
        << QByteArray("void f()\n{\n    for (int i = 0; i < n; ++i)\n        a(i);\n}\n")
        << QByteArray("void f()\n{\n    for (int i = 0; i < n; ++i) {\n        a(i);\n    }\n}\n");
    QTest::newRow("control-braces do")
        << QString(QLL("control-braces"))
        << QByteArray("void f()\n{\n    do\n        a();\n    while (x);\n}\n")
        << QByteArray("void f()\n{\n    do {\n        a();\n    } while (x);\n}\n");
    QTest::newRow("control-braces trailing comment")
        << QString(QLL("control-braces"))
        << QByteArray("void f()\n{\n    while (x)\n        a(); // Why.\n}\n")
        << QByteArray("void f()\n{\n    while (x) {\n        a(); // Why.\n    }\n}\n");
    // Logical not.
    QTest::newRow("not-space")
        << QString(QLL("not-space"))
        << QByteArray("void f()\n{\n    if (!x) {\n        a();\n    }\n}\n")
        << QByteArray("void f()\n{\n    if ( ! x) {\n        a();\n    }\n}\n");
    QTest::newRow("not-space after an operator")
        << QString(QLL("not-space"))
        << QByteArray("bool b = a &&!c;\n")
        << QByteArray("bool b = a && ! c;\n");
    QTest::newRow("not-space keeps a run of signs together")
        << QString(QLL("not-space"))
        << QByteArray("bool b = !!x;\n")
        << QByteArray("bool b = !! x;\n");
    QTest::newRow("not-space leaves the not-equal operator")
        << QString(QLL("not-space"))
        << QByteArray("bool b = a!=c || a != d;\n")
        << QByteArray("bool b = a!=c || a != d;\n");
    QTest::newRow("not-space leaves preprocessor lines")
        << QString(QLL("not-space"))
        << QByteArray("#if !defined(X) && !Y\nint a;\n#endif\n")
        << QByteArray("#if !defined(X) && !Y\nint a;\n#endif\n");
    QTest::newRow("not-space leaves signs that have their spaces")
        << QString(QLL("not-space"))
        << QByteArray("void f()\n{\n    if ( ! x) {\n        a();\n    }\n}\n")
        << QByteArray("void f()\n{\n    if ( ! x) {\n        a();\n    }\n}\n");
    QTest::newRow("not-space takes the indent as the space in front")
        << QString(QLL("not-space"))
        << QByteArray("bool b =\n    !y;\n")
        << QByteArray("bool b =\n    ! y;\n");
    QTest::newRow("not-space leaves the operator name")
        << QString(QLL("not-space"))
        << QByteArray("bool operator!() const;\n")
        << QByteArray("bool operator!() const;\n");
    QTest::newRow("condition-close same line")
        << QString(QLL("condition-close"))
        << QByteArray("void f()\n{\n    if (a\n        && b) {\n        c();\n    }\n}\n")
        << QByteArray("void f()\n{\n    if (a\n        && b\n    ) {\n        c();\n    }\n}\n");
    QTest::newRow("condition-close brace below")
        << QString(QLL("condition-close"))
        << QByteArray("void f()\n{\n    while (a\n        || b)\n    {\n        c();\n    }\n}\n")
        << QByteArray("void f()\n{\n    while (a\n        || b\n    ) {\n        c();\n    }\n}\n");

    // Operators.
    QTest::newRow("operator-first and")
        << QString(QLL("operator-first"))
        << QByteArray("void f()\n{\n    x = a &&\n        b;\n}\n")
        << QByteArray("void f()\n{\n    x = a\n        && b;\n}\n");
    QTest::newRow("operator-first stream")
        << QString(QLL("operator-first"))
        << QByteArray("void f()\n{\n    out << a <<\n        b;\n}\n")
        << QByteArray("void f()\n{\n    out << a\n        << b;\n}\n");
    QTest::newRow("init-list-lead")
        << QString(QLL("init-list-lead"))
        << QByteArray("Foo::Foo() :\n    a(0),\n    b(0)\n{\n}\n")
        << QByteArray("Foo::Foo()\n    : a(0)\n    , b(0)\n{\n}\n");

    // Spacing.
    QTest::newRow("template-close-space")
        << QString(QLL("template-close-space"))
        << QByteArray("QList<QPair<int, int>> a;\nQMap<QString, int> m;\n"
                          "void f()\n{\n    int x = static_cast<int>(y);\n    if (a < b) {\n    }\n}\n")
        << QByteArray("QList<QPair<int, int > > a;\nQMap<QString, int > m;\n"
                          "void f()\n{\n    int x = static_cast<int >(y);\n    if (a < b) {\n    }\n}\n");
    QTest::newRow("reference-space")
        << QString(QLL("reference-space"))
        << QByteArray("void f(const QString &name, QString& out, int &count);\n"
                          "void g()\n{\n    int x = a &b;\n    foo(&c);\n}\n")
        << QByteArray("void f(const QString & name, QString & out, int & count);\n"
                          "void g()\n{\n    int x = a &b;\n    foo(&c);\n}\n");
    QTest::newRow("param-wrap")
        << QString(QLL("param-wrap"))
        << QByteArray("void Widget::reload(const QString & name, int id, const QObject *receiver, int x);\nQSharedPointer<DocumentRepositoryEntry > DocumentRepository::findEntryByName(const QString & name, int id);\n")
        << QByteArray("void Widget::reload(const QString & name, int id, const QObject *receiver,\n                    int x);\nQSharedPointer<DocumentRepositoryEntry > DocumentRepository::findEntryByName(\n        const QString & name, int id);\n");
    QTest::newRow("param-wrap one parameter")
        << QString(QLL("param-wrap"))
        << QByteArray("QSharedPointer<DocumentRepositoryEntry > DocumentRepository::findEntryByName(const QString & name);\n")
        << QByteArray("QSharedPointer<DocumentRepositoryEntry > DocumentRepository::findEntryByName(\n        const QString & name);\n");
    QTest::newRow("param-wrap init list")
        << QString(QLL("param-wrap"))
        << QByteArray("class A : public B\n{\n    inline A()\n        : super(\"name-of-the-rule\", \"A long summary text that runs past eighty columns.\")\n    {\n    }\n    inline A(const char *name, const char *summary, int count) : super(name, summary) {}\n};\n")
        << QByteArray("class A : public B\n{\n    inline A()\n        : super(\"name-of-the-rule\", \"A long summary text that runs past eighty columns.\")\n    {\n    }\n    inline A(const char *name, const char *summary, int count) : super(name, summary) {}\n};\n");
    QTest::newRow("param-wrap one-line body")
        << QString(QLL("param-wrap"))
        << QByteArray("class A\n{\n    static QByteArray absName(const char *name) { return encodeName(QByteArray(name).split('.')); }\n};\n")
        << QByteArray("class A\n{\n    static QByteArray absName(const char *name) { return encodeName(QByteArray(name).split('.')); }\n};\n");
    QTest::newRow("comment-period bare rule")
        << QString(QLL("comment-period"))
        << QByteArray("// ---------------------------------------------------------------------------\nint a;\n")
        << QByteArray("// ---------------------------------------------------------------------------\nint a;\n");
    QTest::newRow("comment-period inline label")
        << QString(QLL("comment-period"))
        << QByteArray("void f()\n{\n    write(data, /*endStream*/ true);\n}\n")
        << QByteArray("void f()\n{\n    write(data, /*endStream*/ true);\n}\n");
    QTest::newRow("continuation-indent")
        << QString(QLL("continuation-indent"))
        << QByteArray("void f()\n{\n    out << a\n      << b;\n    if (x\n            || (y\n        && z)\n    ) {\n    }\n    call(a,\n         b\n         + c);\n    int t[] = { 1,\n                -2 };\n}\n")
        << QByteArray("void f()\n{\n    out << a\n        << b;\n    if (x\n        || (y\n            && z)\n    ) {\n    }\n    call(a,\n         b\n         + c);\n    int t[] = { 1,\n                -2 };\n}\n");
    QTest::newRow("comment-case one word")
        << QString(QLL("comment-case"))
        << QByteArray("static const char t[] = {\n    // ....opti\n    0x01,\n    // ons....h\n    0x02\n};\n")
        << QByteArray("static const char t[] = {\n    // ....opti\n    0x01,\n    // ons....h\n    0x02\n};\n");
    QTest::newRow("comment-period one word")
        << QString(QLL("comment-period"))
        << QByteArray("static const char t[] = {\n    // ....opti\n    0x01,\n    // ons....h\n    0x02\n};\n")
        << QByteArray("static const char t[] = {\n    // ....opti\n    0x01,\n    // ons....h\n    0x02\n};\n");
    QTest::newRow("trailing-comment table marker")
        << QString(QLL("trailing-comment"))
        << QByteArray("// The rows of the table; format: table.\nstatic const char *t[] = {\n    \"a\", // first entry here\n    \"b\" // second one\n};\n")
        << QByteArray("// The rows of the table; format: table.\nstatic const char *t[] = {\n    \"a\", // first entry here\n    \"b\" // second one\n};\n");
    QTest::newRow("comment-period table marker")
        << QString(QLL("comment-period"))
        << QByteArray("// The rows of the table; format: table.\nstatic const char *t[] = {\n    \"a\", // first entry here\n    \"b\" // second one\n};\n")
        << QByteArray("// The rows of the table; format: table.\nstatic const char *t[] = {\n    \"a\", // first entry here\n    \"b\" // second one\n};\n");
    QTest::newRow("outline-this realigns arguments")
        << QString(QLL("outline-this"))
        << QByteArray("class W\n{\n    void report(int a, int b);\n};\nvoid W::f()\n{\n    report(1,\n           2);\n}\n")
        << QByteArray("class W\n{\n    void report(int a, int b);\n};\nvoid W::f()\n{\n    this->report(1,\n                 2);\n}\n");
    QTest::newRow("init-list-lead aligns commas")
        << QString(QLL("init-list-lead"))
        << QByteArray("Foo::Foo()\n    : a(0),\n      b(0)\n{\n}\n")
        << QByteArray("Foo::Foo()\n    : a(0)\n    , b(0)\n{\n}\n");
    QTest::newRow("operator-first keeps directives")
            << QString(QLL("operator-first"))
            << QByteArray("template <class T> struct is_pod\n : integral_constant<bool, (is_integral<T>::value ||\n                            is_floating_point<T>::value ||\n#if !defined(_MSC_VER)\n                            is_enum<T>::value ||\n#endif\n                            is_pointer<T>::value)> { };\n")
            << QByteArray("template <class T> struct is_pod\n : integral_constant<bool, (is_integral<T>::value\n                            || is_floating_point<T>::value ||\n#if !defined(_MSC_VER)\n                            is_enum<T>::value ||\n#endif\n                            is_pointer<T>::value)> { };\n");
    QTest::newRow("code-backticks leaves templates")
            << QString(QLL("code-backticks"))
            << QByteArray("// Note that std::pair<T,U> is no POD.\nint a;\n")
            << QByteArray("// Note that std::pair<T,U> is no POD.\nint a;\n");
    QTest::newRow("inline-keyword after macros")
            << QString(QLL("inline-keyword"))
            << QByteArray("class A\n{\n    inline int a() const { return 1; }\n    Q_SKIP_UNUSED\n    Q_ALWAYS_INLINE_T void b() { }\n    Q_SKIP_UNUSED\n    void c() { }\n};\n")
            << QByteArray("class A\n{\n    inline int a() const { return 1; }\n    Q_SKIP_UNUSED\n    Q_ALWAYS_INLINE_T void b() { }\n    Q_SKIP_UNUSED\n    inline void c() { }\n};\n");
    QTest::newRow("outline-this keeps locals and macro arguments")
        << QString(QLL("outline-this"))
        << QByteArray("class W\n{\n    int y;\n    int m;\n    int end() const;\n    void f();\n};\nvoid W::f()\n{\n    int a, y, m;\n    y = m;\n    for (int i = 0, end = 2; i < end; ++i) {\n    }\n    g(QT_PREPEND_NAMESPACE(end)(1));\n    connect(a, SIGNAL(end()), b, SLOT(end()));\n}\n")
        << QByteArray("class W\n{\n    int y;\n    int m;\n    int end() const;\n    void f();\n};\nvoid W::f()\n{\n    int a, y, m;\n    y = m;\n    for (int i = 0, end = 2; i < end; ++i) {\n    }\n    g(QT_PREPEND_NAMESPACE(end)(1));\n    connect(a, SIGNAL(end()), b, SLOT(end()));\n}\n");
    QTest::newRow("outline-this leaves a lambda without captures")
        << QString(QLL("outline-this"))
        << QByteArray("class W\n{\npublic:\n    void run();\n    int size(int a) const;\nprivate:\n    int m_x;\n};\n"
                      "void W::run()\n{\n    m_x = 0;\n    auto f = [](int a) { return size(a); };\n}\n")
        << QByteArray("class W\n{\npublic:\n    void run();\n    int size(int a) const;\nprivate:\n    int m_x;\n};\n"
                      "void W::run()\n{\n    this->m_x = 0;\n    auto f = [](int a) { return size(a); };\n}\n");
    QTest::newRow("outline-this reaches through a lambda that captures this")
        << QString(QLL("outline-this"))
        << QByteArray("class W\n{\npublic:\n    void run();\n    int size(int a) const;\nprivate:\n    int m_x;\n};\n"
                      "void W::run()\n{\n    m_x = 0;\n    auto f = [this](int a) { return size(a); };\n}\n")
        << QByteArray("class W\n{\npublic:\n    void run();\n    int size(int a) const;\nprivate:\n    int m_x;\n};\n"
                      "void W::run()\n{\n    this->m_x = 0;\n    auto f = [this](int a) { return this->size(a); };\n}\n");
    QTest::newRow("outline-this reaches through a lambda that captures by copy")
        << QString(QLL("outline-this"))
        << QByteArray("class W\n{\npublic:\n    void run();\n    int size(int a) const;\nprivate:\n    int m_x;\n};\n"
                      "void W::run()\n{\n    m_x = 0;\n    auto f = [=](int a) { return size(a); };\n}\n")
        << QByteArray("class W\n{\npublic:\n    void run();\n    int size(int a) const;\nprivate:\n    int m_x;\n};\n"
                      "void W::run()\n{\n    this->m_x = 0;\n    auto f = [=](int a) { return this->size(a); };\n}\n");
    QTest::newRow("outline-this reaches through a lambda that captures by reference")
        << QString(QLL("outline-this"))
        << QByteArray("class W\n{\npublic:\n    void run();\n    int size(int a) const;\nprivate:\n    int m_x;\n};\n"
                      "void W::run()\n{\n    m_x = 0;\n    auto f = [&](int a) { return size(a); };\n}\n")
        << QByteArray("class W\n{\npublic:\n    void run();\n    int size(int a) const;\nprivate:\n    int m_x;\n};\n"
                      "void W::run()\n{\n    this->m_x = 0;\n    auto f = [&](int a) { return this->size(a); };\n}\n");
    QTest::newRow("outline-this leaves a lambda that captures a variable by reference")
        << QString(QLL("outline-this"))
        << QByteArray("class W\n{\npublic:\n    void run();\n    int size(int a) const;\nprivate:\n    int m_x;\n};\n"
                      "void W::run()\n{\n    m_x = 0;\n    auto f = [&n](int a) { return size(a); };\n}\n")
        << QByteArray("class W\n{\npublic:\n    void run();\n    int size(int a) const;\nprivate:\n    int m_x;\n};\n"
                      "void W::run()\n{\n    this->m_x = 0;\n    auto f = [&n](int a) { return size(a); };\n}\n");
    QTest::newRow("inline-keyword after a flags macro")
        << QString(QLL("inline-keyword"))
        << QByteArray("class A\n{\n    enum F { X = 1 };\n    Q_DECLARE_FLAGS(Fs, F)\n    inline int a() const { return 1; }\n    int b() const { return 2; }\n};\n")
        << QByteArray("class A\n{\n    enum F { X = 1 };\n    Q_DECLARE_FLAGS(Fs, F)\n    inline int a() const { return 1; }\n    inline int b() const { return 2; }\n};\n");
    QTest::newRow("inline-keyword head cut by a directive")
        << QString(QLL("inline-keyword"))
        << QByteArray("class A\n{\n    inline A()\n        : a(0),\n#ifdef X\n        b(1),\n#endif\n        c(2) {}\n    int a, b, c;\n};\n")
        << QByteArray("class A\n{\n    inline A()\n        : a(0),\n#ifdef X\n        b(1),\n#endif\n        c(2) {}\n    int a, b, c;\n};\n");
    QTest::newRow("inline-keyword after a macro and an access specifier")
        << QString(QLL("inline-keyword"))
        << QByteArray("class P : public QObjectPrivate\n{\n    Q_DECLARE_PUBLIC(Model)\n\npublic:\n    P() : x(0) {}\n    inline int y() const { return 1; }\n    int x;\n};\n")
        << QByteArray("class P : public QObjectPrivate\n{\n    Q_DECLARE_PUBLIC(Model)\n\npublic:\n    inline P() : x(0) {}\n    inline int y() const { return 1; }\n    int x;\n};\n");
    QTest::newRow("one-statement")
        << QString(QLL("one-statement"))
        << QByteArray("void f()\n{\n    a(); b();\n    for (i = 0; i < n; ++i) {\n    }\n}\n")
        << QByteArray("void f()\n{\n    a();\n    b();\n    for (i = 0; i < n; ++i) {\n    }\n}\n");
    QTest::newRow("super-typedef access first")
        << QString(QLL("super-typedef"))
        << QByteArray("class Foo : public Bar\n{\npublic:\n    void f();\n};\n")
        << QByteArray("class Foo : public Bar\n{\n    typedef Bar super;\npublic:\n    void f();\n};\n");
    QTest::newRow("super-typedef no base")
        << QString(QLL("super-typedef"))
        << QByteArray("class A\n{\nprivate:\n    class B\n    {\n        int b;\n    };\n};\n")
        << QByteArray("class A\n{\nprivate:\n    class B\n    {\n        int b;\n    };\n};\n");
    QTest::newRow("super-typedef after Q_OBJECT")
        << QString(QLL("super-typedef"))
        << QByteArray("class Foo : public QObject\n{\n    Q_OBJECT\npublic:\n};\n")
        << QByteArray("class Foo : public QObject\n{\n    Q_OBJECT\n    typedef QObject super;\npublic:\n};\n");

    // Classes.
    QTest::newRow("outline-this")
        << QString(QLL("outline-this"))
        << QByteArray("class W\n{\npublic:\n    void run();\n    int size() const;\n    static int count();\n"
                          "private:\n    int m_x;\n};\nvoid W::run()\n{\n    m_x = size() + count();\n"
                          "    int y = 0;\n    other.m_x = y;\n}\nint W::count()\n{\n    return size();\n}\n")
        << QByteArray("class W\n{\npublic:\n    void run();\n    int size() const;\n    static int count();\n"
                          "private:\n    int m_x;\n};\nvoid W::run()\n{\n    this->m_x = this->size() + count();\n"
                          "    int y = 0;\n    other.m_x = y;\n}\nint W::count()\n{\n    return size();\n}\n");
    QTest::newRow("inline-keyword")
        << QString(QLL("inline-keyword"))
        << QByteArray("class A\n{\npublic:\n    inline int a() const { return 1; }\n"
                          "    int b() const { return 2; }\n    friend bool f() { return true; }\n};\n"
                          "class B\n{\n    int c() { return 3; }\n};\n")
        << QByteArray("class A\n{\npublic:\n    inline int a() const { return 1; }\n"
                          "    inline int b() const { return 2; }\n    friend bool f() { return true; }\n};\n"
                          "class B\n{\n    int c() { return 3; }\n};\n");
    QTest::newRow("connect-layout")
        << QString(QLL("connect-layout"))
        << QByteArray("void W::run()\n{\n    QObject::connect(sender, SIGNAL(ready()), receiver, "
                          "SLOT(onReady()), Qt::QueuedConnection);\n    connect(a,\n            SIGNAL(x()), "
                          "b, SLOT(y()));\n}\n")
        << QByteArray("void W::run()\n{\n    QObject::connect(sender, SIGNAL(ready()),\n"
                          "                     receiver, SLOT(onReady()),\n"
                          "                     Qt::QueuedConnection);\n"
                          "    connect(a, SIGNAL(x()), b, SLOT(y()));\n}\n");
    QTest::newRow("connect-layout CRLF")
        << QString(QLL("connect-layout"))
        << QByteArray("void W::run()\r\n{\r\n    QObject::connect(sender, SIGNAL(ready()), receiver, "
                          "SLOT(onReady()), Qt::QueuedConnection);\r\n    connect(a,\r\n            "
                          "SIGNAL(x()), b, SLOT(y()));\r\n}\r\n")
        << QByteArray("void W::run()\r\n{\r\n    QObject::connect(sender, SIGNAL(ready()),\r\n"
                          "                     receiver, SLOT(onReady()),\r\n"
                          "                     Qt::QueuedConnection);\r\n"
                          "    connect(a, SIGNAL(x()), b, SLOT(y()));\r\n}\r\n");

    // Comments.
    QTest::newRow("mark-banner line")
        << QString(QLL("mark-banner"))
        << QByteArray("// ---- Helpers ----\nint a;\n")
        << QByteArray("// MARK: Helpers.\nint a;\n");
    QTest::newRow("mark-banner block")
        << QString(QLL("mark-banner"))
        << QByteArray("/* ===== Setup ===== */\nint a;\n")
        << QByteArray("// MARK: Setup.\nint a;\n");
    QTest::newRow("trailing-comment")
        << QString(QLL("trailing-comment"))
        << QByteArray("class A\n{\n    int a; // The count.\n    int b; ///< The size.\n};"
                          "  // Ends A.\n#endif // X\n")
        << QByteArray("class A\n{\n    // The count.\n    int a;\n    /// The size.\n    int b;\n};"
                          "  // Ends A.\n#endif // X\n");
    QTest::newRow("as-with")
        << QString(QLL("as-with"))
        << QByteArray("// As with the cache, it loads `as with`.\nint a;\n")
        << QByteArray("// Just like the cache, it loads `as with`.\nint a;\n");
    QTest::newRow("us-spelling")
        << QString(QLL("us-spelling"))
        << QByteArray("// The colour and behaviour are Cancelled.\nint a;\n")
        << QByteArray("// The color and behavior are Canceled.\nint a;\n");
    QTest::newRow("code-backticks")
        << QString(QLL("code-backticks"))
        << QByteArray("int retryCount;\n// Calls run() and Cache::clear() as retryCount grows; see src/run().\n"
                          "/// Calls run().\nvoid f();\n")
        << QByteArray("int retryCount;\n// Calls `run()` and `Cache::clear()` as `retryCount` grows; see src/run().\n"
                          "/// Calls run().\nvoid f();\n");
    QTest::newRow("code-backticks access chains")
        << QString(QLL("code-backticks"))
        << QByteArray("// Calls watcher()->load() first.\n/// Reads it through watcher()->load(), as run() does.\n"
                          "void f();\n")
        << QByteArray("// Calls `watcher()->load()` first.\n"
                          "/// Reads it through `watcher()->load()`, as run() does.\nvoid f();\n");
    QTest::newRow("operator-wrap")
        << QString(QLL("operator-wrap"))
        << QByteArray("void f()\n{\n    out << \"label\" << firstValue << secondValue << thirdValue << fourthValue"
                          " << fifthValue << sixthValue;\n}\n")
        << QByteArray("void f()\n{\n    out << \"label\" << firstValue << secondValue << thirdValue << fourthValue\n"
                          "        << fifthValue << sixthValue;\n}\n");
    QTest::newRow("operator-wrap leaves an unbreakable line")
        << QString(QLL("operator-wrap"))
        << QByteArray("void f()\n{\n    call(aVeryLongArgumentName, anotherVeryLongArgumentName, aThirdVeryLongOne);\n}\n")
        << QByteArray("void f()\n{\n    call(aVeryLongArgumentName, anotherVeryLongArgumentName, aThirdVeryLongOne);\n}\n");
    QTest::newRow("operator-wrap leaves a compact braced line")
        << QString(QLL("operator-wrap"))
        << QByteArray("void f()\n{\n    if (any || qtype == firstValue) { addA(firstValue); addA(secondValue); addA(thirdValue); }\n}\n")
        << QByteArray("void f()\n{\n    if (any || qtype == firstValue) { addA(firstValue); addA(secondValue); addA(thirdValue); }\n}\n");
    QTest::newRow("operator-wrap leaves a wrapped statement")
        << QString(QLL("operator-wrap"))
        << QByteArray("void f()\n{\n    logWarning(QLL(\"blackhole: cannot listen on \") + m_host.toString() + QLL(\":\") + port\n               + QString::number(m_wantPort));\n}\n")
        << QByteArray("void f()\n{\n    logWarning(QLL(\"blackhole: cannot listen on \") + m_host.toString() + QLL(\":\") + port\n               + QString::number(m_wantPort));\n}\n");
    QTest::newRow("operator-wrap leaves argument operators")
        << QString(QLL("operator-wrap"))
        << QByteArray("void f()\n{\n    deflateInit2(&zs, Z_DEFAULT_COMPRESSION, Z_DEFLATED, 15 + 16, 8, Z_DEFAULT_STRATEGY, more);\n}\n")
        << QByteArray("void f()\n{\n    deflateInit2(&zs, Z_DEFAULT_COMPRESSION, Z_DEFLATED, 15 + 16, 8, Z_DEFAULT_STRATEGY, more);\n}\n");
    QTest::newRow("operator-wrap leaves a conditional expression")
        << QString(QLL("operator-wrap"))
        << QByteArray("void f()\n{\n    QString ftp = p.startsWith(QLatin1Char(\x27/\x27)) ? p : cwd + QLatin1Char(\x27/\x27) + p + more;\n}\n")
        << QByteArray("void f()\n{\n    QString ftp = p.startsWith(QLatin1Char(\x27/\x27)) ? p : cwd + QLatin1Char(\x27/\x27) + p + more;\n}\n");
    QTest::newRow("code-backticks prose semicolon")
        << QString(QLL("code-backticks"))
        << QByteArray("// Wraps run() first (it already wires both directions;\n"
                          "// the other path was missing this).\nint a;\n")
        << QByteArray("// Wraps `run()` first (it already wires both directions;\n"
                          "// the other path was missing this).\nint a;\n");
    QTest::newRow("code-backticks leaves code put out of use")
        << QString(QLL("code-backticks"))
        << QByteArray("// x = run();\n// y = stop();\nint a;\n")
        << QByteArray("// x = run();\n// y = stop();\nint a;\n");
    QTest::newRow("comment-period leaves an SPDX header")
        << QString(QLL("comment-period"))
        << QByteArray("// Copyright (C) 2015 The XD Company Ltd.\n// SPDX-License-Identifier: Apache-2.0\n\nint a;\n")
        << QByteArray("// Copyright (C) 2015 The XD Company Ltd.\n// SPDX-License-Identifier: Apache-2.0\n\nint a;\n");
    QTest::newRow("comment-case")
        << QString(QLL("comment-case"))
        << QByteArray("void f()\n{\n    // loads the file.\n    a();\n}\n")
        << QByteArray("void f()\n{\n    // Loads the file.\n    a();\n}\n");
    QTest::newRow("comment-period")
        << QString(QLL("comment-period"))
        << QByteArray("void f()\n{\n    // Loads the\n    // file\n    a();\n    /* Saves it */\n"
                          "    // x = y;\n}\n")
        << QByteArray("void f()\n{\n    // Loads the\n    // file.\n    a();\n    /* Saves it. */\n"
                          "    // x = y;\n}\n");
}

void tst_FormatRules::testFix_shouldRewrite()
{
    QFETCH(QString, rule);
    QFETCH(QByteArray, input);
    QFETCH(QByteArray, output);
    const FormatResult result = qMove(this->formatWith(rule, input));
    qExpect(result.error)->toBeEmpty();
    QCOMPARE(result.text, output);

    // Formats the result again, which changes nothing.
    const FormatResult again = qMove(this->formatWith(rule, result.text));
    QCOMPARE(again.text, output);
}

void tst_FormatRules::testReport_shouldFlagLines_data()
{
    QTest::addColumn<QString >("rule");
    QTest::addColumn<QByteArray >("input");
    QTest::addColumn<IssueLines >("lines");

    QTest::newRow("control-braces nested")
        << QString(QLL("control-braces"))
        << QByteArray("void f()\n{\n    if (x)\n        if (y)\n            a();\n}\n")
        << (IssueLines() << 3);
    QTest::newRow("super-typedef struct")
        << QString(QLL("super-typedef"))
        << QByteArray("struct Foo : Bar\n{\n    int a;\n};\nclass Baz : public Bar\n{\n"
                          "    typedef Bar super;\n};\n")
        << (IssueLines() << 2);
    QTest::newRow("no-lambda nested")
        << QString(QLL("no-lambda"))
        << QByteArray("void f()\n{\n    auto a = [=]() {\n        auto b = [=]() {\n"
                          "        };\n    };\n}\n")
        << (IssueLines() << 4);
    QTest::newRow("member-init")
        << QString(QLL("member-init"))
        << QByteArray("class A\n{\n    int a = 0;\n    static const int b = 1;\n"
                          "    void f() = delete;\n    virtual void g() = 0;\n};\n")
        << (IssueLines() << 3);
    QTest::newRow("global-static")
        << QString(QLL("global-static"))
        << QByteArray("void f()\n{\n    static QString s;\n    static int n;\n}\n"
                          "static QHash<int, int > cache;\nstatic QString helper(int x);\n")
        << (IssueLines() << 3 << 6);
    QTest::newRow("q-d-macro")
        << QString(QLL("q-d-macro"))
        << QByteArray("void A::f()\n{\n    d_func()->x = 1;\n}\n")
        << (IssueLines() << 3);
    QTest::newRow("excludes")
        << QString(QLL("excludes"))
        << QByteArray("void f()\n{\n    if (!flags.includes(A)) {\n    }\n}\n")
        << (IssueLines() << 3);
    QTest::newRow("code-shape")
        << QString(QLL("code-shape"))
        << QByteArray("void f()\n{\n    *buf.reserve(1) = c;\n    QUrl url(server->url(x));\n"
                          "    QMutexLocker locker(&mutex);\n}\n"
                          "class A\n{\n    inline void g()\n    {\n        a();\n        b();\n        c();\n"
                          "        d();\n        e();\n        f();\n    }\n};\n")
        << (IssueLines() << 3 << 4 << 10);
    QTest::newRow("const-overload-order")
        << QString(QLL("const-overload-order"))
        << QByteArray("class A\n{\n    T &value(int i);\n    const T &value(int i) const;\n"
                          "    const T &at(int i) const;\n    T &at(int i);\n};\n")
        << (IssueLines() << 3);
    QTest::newRow("qpointer-getter")
        << QString(QLL("qpointer-getter"))
        << QByteArray("class A\n{\n    QPointer<QObject > m_o;\npublic:\n"
                          "    QObject *o() const { return m_o.data(); }\n};\n")
        << (IssueLines() << 5);
    QTest::newRow("mark-banner empty comments")
        << QString(QLL("mark-banner"))
        << QByteArray("/**/\nint a; /**/\nint t[] = {\n    /**/ 0, 1,\n};\n/* */\n")
        << IssueLines();
    QTest::newRow("qll")
        << QString(QLL("qll"))
        << QByteArray("QString s = QLatin1String(\"a\") + QLatin1Literal(\"b\") + QLatin1String(p);\n")
        << (IssueLines() << 1 << 1);
    QTest::newRow("mark-banner bare rule")
        << QString(QLL("mark-banner"))
        << QByteArray("// ----------\nint a;\n")
        << (IssueLines() << 1);
    QTest::newRow("comment-case code name")
        << QString(QLL("comment-case"))
        << QByteArray("void f()\n{\n    // retryCount grows.\n    a();\n}\n")
        << (IssueLines() << 3);
    QTest::newRow("comment-case file pattern")
        << QString(QLL("comment-case"))
        << QByteArray("void f()\n{\n    // expires*.cgi answers fresh.\n    a();\n}\n")
        << (IssueLines() << 3);
    QTest::newRow("comment-period comma")
        << QString(QLL("comment-period"))
        << QByteArray("void f()\n{\n    // Ends with a comma,\n    a();\n}\n")
        << (IssueLines() << 3);
    QTest::newRow("comment-width")
        << QString(QLL("comment-width"))
        << QByteArray("// `ThisCodeSpanIsLongEnoughToRunWellPastTheEightiethColumnOfThisFileWithNoSpaceAndPastItsExtensionToo`.\n"
                          "// Short.\n")
        << (IssueLines() << 1);
    QTest::newRow("stub-line")
        << QString(QLL("stub-line"))
        << QByteArray("// This comment wraps at a place that\n// is too soon.\n")
        << (IssueLines() << 2);
    QTest::newRow("doc-placement plain block")
        << QString(QLL("doc-placement"))
        << QByteArray("// Runs it.\nvoid run();\n")
        << (IssueLines() << 1);
    QTest::newRow("doc-placement orphan blank line")
        << QString(QLL("doc-placement"))
        << QByteArray("/// Runs it.\n\nvoid run();\n")
        << (IssueLines() << 1);
    QTest::newRow("doc-placement orphan pair")
        << QString(QLL("doc-placement"))
        << QByteArray("/*! Runs it. */\n/*! Stops it. */\nvoid run();\n")
        << (IssueLines() << 1);
    QTest::newRow("doc-placement stores field")
        << QString(QLL("doc-placement"))
        << QByteArray("class A\n{\n    /// Stores the count.\n    int count;\n"
                          "    /// Returns the size.\n    int size() const;\n};\n")
        << (IssueLines() << 3 << 5);
    QTest::newRow("so-hence")
        << QString(QLL("so-hence"))
        << QByteArray("// It fails, so it retries.\n// So many tries.\n"
                          "// It leaves the port unbound so that the test skips.\n"
                          "// It waits, so that the reply lands.\nint a;\n")
        << (IssueLines() << 1);
    QTest::newRow("slash-and-or")
        << QString(QLL("slash-and-or"))
        << QByteArray("// Reads stdout/stderr.\n// Reads and/or writes `a/b`.\nint a;\n")
        << (IssueLines() << 1);
    QTest::newRow("will-may")
        << QString(QLL("will-may"))
        << QByteArray("// It will fail.\n// It may fail.\nint a;\n")
        << (IssueLines() << 1);
    QTest::newRow("mapping")
        << QString(QLL("mapping"))
        << QByteArray("// The key, i.e. the id.\n// However small it is.\nint a;\n")
        << (IssueLines() << 1 << 2);
    QTest::newRow("plain-words")
        << QString(QLL("plain-words"))
        << QByteArray("// The aforementioned key.\nint a;\n")
        << (IssueLines() << 1);
    QTest::newRow("module-name")
        << QString(QLL("module-name"))
        << QByteArray("// Compiled into Qt5Core.\n// Links Qt5Core.dll and QtCore.\nint a;\n")
        << (IssueLines() << 1);
    QTest::newRow("em-dash")
        << QString(QLL("em-dash"))
        << QByteArray("// A \xe2\x80\x94 B.\nconst char *s = \"x \xe2\x80\x93 y\";\nint a;\n")
        << (IssueLines() << 1 << 2);
    QTest::newRow("host-mention")
        << QString(QLL("host-mention"))
        << QByteArray("// Slow on a single-core box.\nint a;\n")
        << (IssueLines() << 1);
    QTest::newRow("agents-md")
        << QString(QLL("agents-md"))
        << QByteArray("// See AGENTS.md for this.\nint a;\n")
        << (IssueLines() << 1);
}

void tst_FormatRules::testReport_shouldFlagLines()
{
    QFETCH(QString, rule);
    QFETCH(QByteArray, input);
    QFETCH(IssueLines, lines);
    const FormatResult result = qMove(this->formatWith(rule, input));
    qExpect(result.error)->toBeEmpty();
    QCOMPARE(this->issueLines(result, rule), lines);
}

void tst_FormatRules::testRelativeInclude_shouldPinSiblingHeader()
{
    QTemporaryDir dir;
    qExpect(dir.isValid())->toBeTruthy();
    QFile header(dir.path() + QLL("/sibling.h"));
    qExpect(header.open(QIODevice::WriteOnly))->toBeTruthy();
    header.close();
    const QByteArray input = "#include \"sibling.h\"\n#include \"./sibling.h\"\n"
                             "#include \"missing.h\"\n#include <QtCore/QList>\n";
    const FormatResult result = qMove(this->formatWith(QString(QLL("relative-include")), input,
                                                       dir.path() + QLL("/a.cpp")));
    QCOMPARE(result.text, QByteArray("#include \"./sibling.h\"\n#include \"./sibling.h\"\n"
                                     "#include \"missing.h\"\n#include <QtCore/QList>\n"));
}

void tst_FormatRules::testDocPlacement_shouldMoveHeaderDocsToSource()
{
    QTemporaryDir dir;
    qExpect(dir.isValid())->toBeTruthy();
    QFile source(dir.path() + QLL("/widget.cpp"));
    qExpect(source.open(QIODevice::WriteOnly))->toBeTruthy();
    source.close();
    const QByteArray input = "class Widget\n{\npublic:\n    /// Runs it.\n    void run();\n"
                             "    /// Stops it.\n    inline void stop() {}\n};\n";
    const FormatResult result = qMove(this->formatWith(QString(QLL("doc-placement")), input,
                                                       dir.path() + QLL("/widget_p.h")));
    QCOMPARE(this->issueLines(result, QString(QLL("doc-placement"))), IssueLines() << 4);
    qExpect(result.issues.at(0).message)->toContain(QString(QLL("widget.cpp")));
}

void tst_FormatRules::testNoLambda_shouldFlagEveryLambdaInXdTree()
{
    QTemporaryDir dir;
    qExpect(dir.isValid())->toBeTruthy();
    QFile marker(dir.path() + QLL("/XD-mini.pro"));
    qExpect(marker.open(QIODevice::WriteOnly))->toBeTruthy();
    marker.close();
    const QByteArray input = "void f()\n{\n    connect(a, &A::b, [this]() {\n    });\n"
                             "    x = a[1];\n}\n";
    const FormatResult inXd = qMove(this->formatWith(QString(QLL("no-lambda")), input,
                                                     dir.path() + QLL("/src/a.cpp")));
    QCOMPARE(this->issueLines(inXd, QString(QLL("no-lambda"))), IssueLines() << 3);
    const FormatResult outside = qMove(this->formatWith(QString(QLL("no-lambda")), input));
    QCOMPARE(this->issueLines(outside, QString(QLL("no-lambda"))), IssueLines());
}

void tst_FormatRules::testAllRules_shouldKeepCleanCode()
{
    const QByteArray input =
            "#include \"./widget.h\"\n"
            "\n"
            "#include <QtCore/QList>\n"
            "\n"
            "/*!\n"
            "    \\class Widget\n"
            "    \\internal\n"
            "\n"
            "    Shows a list of names, and counts how often each one changed.\n"
            "*/\n"
            "\n"
            "/*!\n"
            "    Builds the widget that shows \\a names.\n"
            "*/\n"
            "Widget::Widget(const QList<QString > & names)\n"
            "    : m_names(names), m_changes(0)\n"
            "{\n"
            "}\n"
            "\n"
            "/*!\n"
            "    Tells whether \\a name is shown and changed at least once.\n"
            "*/\n"
            "bool Widget::hasChanged(const QString & name) const\n"
            "{\n"
            "    // Checks the shown names first; hence a hidden name never counts.\n"
            "    if ( ! this->m_names.contains(name)\n"
            "        || this->m_changes == 0\n"
            "    ) {\n"
            "        return false;\n"
            "    }\n"
            "    for (int i = 0; i < this->m_names.size(); ++i) {\n"
            "        if (this->m_names.at(i) == name) { return true; }\n"
            "    }\n"
            "    return false;\n"
            "}\n";
    const FormatResult result = qMove(this->formatWith(QString(), input));
    qExpect(result.error)->toBeEmpty();
    QCOMPARE(result.text, input);
    QStringList issues;
    for (int i = 0; i < result.issues.size(); ++i) {
        issues << QString(QLL("%1: %2: %3")).arg(result.issues.at(i).line)
                  .arg(QString::fromLatin1(result.issues.at(i).rule), result.issues.at(i).message);
    }
    QCOMPARE(issues, QStringList());
}

/// Runs every rule over comments with no content, in each place a comment
/// may sit; a rule that reads a comment's first line must not fail on them.
void tst_FormatRules::testAllRules_shouldSurviveEmptyComments()
{
    const QByteArray input = "/**/\n//\n///\n/*!*/\n/* */\n"
                             "class A : public B\n{\n    /**/\n    int a; /**/\n    int b; //\n};\n"
                             "int t[] = {\n    /**/ 0, 1,\n    /**/ 2,\n};\n"
                             "void f()\n{\n    g(/**/ 1);\n    //\n}\n";
    const FormatResult result = qMove(this->formatWith(QString(), input));
    qExpect(result.error)->toBeEmpty();
    const FormatResult again = qMove(this->formatWith(QString(), result.text));
    QCOMPARE(again.text, result.text);
}

/// Builds a project folder for the settings tests: a `.git` folder that
/// makes `dir` the root, and the `.formatrc` texts of `files` (a path relative
/// to `dir`, then its text).
static void writeProject(const QTemporaryDir & dir, const QStringList & files)
{
    QDir().mkpath(dir.path() + QLL("/.git"));
    for (int i = 0; i + 1 < files.size(); i += 2) {
        const QString path = dir.path() + QLatin1Char('/') + files.at(i);
        QDir().mkpath(QFileInfo(path).absolutePath());
        QFile file(path);
        if (file.open(QIODevice::WriteOnly)) {
            file.write(files.at(i + 1).toUtf8());
        }
    }
}

void tst_FormatRules::testSettings_shouldReadFromRootDown()
{
    QTemporaryDir dir;
    writeProject(dir, QStringList() << QString(QLL(".formatrc"))
        << QString(QLL("[Types]\nrefSuffix = \"\"\nmoveSuffix = \" \"\n"))
        << QString(QLL("sub/.formatrc")) << QString(QLL("[Types]\nrefSuffix = keep-as-is\n")));
    const QString file = dir.path() + QLL("/sub/a.cpp");
    FormatSettings settings;
    QString error;
    qExpect(FormatSettings::load(file, FormatConfig(), &settings, &error))->toBeTruthy();
    qExpect(settings.refSuffix.keepAsIs)->toBeTruthy();
    qExpect(settings.moveSuffix.keepAsIs)->toBeFalsy();
    qExpect(settings.moveSuffix.text)->toEqual(QByteArray(" "));
    FormatSettings top;
    qExpect(FormatSettings::load(dir.path() + QLL("/a.cpp"), FormatConfig(), &top, &error))
        ->toBeTruthy();
    qExpect(top.refSuffix.keepAsIs)->toBeFalsy();
    qExpect(top.refSuffix.text)->toBeEmpty();
}

void tst_FormatRules::testSettings_shouldRejectUnknownValue()
{
    QTemporaryDir dir;
    writeProject(dir, QStringList() << QString(QLL(".formatrc"))
        << QString(QLL("[Types]\nrefSuffix = tab\n")));
    const FormatResult result = qMove(this->formatWith(QString(QLL("reference-space")),
                                                       "int a;\n", dir.path() + QLL("/a.cpp")));
    qExpect(result.error)->toContain(QString(QLL(".formatrc: [Types] refSuffix = tab: use")));
}

void tst_FormatRules::testReferenceSpace_shouldFollowSettings_data()
{
    QTest::addColumn<QString >("formatrc");
    QTest::addColumn<QByteArray >("output");
    QTest::newRow("defaults") << QString()
        << QByteArray("void f(const QString & a, QString & b, QString & c, T &&d, T && e);\n");
    QTest::newRow("none and space")
        << QString(QLL("[Types]\nrefSuffix = \"\"\nmoveSuffix = \" \"\n"))
        << QByteArray("void f(const QString &a, QString &b, QString &c, T && d, T && e);\n");
    QTest::newRow("keep and none")
        << QString(QLL("[Types]\nrefSuffix = keep-as-is\nmoveSuffix = \"\"\n"))
        << QByteArray("void f(const QString &a, QString & b, QString & c, T &&d, T &&e);\n");
    QTest::newRow("tab and two spaces")
        << QString(QLL("[Types]\nrefSuffix = \"\\t\"\nmoveSuffix = \"  \"\n"))
        << QByteArray("void f(const QString &\ta, QString &\tb, QString &\tc, "
                      "T &&  d, T &&  e);\n");
    QTest::newRow("none alias") << QString(QLL("[Types]\nrefSuffix = none\n"))
        << QByteArray("void f(const QString &a, QString &b, QString &c, T &&d, T && e);\n");
    QTest::newRow("regexp that matches")
        << QString(QLL("[Types]\nrefSuffix = regexp/QString& b/QString &b/ | keep-as-is\n"))
        << QByteArray("void f(const QString &a, QString &b, QString & c, T &&d, T && e);\n");
    QTest::newRow("regexp with commas and spaces after them")
        << QString(QLL("[Types]\nrefSuffix = regexp/QString& b, /QString &b, / | keep-as-is\n"))
        << QByteArray("void f(const QString &a, QString &b, QString & c, T &&d, T && e);\n");
    QTest::newRow("regexp that matches nothing, keeping")
        << QString(QLL("[Types]\nrefSuffix = regexp/nothing/x/ | keep-as-is\n"))
        << QByteArray("void f(const QString &a, QString & b, QString & c, T &&d, T && e);\n");
    QTest::newRow("regexp whose result it no longer matches")
        << QString(QLL("[Types]\nrefSuffix = regexp/ & /&/\n"))
        << QByteArray("void f(const QString &a, QString& b, QString&c, T &&d, T && e);\n");
    QTest::newRow("regexp that matches nothing, default")
        << QString(QLL("[Types]\nrefSuffix = regexp/nothing/x/\n"))
        << QByteArray("void f(const QString & a, QString & b, QString & c, T &&d, T && e);\n");
}

void tst_FormatRules::testReferenceSpace_shouldFollowSettings()
{
    QFETCH(QString, formatrc);
    QFETCH(QByteArray, output);
    QTemporaryDir dir;
    QStringList files;
    if (!formatrc.isEmpty()) {
        files << QString(QLL(".formatrc")) << formatrc;
    }
    writeProject(dir, files);
    const QByteArray input = "void f(const QString &a, QString& b, QString & c, T&&d, T && e);\n";
    const FormatResult result = qMove(this->formatWith(QString(QLL("reference-space")), input,
                                                       dir.path() + QLL("/a.cpp")));
    qExpect(result.error)->toBeEmpty();
    QCOMPARE(result.text, output);
}

void tst_FormatRules::testCommentWidth_shouldWrap_data()
{
    QTest::addColumn<QString >("formatrc");
    QTest::addColumn<QByteArray >("input");
    QTest::addColumn<QByteArray >("output");
    QTest::newRow("punctuation nearest the column") << QString(QLL(""))
        << QByteArray("    // acknowledges the change (see `writeLastPull`), so a short interval keeps that wait brief.\n")
        << QByteArray("    // acknowledges the change (see `writeLastPull`),\n    // so a short interval keeps that wait brief.\n");
    QTest::newRow("plain space") << QString(QLL(""))
        << QByteArray("// The tool waits for the server to answer each request in turn before it sends the next one.\n")
        << QByteArray("// The tool waits for the server to answer each request in turn before it\n// sends the next one.\n");
    QTest::newRow("second run") << QString(QLL("[Comments]\nwrapBegin = 75 | 51\n"))
        << QByteArray("// The tool waits for the server to answer each request in turn before it sends the next one.\n")
        << QByteArray("// The tool waits for the server to answer each\n// request in turn before it sends the next one.\n");
    QTest::newRow("whole paragraph") << QString(QLL(""))
        << QByteArray("// Short first line,\n// The tool waits for the server to answer each request in turn before it sends the next one.\n")
        << QByteArray("// Short first line, The tool waits for the server to answer each request\n// in turn before it sends the next one.\n");
    QTest::newRow("fitting paragraph kept") << QString(QLL(""))
        << QByteArray("// Fits.\n//\n// The tool waits for the server to answer each request in turn before it sends the next one.\n")
        << QByteArray("// Fits.\n//\n// The tool waits for the server to answer each request in turn before it\n// sends the next one.\n");
    QTest::newRow("list item") << QString(QLL(""))
        << QByteArray("// * The tool waits for the server to answer each request in turn before it sends the next one.\n")
        << QByteArray("// * The tool waits for the server to answer each request in turn before it\n//   sends the next one.\n");
    QTest::newRow("no break in code") << QString(QLL(""))
        << QByteArray("// Starts the `QNetworkAccessManager::createRequest(op, request, data)` call with a really long code span inside it.\n")
        << QByteArray("// Starts the `QNetworkAccessManager::createRequest(op, request, data)`\n// call with a really long code span inside it.\n");
    QTest::newRow("doc block") << QString(QLL(""))
        << QByteArray("/*!\n    The tool waits for the server to answer each request in turn before it sends the next one.\n*/\nvoid f();\n")
        << QByteArray("/*!\n    The tool waits for the server to answer each request in turn before it\n    sends the next one.\n*/\nvoid f();\n");
    QTest::newRow("no stub last line") << QString()
        << QByteArray("    /// Reads the sections of `device` into `c`, and its `[Routes/<name>]` sections "
                      "into `routes`; `dir` resolves the relative paths of [Unreachable].\n")
        << QByteArray("    /// Reads the sections of `device` into `c`, and its `[Routes/<name>]`\n"
                      "    /// sections into `routes`; `dir` resolves the relative\n"
                      "    /// paths of [Unreachable].\n");
    QTest::newRow("separator lines kept") << QString()
        << QByteArray("// ---------------------------------------------------------------------------\n// ImapService: an IMAP-ish greeter. Enough for the socket-engine tests, which only\n"
                      "// drive the greeting + NOOP + LOGOUT and echo the client's command tag.\n// ---------------------------------------------------------------------------\n")
        << QByteArray("// ---------------------------------------------------------------------------\n// ImapService: an IMAP-ish greeter. Enough for the socket-engine tests,\n"
                      "// which only drive the greeting + NOOP + LOGOUT and echo the\n"
                      "// client's command tag.\n// ---------------------------------------------------------------------------\n");
    QTest::newRow("keyword past the limit, hyphenated word") << QString()
        << QByteArray(
                      "    /// [TLS] fixedSessionTicketKey: when true, the TLS listener installs a FIXED RFC-5077 session-\n"
                      "    /// ticket key (via QSslConfiguration::setSessionTicketKey) so the tickets it issues resume across separate client sockets. This deliberately defeats ticket-key rotation/forward-secrecy, so it is OFF by default (each connection keeps OpenSSL's random per-context key) and a test that needs observable session resumption turns it on for its own duration via the TestServerEditor.\n")
        << QByteArray(
                      "    /// [TLS] fixedSessionTicketKey: when true, the TLS listener installs a\n"
                      "    /// FIXED RFC-5077 session-ticket key (via QSslConfiguration::setSessionTicketKey)\n"
                      "    /// so the tickets it issues resume across separate client sockets.\n"
                      "    /// This deliberately defeats ticket-key rotation/forward-secrecy,\n"
                      "    /// so it is OFF by default (each connection keeps OpenSSL's random\n"
                      "    /// per-context key) and a test that needs observable session\n"
                      "    /// resumption turns it on for its own duration via the TestServerEditor.\n");
    QTest::newRow("custom limit") << QString(QLL("[Comments]\nwrapMax = 60\n"))
        << QByteArray("// The tool waits for the server to answer each request in turn before it sends the next one.\n")
        << QByteArray("// The tool waits for the server to answer each request in\n// turn before it sends the next one.\n");
    QTest::newRow("keyword past 110 columns") << QString()
        << QByteArray("// Calls `ThisKeywordIsSoLongThatItPassesTheLimitAndItsExtensionThisKeywordIsSoLongThatItPassesTheLimitAndItsExtension()` once.\n")
        << QByteArray("// Calls\n// `ThisKeywordIsSoLongThatItPassesTheLimitAndItsExtensionThisKeywordIsSoLongThatItPassesTheLimitAndItsExtension()`\n// once.\n");
    QTest::newRow("comma after a word, plain break up to the column")
        << QString(QLL("[Comments]\ntarget = any\n"))
        << QByteArray(
                      "        // The one multi-SAN cert (TestServer::serverCert(), runtime-generated)\n"
                      "        // covers both. A name with three addresses,\n"
                      "        // of which only the last answers; the first two are loopback addresses\n"
                      "        // where server-dummy drops each connection attempt.\n")
        << QByteArray(
                      "        // The one multi-SAN cert (TestServer::serverCert(), runtime-generated)\n"
                      "        // covers both. A name with three addresses, of which only the\n"
                      "        // last answers; the first two are loopback addresses where\n"
                      "        // server-dummy drops each connection attempt.\n");
    QTest::newRow("comma after a word, hard")
        << QString(QLL("[Comments]\nwrapComma = hard\ntarget = any\n"))
        << QByteArray(
                      "        // The one multi-SAN cert (TestServer::serverCert(), runtime-generated)\n"
                      "        // covers both. A name with three addresses,\n"
                      "        // of which only the last answers; the first two are loopback addresses\n"
                      "        // where server-dummy drops each connection attempt.\n")
        << QByteArray(
                      "        // The one multi-SAN cert (TestServer::serverCert(), runtime-generated)\n"
                      "        // covers both. A name with three addresses,\n"
                      "        // of which only the last answers; the first two are loopback\n"
                      "        // addresses where server-dummy drops each connection attempt.\n");
    QTest::newRow("short line takes a long next word")
        << QString(QLL("[Comments]\ntarget = any\n"))
        << QByteArray("    // which fall through to a permissive 200) flow on to the routes below.\n"
                      "    // Drives\n"
                      "    // tst_QNetworkReply::sendCustomRequestToHttp(\"connect\"/\"nonsense\").\n")
        << QByteArray("    // which fall through to a permissive 200) flow on to the routes below.\n"
                      "    // Drives tst_QNetworkReply::sendCustomRequestToHttp(\"connect\"/\"nonsense\").\n");
    QTest::newRow("no break that leaves a stub, up to the limit")
        << QString(QLL("[Comments]\ntarget = any\n"))
        << QByteArray("/*!\n"
                      "    A paragraph also wraps again when one of its lines ends\n"
                      "    `wrapEmptyLimit` columns or more short of the wrap column while the\n"
                      "    first word of the next line would still fit on it (a code keyword within\n"
                      "    the extension), since such a break wastes the room.\n"
                      "*/\n")
        << QByteArray("/*!\n"
                      "    A paragraph also wraps again when one of its lines ends `wrapEmptyLimit`\n"
                      "    columns or more short of the wrap column while the first word of the\n"
                      "    next line would still fit on it (a code keyword within the extension),\n"
                      "    since such a break wastes the room.\n"
                      "*/\n");
    QTest::newRow("short lines kept, target long") << QString()
        << QByteArray("// The tool waits for the server to answer\n// each request in turn.\n")
        << QByteArray("// The tool waits for the server to answer\n// each request in turn.\n");
    QTest::newRow("short lines joined, target any") << QString(QLL("[Comments]\ntarget = any\n"))
        << QByteArray("// The tool waits for the server to answer\n// each request in turn.\n")
        << QByteArray("// The tool waits for the server to answer each request in turn.\n");
    QTest::newRow("trailing comment kept") << QString(QLL(""))
        << QByteArray("int a; // The tool waits for the server to answer each request in turn before it sends the next one.\n")
        << QByteArray("int a; // The tool waits for the server to answer each request in turn before it sends the next one.\n");
}

/// Wraps with the `.formatrc` of the row, and wraps the result again,
/// which changes nothing.
void tst_FormatRules::testCommentWidth_shouldWrap()
{
    QFETCH(QString, formatrc);
    QFETCH(QByteArray, input);
    QFETCH(QByteArray, output);
    QTemporaryDir dir;
    QStringList files;
    if (!formatrc.isEmpty()) {
        files << QString(QLL(".formatrc")) << formatrc;
    }
    writeProject(dir, files);
    const QString path = dir.path() + QLL("/a.cpp");
    const FormatResult result = qMove(this->formatWith(QString(QLL("comment-width")), input,
                                                       path));
    qExpect(result.error)->toBeEmpty();
    QCOMPARE(result.text, output);
    const FormatResult again = qMove(this->formatWith(QString(QLL("comment-width")),
                                                      result.text, path));
    QCOMPARE(again.text, output);
}

void tst_FormatRules::testSettings_shouldReadWrapColumns()
{
    QTemporaryDir dir;
    writeProject(dir, QStringList() << QString(QLL(".formatrc"))
        << QString(QLL("[Comments]\nwrapBegin = 75|51\nwrapMax = 90\nwrapExtendsForKeywords = 10\n"
                       "wrapEmptyLimit = 4\nwrapComma = HARD\ntarget = ANY\n")));
    FormatSettings settings;
    QString error;
    qExpect(FormatSettings::load(dir.path() + QLL("/a.cpp"), FormatConfig(), &settings, &error))
        ->toBeTruthy();
    QCOMPARE(settings.commentWrapBegin, QList<int >() << 75 << 51);
    qExpect(settings.commentWrapMax)->toEqual(90);
    QCOMPARE(settings.wrapBegin, QList<int >() << 75);
    qExpect(settings.wrapMax)->toEqual(80);
    qExpect(settings.wrapExtendsForKeywords)->toEqual(10);
    qExpect(settings.wrapEmptyLimit)->toEqual(4);
    qExpect(int(settings.wrapComma))->toEqual(int(FormatSettings::WrapCommaHard));
    qExpect(int(settings.commentTarget))->toEqual(int(FormatSettings::CommentTargetAny));
    const FormatSettings defaults;
    qExpect(defaults.wrapBegin)->toEqual(QList<int >() << 75);
    qExpect(defaults.wrapMax)->toEqual(80);
    qExpect(defaults.commentWrapBegin)->toEqual(QList<int >() << 75);
    qExpect(defaults.commentWrapMax)->toEqual(80);
    qExpect(defaults.wrapExtendsForKeywords)->toEqual(20);
    qExpect(defaults.wrapEmptyLimit)->toEqual(8);
    qExpect(int(defaults.wrapComma))->toEqual(int(FormatSettings::WrapCommaSoft));
    qExpect(int(defaults.commentTarget))->toEqual(int(FormatSettings::CommentTargetLong));
    writeProject(dir, QStringList() << QString(QLL(".formatrc"))
        << QString(QLL("[Comments]\nwrapMax = 70 | 80\n")));
    qExpect(FormatSettings::load(dir.path() + QLL("/a.cpp"), FormatConfig(), &settings, &error))
        ->toBeFalsy();
    qExpect(error)->toContain(QString(QLL("[Comments] wrapMax = 70 | 80: use a number from 20 to 200")));
}

/// Breaks a long `<<` chain in agent mode, and asks the agent to regroup the
/// two lines it made, which the request names as one range.
void tst_FormatRules::testAgentMode_shouldAskForOperatorGroups()
{
    Formatter formatter;
    QString error;
    qExpect(formatter.selectRules(QStringList() << QString(QLL("operator-wrap")), &error))
        ->toBeTruthy();
    const QByteArray input = "void f()\n{\n    out << \"label\" << firstValue << secondValue << thirdValue"
                             " << fourthValue << fifthValue << sixthValue;\n}\n";
    const FormatResult result = qMove(formatter.format(input, QString(), QStringList(),
                                                       FormatConfig(), true));
    qExpect(result.changed)->toBeTruthy();
    qExpect(result.requests.size())->toEqual(1);
    qExpect(result.requests.at(0).rule)->toEqual(QByteArray("operator-wrap"));
    qExpect(result.requests.at(0).line)->toEqual(3);
    qExpect(result.requests.at(0).endLine)->toEqual(4);
    qExpect(result.requests.at(0).message)->toContain(QString(QLL("regroup its lines")));
    qExpect(result.requests.at(0).columnLimit)->toEqual(80);
}

/// Asks the agent about an access chain of a doc-comment in agent mode,
/// instead of wrapping it in backticks, and leaves the comment as it is.
void tst_FormatRules::testAgentMode_shouldAskForDocAccessChains()
{
    Formatter formatter;
    QString error;
    qExpect(formatter.selectRules(QStringList() << QString(QLL("code-backticks")), &error))
        ->toBeTruthy();
    const QByteArray input = "/// Reads it through watcher()->load(), as run() does.\nvoid f();\n";
    const FormatResult result = qMove(formatter.format(input, QString(), QStringList(),
                                                       FormatConfig(), true));
    QCOMPARE(result.text, input);
    qExpect(result.requests.size())->toEqual(1);
    qExpect(result.requests.at(0).line)->toEqual(1);
    qExpect(result.requests.at(0).message)->toContain(QString(QLL("`watcher()->load()`")));
}

void tst_FormatRules::testAgentMode_shouldAskNothingWhenOff()
{
    Formatter formatter;
    QString error;
    qExpect(formatter.selectRules(QStringList() << QString(QLL("operator-wrap"))
                                  << QString(QLL("code-backticks")), &error))->toBeTruthy();
    const QByteArray input = "/// Reads it through watcher()->load().\nvoid f()\n{\n"
                             "    out << \"label\" << firstValue << secondValue << thirdValue"
                             " << fourthValue << fifthValue << sixthValue;\n}\n";
    const FormatResult result = qMove(formatter.format(input, QString(), QStringList()));
    qExpect(result.changed)->toBeTruthy();
    qExpect(result.requests.size())->toEqual(0);
}

QTEST_APPLESS_MAIN(tst_FormatRules)
#include "tst_format_rules.moc"
