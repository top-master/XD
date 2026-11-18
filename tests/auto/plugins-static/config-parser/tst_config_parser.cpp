// Copyright (C) 2015 The XD Company Ltd.
// SPDX-License-Identifier: Apache-2.0

#include "config-any-space.h"
#include "config-array-obj.h"
#include "config-choice.h"
#include "config-condition.h"
#include "config-escape.h"
#include "config-name.h"
#include "config-parser.h"
#include "config-regexp.h"
#include "config-replacement.h"
#include "config-scanner.h"
#include "config-scopes.h"
#include "config-types.h"
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
    void testReadFile_shouldKeepOpenKeys();
    void testAssign_shouldRejectBadOpenValue();
    void testExtendPrefixed_shouldTakeEveryKeyWithPrefix();
    void testAddFollower_shouldFollowItsSectionsKey();

    /// Conditions.
    void testCondition_shouldBindAndCloserThanOr_data();
    void testCondition_shouldBindAndCloserThanOr();
    void testCondition_shouldRejectBadText_data();
    void testCondition_shouldRejectBadText();
    void testCondition_shouldKeepSpellingAndSpaceSigns();
    void testCondition_shouldComparePercents_data();
    void testCondition_shouldComparePercents();
    void testCondition_shouldRejectBadPercent_data();
    void testCondition_shouldRejectBadPercent();
    void testCondition_shouldWarnWithoutPercent();
    void testCondition_shouldEvaluateExpressions();
    void testCondition_shouldCallObjectMembers();
    void testCondition_shouldChooseNumbers();
    void testScopes_shouldStackAndUnregister();
    void testScopes_shouldOfferOnlyOwnSlotsAndProperties();
    void testArrayObj_shouldOfferItems();
    void testTypes_shouldReachReturnedObjects();
    void testScopes_shouldOfferDynamicPropertiesAndPaths();
    void testParser_shouldKeepQuotesOfConditions();
    void testScopes_shouldRunGlobalSetups();
    void testCondition_shouldHandleNameMismatch();
    void testCondition_shouldPassStrings();

private:
    void write(const QString & relative,
               const QByteArray & text = QByteArray()) const;
    QString path(const QString & relative) const;
    static Config::Schema schema();

    /// Gives each test a fresh project folder, with a `.git` folder that
    /// makes it the root.
    QScopedPointer<QTemporaryDir > m_dir;
};

/// Writes `text` into the file `relative` under the test's folder,
/// creating its folders.
void tst_ConfigParser::write(const QString & relative,
                             const QByteArray & text) const
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
    types.addKey(
        QString(QLL("refSuffix")),
        Config::Matchers() << Config::AnySpace::create()
            << Config::Word::create(QString(QLL("keep-as-is")))
            << Config::RegExp::create(),
        QString(QLL(" ")),
        aliases
    );
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
    qExpect(Config::Scanner::levelFolders(root, QDir::tempPath()))->toEqual(
        QStringList() << root
    );
}

void tst_ConfigParser::testLevelFiles_shouldListExistingFilesRootFirst()
{
    this->write(QString(QLL(".toolrc")));
    this->write(QString(QLL("a/b/.toolrc")));
    this->write(QString(QLL("a/b/c/my.txt")));
    qExpect(Config::Scanner::levelFiles(
        this->path(QString(QLL("a/b/c/my.txt"))),
        QString(QLL(".toolrc"))
    ))
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
    qExpect(parser.readFile(this->path(QString(QLL("rc.ini"))), &error))
        ->toBeFalsy();
    qExpect(error)->toContain(QString(QLL("rc.ini: [Types] refSuffix = wide: use spaces "
                                          "and/or tabs (\"\" for none) or \"keep-as-is\"")));
    qExpect(parser.value(QString(QLL("types.refSuffix"))))
        ->toEqual(QString(QLL(" ")));
}

/*!
    Builds a schema whose dotted section `[Replace.types]` is open: any key
    takes a C++ name or `keep-as-is`, beside its own `action` key.
*/
static Config::Schema openSchema()
{
    Config::Section replace(QString(QLL("Replace.types")));
    replace.addKey(QString(QLL("action")),
                   QStringList() << QString(QLL("replace")) << QString(QLL("report")),
                   QString(QLL("replace")));
    replace.setOpen(Config::Matchers() << Config::Name::create(true)
                        << Config::Word::create(QString(QLL("keep-as-is"))));
    Config::Schema schema;
    schema.addSection(replace);
    return schema;
}

/// Reads the keys of an open section in their own spelling and in the order
/// of their lines, a key given twice included, beside a key it lists.
void tst_ConfigParser::testReadFile_shouldKeepOpenKeys()
{
    typedef QList<QPair<QString, QString > > Entries;
    this->write(QString(QLL("rc.ini")),
                "[Replace.types]\n::QLatin1String = ::QLL\naction = report\n"
                "QFoo = keep-as-is\n&::make* = \"::QFoo::create(args.join(\\\", \\\"))\"\n"
                "QFoo = ::QBar\n");
    QString error;
    Config::Parser parser(openSchema());
    QVERIFY2(parser.readFile(this->path(QString(QLL("rc.ini"))), &error),
             qPrintable(error));
    const Entries entries = parser.openEntries(QString(QLL("Replace.Types")));
    qExpect(entries.size())->toEqual(4);
    qExpect(entries.at(0).first)->toEqual(QString(QLL("::QLatin1String")));
    qExpect(entries.at(0).second)->toEqual(QString(QLL("::QLL")));
    qExpect(entries.at(1).second)->toEqual(QString(QLL("keep-as-is")));
    qExpect(entries.at(2).first)->toEqual(QString(QLL("&::make*")));
    qExpect(entries.at(2).second)
        ->toEqual(QString(QLL("::QFoo::create(args.join(\", \"))")));
    qExpect(entries.at(3).first)->toEqual(QString(QLL("QFoo")));
    qExpect(entries.at(3).second)->toEqual(QString(QLL("::QBar")));
    qExpect(parser.value(QString(QLL("replace.types.action"))))
        ->toEqual(QString(QLL("report")));

    // An assignment adds a key too.
    qExpect(parser.assign(QString(QLL("replace.types.QBar=::QBaz")), &error))
        ->toBeTruthy();
    qExpect(parser.openEntries(QString(QLL("replace.types"))).last().first)
        ->toEqual(QString(QLL("QBar")));
}

/// Rejects a value of an open section that its matchers do not take.
void tst_ConfigParser::testAssign_shouldRejectBadOpenValue()
{
    QString error;
    Config::Parser parser(openSchema());
    qExpect(parser.assign(QString(QLL("replace.types.QFoo=1x")), &error))
        ->toBeFalsy();
    qExpect(error)->toContain(QString(QLL("a C++ name")));
    qExpect(parser.openEntries(QString(QLL("replace.types"))).isEmpty())
        ->toBeTruthy();
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
    qExpect(Config::RegExp::split(
        QString(QLL("regexp/a\\/b/c/ | keep-as-is")),
        &head,
        &fallback,
        &hasFallback
    ))->toBeTruthy();
    qExpect(head)->toEqual(QString(QLL("regexp/a\\/b/c/")));
    qExpect(fallback)->toEqual(QString(QLL("keep-as-is")));
    qExpect(hasFallback)->toBeTruthy();
    qExpect(Config::RegExp::split(
        QString(QLL("regexp/a/b/ |")),
        &head,
        &fallback,
        &hasFallback
    ))->toBeTruthy();
    qExpect(hasFallback)->toBeTruthy();
    qExpect(fallback)->toBeEmpty();
    qExpect(Config::RegExp::split(
        QString(QLL("regexp/a/b/")),
        &head,
        &fallback,
        &hasFallback
    ))->toBeTruthy();
    qExpect(hasFallback)->toBeFalsy();
    qExpect(Config::RegExp::split(
        QString(QLL("regexp/a/b/c")),
        &head,
        &fallback,
        &hasFallback
    ))->toBeFalsy();
    qExpect(Config::RegExp::split(
        QString(QLL("regexp/a/b")),
        &head,
        &fallback,
        &hasFallback
    ))->toBeFalsy();
    qExpect(Config::RegExp::split(
        QString(QLL("keep-as-is")),
        &head,
        &fallback,
        &hasFallback
    ))->toBeFalsy();
}

void tst_ConfigParser::testRegExp_shouldParsePatternAndReplacement()
{
    QRegularExpression pattern;
    Config::Replacement replacement;
    qExpect(Config::RegExp::parse(
        QString(QLL("regexp/a\\/(b)/\\1\\/x/ | none")),
        &pattern,
        &replacement
    ))->toBeTruthy();
    qExpect(pattern.pattern())->toEqual(QString(QLL("a/(b)")));
    qExpect(replacement.apply(pattern, QString(QLL("-a/b-"))))->toEqual(QString(QLL("-b/x-")));
    qExpect(Config::RegExp::parse(
        QString(QLL("regexp/(/x/")),
        &pattern,
        &replacement
    ))
        ->toBeFalsy();
    qExpect(Config::RegExp::parse(
        QString(QLL("regexp//x/")),
        &pattern,
        &replacement
    ))
        ->toBeFalsy();
    qExpect(Config::RegExp::parse(
        QString(QLL("regexp/(a)/\\2/")),
        &pattern,
        &replacement
    ))
        ->toBeFalsy();
    qExpect(Config::RegExp::parse(
        QString(QLL("regexp/a/x\\\\/")),
        &pattern,
        &replacement
    ))
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
    qExpect(Config::Replacement::parse(
        replacement,
        expression.captureCount(),
        &parsed
    ))
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
    QTest::newRow("regexp fallback")
        << QString(QLL("regexp/a/b/ | regexp/c/d/"));
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
    this->write(
        QString(QLL("rc.ini")),
        "[Layout]\nwidth = 100\n[Types]\nrefSuffix = "
            + value + "\n"
    );
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
    print.extend(base.section(QString(QLL("Layout"))), QStringList()
        << QString(QLL("Width"))
    );
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

/// Extends the keys of a `[Page]` section that start with `wrap` (in any
/// case) into a `[Print]` section: `wrapBegin` and `WrapMax` follow the
/// base, and `height` stays out of `[Print]`.
void tst_ConfigParser::testExtendPrefixed_shouldTakeEveryKeyWithPrefix()
{
    const QStringList widths = QStringList() << QString(QLL("80"))
        << QString(QLL("100"));
    Config::Section page(QString(QLL("Page")));
    page.addKey(QString(QLL("wrapBegin")), widths, QString(QLL("80")));
    page.addKey(QString(QLL("WrapMax")), widths, QString(QLL("100")));
    page.addKey(QString(QLL("height")), widths, QString(QLL("80")));
    Config::Section print(QString(QLL("Print")));
    print.extendPrefixed(page, QString(QLL("WRAP")));
    Config::Schema schema;
    schema.addSection(page);
    schema.addSection(print);
    qExpect(print.keys().size())->toEqual(2);
    qExpect(schema.key(QString(QLL("print.wrapbegin"))).extends)
        ->toEqual(QString(QLL("page.wrapbegin")));
    qExpect(schema.key(QString(QLL("print.wrapmax"))).extends)
        ->toEqual(QString(QLL("page.wrapmax")));

    QString error;
    Config::Parser parser(schema);
    qExpect(parser.value(QString(QLL("print.wrapMax"))))->toEqual(QString(QLL("100")));
    qExpect(parser.assign(QString(QLL("page.wrapBegin=100")), &error))->toBeTruthy();
    qExpect(parser.value(QString(QLL("print.wrapBegin"))))->toEqual(QString(QLL("100")));
    qExpect(parser.assign(QString(QLL("print.height=80")), &error))->toBeFalsy();
    qExpect(error)->toContain(QString(QLL("unknown setting print.height")));
}

/// Adds a `margin` that follows the `width` of its own `[Page]` section: it
/// gives the width's value until it is set itself, and takes the same values.
void tst_ConfigParser::testAddFollower_shouldFollowItsSectionsKey()
{
    const QStringList widths = QStringList()
        << QString(QLL("80"))
        << QString(QLL("100"));
    Config::Section page(QString(QLL("Page")));
    page.addKey(QString(QLL("width")), widths, QString(QLL("80")));
    page.addFollower(QString(QLL("margin")), QString(QLL("Width")));
    page.addFollower(QString(QLL("gap")), QString(QLL("missing")));
    Config::Schema schema;
    schema.addSection(page);
    qExpect(page.keys().size())->toEqual(2);
    qExpect(schema.key(QString(QLL("page.margin"))).extends)
        ->toEqual(QString(QLL("page.width")));

    QString error;
    Config::Parser parser(schema);
    qExpect(parser.value(QString(QLL("page.margin"))))->toEqual(QString(QLL("80")));
    qExpect(parser.assign(QString(QLL("page.width=100")), &error))->toBeTruthy();
    qExpect(parser.value(QString(QLL("page.margin"))))->toEqual(QString(QLL("100")));
    qExpect(parser.assign(QString(QLL("page.margin=80")), &error))->toBeTruthy();
    qExpect(parser.value(QString(QLL("page.margin"))))->toEqual(QString(QLL("80")));
    qExpect(parser.assign(QString(QLL("page.margin=90")), &error))->toBeFalsy();
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

    // Expects the warning about each name of another case, such as `A`.
    for (int i = 0; i < text.size(); ++i) {
        if (text.at(i).isUpper()) {
            QTest::ignoreMessage(
                QtWarningMsg,
                qPrintable(QString(
                    QLL("failed to find the name %1 (maybe %2 was meant).")
                ).arg(text.at(i))
                    .arg(text.at(i).toLower()))
            );
        }
    }
    qExpect(Config::Condition::compile(
        text,
        conditionNames(),
        &program,
        &error
    ))->toBeTruthy();
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
    qExpect(Config::Condition::compile(
        text,
        conditionNames(),
        &program,
        &message
    ))->toBeFalsy();
    qExpect(message)->toEqual(error);
    QString matched;
    qExpect(Config::Condition(conditionNames()).match(text, &matched))->toBeFalsy();
}

/// Takes a name of another case, which the tool that reads the setting
/// reports as its mode says, and keeps its spelling for that report.
void tst_ConfigParser::testCondition_shouldKeepSpellingAndSpaceSigns()
{
    QString matched;
    const Config::Condition condition(conditionNames());
    qExpect(condition.match(QString(QLL("A|( b  &C )")), &matched))->toBeTruthy();
    qExpect(matched)->toEqual(QString(QLL("A | (b & C)")));
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
    qExpect(matched)->toEqual(QString(QLL("(40% & A) | b")));
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

/// Answers the names that pushTestObjects() adds, the way a tool's
/// own context answers them at the place where it evaluates a condition:
/// `argCount()` gives 7 for the third argument two calls up, `spaceRemaining`
/// gives `space` once it is set, and each call of it counts in `spaceCalls`.
class TestArgumentContext : public Config::Evaluator::Context
{
    typedef Config::Evaluator::Context super;
public:
    inline TestArgumentContext()
        : count(-1), space(-1), spaceCalls(0), percentCalls(0)
    {
    }

    inline qreal argumentCount(int depth, int argument) const
    { return depth == -2 && argument == 3 ? 7 : -1; }

    inline bool percent(qreal *value) const Q_DECL_OVERRIDE_IL
    {
        ++percentCalls;
        *value = 100 - space;
        return space >= 0;
    }

    qreal count;
    qreal space;
    mutable int spaceCalls;
    mutable int percentCalls;
};

/// Offers the names of TestArgumentContext to the conditions of a test:
/// `value` gives `TestArgumentContext::count` (none while it is unset),
/// `argCount()` and `spaceRemaining` answer as TestArgumentContext says.
class TestArgumentObject : public Config::Object
{
    Q_OBJECT
    typedef Config::Object super;
    Q_PROPERTY(QVariant value READ value)
    Q_PROPERTY(QVariant spaceRemaining READ spaceRemaining)
public:
    inline const TestArgumentContext *test() const
    { return static_cast<const TestArgumentContext * >(this->context()); }

    inline QVariant value() const
    { return test()->count >= 0 ? QVariant(test()->count) : QVariant(); }

    inline QVariant spaceRemaining() const
    {
        ++test()->spaceCalls;
        return test()->space >= 0 ? QVariant(test()->space) : QVariant();
    }

    /// Stands for a method that is invokable, yet no public slot; hence no
    /// expression may call it.
    Q_INVOKABLE inline void invokableOnly() {}

public slots:
    inline qreal argCount(int depth, int argument = 0)
    { return test()->argumentCount(depth, argument); }

    inline bool named(const QString & text) const
    { return text == QLL("a b"); }

signals:
    /// Stands for a signal, which no expression may call.
    void changed();
};

/// Hands out a Config::ArrayObj of two items in each way a member may: from
/// a slot whose return type Qt does not know (`items()`), from a property,
/// whose type moc registers (`list`), and as an address that names no
/// object (`stray()`).
class TestHolderObject : public Config::Object
{
    Q_OBJECT
    typedef Config::Object super;
    Q_PROPERTY(Config::ArrayObj *list READ list)
public:
    inline TestHolderObject()
        : m_items(QStringList() << QString(QLL("first"))
                  << QString(QLL("second")), this)
        , m_stray(0)
    {
    }

    inline Config::ArrayObj *list() const
    { return const_cast<Config::ArrayObj * >(&m_items); }

public slots:
    inline Config::ArrayObj *items() const { return this->list(); }

    inline Config::ArrayObj *stray() const
    {
        return reinterpret_cast<Config::ArrayObj * >(
            const_cast<int * >(&m_stray)
        );
    }

private:
    Config::ArrayObj m_items;
    int m_stray;
};

/// Offers three dynamic properties: `stored`, which answers the value
/// `QObject::setProperty()` stored (4), `placed`, which `dynamicValue()`
/// answers in place of its stored value (9), and `share`, a percent (40%).
class TestDynamicObject : public Config::Object
{
    Q_OBJECT
    typedef Config::Object super;
public:
    inline explicit TestDynamicObject(QObject *parent = Q_NULLPTR)
        : super(parent)
    {
        this->setProperty("stored", 4);
        this->setProperty("placed", 0);
        this->setProperty("share", QString(QLL("40%")));
    }

    inline QVariant dynamicValue(
            const QByteArray & name) const Q_DECL_OVERRIDE_IL
    {
        return name == "placed" ? QVariant(9) : super::dynamicValue(name);
    }
};

/// Hands out a TestDynamicObject through the property `settings`.
class TestSettingsHolderObject : public Config::Object
{
    Q_OBJECT
    typedef Config::Object super;
    Q_PROPERTY(QObject *settings READ settings)
public:
    inline TestSettingsHolderObject()
        : m_settings(new TestDynamicObject(this))
    {
    }

    inline QObject *settings() const { return m_settings; }

private:
    TestDynamicObject *m_settings;
};

/// Gives 7 for `globalMark`, a name no other object of the tests offers.
class TestGlobalObject : public Config::Object
{
    Q_OBJECT
    typedef Config::Object super;
    Q_PROPERTY(int globalMark READ globalMark)
public:
    inline int globalMark() const { return 7; }
};

/// Adds a TestGlobalObject to the global scope @a scope.
static void addTestGlobalObject(Config::Scopes::Scope *scope)
{
    scope->add(new TestGlobalObject);
}

/// Gives 1 for `value`, for a scope that hides the test's own `value`.
class TestOneObject : public Config::Object
{
    Q_OBJECT
    typedef Config::Object super;
    Q_PROPERTY(int value READ value)
public:
    inline int value() const { return 1; }
};

/// Pushes a scope of Config::Scopes with a TestArgumentObject, whose names
/// hold for as long as the test keeps the scope.
static QSharedPointer<Config::Scopes::Scope > pushTestObjects()
{
    const QSharedPointer<Config::Scopes::Scope > scope = Config::Scopes::push();
    scope->add(new TestArgumentObject);
    return scope;
}


void tst_ConfigParser::testCondition_shouldEvaluateExpressions()
{
    const QSharedPointer<Config::Scopes::Scope > scope = pushTestObjects();
    const QStringList names = QStringList() << QString(QLL("is-signature"));
    QStringList program;
    QString error;
    const QString expression = QString(QLL(
        "argCount(-2, 3) - 1 == value && (space_remaining <= 18.75% || is_signature)"
    ));
    QVERIFY2(Config::Condition::compile(
        expression,
        names,
        &program,
        &error,
        true
    ), qPrintable(error));
    TestArgumentContext context;
    context.count = 6;
    context.space = 18.75;
    bool ok = false;
    QVERIFY(Config::Condition::evaluate(
        program,
        QSet<QString >(),
        &context,
        &ok
    ));
    QVERIFY(ok);
    context.space = 19;
    QVERIFY( ! Config::Condition::evaluate(
        program,
        QSet<QString >(),
        &context,
        &ok
    ));
    QVERIFY(ok);
    QVERIFY(Config::Condition::evaluate(
        program,
        QSet<QString >() << QString(QLL("is-signature")),
        &context,
        &ok
    ));
    QVERIFY( ! Config::Condition::compile(
        QString(QLL("space-remaining < 20%")), names, &program, &error, true
    ));
    QVERIFY(Config::Condition::compile(
        QString(QLL("is-signature-1 == 0")), names, &program, &error, true
    ));
    QVERIFY(Config::Condition::evaluate(
        program,
        QSet<QString >() << QString(QLL("is-signature")),
        &context,
        &ok
    ));
    QVERIFY( ! Config::Condition::compile(
        QString(QLL("odd-name")),
        names + (QStringList() << QString(QLL("odd-name"))),
        &program, &error, true
    ));
}

/// Calls an object's member only when its term runs, with the context the
/// caller passes, and asks the context for the percent value only when a
/// percent term needs it, once.
void tst_ConfigParser::testCondition_shouldCallObjectMembers()
{
    const QSharedPointer<Config::Scopes::Scope > scope = pushTestObjects();
    const QStringList names = QStringList() << QString(QLL("is-signature"));
    QStringList program;
    QString error;
    QVERIFY2(Config::Condition::compile(
        QString(QLL("is-signature && space_remaining() >= 40%")),
        names,
        &program,
        &error,
        true
    ), qPrintable(error));
    TestArgumentContext context;
    context.space = 50;
    bool ok = false;
    qExpect(Config::Condition::evaluate(
        program, QSet<QString >(), &context, &ok
    ))->toBeFalsy();
    qExpect(ok)->toBeTruthy();
    qExpect(context.spaceCalls)->toEqual(0);
    qExpect(Config::Condition::evaluate(
        program,
        QSet<QString >() << QString(QLL("is-signature")),
        &context,
        &ok
    ))->toBeTruthy();
    qExpect(context.spaceCalls)->toEqual(1);

    // Asks for the percent value only where a percent term needs it.
    QVERIFY2(Config::Condition::compile(
        QString(QLL("is-signature && 40%")), names, &program, &error, true
    ), qPrintable(error));
    qExpect(Config::Condition::evaluate(
        program, QSet<QString >(), &context, &ok
    ))->toBeFalsy();
    qExpect(context.percentCalls)->toEqual(0);
    qExpect(Config::Condition::evaluate(
        program,
        QSet<QString >() << QString(QLL("is-signature")),
        &context,
        &ok
    ))->toBeTruthy();
    qExpect(context.percentCalls)->toEqual(1);
    QVERIFY2(Config::Condition::compile(
        QString(QLL("is-signature && space_remaining() >= 40%")),
        names,
        &program,
        &error,
        true
    ), qPrintable(error));

    // A call the place cannot answer misses a value.
    context.space = -1;
    qExpect(Config::Condition::evaluate(
        program,
        QSet<QString >() << QString(QLL("is-signature")),
        &context,
        &ok
    ))->toBeFalsy();
    qExpect(ok)->toBeFalsy();

    // A call without a context misses a value as well.
    qExpect(Config::Condition::evaluate(
        program,
        QSet<QString >() << QString(QLL("is-signature")),
        Q_NULLPTR,
        &ok
    ))->toBeFalsy();
    qExpect(ok)->toBeFalsy();

    // A call takes as many arguments as its registration allows.
    qExpect(Config::Condition::compile(
        QString(QLL("argCount() > 1")), names, &program, &error, true
    ))->toBeFalsy();
    qExpect(Config::Condition::compile(
        QString(QLL("argCount(1, 2, 3) > 1")), names, &program, &error, true
    ))->toBeFalsy();
    qExpect(Config::Condition::compile(
        QString(QLL("spaceRemaining(1) > 1")), names, &program, &error, true
    ))->toBeFalsy();
}

/// Picks a number by a C-style choice `<condition> ? <then> : <else>`, which
/// nests to the right and needs no `;`, and runs only the value it picks.
void tst_ConfigParser::testCondition_shouldChooseNumbers()
{
    const QSharedPointer<Config::Scopes::Scope > scope = pushTestObjects();
    const QStringList names = QStringList() << QString(QLL("is-header"))
        << QString(QLL("is-signature"));
    QStringList program;
    QString error;
    QVERIFY2(Config::Condition::compile(
        QString(QLL("is-header ? 2 : is-signature ? 3 : spaceRemaining + 1")),
        names,
        &program,
        &error,
        true
    ), qPrintable(error));
    TestArgumentContext context;
    context.space = 40;
    qreal result = 0;
    qExpect(Config::Condition::value(
        program,
        QSet<QString >() << QString(QLL("is-header")),
        &context,
        &result
    ))->toBeTruthy();
    qExpect(result)->toEqual(qreal(2));
    qExpect(Config::Condition::value(
        program,
        QSet<QString >() << QString(QLL("is-signature")),
        &context,
        &result
    ))->toBeTruthy();
    qExpect(result)->toEqual(qreal(3));
    qExpect(context.spaceCalls)->toEqual(0);
    qExpect(Config::Condition::value(
        program,
        QSet<QString >(),
        &context,
        &result
    ))->toBeTruthy();
    qExpect(result)->toEqual(qreal(41));
    qExpect(context.spaceCalls)->toEqual(1);

    // A choice inside parentheses, and around a condition with `&&`.
    QVERIFY2(Config::Condition::compile(
        QString(QLL("(is-header && argCount(-2, 3) == 7 ? 4 : 1) * 2")),
        names,
        &program,
        &error,
        true
    ), qPrintable(error));
    qExpect(Config::Condition::value(
        program,
        QSet<QString >() << QString(QLL("is-header")),
        &context,
        &result
    ))->toBeTruthy();
    qExpect(result)->toEqual(qreal(8));

    // A condition that holds gives 1, and one that does not gives 0.
    QVERIFY(Config::Condition::compile(
        QString(QLL("is-header")), names, &program, &error, true
    ));
    qExpect(Config::Condition::value(
        program,
        QSet<QString >(),
        &context,
        &result
    ))->toBeTruthy();
    qExpect(result)->toEqual(qreal(0));

    // A choice misses neither of its values, nor its `:`.
    qExpect(Config::Condition::compile(
        QString(QLL("is-header ? 2")), names, &program, &error, true
    ))->toBeFalsy();
    qExpect(Config::Condition::compile(
        QString(QLL("is-header ? : 1")), names, &program, &error, true
    ))->toBeFalsy();
}

/// Finds a name in the newest scope whose objects offer it, forgets a
/// scope's objects once its last pointer goes, and warns through popUntil()
/// about a scope that something still holds.
void tst_ConfigParser::testScopes_shouldStackAndUnregister()
{
    const QStringList names;
    QStringList program;
    QString error;
    const int depth = Config::Scopes::count();
    qExpect(Config::Condition::compile(
        QString(QLL("value > 0")), names, &program, &error, true
    ))->toBeFalsy();

    TestArgumentContext context;
    context.count = 5;
    QSharedPointer<Config::Scopes::Scope > outer = pushTestObjects();
    qExpect(Config::Scopes::count())->toEqual(depth + 1);
    QVERIFY2(Config::Condition::compile(
        QString(QLL("value")), names, &program, &error, true
    ), qPrintable(error));
    qreal result = 0;
    qExpect(Config::Condition::value(
        program, QSet<QString >(), &context, &result
    ))->toBeTruthy();
    qExpect(result)->toEqual(qreal(5));

    // A newer scope hides the older one's `value` while it lives.
    {
        const QSharedPointer<Config::Scopes::Scope > inner =
            Config::Scopes::push();
        inner->add(new TestOneObject);
        qExpect(Config::Condition::value(
            program, QSet<QString >(), &context, &result
        ))->toBeTruthy();
        qExpect(result)->toEqual(qreal(1));
    }
    qExpect(Config::Condition::value(
        program, QSet<QString >(), &context, &result
    ))->toBeTruthy();
    qExpect(result)->toEqual(qreal(5));

    // The scope still referenced is only warned about, and keeps working.
    QTest::ignoreMessage(QtWarningMsg, QRegularExpression(
        QString(QLL("^Config::Scopes: scope %1 is still referenced$"))
            .arg(depth + 1)
    ));
    Config::Scopes::popUntil(depth);
    qExpect(Config::Condition::value(
        program, QSet<QString >(), &context, &result
    ))->toBeTruthy();

    // Once its last pointer goes, its objects go too.
    outer.clear();
    Config::Scopes::popUntil(depth);
    qExpect(Config::Scopes::count())->toEqual(depth);
    qExpect(Config::Condition::value(
        program, QSet<QString >(), &context, &result
    ))->toBeFalsy();
}

/// Offers to expressions only the public slots and the properties that a
/// subclass of Config::Object declares: no member of QObject itself (such
/// as `objectName` or `deleteLater()`), no signal, and no other method that
/// is merely invokable.
void tst_ConfigParser::testScopes_shouldOfferOnlyOwnSlotsAndProperties()
{
    const QSharedPointer<Config::Scopes::Scope > scope = pushTestObjects();
    const Config::Scopes::Member argCount =
        Config::Scopes::find(QString(QLL("argCount")));
    qExpect(argCount.isNull())->toBeFalsy();
    qExpect(argCount.isProperty)->toBeFalsy();
    qExpect(argCount.minimumArguments)->toEqual(1);
    qExpect(argCount.maximumArguments)->toEqual(2);
    qExpect(Config::Scopes::find(QString(QLL("value"))).isProperty)
        ->toBeTruthy();

    const QStringList hidden = QStringList() << QString(QLL("objectName"))
        << QString(QLL("deleteLater")) << QString(QLL("destroyed"))
        << QString(QLL("changed")) << QString(QLL("invokableOnly"));
    const QStringList names = Config::Scopes::names();
    for (int i = 0; i < hidden.size(); ++i) {
        qExpect(Config::Scopes::find(hidden.at(i)).isNull())->toBeTruthy();
        qExpect(names.contains(hidden.at(i)))->toBeFalsy();
    }

    // Reading or calling them directly fails as well.
    TestArgumentObject object;
    QVariant result;
    qExpect(Config::Scopes::invoke(
        &object, QString(QLL("objectName")), QVariantList(), &result
    ))->toBeFalsy();
    qExpect(Config::Scopes::invoke(
        &object, QString(QLL("deleteLater")), QVariantList(), &result
    ))->toBeFalsy();
    qExpect(Config::Scopes::invoke(
        &object, QString(QLL("changed")), QVariantList(), &result
    ))->toBeFalsy();
}

/// Gives expressions the items of a list through `size`, `at()` and
/// `join()`, with no value for an index past the items.
void tst_ConfigParser::testArrayObj_shouldOfferItems()
{
    Config::ArrayObj array(QStringList() << QString(QLL("first"))
                           << QString(QLL("second")));
    QVariant result;
    qExpect(Config::Scopes::invoke(
        &array, QString(QLL("size")), QVariantList(), &result
    ))->toBeTruthy();
    qExpect(result.toInt())->toEqual(2);
    qExpect(Config::Scopes::invoke(
        &array, QString(QLL("join")),
        QVariantList() << QString(QLL(", ")), &result
    ))->toBeTruthy();
    qExpect(result.toString())->toEqual(QString(QLL("first, second")));
    qExpect(Config::Scopes::invoke(
        &array, QString(QLL("at")), QVariantList() << 1, &result
    ))->toBeTruthy();
    qExpect(result.toString())->toEqual(QString(QLL("second")));
    qExpect(Config::Scopes::invoke(
        &array, QString(QLL("at")), QVariantList() << 2, &result
    ))->toBeTruthy();
    qExpect(result.isValid())->toBeFalsy();
}

/// Leads an expression on to the Config::Object that a member answers,
/// whether Qt knows the member's type or not, and never to an address that
/// names no object; a condition takes no object as a number.
void tst_ConfigParser::testTypes_shouldReachReturnedObjects()
{
    const QSharedPointer<Config::Scopes::Scope > scope =
        Config::Scopes::push();
    scope->add(new TestHolderObject);
    const QStringList names = QStringList()
        << QString(QLL("items")) << QString(QLL("list"));
    for (int i = 0; i < names.size(); ++i) {
        QVariant answer;
        qExpect(Config::Scopes::answer(
            Config::Scopes::find(names.at(i)), Q_NULLPTR, QVariantList(),
            &answer
        ))->toBeTruthy();
        Config::Object *array = Config::Types::object(answer);
        qExpect(array)->Not->toBeNull();
        QVariant joined;
        qExpect(Config::Scopes::invoke(
            array, QString(QLL("join")),
            QVariantList() << QString(QLL("+")), &joined
        ))->toBeTruthy();
        qExpect(joined.toString())->toEqual(QString(QLL("first+second")));
    }

    // An address that names no `Config::Object` gives no answer.
    QVariant stray;
    qExpect(Config::Scopes::answer(
        Config::Scopes::find(QString(QLL("stray"))), Q_NULLPTR,
        QVariantList(), &stray
    ))->toBeFalsy();

    // A condition takes no object as a number.
    QStringList program;
    QString error;
    QVERIFY2(Config::Condition::compile(
        QString(QLL("items > 0")), QStringList(), &program, &error, true
    ), qPrintable(error));
    TestArgumentContext context;
    qreal result = 0;
    qExpect(Config::Condition::value(
        program, QSet<QString >(), &context, &result
    ))->toBeFalsy();
}

/// Offers the dynamic properties of an object as names, through
/// Config::Object::dynamicValue(), and leads a condition on with `.` to a
/// member of the object a name answers; a text such as `40%` answers a
/// percent, and a member it lacks fails to compile.
void tst_ConfigParser::testScopes_shouldOfferDynamicPropertiesAndPaths()
{
    const QSharedPointer<Config::Scopes::Scope > scope =
        Config::Scopes::push();
    scope->add(new TestDynamicObject);
    scope->add(new TestSettingsHolderObject);
    qExpect(Config::Scopes::find(QString(QLL("stored"))).isNull())->toBeFalsy();
    qExpect(Config::Scopes::names().contains(QString(QLL("placed"))))
        ->toBeTruthy();

    // Reads both properties, directly and through `settings`.
    TestArgumentContext context;
    const QStringList texts = QStringList()
        << QString(QLL("stored + placed"))
        << QString(QLL("settings.stored + settings.placed"));
    for (int i = 0; i < texts.size(); ++i) {
        QStringList program;
        QString error;
        QVERIFY2(Config::Condition::compile(
            texts.at(i), QStringList(), &program, &error, true
        ), qPrintable(error));
        qreal result = 0;
        qExpect(Config::Condition::value(
            program, QSet<QString >(), &context, &result
        ))->toBeTruthy();
        QCOMPARE(result, qreal(13));
    }

    // Keeps a percent a percent through math: `share * 0.875` holds past 35%.
    QStringList program;
    QString error;
    QVERIFY2(Config::Condition::compile(
        QString(QLL("share * 0.875")), QStringList(), &program, &error, true
    ), qPrintable(error));
    context.space = 64;
    qExpect(Config::Condition::evaluate(
        program, QSet<QString >(), &context
    ))->toBeTruthy();
    context.space = 66;
    qExpect(Config::Condition::evaluate(
        program, QSet<QString >(), &context
    ))->toBeFalsy();

    // A member the object lacks fails to compile.
    qExpect(Config::Condition::compile(
        QString(QLL("settings.missing > 1")), QStringList(), &program,
        &error, true
    ))->toBeFalsy();
}

/// Keeps the double quotes of a value whose matcher reads conditions, as
/// their texts need them, while the `;` after it still starts a comment.
void tst_ConfigParser::testParser_shouldKeepQuotesOfConditions()
{
    Config::Schema schema;
    Config::Section section(QString(QLL("Test")));
    section.addKey(
        QString(QLL("pick")),
        Config::Matchers() << QSharedPointer<const Config::ValueMatcher >(
            new Config::Choice(QStringList() << QString(QLL("ready")),
                               QStringList(), true)),
        QString()
    );
    section.addKey(QString(QLL("word")), QStringList() << QString(QLL("x y")),
                   QString());
    schema.addSection(section);
    Config::Parser parser(schema);
    QString error;
    QVERIFY2(parser.readData(
        "[Test]\npick = ready ? \"yes\" : \"no\" ; a note\nword = \"x y\"\n",
        QLL("inline"), &error
    ), qPrintable(error));
    QCOMPARE(parser.value(QString(QLL("test.pick"))),
             QString(QLL("ready ? \"yes\" : \"no\"")));
    QCOMPARE(parser.value(QString(QLL("test.word"))), QString(QLL("x y")));
}

/// Adds the objects of a setup function to the global scope, at once when
/// that scope is already in use.
void tst_ConfigParser::testScopes_shouldRunGlobalSetups()
{
    qExpect(Config::Scopes::find(QString(QLL("globalMark"))).isNull())
        ->toBeTruthy();
    Config::Scopes::addGlobalSetup(&addTestGlobalObject);
    const Config::Scopes::Member member =
        Config::Scopes::find(QString(QLL("globalMark")));
    qExpect(member.isNull())->toBeFalsy();
    QVariant answer;
    qExpect(Config::Scopes::answer(
        member,
        Q_NULLPTR,
        QVariantList(),
        &answer
    ))->toBeTruthy();
    qExpect(answer.toInt())->toEqual(7);
}

/// Finds the name that a name of another case means, and suggests it in
/// the spelling the condition uses (`-`, `_`, or none): `warn` logs the
/// suggestion and uses it, `ignore` uses it silently, while `error` and
/// `critical` fail with it as the error, and tell it was a name mismatch.
void tst_ConfigParser::testCondition_shouldHandleNameMismatch()
{
    const QStringList names = QStringList() << QString(QLL("is-header"));
    QStringList program;
    QString error;
    bool mismatched = false;

    // `warn` logs, then uses the name meant.
    QTest::ignoreMessage(QtWarningMsg,
        "failed to find the name is-heaDer (maybe is-header was meant).");
    qExpect(Config::Condition::compile(
        QString(QLL("is-heaDer")), names, &program, &error, false,
        Config::Condition::NameMismatchWarn, &mismatched
    ))->toBeTruthy();
    qExpect(mismatched)->toBeFalsy();
    qExpect(Config::Condition::evaluate(
        program, QSet<QString >() << QString(QLL("is-header"))
    ))->toBeTruthy();

    // The suggestion follows the spelling: no sign, then `_`.
    QTest::ignoreMessage(QtWarningMsg,
        "failed to find the name isHeAder (maybe isHeader was meant).");
    qExpect(Config::Condition::compile(
        QString(QLL("isHeAder")), names, &program, &error
    ))->toBeTruthy();
    QTest::ignoreMessage(QtWarningMsg,
        "failed to find the name is_heaDer (maybe is_header was meant).");
    qExpect(Config::Condition::compile(
        QString(QLL("is_heaDer")), names, &program, &error
    ))->toBeTruthy();

    // `ignore` uses the name meant without a word.
    qExpect(Config::Condition::compile(
        QString(QLL("Is-Header")), names, &program, &error, false,
        Config::Condition::NameMismatchIgnore
    ))->toBeTruthy();

    // `error` and `critical` fail with the suggestion.
    const Config::Condition::NameMismatch failing[] = {
        Config::Condition::NameMismatchError,
        Config::Condition::NameMismatchCritical
    };
    for (int i = 0; i < 2; ++i) {
        mismatched = false;
        qExpect(Config::Condition::compile(
            QString(QLL("is-heaDer")), names, &program, &error, false,
            failing[i], &mismatched
        ))->toBeFalsy();
        qExpect(mismatched)->toBeTruthy();
        qExpect(error)->toEqual(QString(QLL(
            "failed to find the name is-heaDer (maybe is-header was meant)."
        )));
    }

    // A member of the scopes is a function, and a name of no other case
    // stays unknown, with no suggestion.
    const QSharedPointer<Config::Scopes::Scope > scope = pushTestObjects();
    QTest::ignoreMessage(QtWarningMsg,
        "failed to find the function spaceremaining "
        "(maybe spaceRemaining was meant).");
    qExpect(Config::Condition::compile(
        QString(QLL("spaceremaining > 10")), names, &program, &error
    ))->toBeTruthy();
    mismatched = false;
    qExpect(Config::Condition::compile(
        QString(QLL("is-footer")), names, &program, &error, false,
        Config::Condition::NameMismatchError, &mismatched
    ))->toBeFalsy();
    qExpect(mismatched)->toBeFalsy();
    qExpect(int(Config::Condition::nameMismatch(QString(QLL("Critical")))))
        ->toEqual(int(Config::Condition::NameMismatchCritical));
    qExpect(int(Config::Condition::nameMismatch(QString(QLL("other")))))
        ->toEqual(int(Config::Condition::NameMismatchWarn));
}

/// Passes a string, in double quotes with the escapes of a C++ string
/// literal, as a call's argument; a string elsewhere gives no answer, and a
/// choice's `?` or `:` inside a string splits nothing.
void tst_ConfigParser::testCondition_shouldPassStrings()
{
    const QSharedPointer<Config::Scopes::Scope > scope = pushTestObjects();
    TestArgumentContext context;
    QStringList program;
    QString error;
    bool ok = false;
    QVERIFY2(Config::Condition::compile(
        QString(QLL("named(\"a b\") && 1")), QStringList(), &program, &error
    ), qPrintable(error));
    qExpect(Config::Condition::evaluate(program, QSet<QString >(), &context, &ok))
        ->toBeTruthy();
    qExpect(ok)->toBeTruthy();
    QVERIFY2(Config::Condition::compile(
        QString(QLL("named(\"a\\040b\")")), QStringList(), &program, &error
    ), qPrintable(error));
    qExpect(Config::Condition::evaluate(program, QSet<QString >(), &context, &ok))
        ->toBeTruthy();
    QVERIFY2(Config::Condition::compile(
        QString(QLL("named(\"other\")")), QStringList(), &program, &error
    ), qPrintable(error));
    qExpect(Config::Condition::evaluate(program, QSet<QString >(), &context, &ok))
        ->toBeFalsy();
    qExpect(ok)->toBeTruthy();

    // A string where a number belongs gives no answer.
    QVERIFY2(Config::Condition::compile(
        QString(QLL("\"a b\" == 1")), QStringList(), &program, &error
    ), qPrintable(error));
    Config::Condition::evaluate(program, QSet<QString >(), &context, &ok);
    qExpect(ok)->toBeFalsy();

    // A choice splits outside its strings alone.
    QString picked;
    QVERIFY2(Config::Choice::resolve(
        QString(QLL("named(\"a?b:c\") ? yes : no")), QStringList(),
        QStringList() << QString(QLL("yes")) << QString(QLL("no")),
        &context, &picked, &error
    ), qPrintable(error));
    qExpect(picked)->toEqual(QString(QLL("no")));
}

QTEST_APPLESS_MAIN(tst_ConfigParser)
#include "tst_config_parser.moc"
