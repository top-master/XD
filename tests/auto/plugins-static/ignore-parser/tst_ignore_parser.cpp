// Copyright (C) 2015 The XD Company Ltd.
// SPDX-License-Identifier: Apache-2.0

#include "ignore-parser.h"

#include <QtCore/QDir>
#include <QtCore/QFile>
#include <QtCore/QFileInfo>
#include <QtCore/QTemporaryDir>
#include <QtTest/QtTest>

/// Tests the ignore-file parser: git's pattern forms, the order in which
/// the ignore files of nested folders and of several names apply, the
/// files above the project root, and the paths named directly.
class tst_IgnoreParser : public QObject
{
    Q_OBJECT
    typedef QObject super;
private slots:
    void init();

    /// Patterns.
    void testParseRule_shouldFollowGitPatterns_data();
    void testParseRule_shouldFollowGitPatterns();
    void testParseRule_shouldSkipBlankAndComment();

    /// Ignore files.
    void testIgnoreFileNames_shouldStartWithGitignore();
    void testIsIgnored_shouldLetDeeperFoldersOverride();
    void testIsIgnored_shouldLetLaterFileNamesOverride();
    void testIsIgnored_shouldLeaveOutGitFolder();
    void testIsIgnored_shouldLeaveOutContentOfIgnoredFolder();
    void testIsIgnored_shouldReadNothingAboveRoot();

    /// Paths named directly.
    void testAddExplicitPath_shouldKeepNamedPath();
    void testFindFiles_shouldListKeptFilesOnly();

private:
    void write(const QString & relative, const QByteArray & text = QByteArray()) const;
    QString path(const QString & relative) const;

    /// Gives each test a fresh project folder, with a `.git` folder that
    /// makes it the root.
    QScopedPointer<QTemporaryDir > m_dir;
};

/// Writes `text` into the file `relative` under the test's folder,
/// creating its folders.
void tst_IgnoreParser::write(const QString & relative, const QByteArray & text) const
{
    const QString file = this->path(relative);
    QDir().mkpath(QFileInfo(file).absolutePath());
    QFile output(file);
    if (output.open(QIODevice::WriteOnly)) {
        output.write(text);
    }
}

/// Builds the absolute path of `relative` under the test's folder.
QString tst_IgnoreParser::path(const QString & relative) const
{
    return QDir::cleanPath(this->m_dir->path() + QLatin1Char('/') + relative);
}

void tst_IgnoreParser::init()
{
    this->m_dir.reset(new QTemporaryDir);
    qExpect(this->m_dir->isValid())->toBeTruthy();
    QDir().mkpath(this->path(QString(QLL(".git"))));
}

void tst_IgnoreParser::testParseRule_shouldFollowGitPatterns_data()
{
    QTest::addColumn<QString >("pattern");
    QTest::addColumn<QString >("relative");
    QTest::addColumn<bool >("isDirectory");
    QTest::addColumn<bool >("matches");
    QTest::newRow("name at any depth") << QString(QLL("*.o")) << QString(QLL("a/b/x.o")) << false << true;
    QTest::newRow("star stops at slash") << QString(QLL("a*")) << QString(QLL("ab/c")) << false << false;
    QTest::newRow("anchored by leading slash") << QString(QLL("/build")) << QString(QLL("x/build")) << true << false;
    QTest::newRow("anchored at root") << QString(QLL("/build")) << QString(QLL("build")) << true << true;
    QTest::newRow("anchored by middle slash") << QString(QLL("doc/out")) << QString(QLL("x/doc/out")) << true << false;
    QTest::newRow("folder only, on a file") << QString(QLL("tmp/")) << QString(QLL("a/tmp")) << false << false;
    QTest::newRow("folder only, on a folder") << QString(QLL("tmp/")) << QString(QLL("a/tmp")) << true << true;
    QTest::newRow("leading double star") << QString(QLL("**/cache")) << QString(QLL("a/b/cache")) << true << true;
    QTest::newRow("leading double star at root") << QString(QLL("**/cache")) << QString(QLL("cache")) << true << true;
    QTest::newRow("trailing double star") << QString(QLL("gen/**")) << QString(QLL("gen/a/b.cpp")) << false << true;
    QTest::newRow("middle double star") << QString(QLL("a/**/z")) << QString(QLL("a/z")) << false << true;
    QTest::newRow("middle double star deep") << QString(QLL("a/**/z")) << QString(QLL("a/b/c/z")) << false << true;
    QTest::newRow("question mark") << QString(QLL("?.h")) << QString(QLL("a.h")) << false << true;
    QTest::newRow("character class") << QString(QLL("[ab].h")) << QString(QLL("c.h")) << false << false;
    QTest::newRow("negated class") << QString(QLL("[!ab].h")) << QString(QLL("c.h")) << false << true;
    QTest::newRow("escaped hash") << QString(QLL("\\#tmp")) << QString(QLL("#tmp")) << false << true;
    QTest::newRow("escaped bang") << QString(QLL("\\!x")) << QString(QLL("!x")) << false << true;
}

void tst_IgnoreParser::testParseRule_shouldFollowGitPatterns()
{
    QFETCH(QString, pattern);
    QFETCH(QString, relative);
    QFETCH(bool, isDirectory);
    QFETCH(bool, matches);
    IgnoreRule rule;
    qExpect(IgnoreParser::parseRule(pattern, QString(), &rule))->toBeTruthy();
    const bool matched = (!rule.directoryOnly || isDirectory) && rule.pattern.match(relative).hasMatch();
    qExpect(matched)->toEqual(matches);
}

void tst_IgnoreParser::testParseRule_shouldSkipBlankAndComment()
{
    IgnoreRule rule;
    qExpect(IgnoreParser::parseRule(QString(), QString(), &rule))->toBeFalsy();
    qExpect(IgnoreParser::parseRule(QString(QLL("   ")), QString(), &rule))->toBeFalsy();
    qExpect(IgnoreParser::parseRule(QString(QLL("# note")), QString(), &rule))->toBeFalsy();
    qExpect(IgnoreParser::parseRule(QString(QLL("!keep  ")), QString(), &rule))->toBeTruthy();
    qExpect(rule.negated)->toBeTruthy();
    qExpect(rule.pattern.match(QString(QLL("keep"))).hasMatch())->toBeTruthy();
}

void tst_IgnoreParser::testIgnoreFileNames_shouldStartWithGitignore()
{
    IgnoreParser parser;
    qExpect(parser.ignoreFileNames().size())->toEqual(1);
    qExpect(parser.ignoreFileNames().at(0))->toEqualString(QLL(".gitignore"));
    qExpect(parser.ignoreFileNames().capacity())->toEqual(5);
    parser.addIgnoreFileName(QString(QLL(".formatignore")));
    qExpect(parser.ignoreFileNames().size())->toEqual(2);
    qExpect(parser.ignoreFileNames().at(1))->toEqualString(QLL(".formatignore"));
}

void tst_IgnoreParser::testIsIgnored_shouldLetDeeperFoldersOverride()
{
    this->write(QString(QLL(".gitignore")), "*.log\n");
    this->write(QString(QLL("sub/.gitignore")), "!keep.log\n");
    IgnoreParser parser;
    qExpect(parser.isIgnored(this->path(QString(QLL("keep.log"))), false))->toBeTruthy();
    qExpect(parser.isIgnored(this->path(QString(QLL("sub/keep.log"))), false))->toBeFalsy();
    qExpect(parser.isIgnored(this->path(QString(QLL("sub/x.log"))), false))->toBeTruthy();
    qExpect(parser.isIgnored(this->path(QString(QLL("sub/x.cpp"))), false))->toBeFalsy();
}

void tst_IgnoreParser::testIsIgnored_shouldLetLaterFileNamesOverride()
{
    this->write(QString(QLL(".gitignore")), "/shared\n*.tmp\n");
    this->write(QString(QLL(".formatignore")), "!/shared\n");
    IgnoreParser gitOnly;
    qExpect(gitOnly.isIgnored(this->path(QString(QLL("shared"))), true))->toBeTruthy();
    IgnoreParser parser;
    parser.addIgnoreFileName(QString(QLL(".formatignore")));
    qExpect(parser.isIgnored(this->path(QString(QLL("shared"))), true))->toBeFalsy();
    qExpect(parser.isIgnored(this->path(QString(QLL("shared/a.cpp"))), false))->toBeFalsy();
    qExpect(parser.isIgnored(this->path(QString(QLL("shared/a.tmp"))), false))->toBeTruthy();
}

void tst_IgnoreParser::testIsIgnored_shouldLeaveOutGitFolder()
{
    IgnoreParser parser;
    qExpect(parser.isIgnored(this->path(QString(QLL(".git"))), true))->toBeTruthy();
    qExpect(parser.isIgnored(this->path(QString(QLL(".git/hooks/x.cpp"))), false))->toBeTruthy();
    qExpect(parser.isIgnored(this->path(QString(QLL(".github/x.cpp"))), false))->toBeFalsy();
}

void tst_IgnoreParser::testIsIgnored_shouldLeaveOutContentOfIgnoredFolder()
{
    this->write(QString(QLL(".gitignore")), "build/\n!build/keep.cpp\n");
    IgnoreParser parser;
    qExpect(parser.isIgnored(this->path(QString(QLL("build/keep.cpp"))), false))->toBeTruthy();
    qExpect(parser.isIgnored(this->path(QString(QLL("src/build/x.cpp"))), false))->toBeTruthy();
}

void tst_IgnoreParser::testIsIgnored_shouldReadNothingAboveRoot()
{
    this->write(QString(QLL(".gitignore")), "*.cpp\n");
    this->write(QString(QLL("inner/.git")), "gitdir: elsewhere\n");
    IgnoreParser parser;
    qExpect(parser.isIgnored(this->path(QString(QLL("inner/a.cpp"))), false))->toBeFalsy();
    qExpect(parser.isIgnored(this->path(QString(QLL("a.cpp"))), false))->toBeTruthy();
}

void tst_IgnoreParser::testAddExplicitPath_shouldKeepNamedPath()
{
    this->write(QString(QLL(".gitignore")), "gen/\n*.o.cpp\n");
    IgnoreParser parser;
    parser.addExplicitPath(this->path(QString(QLL("gen/deep/a.cpp"))));
    parser.addExplicitPath(this->path(QString(QLL("b.o.cpp"))));
    qExpect(parser.isIgnored(this->path(QString(QLL("gen/deep/a.cpp"))), false))->toBeFalsy();
    qExpect(parser.isIgnored(this->path(QString(QLL("gen"))), true))->toBeFalsy();
    qExpect(parser.isIgnored(this->path(QString(QLL("b.o.cpp"))), false))->toBeFalsy();
    qExpect(parser.isIgnored(this->path(QString(QLL("gen/other.cpp"))), false))->toBeTruthy();
    qExpect(parser.isIgnored(this->path(QString(QLL("c.o.cpp"))), false))->toBeTruthy();
}

void tst_IgnoreParser::testFindFiles_shouldListKeptFilesOnly()
{
    this->write(QString(QLL(".gitignore")), "out/\n*.gen.cpp\n");
    this->write(QString(QLL("sub/.gitignore")), "!x.gen.cpp\n");
    this->write(QString(QLL("a.cpp")));
    this->write(QString(QLL("a.gen.cpp")));
    this->write(QString(QLL("notes.txt")));
    this->write(QString(QLL("out/b.cpp")));
    this->write(QString(QLL(".hidden/c.cpp")));
    this->write(QString(QLL(".git/d.cpp")));
    this->write(QString(QLL("sub/x.gen.cpp")));
    IgnoreParser parser;
    const QStringList found = parser.findFiles(this->m_dir->path(), QStringList() << QString(QLL("*.cpp")));
    const QStringList expected = QStringList() << this->path(QString(QLL(".hidden/c.cpp")))
        << this->path(QString(QLL("a.cpp"))) << this->path(QString(QLL("sub/x.gen.cpp")));
    QCOMPARE(found, expected);

    // Lists an ignored folder's files once that folder is named.
    parser.addExplicitPath(this->path(QString(QLL("out"))));
    QCOMPARE(parser.findFiles(this->path(QString(QLL("out"))), QStringList() << QString(QLL("*.cpp"))),
             QStringList() << this->path(QString(QLL("out/b.cpp"))));
}

QTEST_APPLESS_MAIN(tst_IgnoreParser)
#include "tst_ignore_parser.moc"
