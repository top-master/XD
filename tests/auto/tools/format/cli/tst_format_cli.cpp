// Copyright (C) 2015 The XD Company Ltd.
// SPDX-License-Identifier: Apache-2.0

#include <QtCore/QByteArray>
#include <QtCore/QDir>
#include <QtCore/QFile>
#include <QtCore/QFileInfo>
#include <QtCore/QProcess>
#include <QtCore/QTemporaryDir>
#include <QtTest/QtTest>

/// Runs the built `format` tool with command lines, and checks what it
/// prints and its exit code; the rules themselves are tested elsewhere.
class tst_FormatCli : public QObject
{
    Q_OBJECT
    typedef QObject super;
public:
    tst_FormatCli();

private slots:
    void initTestCase();

    /// Command line.
    void testHelp_shouldListEveryOption();
    void testVersion_shouldSucceed();
    void testNoPaths_shouldFailWithUsageError();
    void testListRules_shouldSucceed();
    void testRules_shouldRejectUnknownName();
    void testJobs_shouldRejectNonPositiveCount_data();
    void testJobs_shouldRejectNonPositiveCount();
    void testRunner_shouldRejectUnknownKind();

    /// Paths.
    void testTrailingSlash_shouldFormatDirectory_data();
    void testTrailingSlash_shouldFormatDirectory();
    void testTrailingSlash_shouldRejectFile();
    void testPlainPath_shouldRejectDirectory();
    void testDirOption_shouldFormatDirectoryWithoutSlash();
    void testDirOption_shouldFormatNamedIgnoredFolder();
    void testPlainPath_shouldWarnAboutIgnoredFile();
    void testPlainPath_shouldStopWhenTheAnswerIsNo();
    void testHeadless_shouldFormatIgnoredFileSilently();
    void testFormatrc_shouldRejectUnknownValue();
    void testConfig_shouldOverrideFormatrc_data();
    void testConfig_shouldOverrideFormatrc();
    void testConfig_shouldRejectUnknownSetting();
    void testConfigFile_shouldReplaceEveryFormatrc();
    void testConfigAdd_shouldOverrideOnlyItsKeys();
    void testAgent_shouldAskAndFormatChangedFilesAgain();
    void testAgent_shouldStopWhenNothingChanged();
    void testAgent_shouldHandPromptFileAndRemoveIt();
    void testAgent_shouldRejectCheck();
    void testAgent_shouldKeepFilesWhenAskingNothing();
    void testAgent_shouldAskAboutIgnoredPathOnHashLine();
    void testInclude_shouldFindClassHeader();
    void testSkipIgnored_shouldDropIgnoredNamedFile();
    void testProject_shouldFormatItsFiles();
    void testThisMode_shouldLeaveOwnMembersOfSourceBare();
    void testSpecs_shouldFormatFilesOfEverySpec();
    void testAnyComment_shouldRewrapShortComments();
    void testDirOption_shouldRejectFile();

    /// Runners.
    void testRunner_shouldReportEachFile_data();
    void testRunner_shouldReportEachFile();
    void testRunner_shouldPassEmptyFolderQuietly_data();
    void testRunner_shouldPassEmptyFolderQuietly();

    /// Line ranges.
    void testLineRange_shouldAcceptGitBlameForms_data();
    void testLineRange_shouldAcceptGitBlameForms();
    void testLineRange_shouldRejectBadRange_data();
    void testLineRange_shouldRejectBadRange();

private:
    int run(const QStringList & arguments, QByteArray *out,
            QByteArray *err) const;
    int runWithInput(const QStringList & arguments, const QByteArray & input,
                     QByteArray *out, QByteArray *err) const;
    QString writeFile(const QString & relative, const QByteArray & text) const;
    QByteArray readFile(const QString & path) const;
    bool waitForError(QProcess *process, QByteArray *err, const QByteArray & text, int count) const;
    QString agentUsage(const QByteArray & err) const;
    QStringList workerFiles(const QByteArray & out) const;
    bool hasRunner(const QString & kind) const;

    /// Gives the tests a folder for the files they format, and removes it
    /// once the tests have ended.
    QTemporaryDir m_dir;
    /// Points at the built tool the tests run.
    QString m_format;
};

tst_FormatCli::tst_FormatCli()
    : m_format(QString::fromLocal8Bit(FORMAT_BINS) + QLL("/format"))
{
}

/// Reads back the file at `path`, as the tool left it.
QByteArray tst_FormatCli::readFile(const QString & path) const
{
    QFile input(path);
    return input.open(QIODevice::ReadOnly) ? input.readAll() : QByteArray();
}

/// Reads the output of `process` (its standard output and error, merged)
/// into `err` until `text` shows up in it `count` times, or the process
/// ends; tells whether it did.
bool tst_FormatCli::waitForError(QProcess *process, QByteArray *err, const QByteArray & text,
                                 int count) const
{
    while (err->count(text) < count) {
        if (!process->waitForReadyRead(120000)) {
            *err += process->readAll();
            return err->count(text) >= count;
        }
        *err += process->readAll();
    }
    return true;
}

/// Runs the tool with `arguments`, gathers its standard output into `out`
/// and its standard error into `err`, and returns its exit code (-1 when it
/// crashed or did not end in time).
int tst_FormatCli::run(const QStringList & arguments, QByteArray *out,
                       QByteArray *err) const
{
    return this->runWithInput(arguments, QByteArray(), out, err);
}

/// Runs the tool just like run(), with `input` as its standard input; the
/// input then ends, as a closed terminal would.
int tst_FormatCli::runWithInput(const QStringList & arguments, const QByteArray & input,
                                QByteArray *out, QByteArray *err) const
{
    QProcess process;
    process.start(this->m_format, arguments);
    process.write(input);
    process.closeWriteChannel();
    if (!process.waitForFinished(120000)) {
        process.kill();
        process.waitForFinished();
        return -1;
    }
    *out = process.readAllStandardOutput();
    *err = process.readAllStandardError();
    return process.exitStatus() == QProcess::NormalExit ? process.exitCode() : -1;
}

/// Writes `text` into the file `relative` under the test's directory,
/// creating its folders, and returns the file's path.
QString tst_FormatCli::writeFile(const QString & relative,
                                 const QByteArray & text) const
{
    const QString path = this->m_dir.path() + QLatin1Char('/') + relative;
    QDir().mkpath(QFileInfo(path).absolutePath());
    QFile file(path);
    if (file.open(QIODevice::WriteOnly)) {
        file.write(text);
    }
    return path;
}

/// Builds the path of the usage instructions that the line `Read this
/// tool's usage instructions at: "<path>"` of `err` names, or an empty path
/// without that line.
QString tst_FormatCli::agentUsage(const QByteArray & err) const
{
    const QByteArray marker = "Read this tool's usage instructions at: \"";
    const int at = err.indexOf(marker);
    if (at < 0) {
        return QString();
    }
    QByteArray path = err.mid(at + marker.size());
    path = path.left(path.indexOf('"'));
    return QString::fromLocal8Bit(path);
}

/// Lists the files that a `--worker` run printed results for, in order.
QStringList tst_FormatCli::workerFiles(const QByteArray & out) const
{
    QStringList files;
    const QList<QByteArray > lines = out.split('\n');
    for (int i = 0; i < lines.size(); ++i) {
        const QList<QByteArray > fields = lines.at(i).split('\t');
        if (fields.size() == 4 && fields.at(0) == "file") {
            files << QString::fromUtf8(QByteArray::fromPercentEncoding(fields.at(1)));
        }
    }
    return files;
}

/// Tells whether this build of the tool has the runner `kind`, as its
/// `--help` lists the kinds it has.
bool tst_FormatCli::hasRunner(const QString & kind) const
{
    QByteArray out;
    QByteArray err;
    this->run(QStringList() << QString(QLL("--help")), &out, &err);
    const int at = out.indexOf("--runner");
    return at >= 0 && out.mid(at, out.indexOf('\n', out.indexOf(';', at)) - at).contains(kind.toLatin1());
}

void tst_FormatCli::initTestCase()
{
    qExpect(QFileInfo(this->m_format).isExecutable()
            || QFileInfo(this->m_format + QLL(".exe")).isExecutable())->toBeTruthy();
    qExpect(this->m_dir.isValid())->toBeTruthy();
    this->writeFile(QString(QLL("tree/a.cpp")), "int a;\n");
    this->writeFile(QString(QLL("tree/sub/b.h")), "int b;\n");
    this->writeFile(QString(QLL("tree/sub/g.m")), "int g;\n");
    this->writeFile(QString(QLL("tree/notes.txt")), "not code\n");
    this->writeFile(QString(QLL("tree/.hidden/c.cpp")), "int c;\n");
    this->writeFile(QString(QLL("tree/.d.cpp")), "int d;\n");
    this->writeFile(QString(QLL("tree/3rdparty/lib/e.cpp")), "int e;\n");
    this->writeFile(QString(QLL("tree/.git/f.cpp")), "int f;\n");
    this->writeFile(QString(QLL("tree/.gitignore")), ".hidden/\n");
    this->writeFile(QString(QLL("tree/.formatignore")), "3rdparty/\n");
    this->writeFile(QString(QLL("lines.cpp")),
                    "int main()\n{\n    return foo;\n}\n\nint other()\n{\n    return 0;\n}\n");
}

void tst_FormatCli::testHelp_shouldListEveryOption()
{
    QByteArray out;
    QByteArray err;
    qExpect(this->run(QStringList() << QString(QLL("--help")), &out, &err))->toEqual(0);
    const char *options[] = { "-L, --range <range>", "--dir <path>", "--check", "--rules <names>",
                              "--list-rules", "--jobs <count>", "--runner <kind>", "--headless", "--worker",
                              "--config <setting>", "--config-file <file>",
                              "--config-add <file>", "--ai", "-I, -i <dir>",
                              "--skip-ignored", "-p, --project <file>", "--specs <names>",
                              "--qmake <path>", "--any-comment" };
    for (size_t i = 0; i < sizeof(options) / sizeof(options[0]); ++i) {
        qExpect(out)->toContain(QByteArray(options[i]));
    }
}

void tst_FormatCli::testVersion_shouldSucceed()
{
    QByteArray out;
    QByteArray err;
    qExpect(this->run(QStringList() << QString(QLL("--version")), &out, &err))->toEqual(0);
    qExpect(out)->toContain(QByteArray("format"));
}

void tst_FormatCli::testNoPaths_shouldFailWithUsageError()
{
    QByteArray out;
    QByteArray err;
    qExpect(this->run(QStringList(), &out, &err))->toEqual(2);
    qExpect(err)->toContain(QByteArray("no files given"));
}

void tst_FormatCli::testListRules_shouldSucceed()
{
    QByteArray out;
    QByteArray err;
    qExpect(this->run(QStringList() << QString(QLL("--list-rules")), &out, &err))->toEqual(0);
    qExpect(err)->toBeEmpty();
}

void tst_FormatCli::testRules_shouldRejectUnknownName()
{
    QByteArray out;
    QByteArray err;
    const QStringList arguments = QStringList() << QString(QLL("--rules=no-such-rule"))
        << this->m_dir.path() + QLL("/lines.cpp");
    qExpect(this->run(arguments, &out, &err))->toEqual(2);
    qExpect(err)->toContain(QByteArray("unknown rule: no-such-rule"));
}

void tst_FormatCli::testJobs_shouldRejectNonPositiveCount_data()
{
    QTest::addColumn<QString >("count");
    QTest::newRow("zero") << QString(QLL("0"));
    QTest::newRow("negative") << QString(QLL("-3"));
    QTest::newRow("word") << QString(QLL("many"));
}

void tst_FormatCli::testJobs_shouldRejectNonPositiveCount()
{
    QFETCH(QString, count);
    QByteArray out;
    QByteArray err;
    const QStringList arguments = QStringList() << QString(QLL("--jobs")) << count
        << this->m_dir.path() + QLL("/lines.cpp");
    qExpect(this->run(arguments, &out, &err))->toEqual(2);
    qExpect(err)->toContain(QByteArray("not a job count: ") + count.toLatin1());
}

void tst_FormatCli::testRunner_shouldRejectUnknownKind()
{
    QByteArray out;
    QByteArray err;
    const QStringList arguments = QStringList() << QString(QLL("--runner=fiber"))
        << this->m_dir.path() + QLL("/lines.cpp");
    qExpect(this->run(arguments, &out, &err))->toEqual(2);
    qExpect(err)->toContain(QByteArray("unknown runner: fiber"));
}

void tst_FormatCli::testTrailingSlash_shouldFormatDirectory_data()
{
    QTest::addColumn<QString >("separator");
    QTest::newRow("slash") << QString(QLL("/"));
    QTest::newRow("backslash") << QString(QLL("\\"));
}

void tst_FormatCli::testTrailingSlash_shouldFormatDirectory()
{
    QFETCH(QString, separator);
    QByteArray out;
    QByteArray err;
    const QString tree = this->m_dir.path() + QLL("/tree");
    const QStringList arguments = QStringList() << QString(QLL("--worker"))
        << tree + separator;
    qExpect(this->run(arguments, &out, &err))->toEqual(0);
    qExpect(err)->toBeEmpty();
    const QStringList expected = QStringList() << tree + QLL("/.d.cpp") << tree + QLL("/a.cpp")
        << tree + QLL("/sub/b.h") << tree + QLL("/sub/g.m");
    qExpect(this->workerFiles(out))->toEqual(expected);
}

void tst_FormatCli::testTrailingSlash_shouldRejectFile()
{
    QByteArray out;
    QByteArray err;
    const QString path = this->m_dir.path() + QLL("/lines.cpp/");
    qExpect(this->run(QStringList() << path, &out, &err))->toEqual(2);
    qExpect(err)->toContain(QByteArray("not a directory: ") + path.toLocal8Bit());
}

void tst_FormatCli::testPlainPath_shouldRejectDirectory()
{
    QByteArray out;
    QByteArray err;
    const QString tree = this->m_dir.path() + QLL("/tree");
    qExpect(this->run(QStringList() << tree, &out, &err))->toEqual(2);
    qExpect(err)->toContain(QByteArray("is a directory, end it with / or pass --dir"));
}

void tst_FormatCli::testDirOption_shouldFormatDirectoryWithoutSlash()
{
    QByteArray out;
    QByteArray err;
    const QString tree = this->m_dir.path() + QLL("/tree");
    const QStringList arguments = QStringList() << QString(QLL("--worker"))
        << QString(QLL("--dir")) << tree;
    qExpect(this->run(arguments, &out, &err))->toEqual(0);
    const QStringList expected = QStringList() << tree + QLL("/.d.cpp") << tree + QLL("/a.cpp")
        << tree + QLL("/sub/b.h") << tree + QLL("/sub/g.m");
    qExpect(this->workerFiles(out))->toEqual(expected);
}

void tst_FormatCli::testDirOption_shouldFormatNamedIgnoredFolder()
{
    QByteArray out;
    QByteArray err;
    const QString folder = this->m_dir.path() + QLL("/tree/3rdparty");
    const QStringList arguments = QStringList() << QString(QLL("--worker"))
        << folder + QLL("/");
    qExpect(this->run(arguments, &out, &err))->toEqual(0);
    QCOMPARE(this->workerFiles(out), QStringList() << folder + QLL("/lib/e.cpp"));
}

/// Names a file that the ignore files leave out, without `--headless`: the
/// tool warns with the rule, asks, and an ended input counts as yes.
void tst_FormatCli::testPlainPath_shouldWarnAboutIgnoredFile()
{
    QByteArray out;
    QByteArray err;
    const QString file = this->m_dir.path() + QLL("/tree/.hidden/c.cpp");
    qExpect(this->run(QStringList() << file, &out, &err))->toEqual(0);
    qExpect(err)->toContain(QByteArray("\nWARNING: "));
    qExpect(err)->toContain(QByteArray(".gitignore:1: .hidden/"));
    qExpect(err)->toContain(QByteArray("[Y/n] "));
}

void tst_FormatCli::testPlainPath_shouldStopWhenTheAnswerIsNo()
{
    QByteArray out;
    QByteArray err;
    const QString file = this->m_dir.path() + QLL("/tree/.hidden/c.cpp");
    qExpect(this->runWithInput(QStringList() << file, "n\n", &out, &err))->toEqual(2);
    qExpect(err)->toContain(QByteArray("format: stopped: a named path is left out"));
    qExpect(err)->Not->toContain(QByteArray("#: "));
    qExpect(out)->toBeEmpty();
}

void tst_FormatCli::testHeadless_shouldFormatIgnoredFileSilently()
{
    QByteArray out;
    QByteArray err;
    const QString file = this->m_dir.path() + QLL("/tree/.hidden/c.cpp");
    const QStringList arguments = QStringList() << QString(QLL("--headless"))
        << QString(QLL("--worker")) << file;
    qExpect(this->run(arguments, &out, &err))->toEqual(0);
    qExpect(err)->toBeEmpty();
    QCOMPARE(this->workerFiles(out), QStringList() << file);
}

void tst_FormatCli::testFormatrc_shouldRejectUnknownValue()
{
    this->writeFile(QString(QLL("rc/.git/HEAD")), "ref: refs/heads/main\n");
    this->writeFile(QString(QLL("rc/.formatrc")), "[Types]\nmoveSuffix = wide\n");
    const QString file = this->writeFile(QString(QLL("rc/a.cpp")), "int a;\n");
    QByteArray out;
    QByteArray err;
    qExpect(this->run(QStringList() << file, &out, &err))->toEqual(2);
    qExpect(err)->toContain(QByteArray("a.cpp: error: "));
    qExpect(err)->toContain(QByteArray(".formatrc: [Types] moveSuffix = wide: use"));
}

/// Runs a `.formatrc` project whose `refSuffix` keeps the spacing found,
/// with each spelling of the `-c` option.
void tst_FormatCli::testConfig_shouldOverrideFormatrc_data()
{
    QTest::addColumn<QStringList >("arguments");
    QTest::newRow("short with =") << (QStringList() << QString(QLL("-c=types.refsuffix=none")));
    QTest::newRow("short") << (QStringList() << QString(QLL("-c"))
                                             << QString(QLL("Types.RefSuffix=\"\"")));
    QTest::newRow("long") << (QStringList() << QString(QLL("--config=TYPES.refSuffix=none")));
    QTest::newRow("regexp") << (QStringList() << QString(QLL("-c"))
        << QString(QLL("types.refSuffix=regexp/int & a/int &a/ | keep-as-is")));
    QTest::newRow("process runner") << (QStringList() << QString(QLL("--runner=process"))
                                                      << QString(QLL("--config"))
                                                      << QString(QLL("types.refsuffix=none")));
}

void tst_FormatCli::testConfig_shouldOverrideFormatrc()
{
    QFETCH(QStringList, arguments);
    if (arguments.contains(QString(QLL("--runner=process")))
        && !this->hasRunner(QString(QLL("process")))
    ) {
        QSKIP("This build has no process runner.");
    }
    this->writeFile(QString(QLL("cfg/.git/HEAD")), "ref: refs/heads/main\n");
    this->writeFile(QString(QLL("cfg/.formatrc")), "[Types]\nrefSuffix = keep-as-is\n");
    const QString file = this->writeFile(QString(QLL("cfg/a.cpp")),
                                         "void f(const int & a);\n");
    QByteArray out;
    QByteArray err;
    qExpect(this->run(arguments << QString(QLL("--headless")) << file, &out, &err))->toEqual(1);
    qExpect(this->readFile(file))->toEqual(QByteArray("void f(const int &a);\n"));
}

void tst_FormatCli::testConfig_shouldRejectUnknownSetting()
{
    QByteArray out;
    QByteArray err;
    const QString file = this->writeFile(QString(QLL("cfg-bad/a.cpp")), "int a;\n");
    const QStringList arguments = QStringList() << QString(QLL("-c=types.width=1")) << file;
    qExpect(this->run(arguments, &out, &err))->toEqual(2);
    qExpect(err)->toContain(QByteArray("format: -c unknown setting types.width"));
    qExpect(this->readFile(file))->toEqual(QByteArray("int a;\n"));
}

/// Gives a project a `.formatrc` with a value the tool does not know, which
/// `--config-file` keeps from being read at all.
void tst_FormatCli::testConfigFile_shouldReplaceEveryFormatrc()
{
    this->writeFile(QString(QLL("cfg-file/.git/HEAD")), "ref: refs/heads/main\n");
    this->writeFile(QString(QLL("cfg-file/.formatrc")), "[Types]\nrefSuffix = wide\n");
    const QString settings = this->writeFile(QString(QLL("cfg-file.ini")),
                                             "[Types]\nrefSuffix = none\n");
    const QString file = this->writeFile(QString(QLL("cfg-file/a.cpp")),
                                         "void f(const int & a);\n");
    QByteArray out;
    QByteArray err;
    const QStringList arguments = QStringList() << QString(QLL("--config-file")) << settings
        << file;
    qExpect(this->run(arguments, &out, &err))->toEqual(1);
    qExpect(this->readFile(file))->toEqual(QByteArray("void f(const int &a);\n"));
}

void tst_FormatCli::testConfigAdd_shouldOverrideOnlyItsKeys()
{
    this->writeFile(QString(QLL("cfg-add/.git/HEAD")), "ref: refs/heads/main\n");
    this->writeFile(QString(QLL("cfg-add/.formatrc")), "[Types]\nrefSuffix = none\n");
    const QString settings = this->writeFile(QString(QLL("cfg-add.ini")),
                                             "[Types]\nmoveSuffix = \" \"\n");
    const QString file = this->writeFile(QString(QLL("cfg-add/a.cpp")),
                                         "void f(const int & a, int &&b);\n");
    QByteArray out;
    QByteArray err;
    const QStringList arguments = QStringList() << QString(QLL("--config-add=") + settings) << file;
    qExpect(this->run(arguments, &out, &err))->toEqual(1);
    qExpect(this->readFile(file))->toEqual(QByteArray("void f(const int &a, int && b);\n"));
}

/// Runs `--ai` on a file whose long `<<` line the tool breaks: the tool asks
/// the agent to regroup the lines, the agent rewrites the file while the tool
/// waits, and after the `q` the tool formats the file again, and asks again.
void tst_FormatCli::testAgent_shouldAskAndFormatChangedFilesAgain()
{
    this->writeFile(QString(QLL("agent/.git/HEAD")), "ref: refs/heads/main\n");
    const QString file = this->writeFile(QString(QLL("agent/a.cpp")), "void f()\n{\n    out << \"label\" << firstValue << secondValue << thirdValue << fourthValue << fifthValue << sixthValue;\n}\n");
    QProcess process;
    process.setProcessChannelMode(QProcess::MergedChannels);
    process.start(this->m_format, QStringList() << QString(QLL("--ai"))
                  << QString(QLL("--headless")) << file);
    QByteArray err;
    qExpect(this->waitForError(&process, &err, "(type q to continue)", 1))->toBeTruthy();
    qExpect(err)->toContain(QByteArray("a.cpp:3-4: operator-wrap: Run prompt at: \""));
    this->writeFile(QString(QLL("agent/a.cpp")), "void f()\n{\n    out << \"other\" << firstValue << secondValue << thirdValue << fourthValue << fifthValue << sixthValue;\n}\n");
    process.write("q\n");
    qExpect(this->waitForError(&process, &err, "(type q to continue)", 2))->toBeTruthy();
    process.write("q\n");
    process.closeWriteChannel();
    qExpect(process.waitForFinished(120000))->toBeTruthy();
    err += process.readAll();
    qExpect(err.count(": formatted"))->toEqual(2);
    qExpect(process.exitCode())->toEqual(1);
    qExpect(this->readFile(file))->toContain(QByteArray("\"other\""));
}

/// Answers `q` without changing the file: the tool formats nothing again.
void tst_FormatCli::testAgent_shouldStopWhenNothingChanged()
{
    this->writeFile(QString(QLL("agent-same/.git/HEAD")), "ref: refs/heads/main\n");
    const QString file = this->writeFile(QString(QLL("agent-same/a.cpp")), "void f()\n{\n    out << \"label\" << firstValue << secondValue << thirdValue << fourthValue << fifthValue << sixthValue;\n}\n");
    QByteArray out;
    QByteArray err;
    const QStringList arguments = QStringList() << QString(QLL("--ai"))
        << QString(QLL("--headless")) << file;
    qExpect(this->runWithInput(arguments, "q\n", &out, &err))->toEqual(1);
    qExpect(err.count("(type q to continue)"))->toEqual(1);
    qExpect(out.count(": formatted"))->toEqual(1);
}

/// Runs `--ai` on a call the tool breaks after its `(`: the tool first names
/// its usage instructions, then a request names a prompt file in the same
/// temporary folder, filled with the call's lines and the column limit; the
/// tool removes both files once the agent quits it.
void tst_FormatCli::testAgent_shouldHandPromptFileAndRemoveIt()
{
    this->writeFile(QString(QLL("agent-prompt/.git/HEAD")), "ref: refs/heads/main\n");
    const QString file = this->writeFile(
        QString(QLL("agent-prompt/a.cpp")),
        "void f()\n{\n    run(firstArgumentName, secondArgumentName, thirdArgumentName, fourthArgumentName);\n}\n"
    );
    QProcess process;
    process.setProcessChannelMode(QProcess::MergedChannels);
    process.start(this->m_format, QStringList() << QString(QLL("--ai"))
                  << QString(QLL("--headless")) << QString(QLL("--rules=args-wrap")) << file);
    QByteArray err;
    qExpect(this->waitForError(&process, &err, "(type q to continue)", 1))->toBeTruthy();
    const QByteArray marker = "a.cpp:3-8: args-wrap: Run prompt at: \"";
    const int at = err.indexOf(marker);
    qExpect(at)->toBeGreaterOrEqual(0);
    QByteArray prompt = err.mid(at + marker.size());
    prompt = prompt.left(prompt.indexOf('"'));
    const QString path = QString::fromLocal8Bit(prompt);
    const QByteArray usageMarker = "Read this tool's usage instructions at: \"";
    const int usageAt = err.indexOf(usageMarker);
    qExpect(usageAt)->toBeGreaterOrEqual(0);
    qExpect(usageAt)->toBeLessThan(at);
    QByteArray usage = err.mid(usageAt + usageMarker.size());
    usage = usage.left(usage.indexOf('"'));
    const QString usagePath = QString::fromLocal8Bit(usage);
    qExpect(QFile::exists(usagePath))->toBeTruthy();
    QFile guide(path);
    qExpect(guide.open(QIODevice::ReadOnly))->toBeTruthy();
    const QByteArray text = guide.readAll();
    guide.close();
    qExpect(text)->toContain(QByteArray("3-8"));
    qExpect(text)->toContain(QByteArray("within 80 columns"));
    process.write("q\n");
    process.closeWriteChannel();
    qExpect(process.waitForFinished(120000))->toBeTruthy();
    qExpect(process.exitCode())->toEqual(1);
    qExpect(QFile::exists(path))->toBeFalsy();
    qExpect(QFile::exists(usagePath))->toBeFalsy();
}

void tst_FormatCli::testAgent_shouldRejectCheck()
{
    QByteArray out;
    QByteArray err;
    const QString file = this->writeFile(QString(QLL("agent-check/a.cpp")), "int a;\n");
    const QStringList arguments = QStringList() << QString(QLL("--ai")) << QString(QLL("--check"))
        << file;
    qExpect(this->run(arguments, &out, &err))->toEqual(2);
    qExpect(err)->toContain(QByteArray("--ai cannot run with --check"));
    QDir(QFileInfo(this->agentUsage(err)).absolutePath()).removeRecursively();
}

/// Runs `--ai` on a file that needs nothing: the tool asks nothing, and
/// keeps its usage instructions for the agent to read after it ended; the
/// next run with `--ai` removes that folder.
void tst_FormatCli::testAgent_shouldKeepFilesWhenAskingNothing()
{
    this->writeFile(QString(QLL("agent-keep/.git/HEAD")), "ref: refs/heads/main\n");
    const QString file = this->writeFile(
        QString(QLL("agent-keep/a.cpp")),
        "int a;\n"
    );
    const QStringList arguments = QStringList() << QString(QLL("--ai"))
        << QString(QLL("--headless")) << file;
    QByteArray out;
    QByteArray err;
    qExpect(this->run(arguments, &out, &err))->toEqual(0);
    const QString usage = this->agentUsage(err);
    qExpect(usage)->Not->toBeEmpty();
    qExpect(QFile::exists(usage))->toBeTruthy();
    const QString first = QFileInfo(usage).absolutePath();
    qExpect(QFile::exists(first + QLL("/.lock")))->toBeFalsy();
    qExpect(this->run(arguments, &out, &err))->toEqual(0);
    const QString second = QFileInfo(this->agentUsage(err)).absolutePath();
    qExpect(QDir(first).exists())->toBeFalsy();
    qExpect(QDir(second).exists())->toBeTruthy();
    QDir(second).removeRecursively();
}

/// Names a file that the ignore files leave out, with `--ai`: the tool
/// warns, then asks on a `#:` line, after the line that names its usage
/// instructions; the answer `n` stops it.
void tst_FormatCli::testAgent_shouldAskAboutIgnoredPathOnHashLine()
{
    QByteArray out;
    QByteArray err;
    const QString file = this->m_dir.path() + QLL("/tree/.hidden/c.cpp");
    const QStringList arguments = QStringList() << QString(QLL("--ai")) << file;
    qExpect(this->runWithInput(arguments, "n\n", &out, &err))->toEqual(2);
    const int usage = err.indexOf("Read this tool's usage instructions at: \"");
    const int question = err.indexOf("#: Format the named paths that the ignore files leave out "
                                     "anyway? [Y/n] ");
    qExpect(usage)->toEqual(0);
    qExpect(question)->toBeGreaterThan(usage);
    qExpect(err)->toContain(QByteArray("\nWARNING: "));
    QDir(QFileInfo(this->agentUsage(err)).absolutePath()).removeRecursively();
}

/// Formats an outline member definition whose class sits in a header of an
/// include folder: only with `-I` does the tool find the class, and reach the
/// member through `this->`.
void tst_FormatCli::testInclude_shouldFindClassHeader()
{
    this->writeFile(QString(QLL("inc-run/.git/HEAD")), "ref: refs/heads/main\n");
    this->writeFile(QString(QLL("inc-run/inc/widget.h")),
                    "class Widget\n{\npublic:\n    void run();\n    int count;\n};\n");
    const QByteArray source = "#include \"widget.h\"\n\nvoid Widget::run()\n{\n    count = 1;\n}\n";
    const QString file = this->writeFile(QString(QLL("inc-run/src/widget.cpp")), source);
    QByteArray out;
    QByteArray err;
    const QStringList plain = QStringList() << QString(QLL("--headless"))
        << QString(QLL("--rules=outline-this")) << QString(QLL("-c=relations.thisMode=always"))
        << file;
    qExpect(this->run(plain, &out, &err))->toEqual(0);
    qExpect(this->readFile(file))->toEqual(source);
    const QStringList included = QStringList() << QString(QLL("--headless"))
        << QString(QLL("--rules=outline-this")) << QString(QLL("-c=relations.thisMode=always"))
        << QString(QLL("-I"))
        << this->m_dir.path() + QLL("/inc-run/inc") << file;
    qExpect(this->run(included, &out, &err))->toEqual(1);
    qExpect(this->readFile(file))->toContain(QByteArray("    this->count = 1;"));
}

/// Names a file that `.formatignore` leaves out, with `--skip-ignored`: the
/// tool skips it, asks nothing, and exits clean.
void tst_FormatCli::testSkipIgnored_shouldDropIgnoredNamedFile()
{
    this->writeFile(QString(QLL("skip-run/.git/HEAD")), "ref: refs/heads/main\n");
    this->writeFile(QString(QLL("skip-run/.formatignore")), "vendor/\n");
    const QByteArray source = "int  a ;\n// lower case.\n";
    const QString file = this->writeFile(QString(QLL("skip-run/vendor/a.cpp")), source);
    QByteArray out;
    QByteArray err;
    const QStringList arguments = QStringList() << QString(QLL("--skip-ignored")) << file;
    qExpect(this->run(arguments, &out, &err))->toEqual(0);
    qExpect(err)->toBeEmpty();
    qExpect(this->readFile(file))->toEqual(source);
}

/// Formats a qmake project through the subdirs project that holds it: the
/// source it lists, with the class header found through its `INCLUDEPATH`; a
/// file that `.formatignore` leaves out stays as it is.
/// Formats a copy of a real source and header pair (QHostInfo's): the
/// default `thisMode` adds no `this->` to a member of the source's own class,
/// such as `resultEmitter`, and keeps every `this->` the source already has;
/// `thisMode = always` then adds it.
void tst_FormatCli::testThisMode_shouldLeaveOwnMembersOfSourceBare()
{
    const QString kernel = QString::fromLocal8Bit(FORMAT_SOURCE_TREE)
        + QLL("/src/network/kernel/");
    QByteArray source;
    QByteArray header;
    {
        QFile sourceFile(kernel + QLL("qhostinfo.cpp"));
        QFile headerFile(kernel + QLL("qhostinfo_p.h"));
        QVERIFY(sourceFile.open(QIODevice::ReadOnly));
        QVERIFY(headerFile.open(QIODevice::ReadOnly));
        source = sourceFile.readAll();
        header = headerFile.readAll();
    }
    this->writeFile(QString(QLL("real/.git/HEAD")), "ref: refs/heads/main\n");
    this->writeFile(QString(QLL("real/qhostinfo_p.h")), header);
    const QString file = this->writeFile(
        QString(QLL("real/qhostinfo.cpp")),
        source
    );
    QByteArray out;
    QByteArray err;
    const QStringList arguments = QStringList() << QString(QLL("--headless"))
        << QString(QLL("--rules=outline-this")) << file;
    qExpect(this->run(arguments, &out, &err))->Not->toEqual(2);
    qExpect(err)->toBeEmpty();
    const QByteArray formatted = this->readFile(file);
    qExpect(formatted)->Not->toContain(QByteArray("this->resultEmitter"));
    const QList<QByteArray > lines = source.split('\n');
    for (int i = 0; i < lines.size(); ++i) {
        if (lines.at(i).contains("this->")) {
            qExpect(formatted)->toContain(lines.at(i));
        }
    }
    this->writeFile(QString(QLL("real/qhostinfo.cpp")), source);
    qExpect(this->run(QStringList(arguments) << QString(QLL("-c=relations.thisMode=always")),
                      &out, &err))->toEqual(1);
    qExpect(this->readFile(file))->toContain(QByteArray("this->resultEmitter"));
}

void tst_FormatCli::testProject_shouldFormatItsFiles()
{
    this->writeFile(QString(QLL("project-run/.git/HEAD")), "ref: refs/heads/main\n");
    this->writeFile(QString(QLL("project-run/.formatignore")), "vendor/\n");
    this->writeFile(QString(QLL("project-run/inc/widget.h")),
                    "class Widget\n{\npublic:\n    void run();\n    int count;\n};\n");
    const QString source = this->writeFile(QString(QLL("project-run/src/widget.cpp")),
        "#include \"widget.h\"\n\nvoid Widget::run()\n{\n    count = 1;\n}\n");
    const QByteArray vendored = "void f()\n{\n    if (a) b();\n}\n";
    const QString vendor = this->writeFile(QString(QLL("project-run/vendor/v.cpp")), vendored);
    this->writeFile(QString(QLL("project-run/app/app.pro")),
        "SOURCES = ../src/widget.cpp ../vendor/v.cpp\nHEADERS = ../inc/widget.h\n"
        "INCLUDEPATH = ../inc\n");
    const QString project = this->writeFile(QString(QLL("project-run/top.pro")),
                                            "TEMPLATE = subdirs\nSUBDIRS = app\n");
    QByteArray out;
    QByteArray err;
    const QStringList arguments = QStringList() << QString(QLL("--rules=outline-this,control-braces"))
        << QString(QLL("-c=relations.thisMode=always")) << QString(QLL("--project=") + project);
    qExpect(this->run(arguments, &out, &err))->toEqual(1);
    qExpect(this->readFile(source))->toContain(QByteArray("    this->count = 1;"));
    qExpect(this->readFile(vendor))->toEqual(vendored);
    qExpect(err)->toBeEmpty();
}

/// Reads a project whose platform scopes list one file each: the default spec
/// (the platform the tool runs on) formats the Unix file alone, and `--specs`
/// with a Unix and a Windows spec formats both files, the shared one once.
void tst_FormatCli::testSpecs_shouldFormatFilesOfEverySpec()
{
    this->writeFile(QString(QLL("specs-run/.git/HEAD")), "ref: refs/heads/main\n");
    const QByteArray unbraced = "void f()\n{\n    if (a) b();\n}\n";
    const QString shared = this->writeFile(QString(QLL("specs-run/shared.cpp")), unbraced);
    const QString unixFile = this->writeFile(QString(QLL("specs-run/unix.cpp")), unbraced);
    const QString windowsFile = this->writeFile(QString(QLL("specs-run/windows.cpp")), unbraced);
    const QString project = this->writeFile(QString(QLL("specs-run/specs.pro")),
        "SOURCES = shared.cpp\nunix: SOURCES += unix.cpp\nwin32: SOURCES += windows.cpp\n");
    QByteArray out;
    QByteArray err;
    const QStringList arguments = QStringList() << QString(QLL("--rules=control-braces"))
        << QString(QLL("-p")) << project;
#if !defined(Q_OS_WIN)
    qExpect(this->run(arguments, &out, &err))->toEqual(1);
    qExpect(this->readFile(unixFile))->Not->toEqual(unbraced);
    qExpect(this->readFile(windowsFile))->toEqual(unbraced);
    this->writeFile(QString(QLL("specs-run/shared.cpp")), unbraced);
    this->writeFile(QString(QLL("specs-run/unix.cpp")), unbraced);
#endif
    qExpect(this->run(QStringList(arguments) << QString(QLL("--specs=linux-g++,win32-g++")),
                      &out, &err))->toEqual(1);
    qExpect(this->readFile(shared))->Not->toEqual(unbraced);
    qExpect(this->readFile(unixFile))->Not->toEqual(unbraced);
    qExpect(this->readFile(windowsFile))->Not->toEqual(unbraced);
    qExpect(out.count(": formatted"))->toEqual(3);
    qExpect(err)->toBeEmpty();
}

/// Leaves a comment whose lines all fit as it is, and joins its lines once
/// `--any-comment` asks for every comment.
void tst_FormatCli::testAnyComment_shouldRewrapShortComments()
{
    this->writeFile(QString(QLL("any-run/.git/HEAD")), "ref: refs/heads/main\n");
    const QByteArray source = "// The tool waits for the server to answer\n// each request in turn.\n";
    const QString file = this->writeFile(QString(QLL("any-run/a.cpp")), source);
    QByteArray out;
    QByteArray err;
    const QStringList arguments = QStringList() << QString(QLL("--rules=comment-width")) << file;
    qExpect(this->run(arguments, &out, &err))->toEqual(0);
    qExpect(this->readFile(file))->toEqual(source);
    qExpect(this->run(QStringList(arguments) << QString(QLL("--any-comment")), &out, &err))
        ->toEqual(1);
    qExpect(this->readFile(file))
        ->toEqual(QByteArray("// The tool waits for the server to answer each request in turn.\n"));
    qExpect(err)->toBeEmpty();
}

void tst_FormatCli::testDirOption_shouldRejectFile()
{
    QByteArray out;
    QByteArray err;
    const QString path = this->m_dir.path() + QLL("/lines.cpp");
    const QStringList arguments = QStringList() << QString(QLL("--dir")) << path;
    qExpect(this->run(arguments, &out, &err))->toEqual(2);
    qExpect(err)->toContain(QByteArray("not a directory: ") + path.toLocal8Bit());
}

void tst_FormatCli::testRunner_shouldReportEachFile_data()
{
    QTest::addColumn<QString >("kind");
    QTest::newRow("serial") << QString(QLL("serial"));
    QTest::newRow("thread") << QString(QLL("thread"));
    QTest::newRow("process") << QString(QLL("process"));
}

/// Formats a readable file and a missing one with each runner, in two jobs;
/// hence the thread and process runners spread them, and every runner must
/// report the missing one alone.
void tst_FormatCli::testRunner_shouldReportEachFile()
{
    QFETCH(QString, kind);
    if ( ! this->hasRunner(kind)) {
        QSKIP("This build of the tool lacks the runner's Qt feature.");
    }
    QByteArray out;
    QByteArray err;
    const QString missing = this->m_dir.path() + QLL("/missing.cpp");
    const QStringList arguments = QStringList() << QString(QLL("--runner=")) + kind
        << QString(QLL("-j2"))
        << this->m_dir.path() + QLL("/lines.cpp")
        << missing;
    qExpect(this->run(arguments, &out, &err))->toEqual(2);
    qExpect(out)->toBeEmpty();
    qExpect(err.trimmed())->toStartWith(QDir::toNativeSeparators(missing).toLocal8Bit()
                                        + ": error: cannot read: ");
    qExpect(err.trimmed().count('\n'))->toEqual(0);
}

void tst_FormatCli::testRunner_shouldPassEmptyFolderQuietly_data()
{
    QTest::addColumn<QString >("kind");
    QTest::newRow("serial") << QString(QLL("serial"));
    QTest::newRow("thread") << QString(QLL("thread"));
    QTest::newRow("process") << QString(QLL("process"));
}

/// With each runner, checks a folder that holds no file to format; the run
/// then reports nothing.
void tst_FormatCli::testRunner_shouldPassEmptyFolderQuietly()
{
    QFETCH(QString, kind);
    if ( ! this->hasRunner(kind)) {
        QSKIP("This build of the tool lacks the runner's Qt feature.");
    }
    const QString empty = this->m_dir.path() + QLL("/empty-") + kind;
    qExpect(QDir().mkpath(empty))->toBeTruthy();
    QByteArray out;
    QByteArray err;
    const QStringList arguments = QStringList() << QString(QLL("--headless"))
        << QString(QLL("--check")) << QString(QLL("--runner=")) + kind
        << empty + QLatin1Char('/');
    qExpect(this->run(arguments, &out, &err))->toEqual(0);
    qExpect(out)->toBeEmpty();
    qExpect(err)->toBeEmpty();
}

void tst_FormatCli::testLineRange_shouldAcceptGitBlameForms_data()
{
    QTest::addColumn<QStringList >("ranges");
    QTest::newRow("start,end") << (QStringList() << QString(QLL("2,3")));
    QTest::newRow("start,") << (QStringList() << QString(QLL("2,")));
    QTest::newRow(",end") << (QStringList() << QString(QLL(",3")));
    QTest::newRow("start") << (QStringList() << QString(QLL("6")));
    QTest::newRow("start,+count") << (QStringList() << QString(QLL("2,+2")));
    QTest::newRow("start,-count") << (QStringList() << QString(QLL("4,-2")));
    QTest::newRow("regex") << (QStringList() << QString(QLL("/return/,+1")));
    QTest::newRow("anchored regex") << (QStringList() << QString(QLL("6,7"))
        << QString(QLL("^/main/,+1")));
    QTest::newRow("funcname") << (QStringList() << QString(QLL(":other")));
    QTest::newRow("two ranges") << (QStringList() << QString(QLL("1,2"))
        << QString(QLL("/return/")));
}

void tst_FormatCli::testLineRange_shouldAcceptGitBlameForms()
{
    QFETCH(QStringList, ranges);
    QStringList arguments;
    for (int i = 0; i < ranges.size(); ++i) {
        arguments << QString(QLL("-L")) << ranges.at(i);
    }
    arguments << this->m_dir.path() + QLL("/lines.cpp");
    QByteArray out;
    QByteArray err;
    qExpect(this->run(arguments, &out, &err))->toEqual(0);
    qExpect(err)->toBeEmpty();
}

void tst_FormatCli::testLineRange_shouldRejectBadRange_data()
{
    QTest::addColumn<QString >("range");
    QTest::addColumn<QByteArray >("error");
    QTest::newRow("past the end") << QString(QLL("12,13")) << QByteArray("file has only 9 lines");
    QTest::newRow("not a number") << QString(QLL("two")) << QByteArray("-L: bad line two");
    QTest::newRow("bad count") << QString(QLL("2,+x")) << QByteArray("bad line count");
    QTest::newRow("no match") << QString(QLL("/nowhere/")) << QByteArray("no match for /nowhere/");
    QTest::newRow("unterminated") << QString(QLL("/main")) << QByteArray("unterminated regex");
    QTest::newRow("bad regex") << QString(QLL("/(/")) << QByteArray("bad regex in -L /(/");
    QTest::newRow("no function") << QString(QLL(":nothing")) << QByteArray("-L :nothing: no match");
}

void tst_FormatCli::testLineRange_shouldRejectBadRange()
{
    QFETCH(QString, range);
    QFETCH(QByteArray, error);
    QByteArray out;
    QByteArray err;
    const QStringList arguments = QStringList() << QString(QLL("-L")) << range
        << this->m_dir.path() + QLL("/lines.cpp");
    qExpect(this->run(arguments, &out, &err))->toEqual(2);
    qExpect(err)->toContain(error);
}

QTEST_MAIN(tst_FormatCli)
#include "tst_format_cli.moc"
