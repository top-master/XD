// Copyright (C) 2015 The XD Company Ltd.
// SPDX-License-Identifier: Apache-2.0

#include "config-any-space.h"
#include "config-condition.h"
#include "config-escape.h"
#include "config-parser.h"
#include "config-regexp.h"
#include "config-replacement.h"
#include "config-scanner.h"
#include "config-word.h"

#include <QtCore/QDir>
#include <QtCore/QFile>
#include <QtCore/QFileInfo>
#include <QtCore/QTemporaryDir>
#include <QtTest/QtTest>

/// Tests the settings parser: the project root and the folders and files
/// it reads level by level, the schema's keys, values and aliases, and the
/// order in which files and assignments override each other.
class tst_ConfigParser : public QObject
{
    Q_OBJECT
    typedef QObject super;
private slots:
    void init();

    /// Levels.
    void testFindRoot_shouldStopAtClosestGit();
    void testLevelFolders_shouldListRootDownToFolder();
    void testLevelFiles_shouldListExistingFilesRootFirst();

    /// Schema.
    void testKey_shouldBeNullForUnknownName();
    void testKey_shouldIgnoreCase();
    void testSection_shouldListItsKeys();

    /// Reading.
    void testValue_shouldStartAtDefault();
    void testReadLevels_shouldLetDeeperFilesOverride();
    void testReadFile_shouldIgnoreCaseOfSectionAndKey();
    void testReadFile_shouldRejectUnknownValue();
    void testReadFile_shouldRejectUnknownKey();
    void testReadFile_shouldReadEverySection();
    void testReadFile_shouldRejectUnknownSection();
    void testReadFile_shouldRejectMissingFile();
    void testAssign_shouldIgnoreCaseAndTakeAliases();
    void testAssign_shouldStripQuotes();
    void testAssign_shouldTakeAnySpaceRun();
    void testAssign_shouldTakeUnquotedValueAsItIs();
    void testEscape_shouldDecodeCppEscapes_data();
    void testEscape_shouldDecodeCppEscapes();
    void testEscape_shouldRejectUnknownEscape_data();
    void testEscape_shouldRejectUnknownEscape();

    /// Regular expressions.
    void testRegExp_shouldSplitHeadAndFallback();
    void testRegExp_shouldParsePatternAndReplacement();
    void testReplacement_shouldFillGroupReferences_data();
    void testReplacement_shouldFillGroupReferences();
    void testAssign_shouldTakeRegExpWithFallback();
    void testAssign_shouldRejectBadRegExp_data();
    void testAssign_shouldRejectBadRegExp();
    void testReadFile_shouldTakeRegExp();
    void testReadFile_shouldKeepCommasOfValue_data();
    void testReadFile_shouldKeepCommasOfValue();
    void testAssign_shouldRejectMissingEquals();
    void testExtend_shouldFollowBaseUntilOverridden();

    /// Conditions.
    void testCondition_shouldBindAndCloserThanOr_data();
    void testCondition_shouldBindAndCloserThanOr();
    void testCondition_shouldRejectBadText_data();
    void testCondition_shouldRejectBadText();
    void testCondition_shouldMatchInSchemaSpelling();
    void testCondition_shouldComparePercents_data();
    void testCondition_shouldComparePercents();
    void testCondition_shouldRejectBadPercent_data();
    void testCondition_shouldRejectBadPercent();
    void testCondition_shouldWarnWithoutPercent();

private:
    void write(const QString & relative, const QByteArray & text = QByteArray()) const;
    QString path(const QString & relative) const;
    static Config::Schema schema();

    /// Gives each test a fresh project folder, with a `.git` folder that
    /// makes it the root.
    QScopedPointer<QTemporaryDir > m_dir;
};

/// Writes `text` into the file `relative` under the test's folder,
/// creating its folders.
void tst_ConfigParser::write(const QString & relative, const QByteArray & text) const
{
    const QString file = this->path(relative);
    QDir().mkpath(QFileInfo(file).absolutePath());
    QFile output(file);
    if (output.open(QIODevice::WriteOnly)) {
        output.write(text);
    }
}

/// Builds the absolute path of `relative` under the test's folder.
QString tst_ConfigParser::path(const QString & relative) const
{
    return QDir::cleanPath(this->m_dir->path() + QLatin1Char('/') + relative);
}

/// Builds a schema of two sections: `[Types]` with the spacing key
/// `refSuffix`, that takes any run of spaces and/or tabs (`none` for an
/// empty one), `keep-as-is`, or a `regexp/<pattern>/<replacement>/`, and
/// starts as `" "`; and `[Layout]` with
/// `width`, that takes the words `80` or `100`, and starts as `80`.
Config::Schema tst_ConfigParser::schema()
{
    QHash<QString, QString > aliases;
    aliases.insert(QString(QLL("none")), QString());
    Config::Section types(QString(QLL("Types")));
    types.addKey(QString(QLL("refSuffix")),
                 Config::Matchers() << Config::AnySpace::create()
                                  << Config::Word::create(QString(QLL("keep-as-is")))
                                  << Config::RegExp::create(),
                 QString(QLL(" ")), aliases);
    Config::Section layout(QString(QLL("Layout")));
    layout.addKey(QString(QLL("width")), QStringList() << QString(QLL("80"))
                                                       << QString(QLL("100")),
                  QString(QLL("80")));
    Config::Schema schema;
    schema.addSection(types);
    schema.addSection(layout);
    return schema;
}

void tst_ConfigParser::init()
{
    this->m_dir.reset(new QTemporaryDir);
    qExpect(this->m_dir->isValid())->toBeTruthy();
    QDir().mkpath(this->path(QString(QLL(".git"))));
}

void tst_ConfigParser::testFindRoot_shouldStopAtClosestGit()
{
    this->write(QString(QLL("inner/.git")), "gitdir: ../.git/worktrees/inner\n");
    this->write(QString(QLL("inner/src/a.cpp")));
    qExpect(Config::Scanner::findRoot(this->path(QString(QLL("inner/src/a.cpp")))))
        ->toEqual(this->path(QString(QLL("inner"))));
    qExpect(Config::Scanner::findRoot(this->path(QString(QLL("other")))))
        ->toEqual(this->m_dir->path());
}

void tst_ConfigParser::testLevelFolders_shouldListRootDownToFolder()
{
    const QString root = this->m_dir->path();
    const QString file = this->path(QString(QLL("a/b/c/my.txt")));
    const QStringList folders = Config::Scanner::levelFolders(root, file);
    qExpect(folders)->toEqual(QStringList() << root << this->path(QString(QLL("a")))
                                            << this->path(QString(QLL("a/b")))
                                            << this->path(QString(QLL("a/b/c"))));
    qExpect(Config::Scanner::levelFolders(root, QDir::tempPath()))->toEqual(QStringList() << root);
}

void tst_ConfigParser::testLevelFiles_shouldListExistingFilesRootFirst()
{
    this->write(QString(QLL(".toolrc")));
    this->write(QString(QLL("a/b/.toolrc")));
    this->write(QString(QLL("a/b/c/my.txt")));
    qExpect(Config::Scanner::levelFiles(this->path(QString(QLL("a/b/c/my.txt"))),
                                     QString(QLL(".toolrc"))))
        ->toEqual(QStringList() << this->path(QString(QLL(".toolrc")))
                                << this->path(QString(QLL("a/b/.toolrc"))));
}

void tst_ConfigParser::testKey_shouldBeNullForUnknownName()
{
    const Config::Schema schema = tst_ConfigParser::schema();
    qExpect(schema.key(QString(QLL("types.other"))).isNull())->toBeTruthy();
    qExpect(Config::Key().isNull())->toBeTruthy();
}

void tst_ConfigParser::testKey_shouldIgnoreCase()
{
    const Config::Key key = tst_ConfigParser::schema().key(QString(QLL("Types.REFSUFFIX")));
    qExpect(key.isNull())->toBeFalsy();
    qExpect(key.name)->toEqual(QString(QLL("types.refsuffix")));
}

void tst_ConfigParser::testSection_shouldListItsKeys()
{
    const Config::Schema schema = tst_ConfigParser::schema();
    qExpect(schema.sections().size())->toEqual(2);
    const Config::Section layout = schema.section(QString(QLL("LAYOUT")));
    qExpect(layout.isNull())->toBeFalsy();
    qExpect(layout.keys().size())->toEqual(1);
    qExpect(layout.keys().at(0).name)->toEqual(QString(QLL("layout.width")));
    qExpect(schema.section(QString(QLL("Other"))).isNull())->toBeTruthy();
    qExpect(schema.key(QString(QLL("layout.refSuffix"))).isNull())->toBeTruthy();
}

void tst_ConfigParser::testValue_shouldStartAtDefault()
{
    Config::Parser parser(tst_ConfigParser::schema());
    qExpect(parser.value(QString(QLL("types.refSuffix"))))->toEqual(QString(QLL(" ")));
}

void tst_ConfigParser::testReadLevels_shouldLetDeeperFilesOverride()
{
    this->write(QString(QLL(".toolrc")), "[Types]\nrefSuffix = \"\"\n");
    this->write(QString(QLL("a/.toolrc")), "[Types]\nrefSuffix = keep-as-is\n");
    this->write(QString(QLL("b/my.txt")));
    QString error;
    Config::Parser deep(tst_ConfigParser::schema());
    qExpect(deep.readLevels(this->path(QString(QLL("a/my.txt"))), QString(QLL(".toolrc")),
                            &error))->toBeTruthy();
    qExpect(deep.value(QString(QLL("types.refsuffix"))))->toEqual(QString(QLL("keep-as-is")));
    Config::Parser side(tst_ConfigParser::schema());
    qExpect(side.readLevels(this->path(QString(QLL("b/my.txt"))), QString(QLL(".toolrc")),
                            &error))->toBeTruthy();
    qExpect(side.value(QString(QLL("types.refsuffix"))))->toEqual(QString());
}

void tst_ConfigParser::testReadFile_shouldIgnoreCaseOfSectionAndKey()
{
    this->write(QString(QLL("rc.ini")), "[TYPES]\nRefSuffix = KEEP-AS-IS\n");
    QString error;
    Config::Parser parser(tst_ConfigParser::schema());
    qExpect(parser.readFile(this->path(QString(QLL("rc.ini"))), &error))->toBeTruthy();
    qExpect(parser.value(QString(QLL("types.refSuffix"))))->toEqual(QString(QLL("keep-as-is")));
}

void tst_ConfigParser::testReadFile_shouldRejectUnknownValue()
{
    this->write(QString(QLL("rc.ini")), "[Types]\nrefSuffix = wide\n");
    QString error;
    Config::Parser parser(tst_ConfigParser::schema());
    qExpect(parser.readFile(this->path(QString(QLL("rc.ini"))), &error))->toBeFalsy();
    qExpect(error)->toContain(QString(QLL("rc.ini: [Types] refSuffix = wide: use spaces "
                                          "and/or tabs (\"\" for none) or \"keep-as-is\"")));
    qExpect(parser.value(QString(QLL("types.refSuffix"))))->toEqual(QString(QLL(" ")));
}

void tst_ConfigParser::testReadFile_shouldRejectUnknownKey()
{
    this->write(QString(QLL("rc.ini")), "[Types]\nwidth = 3\n");
    QString error;
    Config::Parser parser(tst_ConfigParser::schema());
    qExpect(parser.readFile(this->path(QString(QLL("rc.ini"))), &error))->toBeFalsy();
    qExpect(error)->toContain(QString(QLL("rc.ini: unknown setting [Types] width")));
}

void tst_ConfigParser::testReadFile_shouldReadEverySection()
{
    this->write(QString(QLL("rc.ini")), "[Types]\nrefSuffix = none\n[Layout]\nwidth = 100\n");
    QString error;
    Config::Parser parser(tst_ConfigParser::schema());
    qExpect(parser.readFile(this->path(QString(QLL("rc.ini"))), &error))->toBeTruthy();
    qExpect(parser.value(QString(QLL("types.refSuffix"))))->toEqual(QString());
    qExpect(parser.value(QString(QLL("layout.width"))))->toEqual(QString(QLL("100")));
}

void tst_ConfigParser::testReadFile_shouldRejectUnknownSection()
{
    this->write(QString(QLL("rc.ini")), "[Colors]\nwidth = 100\n");
    QString error;
    Config::Parser parser(tst_ConfigParser::schema());
    qExpect(parser.readFile(this->path(QString(QLL("rc.ini"))), &error))->toBeFalsy();
    qExpect(error)->toContain(QString(QLL("rc.ini: unknown section of setting [Colors] width")));
}

void tst_ConfigParser::testReadFile_shouldRejectMissingFile()
{
    QString error;
    Config::Parser parser(tst_ConfigParser::schema());
    qExpect(parser.readFile(this->path(QString(QLL("absent.ini"))), &error))->toBeFalsy();
    qExpect(error)->toContain(QString(QLL("absent.ini: no such settings file")));
}

void tst_ConfigParser::testAssign_shouldIgnoreCaseAndTakeAliases()
{
    QString error;
    Config::Parser parser(tst_ConfigParser::schema());
    qExpect(parser.assign(QString(QLL("types.refsuffix=none")), &error))->toBeTruthy();
    qExpect(parser.value(QString(QLL("types.refSuffix"))))->toEqual(QString());
    qExpect(parser.assign(QString(QLL("TYPES.RefSuffix=Keep-As-Is")), &error))->toBeTruthy();
    qExpect(parser.value(QString(QLL("types.refSuffix"))))->toEqual(QString(QLL("keep-as-is")));
}

void tst_ConfigParser::testAssign_shouldStripQuotes()
{
    QString error;
    Config::Parser parser(tst_ConfigParser::schema());
    qExpect(parser.assign(QString(QLL("types.refSuffix=\"\"")), &error))->toBeTruthy();
    qExpect(parser.value(QString(QLL("types.refSuffix"))))->toEqual(QString());
    qExpect(parser.assign(QString(QLL("types.refSuffix=\" \"")), &error))->toBeTruthy();
    qExpect(parser.value(QString(QLL("types.refSuffix"))))->toEqual(QString(QLL(" ")));
}

void tst_ConfigParser::testAssign_shouldTakeAnySpaceRun()
{
    QString error;
    Config::Parser parser(tst_ConfigParser::schema());
    qExpect(parser.assign(QString(QLL("types.refSuffix=\"  \"")), &error))->toBeTruthy();
    qExpect(parser.value(QString(QLL("types.refSuffix"))))->toEqual(QString(QLL("  ")));
    qExpect(parser.assign(QString(QLL("types.refSuffix=\"\\t\"")), &error))->toBeTruthy();
    qExpect(parser.value(QString(QLL("types.refSuffix"))))->toEqual(QString(QLL("\t")));
    qExpect(parser.assign(QString(QLL("types.refSuffix=\" x\"")), &error))->toBeFalsy();
}

/// Leaves a value without double quotes as it is, backslashes included, for
/// a regular expression's sake.
void tst_ConfigParser::testAssign_shouldTakeUnquotedValueAsItIs()
{
    QString error;
    Config::Parser parser(tst_ConfigParser::schema());
    qExpect(parser.assign(QString(QLL("types.refSuffix=\\t")), &error))->toBeFalsy();
    qExpect(parser.assign(QString(QLL("types.refSuffix=regexp/\\s+&/&/")), &error))
        ->toBeTruthy();
    qExpect(parser.value(QString(QLL("types.refSuffix"))))
        ->toEqual(QString(QLL("regexp/\\s+&/&/")));
}

void tst_ConfigParser::testEscape_shouldDecodeCppEscapes_data()
{
    QTest::addColumn<QString >("text");
    QTest::addColumn<QString >("decoded");
    QTest::newRow("simple") << QString(QLL("\\t\\n\\r\\a\\b\\f\\v"))
                            << QString(QLL("\t\n\r\a\b\f\v"));
    QTest::newRow("quotes") << QString(QLL("\\'\\\"\\?\\\\"))
                            << QString(QLL("'\"?\\"));
    QTest::newRow("octal") << QString(QLL("\\101\\0x")) << QString(QLL("A")) + QChar(0)
                                                             + QLatin1Char('x');
    QTest::newRow("hex") << QString(QLL("\\x41\\x7e")) << QString(QLL("A~"));
    QTest::newRow("code point") << QString(QLL("\\u00e9"))
                                << QString(QChar(ushort(0xe9)));
    QTest::newRow("wide code point") << QString(QLL("\\U0001F600"))
        << QString(QChar(QChar::highSurrogate(0x1F600))) + QChar(QChar::lowSurrogate(0x1F600));
    QTest::newRow("plain") << QString(QLL("a b")) << QString(QLL("a b"));
}

void tst_ConfigParser::testEscape_shouldDecodeCppEscapes()
{
    QFETCH(QString, text);
    QFETCH(QString, decoded);
    QString result;
    QString error;
    qExpect(Config::Escape::decode(text, &result, &error))->toBeTruthy();
    qExpect(result)->toEqual(decoded);
}

void tst_ConfigParser::testEscape_shouldRejectUnknownEscape_data()
{
    QTest::addColumn<QString >("text");
    QTest::newRow("unknown letter") << QString(QLL("\\q"));
    QTest::newRow("lone backslash") << QString(QLL("a\\"));
    QTest::newRow("short code point") << QString(QLL("\\u12"));
    QTest::newRow("hex without digits") << QString(QLL("\\xg"));
}

void tst_ConfigParser::testEscape_shouldRejectUnknownEscape()
{
    QFETCH(QString, text);
    QString result;
    QString error;
    qExpect(Config::Escape::decode(text, &result, &error))->toBeFalsy();
    qExpect(error)->Not->toBeEmpty();
}

void tst_ConfigParser::testRegExp_shouldSplitHeadAndFallback()
{
    QString head;
    QString fallback;
    bool hasFallback = false;
    qExpect(Config::RegExp::split(QString(QLL("regexp/a\\/b/c/ | keep-as-is")), &head,
                                  &fallback, &hasFallback))->toBeTruthy();
    qExpect(head)->toEqual(QString(QLL("regexp/a\\/b/c/")));
    qExpect(fallback)->toEqual(QString(QLL("keep-as-is")));
    qExpect(hasFallback)->toBeTruthy();
    qExpect(Config::RegExp::split(QString(QLL("regexp/a/b/ |")), &head, &fallback,
                                  &hasFallback))->toBeTruthy();
    qExpect(hasFallback)->toBeTruthy();
    qExpect(fallback)->toBeEmpty();
    qExpect(Config::RegExp::split(QString(QLL("regexp/a/b/")), &head, &fallback,
                                  &hasFallback))->toBeTruthy();
    qExpect(hasFallback)->toBeFalsy();
    qExpect(Config::RegExp::split(QString(QLL("regexp/a/b/c")), &head, &fallback,
                                  &hasFallback))->toBeFalsy();
    qExpect(Config::RegExp::split(QString(QLL("regexp/a/b")), &head, &fallback,
                                  &hasFallback))->toBeFalsy();
    qExpect(Config::RegExp::split(QString(QLL("keep-as-is")), &head, &fallback,
                                  &hasFallback))->toBeFalsy();
}

void tst_ConfigParser::testRegExp_shouldParsePatternAndReplacement()
{
    QRegularExpression pattern;
    Config::Replacement replacement;
    qExpect(Config::RegExp::parse(QString(QLL("regexp/a\\/(b)/\\1\\/x/ | none")),
                                  &pattern, &replacement))->toBeTruthy();
    qExpect(pattern.pattern())->toEqual(QString(QLL("a/(b)")));
    qExpect(replacement.apply(pattern, QString(QLL("-a/b-"))))->toEqual(QString(QLL("-b/x-")));
    qExpect(Config::RegExp::parse(QString(QLL("regexp/(/x/")), &pattern, &replacement))
        ->toBeFalsy();
    qExpect(Config::RegExp::parse(QString(QLL("regexp//x/")), &pattern, &replacement))
        ->toBeFalsy();
    qExpect(Config::RegExp::parse(QString(QLL("regexp/(a)/\\2/")), &pattern, &replacement))
        ->toBeFalsy();
    qExpect(Config::RegExp::parse(QString(QLL("regexp/a/x\\\\/")), &pattern, &replacement))
        ->toBeTruthy();
    qExpect(replacement.apply(pattern, QString(QLL("a"))))->toEqual(QString(QLL("x\\")));
}

void tst_ConfigParser::testReplacement_shouldFillGroupReferences_data()
{
    QTest::addColumn<QString >("pattern");
    QTest::addColumn<QString >("replacement");
    QTest::addColumn<QString >("subject");
    QTest::addColumn<QString >("result");
    QTest::newRow("whole match") << QString(QLL("b+")) << QString(QLL("[\\0]"))
                                 << QString(QLL("abbc")) << QString(QLL("a[bb]c"));
    QTest::newRow("swapped groups") << QString(QLL("(abc.*)(xyz.*)"))
                                    << QString(QLL("\\2\\1")) << QString(QLL("abc1xyz2"))
                                    << QString(QLL("xyz2abc1"));
    QTest::newRow("every match") << QString(QLL("(\\d)")) << QString(QLL("<\\1>"))
                                 << QString(QLL("a1b2")) << QString(QLL("a<1>b<2>"));
    QTest::newRow("literal backslash") << QString(QLL("b")) << QString(QLL("\\\\1"))
                                       << QString(QLL("abc")) << QString(QLL("a\\1c"));
    QTest::newRow("tenth group") << QString(QLL("(a)(b)(c)(d)(e)(f)(g)(h)(i)(j)"))
                                 << QString(QLL("\\10")) << QString(QLL("abcdefghij"))
                                 << QString(QLL("j"));
    QTest::newRow("first group then 0") << QString(QLL("(a)")) << QString(QLL("\\10"))
                                        << QString(QLL("a")) << QString(QLL("a0"));
    QTest::newRow("other backslash") << QString(QLL("a")) << QString(QLL("\\n"))
                                     << QString(QLL("a")) << QString(QLL("\\n"));
}

void tst_ConfigParser::testReplacement_shouldFillGroupReferences()
{
    QFETCH(QString, pattern);
    QFETCH(QString, replacement);
    QFETCH(QString, subject);
    QFETCH(QString, result);
    const QRegularExpression expression(pattern);
    Config::Replacement parsed;
    qExpect(Config::Replacement::parse(replacement, expression.captureCount(), &parsed))
        ->toBeTruthy();
    qExpect(parsed.apply(expression, subject))->toEqual(result);
}

void tst_ConfigParser::testAssign_shouldTakeRegExpWithFallback()
{
    QString error;
    Config::Parser parser(tst_ConfigParser::schema());
    qExpect(parser.assign(QString(QLL("types.refSuffix=regexp/[^&]&\\s*/&/|KEEP-AS-IS")),
                          &error))->toBeTruthy();
    qExpect(parser.value(QString(QLL("types.refSuffix"))))
        ->toEqual(QString(QLL("regexp/[^&]&\\s*/&/ | keep-as-is")));
    qExpect(parser.assign(QString(QLL("types.refSuffix=regexp/x/y/ | none")), &error))
        ->toBeTruthy();
    qExpect(parser.value(QString(QLL("types.refSuffix"))))
        ->toEqual(QString(QLL("regexp/x/y/ |")));
}

void tst_ConfigParser::testAssign_shouldRejectBadRegExp_data()
{
    QTest::addColumn<QString >("value");
    QTest::newRow("bad pattern") << QString(QLL("regexp/(/x/"));
    QTest::newRow("no closing slash") << QString(QLL("regexp/a/b"));
    QTest::newRow("text after it") << QString(QLL("regexp/a/b/ c"));
    QTest::newRow("unknown fallback") << QString(QLL("regexp/a/b/ | wide"));
    QTest::newRow("regexp fallback") << QString(QLL("regexp/a/b/ | regexp/c/d/"));
}

void tst_ConfigParser::testAssign_shouldRejectBadRegExp()
{
    QFETCH(QString, value);
    QString error;
    Config::Parser parser(tst_ConfigParser::schema());
    qExpect(parser.assign(QLL("types.refSuffix=") + value, &error))->toBeFalsy();
    qExpect(error)->toContain(QString(QLL("regexp/<pattern>/<replacement>/")));
}

/// Writes a regular expression into an `.ini` file with its backslash
/// doubled, as the `.ini` form asks, and reads it back with one.
void tst_ConfigParser::testReadFile_shouldTakeRegExp()
{
    this->write(QString(QLL("rc.ini")),
                "[Types]\nrefSuffix = regexp/\\\\s+&/&/ | keep-as-is\n");
    QString error;
    Config::Parser parser(tst_ConfigParser::schema());
    qExpect(parser.readFile(this->path(QString(QLL("rc.ini"))), &error))->toBeTruthy();
    qExpect(error)->toBeEmpty();
    qExpect(parser.value(QString(QLL("types.refSuffix"))))
        ->toEqual(QString(QLL("regexp/\\s+&/&/ | keep-as-is")));
}

void tst_ConfigParser::testReadFile_shouldKeepCommasOfValue_data()
{
    QTest::addColumn<QByteArray >("value");
    QTest::addColumn<QString >("read");
    QTest::newRow("unquoted") << QByteArray("regexp/(a),(b)/x/")
        << QString(QLL("regexp/(a),(b)/x/"));
    QTest::newRow("quoted") << QByteArray("\"regexp/(a),(b)/x/\"")
        << QString(QLL("regexp/(a),(b)/x/"));
    QTest::newRow("space after comma") << QByteArray("regexp/(a), (b)/x/")
        << QString(QLL("regexp/(a), (b)/x/"));
    QTest::newRow("space before comma") << QByteArray("regexp/(a) ,(b)/x/")
        << QString(QLL("regexp/(a) ,(b)/x/"));
    QTest::newRow("hex digit after comma") << QByteArray("regexp/[a,b,1]/x/")
        << QString(QLL("regexp/[a,b,1]/x/"));
    QTest::newRow("with fallback") << QByteArray("regexp/(a),(b)/x/ | keep-as-is")
        << QString(QLL("regexp/(a),(b)/x/ | keep-as-is"));
}

/// Reads a value with commas, which the `.ini` form would split into a
/// list, as the one string it is.
void tst_ConfigParser::testReadFile_shouldKeepCommasOfValue()
{
    QFETCH(QByteArray, value);
    QFETCH(QString, read);
    this->write(QString(QLL("rc.ini")), "[Layout]\nwidth = 100\n[Types]\nrefSuffix = "
                                        + value + "\n");
    QString error;
    Config::Parser parser(tst_ConfigParser::schema());
    qExpect(parser.readFile(this->path(QString(QLL("rc.ini"))), &error))->toBeTruthy();
    qExpect(error)->toBeEmpty();
    qExpect(parser.value(QString(QLL("types.refSuffix"))))->toEqual(read);
    qExpect(parser.value(QString(QLL("layout.width"))))->toEqual(QString(QLL("100")));
}

void tst_ConfigParser::testAssign_shouldRejectMissingEquals()
{
    QString error;
    Config::Parser parser(tst_ConfigParser::schema());
    qExpect(parser.assign(QString(QLL("types.refSuffix")), &error))->toBeFalsy();
    qExpect(error)->toContain(QString(QLL("not a <section>.<key>=<value> setting")));
    qExpect(parser.assign(QString(QLL("types.width=1")), &error))->toBeFalsy();
    qExpect(error)->toContain(QString(QLL("unknown setting types.width")));
}

/// Extends the `[Layout]` width into a `[Print]` section: the extending key
/// starts at the base's default, follows each value the base gets, and keeps
/// its own once a file overrides it, even when the base changes later.
void tst_ConfigParser::testExtend_shouldFollowBaseUntilOverridden()
{
    Config::Schema base = tst_ConfigParser::schema();
    Config::Section print(QString(QLL("Print")));
    print.extend(base.section(QString(QLL("Layout"))), QStringList() << QString(QLL("Width")));
    Config::Schema schema = base;
    schema.addSection(print);
    qExpect(schema.key(QString(QLL("print.width"))).extends)
        ->toEqual(QString(QLL("layout.width")));

    QString error;
    Config::Parser parser(schema);
    qExpect(parser.value(QString(QLL("print.width"))))->toEqual(QString(QLL("80")));
    qExpect(parser.assign(QString(QLL("layout.width=100")), &error))->toBeTruthy();
    qExpect(parser.value(QString(QLL("print.width"))))->toEqual(QString(QLL("100")));
    this->write(QString(QLL("rc.ini")), "[Print]\nwidth = 80\n");
    qExpect(parser.readFile(this->path(QString(QLL("rc.ini"))), &error))->toBeTruthy();
    qExpect(parser.assign(QString(QLL("layout.width=100")), &error))->toBeTruthy();
    qExpect(parser.value(QString(QLL("print.width"))))->toEqual(QString(QLL("80")));
    qExpect(parser.value(QString(QLL("layout.width"))))->toEqual(QString(QLL("100")));
    qExpect(parser.assign(QString(QLL("print.width=90")), &error))->toBeFalsy();
    qExpect(error)->toContain(QString(QLL("print.width = 90: use")));
}

/// Builds the names the condition tests take: `a`, `b`, and `c`.
static QStringList conditionNames()
{
    return QStringList() << QString(QLL("a")) << QString(QLL("b"))
        << QString(QLL("c"));
}

void tst_ConfigParser::testCondition_shouldBindAndCloserThanOr_data()
{
    QTest::addColumn<QString >("text");
    QTest::addColumn<QStringList >("holding");
    QTest::addColumn<bool >("expected");
    const QString a = QString(QLL("a"));
    const QString b = QString(QLL("b"));
    const QString c = QString(QLL("c"));
    QTest::newRow("one name that holds") << a << (QStringList() << a) << true;
    QTest::newRow("one name that does not hold") << a << (QStringList() << b)
        << false;
    QTest::newRow("or of a name that holds")
        << QString(QLL("a | b")) << (QStringList() << b) << true;
    QTest::newRow("and of a name that does not hold")
        << QString(QLL("a & b")) << (QStringList() << a) << false;
    QTest::newRow("and before or") << QString(QLL("a | b & c"))
        << (QStringList() << a) << true;
    QTest::newRow("and before or, the and failing")
        << QString(QLL("a | b & c")) << (QStringList() << b) << false;
    QTest::newRow("parentheses before and")
        << QString(QLL("(a | b) & c")) << (QStringList() << a) << false;
    QTest::newRow("parentheses before and, holding")
        << QString(QLL("(a | b) & c")) << (QStringList() << b << c) << true;
    QTest::newRow("nested parentheses and no spaces")
        << QString(QLL("((a|b))&c")) << (QStringList() << a << c) << true;
    QTest::newRow("upper-case names") << QString(QLL("A | B"))
        << (QStringList() << b) << true;
}

void tst_ConfigParser::testCondition_shouldBindAndCloserThanOr()
{
    QFETCH(QString, text);
    QFETCH(QStringList, holding);
    QFETCH(bool, expected);
    QStringList program;
    QString error;
    qExpect(Config::Condition::compile(text, conditionNames(), &program, &error))->toBeTruthy();
    qExpect(error)->toBeEmpty();
    qExpect(Config::Condition::evaluate(program, holding.toSet()))->toEqual(expected);
}

void tst_ConfigParser::testCondition_shouldRejectBadText_data()
{
    QTest::addColumn<QString >("text");
    QTest::addColumn<QString >("error");
    QTest::newRow("empty") << QString()
        << QString(QLL("a name is missing at the end"));
    QTest::newRow("trailing sign")
        << QString(QLL("a |")) << QString(QLL("a name is missing at the end"));
    QTest::newRow("leading sign") << QString(QLL("& a"))
        << QString(QLL("a name is missing before &"));
    QTest::newRow("two names") << QString(QLL("a b"))
        << QString(QLL("a sign is missing before b"));
    QTest::newRow("unclosed parenthesis") << QString(QLL("(a"))
        << QString(QLL("a ) is missing"));
    QTest::newRow("unopened parenthesis") << QString(QLL("a)"))
        << QString(QLL("a ( is missing"));
    QTest::newRow("empty parentheses")
        << QString(QLL("()")) << QString(QLL("a name is missing before )"));
    QTest::newRow("unknown name") << QString(QLL("a | d"))
        << QString(QLL("unknown name d"));
    QTest::newRow("unknown sign") << QString(QLL("a ^ b"))
        << QString(QLL("unknown sign ^"));
    QTest::newRow("percent where the key takes none") << QString(QLL("40% | a"))
        << QString(QLL("the key takes no percent, as in 40%"));
}

void tst_ConfigParser::testCondition_shouldRejectBadText()
{
    QFETCH(QString, text);
    QFETCH(QString, error);
    QStringList program;
    QString message;
    qExpect(Config::Condition::compile(text, conditionNames(), &program, &message))->toBeFalsy();
    qExpect(message)->toEqual(error);
    QString matched;
    qExpect(Config::Condition(conditionNames()).match(text, &matched))->toBeFalsy();
}

void tst_ConfigParser::testCondition_shouldMatchInSchemaSpelling()
{
    QString matched;
    const Config::Condition condition(conditionNames());
    qExpect(condition.match(QString(QLL("A|( b  &C )")), &matched))->toBeTruthy();
    qExpect(matched)->toEqual(QString(QLL("a | (b & c)")));
    qExpect(condition.describe())->toEqual(QString(QLL("a condition of a, b, c, joined by | and &")));
}

void tst_ConfigParser::testCondition_shouldComparePercents_data()
{
    QTest::addColumn<QString >("text");
    QTest::addColumn<QStringList >("holding");
    QTest::addColumn<qreal >("percent");
    QTest::addColumn<bool >("expected");
    const QString a = QString(QLL("a"));
    QTest::newRow("past the percent") << QString(QLL("40%")) << QStringList() << qreal(52.5)
        << true;
    QTest::newRow("at the percent") << QString(QLL("40%")) << QStringList() << qreal(40)
        << false;
    QTest::newRow("below the percent") << QString(QLL("40%")) << QStringList() << qreal(10)
        << false;
    QTest::newRow("a percent with a name")
        << QString(QLL("(40% & a) | (30% & b)")) << (QStringList() << a) << qreal(45) << true;
    QTest::newRow("a percent with a name, the other one")
        << QString(QLL("(40% & a) | (30% & b)")) << (QStringList() << a) << qreal(35) << false;
    QTest::newRow("a name alone") << a << (QStringList() << a) << qreal(0)
        << true;
    QTest::newRow("no percent") << QString(QLL("0%")) << QStringList()
        << qreal(0) << false;
    QTest::newRow("every percent") << QString(QLL("100%")) << QStringList() << qreal(100)
        << false;
}

void tst_ConfigParser::testCondition_shouldComparePercents()
{
    QFETCH(QString, text);
    QFETCH(QStringList, holding);
    QFETCH(qreal, percent);
    QFETCH(bool, expected);
    QStringList program;
    QString error;
    qExpect(Config::Condition::compile(
        text,
        conditionNames(),
        &program,
        &error,
        true
    ))
        ->toBeTruthy();
    bool ok = false;
    qExpect(Config::Condition::evaluate(
        program,
        holding.toSet(),
        percent,
        &ok
    ))
        ->toEqual(expected);
    qExpect(ok)->toBeTruthy();
}

void tst_ConfigParser::testCondition_shouldRejectBadPercent_data()
{
    QTest::addColumn<QString >("text");
    QTest::addColumn<QString >("error");
    QTest::newRow("past 100%") << QString(QLL("101%"))
        << QString(QLL("101% is not from 0% to 100%"));
    QTest::newRow("a number without %") << QString(QLL("40"))
        << QString(QLL("unknown name 40"));
    QTest::newRow("two percents") << QString(QLL("40% 30%"))
        << QString(QLL("a sign is missing before 30%"));
}

void tst_ConfigParser::testCondition_shouldRejectBadPercent()
{
    QFETCH(QString, text);
    QFETCH(QString, error);
    QStringList program;
    QString message;
    qExpect(Config::Condition::compile(
        text,
        conditionNames(),
        &program,
        &message,
        true
    ))
        ->toBeFalsy();
    qExpect(message)->toEqual(error);
}

/// Evaluates a percent term before its value is set: the condition logs
/// it, fails, and says so through `ok`; a name alone needs no value.
void tst_ConfigParser::testCondition_shouldWarnWithoutPercent()
{
    QStringList program;
    QString error;
    qExpect(Config::Condition::compile(
        QString(QLL("a | 40%")),
        conditionNames(),
        &program,
        &error,
        true
    ))->toBeTruthy();
    const QSet<QString > holding = QSet<QString >() << QString(QLL("b"));
    bool ok = true;
    QTest::ignoreMessage(QtWarningMsg, "Config::Condition: no value is set for the term 40%");
    qExpect(Config::Condition::evaluate(program, holding, &ok))->toBeFalsy();
    qExpect(ok)->toBeFalsy();

    const Config::Condition condition(conditionNames(), true);
    QString matched;
    qExpect(condition.match(QString(QLL("(40%&A) | b")), &matched))->toBeTruthy();
    qExpect(matched)->toEqual(QString(QLL("(40% & a) | b")));
    qExpect(Config::Condition::compile(
        QString(QLL("a")),
        conditionNames(),
        &program,
        &error,
        true
    ))->toBeTruthy();
    qExpect(Config::Condition::evaluate(program, holding, &ok))->toBeFalsy();
    qExpect(ok)->toBeTruthy();
}

QTEST_APPLESS_MAIN(tst_ConfigParser)
#include "tst_config_parser.moc"
