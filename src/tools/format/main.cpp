// Copyright (C) 2015 The XD Company Ltd.
// SPDX-License-Identifier: Apache-2.0

#include "./format-agent-guides.h"
#include "./format-process-runner.h"
#include "./format-project-files.h"
#include "./format-project-reader.h"
#include "./format-runner.h"
#include "./format-settings.h"
#include "./formatter.h"

#include "config-scanner.h"
#include "console.h"
#include "ignore-parser.h"
#include "logger.h"

#include <QtCore/QCommandLineOption>
#include <QtCore/QCommandLineParser>
#include <QtCore/QCoreApplication>
#include <QtCore/QDir>
#include <QtCore/QFile>
#include <QtCore/QFileInfo>

#if defined(Q_OS_WIN)
#  include <io.h>
#else
#  include <unistd.h>
#endif

/*!
    Names the exit codes of the tool: no change and no issue, a change made
    or needed (or an issue left), and an error that kept a file, or the
    whole run, from being formatted.
*/
enum ExitCode {
    ExitClean = 0,
    ExitDirty = 1,
    ExitError = 2
};

/*!
    Tells whether @a path names a directory by its spelling: a path that
    ends with a slash or a backslash.
*/
static bool isDirectorySpelling(const QString & path)
{
    return path.endsWith(QLatin1Char('/')) || path.endsWith(QLatin1Char('\\'));
}

/*!
    Builds the path that the relative @p path names from the shell's own
    folder, the logical working folder in `$PWD`, which keeps each symbolic
    link the shell went through; hence a `..` leaves such a link to the
    link's own parent, not to the parent of its target.

    @p path stays as given, and resolves from the process's working folder
    (the link's target), when it is absolute, when `$PWD` names no folder or
    another folder than the working one, when nothing exists at the logical
    path, or when both folders reach the same file. A directory spelling
    keeps its trailing slash (see isDirectorySpelling()).
*/
static QString logicalPath(const QString & path)
{
    if (path.isEmpty() || QFileInfo(path).isAbsolute()) {
        return path;
    }
    const QString shellFolder = QDir::fromNativeSeparators(
        QString::fromLocal8Bit(qgetenv("PWD")));
    const QString workingFolder = QDir::currentPath();
    if (shellFolder.isEmpty() || ! QFileInfo(shellFolder).isAbsolute()
        || shellFolder == workingFolder
        || QFileInfo(shellFolder).canonicalFilePath()
            != QFileInfo(workingFolder).canonicalFilePath()
    ) {
        return path;
    }
    QString logical = QDir::cleanPath(shellFolder + QLatin1Char('/')
                                      + QDir::fromNativeSeparators(path));
    const QFileInfo logicalInfo(logical);
    const QFileInfo workingInfo(path);
    if ( ! logicalInfo.exists()
        || (workingInfo.exists()
            && workingInfo.canonicalFilePath() == logicalInfo.canonicalFilePath())
    ) {
        return path;
    }
    if (isDirectorySpelling(path)) {
        logical += QLatin1Char('/');
    }
    return logical;
}

/*!
    Builds the list of @p paths, each one resolved through logicalPath().
*/
static QStringList logicalPaths(const QStringList & paths)
{
    QStringList resolved;
    for (int i = 0; i < paths.size(); ++i) {
        resolved << logicalPath(paths.at(i));
    }
    return resolved;
}

/*!
    Builds the name filters of the C, C++, Objective-C and Objective-C++
    files a folder scan formats.
*/
static QStringList sourceFilters()
{
    QStringList filters;
    filters << QString(QLL("*.h")) << QString(QLL("*.hh")) << QString(QLL("*.hpp"))
        << QString(QLL("*.hxx")) << QString(QLL("*.c")) << QString(QLL("*.cc"))
        << QString(QLL("*.cpp")) << QString(QLL("*.cxx")) << QString(QLL("*.m"))
        << QString(QLL("*.mm"));
    return filters;
}

/*!
    Builds the path of the qmake that runs a project: the one beside the tool
    when it is there, else the first `qmake` on the `PATH`.
*/
static QString defaultQmake()
{
#if defined(Q_OS_WIN)
    const QString beside = QCoreApplication::applicationDirPath() + QLL("/qmake.exe");
#else
    const QString beside = QCoreApplication::applicationDirPath() + QLL("/qmake");
#endif
    return QFileInfo(beside).isFile() ? beside : QString(QLL("qmake"));
}

/*!
    Builds the name of the qmake spec that `--specs` reads a project for when
    it names none: the spec of the platform and compiler the tool was built
    for, such as `linux-clang`.
*/
static QString runtimeSpec()
{
#if defined(Q_OS_WIN) && defined(Q_CC_MSVC)
#  if _MSC_VER >= 1910
    return QString(QLL("win32-msvc2017"));
#  elif _MSC_VER >= 1900
    return QString(QLL("win32-msvc2015"));
#  elif _MSC_VER >= 1800
    return QString(QLL("win32-msvc2013"));
#  elif _MSC_VER >= 1700
    return QString(QLL("win32-msvc2012"));
#  elif _MSC_VER >= 1600
    return QString(QLL("win32-msvc2010"));
#  elif _MSC_VER >= 1500
    return QString(QLL("win32-msvc2008"));
#  else
    return QString(QLL("win32-msvc2005"));
#  endif
#elif defined(Q_OS_WIN) && defined(Q_CC_CLANG)
    return QString(QLL("win32-clang"));
#elif defined(Q_OS_WIN)
    return QString(QLL("win32-g++"));
#elif defined(Q_OS_MAC) && defined(Q_CC_CLANG)
    return QString(QLL("macx-clang"));
#elif defined(Q_OS_MAC)
    return QString(QLL("macx-g++"));
#elif defined(Q_OS_FREEBSD) && defined(Q_CC_CLANG)
    return QString(QLL("freebsd-clang"));
#elif defined(Q_OS_FREEBSD)
    return QString(QLL("freebsd-g++"));
#elif defined(Q_CC_CLANG)
    return QString(QLL("linux-clang"));
#else
    return QString(QLL("linux-g++"));
#endif
}

/*!
    Builds the list of @a taken files that the ignore files of @a ignores
    keep and that @a known does not hold yet.
*/
static QStringList keptProjectFiles(
        const QStringList & taken, IgnoreParser *ignores,
        const QStringList & known)
{
    QStringList kept;
    for (int i = 0; i < taken.size(); ++i) {
        if ( ! known.contains(taken.at(i))
            && ! ignores->isIgnored(taken.at(i), false)
        ) {
            kept << taken.at(i);
        }
    }
    return kept;
}

/*!
    Tells whether the file descriptor @a fd is a terminal; Qt has no
    interface for it, hence the platform's own call.
*/
static bool isTerminal(int fd)
{
#if defined(Q_OS_WIN)
    return _isatty(fd) != 0;
#else
    return isatty(fd) != 0;
#endif
}

/*!
    Builds the requests of @a results for the agent, one `#: <path>:<line>:
    <rule>: <message>` line each (`<line>-<last line>` for several lines),
    and lists each file they concern in @a asked, once. A request whose rule
    has a prompt template gets, in place of its message, the prompt file that
    @a guides writes for it: `Run prompt at: "<path>"`.
*/
static QString agentRequests(const QList<FormatFileResult > & results,
                             FormatAgentGuides *guides, QStringList *asked)
{
    QString text;
    for (int i = 0; i < results.size(); ++i) {
        const FormatFileResult & result = results.at(i);
        for (int k = 0; k < result.requests.size(); ++k) {
            const FormatIssue & request = result.requests.at(k);
            const QString lines = request.endLine > request.line
                ? QString(QLL("%1-%2")).arg(request.line).arg(request.endLine)
                : QString::number(request.line);
            const QString prompt = guides->write(result.path, request);
            const QString message = prompt.isEmpty()
                ? request.message : QString(QLL("Run prompt at: \"%1\"")).arg(prompt);
            text += QString(QLL("#: %1:%2: %3: %4\n")).arg(
                QDir::fromNativeSeparators(result.path),
                lines,
                QString::fromLatin1(request.rule),
                message
            );
            if ( ! asked->contains(result.path)) {
                *asked << result.path;
            }
        }
    }
    return text;
}

/*!
    Builds the bytes of the file at @a path, or an empty array for a file
    that cannot be read.
*/
static QByteArray readFile(const QString & path)
{
    QFile file(path);
    return file.open(QIODevice::ReadOnly) ? file.readAll() : QByteArray();
}

/*!
    Writes each of @a results with @a logger the way a person reads them,
    and returns the exit code they add up to; @a check tells whether the
    run only checked the files.
*/
static int logResults(Logger *logger, const QList<FormatFileResult > & results,
                      bool check)
{
    int code = ExitClean;
    for (int i = 0; i < results.size(); ++i) {
        const FormatFileResult & result = results.at(i);
        const QString path = QDir::fromNativeSeparators(result.path);
        if ( ! result.error.isEmpty()) {
            logger->logDiagnostic(QString(QLL("%1: error: %2")).arg(
                path,
                result.error
            ));
            code = ExitError;
            continue;
        }
        if (result.changed) {
            logger->logLine(path
                + (check ? QLL(": needs formatting") : QLL(": formatted"))
            );
            code = qMax(code, int(ExitDirty));
        }
        for (int k = 0; k < result.issues.size(); ++k) {
            const FormatIssue & issue = result.issues.at(k);
            logger->logLine(QString(QLL("%1:%2: %3: %4")).arg(path).arg(issue.line)
                            .arg(
                                QString::fromLatin1(issue.rule),
                                issue.message
                            ));
            code = qMax(code, int(ExitDirty));
        }
    }
    return code;
}

/*!
    Runs the tool: reads the command line, collects the files, formats them
    with the chosen runner, and prints the results.
*/
int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);
    QCoreApplication::setApplicationName(QString(QLL("format")));
    QCoreApplication::setApplicationVersion(QString::fromLatin1(QT_VERSION_STR));

    // Opens the standard streams for the logger and the console; the input
    // reads unbuffered, since `QFile` fills a buffered read of a pipe in
    // full, which waits forever for an answer that a pipe kept open sends.
    QFile output;
    output.open(1, QIODevice::WriteOnly);
    QFile errors;
    errors.open(2, QIODevice::WriteOnly | QIODevice::Unbuffered);
    QFile input;
    input.open(0, QIODevice::ReadOnly | QIODevice::Unbuffered);
    Logger logger(QCoreApplication::applicationName(), &output, &errors);
    logger.setStyled(isTerminal(2));
    const int columns = qgetenv("COLUMNS").toInt();
    logger.setWidth(columns > 0 ? columns : 80);
    Console console(&input, &errors);
    console.setTerminal(isTerminal(0) ? 0 : -1, isTerminal(2));

    // Declares the options.
    QCommandLineParser parser;
    parser.setApplicationDescription(QString(QLL(
            "Applies the formatting rules to C, C++ and Objective-C files, and reports\n"
            "what a rule cannot fix on its own. A path that ends with / or \\ is a\n"
            "directory, which is searched for the files that its .gitignore and\n"
            ".formatignore files keep; any other path is a file.")));
    parser.addHelpOption();
    parser.addVersionOption();
    QCommandLineOption lineOption(
        QStringList() << QString(QLL("L")) << QString(QLL("range")),
        QString(QLL("Formats only the lines of <range>, in any form git blame's -L " "takes; may be given more than once.")),
        QString(QLL("range"))
    );
    parser.addOption(lineOption);
    QCommandLineOption dirOption(
        QStringList() << QString(QLL("dir")),
        QString(QLL("Formats the source files under <path>, a directory with or " "without a trailing slash; may be given more than once.")),
        QString(QLL("path"))
    );
    parser.addOption(dirOption);
    QCommandLineOption checkOption(
        QStringList() << QString(QLL("check")),
        QString(QLL("Writes nothing, only tells which files need formatting."))
    );
    parser.addOption(checkOption);
    QCommandLineOption rulesOption(
        QStringList() << QString(QLL("rules")),
        QString(QLL("Runs only the comma-separated rules of <names>.")),
        QString(QLL("names"))
    );
    parser.addOption(rulesOption);
    QCommandLineOption configOption(
        QStringList() << QString(QLL("c")) << QString(QLL("config")),
        QString(QLL("Sets <setting> (<section>.<key>=<value>, the name in any case) over " "the .formatrc files; may be given more than once.")),
        QString(QLL("setting"))
    );
    parser.addOption(configOption);
    QCommandLineOption configFileOption(
        QStringList() << QString(QLL("config-file")),
        QString(QLL("Reads the settings of <file> in place of every .formatrc file.")),
        QString(QLL("file"))
    );
    parser.addOption(configFileOption);
    QCommandLineOption configAddOption(
        QStringList() << QString(QLL("config-add")),
        QString(QLL("Reads the settings of <file> over the .formatrc files, overriding " "only the keys it sets; may be given more than once.")),
        QString(QLL("file"))
    );
    parser.addOption(configAddOption);
    QCommandLineOption anyCommentOption(
        QStringList() << QString(QLL("any-comment")),
        QString(QLL("Rewraps every comment that leaves room unused, not only the " "ones past the limit; the same as -c comments.target=any."))
    );
    parser.addOption(anyCommentOption);
    QCommandLineOption listRulesOption(
        QStringList() << QString(QLL("list-rules")),
        QString(QLL("Lists the rules, and exits."))
    );
    parser.addOption(listRulesOption);
    QCommandLineOption jobsOption(
        QStringList() << QString(QLL("j")) << QString(QLL("jobs")),
        QString(QLL("Formats up to <count> files at once; one per CPU core by default.")),
        QString(QLL("count"))
    );
    parser.addOption(jobsOption);
    QCommandLineOption runnerOption(
        QStringList() << QString(QLL("runner")),
        QString(QLL("Spreads the files with <kind>: %1; %2 by default.")) .arg(FormatRunner::kinds().join(QString(QLL(", ")))) .arg(FormatRunner::defaultKind()),
        QString(QLL("kind"))
    );
    parser.addOption(runnerOption);
    QCommandLineOption includeOption(
        QStringList() << QString(QLL("I")) << QString(QLL("i")),
        QString(QLL("Looks for the headers that quoted includes name in <dir> too, after " "the file's own folder; may be given more than once.")),
        QString(QLL("dir"))
    );
    parser.addOption(includeOption);
    QCommandLineOption projectOption(
        QStringList() << QString(QLL("p")) << QString(QLL("project")),
        QString(QLL("Formats the sources and headers that the qmake project <file> " "lists, and those of the projects it holds, with its include folders, " "and skips the ones the ignore files leave out; may be given more " "than once.")),
        QString(QLL("file"))
    );
    parser.addOption(projectOption);
    QCommandLineOption specsOption(
        QStringList() << QString(QLL("specs")),
        QString(QLL("Reads each --project for each qmake spec of the comma-separated " "<names>, which pick the platform scopes, and formats a file that " "several of them list once; may be given more than once. The spec " "of the platform the tool runs on by default.")),
        QString(QLL("names"))
    );
    parser.addOption(specsOption);
    QCommandLineOption qmakeOption(
        QStringList() << QString(QLL("qmake")),
        QString(QLL("Reads each --project with the qmake at <path>; the one beside the " "tool by default, else the one on the PATH.")),
        QString(QLL("path"))
    );
    parser.addOption(qmakeOption);
    QCommandLineOption skipIgnoredOption(
        QStringList() << QString(QLL("skip-ignored")),
        QString(QLL("Skips each named path that the ignore files leave out, without a " "word, instead of formatting it."))
    );
    parser.addOption(skipIgnoredOption);
    QCommandLineOption agentOption(
        QStringList() << QString(QLL("ai")),
        QString(QLL("Asks the agent that runs the tool for the changes only it can " "make, as #: lines on the standard error, waits for q on the " "standard input, then formats each file it changed again."))
    );
    parser.addOption(agentOption);
    QCommandLineOption headlessOption(
        QStringList() << QString(QLL("headless")),
        QString(QLL("Asks nothing: formats a named path that the ignore files leave " "out without a warning."))
    );
    parser.addOption(headlessOption);
    QCommandLineOption workerOption(
        QStringList() << QString(QLL("worker")),
        QString(QLL("Prints the results for a parent format process (internal)."))
    );
    parser.addOption(workerOption);
    parser.addPositionalArgument(
        QString(QLL("paths")),
        QString(QLL("The files, and the directories (ending with / or \\), to format.")),
        QString(QLL("[paths...]"))
    );
    parser.process(app);

    // Hands the agent the usage instructions of agent mode before any other
    // line, since they say how to handle each line.
    FormatAgentGuides guides;
    if (parser.isSet(agentOption) && ! parser.isSet(workerOption)) {
        const QString usage = guides.writeUsage();
        if ( ! usage.isEmpty()) {
            logger.logDiagnostic(
                QString(QLL("Read this tool's usage instructions at: \"%1\"")).arg(usage));
        }
    }

    // Selects the rules.
    Formatter formatter;
    QString error;
    QStringList rules;
    if (parser.isSet(rulesOption)) {
        rules = parser.value(rulesOption).split(
            QLatin1Char(','),
            QString::SkipEmptyParts
        );
    }
    if ( ! formatter.selectRules(rules, &error)) {
        logger.logError(error);
        return ExitError;
    }
    if (parser.isSet(listRulesOption)) {
        const QList<const FormatRule * > all = formatter.rules();
        for (int i = 0; i < all.size(); ++i) {
            logger.logLine(QString::fromLatin1(all.at(i)->name()) + QLatin1Char('\t')
                           + all.at(i)->summary());
        }
        return ExitClean;
    }

    // Reads the settings over the `.formatrc` files; a short `-c=<setting>`
    // hands its `=` on as the first character of the value.
    FormatConfig config;
    config.file = logicalPath(parser.value(configFileOption));
    config.adds = logicalPaths(parser.values(configAddOption));
    const QStringList includes = logicalPaths(parser.values(includeOption));
    for (int i = 0; i < includes.size(); ++i) {
        config.includePaths
            << QDir::cleanPath(QFileInfo(includes.at(i)).absoluteFilePath());
    }

    // Learns the files and the include folders of each project from qmake,
    // for the first spec now; a thread reads the other specs once the
    // formatting starts.
    const QStringList projects = logicalPaths(parser.values(projectOption));
    const QString qmake = parser.isSet(qmakeOption)
        ? logicalPath(parser.value(qmakeOption)) : defaultQmake();
    QStringList specs;
    const QStringList specLists = parser.values(specsOption);
    for (int i = 0; i < specLists.size(); ++i) {
        const QStringList names = specLists.at(i).split(
            QLatin1Char(','),
            QString::SkipEmptyParts
        );
        for (int k = 0; k < names.size(); ++k) {
            const QString name = names.at(k).trimmed();
            if ( ! name.isEmpty() && ! specs.contains(name)) {
                specs << name;
            }
        }
    }
    if (specs.isEmpty()) {
        specs << runtimeSpec();
    }
    FormatProjectFiles projectFiles;
    FormatProjectReader reader(&projectFiles, projects, qmake, specs);
    if ( ! projects.isEmpty() && ! reader.readSpec(0, &error)) {
        logger.logError(error);
        return ExitError;
    }
    if (projects.isEmpty() || specs.size() == 1) {
        projectFiles.finish();
    }
    const QStringList projectFolders = projectFiles.includePaths();
    for (int i = 0; i < projectFolders.size(); ++i) {
        if ( ! config.includePaths.contains(projectFolders.at(i))) {
            config.includePaths << projectFolders.at(i);
        }
    }
    const QStringList assignments = parser.values(configOption);
    for (int i = 0; i < assignments.size(); ++i) {
        QString assignment = assignments.at(i);
        if (assignment.startsWith(QLatin1Char('='))) {
            assignment.remove(0, 1);
        }
        config.assignments << assignment;
    }
    if (parser.isSet(anyCommentOption)) {
        config.assignments << QString(QLL("comments.target=any"));
    }

    // Checks the settings once, before any file is read.
    Config::Parser configCheck(FormatSettings::schema());
    if (( ! config.file.isEmpty() && ! configCheck.readFile(
            config.file,
            &error
        ))
        || ! FormatSettings::readConfig(&configCheck, config, &error)
    ) {
        logger.logError(error);
        return ExitError;
    }

    // Collects the files: every path named on the command line counts, and a
    // folder's files are the ones its ignore files keep.
    IgnoreParser ignores;
    ignores.addIgnoreFileName(QString(QLL(".formatignore")));
    QStringList folders;
    const QStringList dirs = logicalPaths(parser.values(dirOption));
    for (int i = 0; i < dirs.size(); ++i) {
        if ( ! QFileInfo(dirs.at(i)).isDir()) {
            logger.logError(QString(QLL("not a directory: %1"))
                           .arg(QDir::fromNativeSeparators(dirs.at(i))));
            return ExitError;
        }
        folders << dirs.at(i);
    }
    QStringList files;
    const QStringList paths = logicalPaths(parser.positionalArguments());
    for (int i = 0; i < paths.size(); ++i) {
        const QString & path = paths.at(i);
        if (isDirectorySpelling(path)) {
            QString dir = path;
            dir.chop(1);
            if ( ! QFileInfo(dir.isEmpty() ? path : dir).isDir()) {
                logger.logError(QString(QLL("not a directory: %1"))
                           .arg(QDir::fromNativeSeparators(path)));
                return ExitError;
            }
            folders << (dir.isEmpty() ? path : dir);
            continue;
        }
        const QFileInfo info(path);
        if (info.isDir()) {
            logger.logError(QString(QLL("is a directory, end it with / or pass --dir: %1"))
                       .arg(QDir::fromNativeSeparators(path)));
            return ExitError;
        }
        files.append(path);
        ignores.addExplicitPath(path);
    }
    for (int i = 0; i < folders.size(); ++i) {
        ignores.addExplicitPath(folders.at(i));
    }

    // Skips each named path that the ignore files leave out, when asked to.
    if (parser.isSet(skipIgnoredOption)) {
        QStringList keptFiles;
        for (int i = 0; i < files.size(); ++i) {
            if (ignores.excludingRule(files.at(i), false).isEmpty()) {
                keptFiles << files.at(i);
            }
        }
        QStringList keptFolders;
        for (int i = 0; i < folders.size(); ++i) {
            if (ignores.excludingRule(folders.at(i), true).isEmpty()) {
                keptFolders << folders.at(i);
            }
        }
        const bool named = ! files.isEmpty() || ! folders.isEmpty();
        files = keptFiles;
        folders = keptFolders;
        if (named && files.isEmpty() && folders.isEmpty()) {
            return ExitClean;
        }
    }

    // Warns about each named path that the ignore files leave out, and asks
    // whether to format it anyway.
    if ( ! parser.isSet(headlessOption) && ! parser.isSet(workerOption)
        && ! parser.isSet(skipIgnoredOption)
    ) {
        QStringList named = files;
        named << folders;
        int excluded = 0;
        for (int i = 0; i < named.size(); ++i) {
            const QString rule = ignores.excludingRule(
                named.at(i),
                QFileInfo(named.at(i)).isDir()
            );
            if ( ! rule.isEmpty()) {
                logger.logWarning(QString(QLL("%1 is left out by the ignore rule %2"))
                                  .arg(
                                      QDir::fromNativeSeparators(named.at(i)),
                                      rule
                                  ));
                ++excluded;
            }
        }
        // Asks with a `#:` line in agent mode, where only those lines ask.
        QString question = QString(QLL("Format the named paths that the ignore files leave "
                                       "out anyway?"));
        if (parser.isSet(agentOption)) {
            question.prepend(QLL("#: "));
        }
        if (excluded > 0 && ! console.confirm(question)) {
            logger.logError(QString(QLL("stopped: a named path is left out by the ignore "
                                        "files")));
            return ExitError;
        }
    }
    for (int i = 0; i < folders.size(); ++i) {
        const QString root = QDir::cleanPath(folders.at(i));
        const QStringList found = ignores.findFiles(root, sourceFilters());

        // Names the files the way the folder was named on the command line.
        const QString absolute = QDir::cleanPath(QFileInfo(root).absoluteFilePath());
        for (int f = 0; f < found.size(); ++f) {
            files.append(root + found.at(f).mid(absolute.size()));
        }
    }

    // Adds the projects' files that the ignore files keep.
    files
        << keptProjectFiles(projectFiles.takeUnparsed(false), &ignores, files);
    if (files.isEmpty()
        && folders.isEmpty()
        && ! projects.isEmpty()
        && projectFiles.isFinished()
    ) {
        return ExitClean;
    }
    if (files.isEmpty() && folders.isEmpty() && projects.isEmpty()) {
        logger.logError(QString(QLL("no files given; see --help")));
        return ExitError;
    }

    // Reads the run's options.
    FormatOptions options;
    options.lineSpecs = parser.values(lineOption);
    options.rules = rules;
    options.config = config;
    options.check = parser.isSet(checkOption);
    options.agent = parser.isSet(agentOption);
    if (options.agent && options.check) {
        logger.logError(QString(QLL("--ai cannot run with --check: the agent's lines are the "
                                    "ones the tool writes")));
        return ExitError;
    }
    if (parser.isSet(jobsOption)) {
        bool ok = false;
        options.jobs = parser.value(jobsOption).toInt(&ok);
        if ( ! ok || options.jobs < 1) {
            logger.logError(QString(QLL("not a job count: %1")).arg(parser.value(jobsOption)));
            return ExitError;
        }
    }
    QScopedPointer<FormatRunner > runner(FormatRunner::create(
        parser.value(runnerOption),
        &formatter,
        options
    ));
    if (runner.isNull()) {
        logger.logError(QString(QLL("unknown runner: %1 (this build has: %2)"))
                   .arg(parser.value(runnerOption), FormatRunner::kinds().join(QString(QLL(", ")))));
        return ExitError;
    }

    // Formats the files while the thread reads the projects for the others
    // of the specs, if any.
#ifndef QT_NO_THREAD
    if ( ! projectFiles.isFinished()) {
        reader.start();
    }
#else
    if ( ! projectFiles.isFinished()) {
        reader.readFrom(1);
    }
#endif
    QList<FormatFileResult > results = qMove(runner->run(files));
    if (parser.isSet(workerOption)) {
#ifndef QT_NO_PROCESS
        for (int i = 0; i < results.size(); ++i) {
            logger.logRaw(FormatProcessRunner::encode(results.at(i)));
        }
#endif
        return ExitClean;
    }
    int code = logResults(&logger, results, options.check);

    // Formats the files that the other specs add, batch by batch, each with
    // the include folders found so far.
    QStringList formatted = files;
    for (;;) {
        const QStringList taken = projectFiles.takeUnparsed(true);
        if (taken.isEmpty()) {
            break;
        }
        const QStringList batch = keptProjectFiles(
            taken,
            &ignores,
            formatted
        );
        if (batch.isEmpty()) {
            continue;
        }
        const QStringList found = projectFiles.includePaths();
        for (int i = 0; i < found.size(); ++i) {
            if ( ! options.config.includePaths.contains(found.at(i))) {
                options.config.includePaths << found.at(i);
            }
        }
        runner.reset(FormatRunner::create(
            parser.value(runnerOption),
            &formatter,
            options
        ));
        const QList<FormatFileResult > batchResults = qMove(runner->run(batch));
        results << batchResults;
        formatted << batch;
        code = qMax(code, logResults(&logger, batchResults, options.check));
    }
#ifndef QT_NO_THREAD
    reader.wait();
#endif
    if ( ! reader.error().isEmpty()) {
        logger.logError(reader.error());
        code = ExitError;
    }

    // Asks the agent for what only it can change, and formats each file it
    // changed again, until no file it was asked about changes; each round
    // replaces the prompt files of the one before.
    bool answered = false;
    while (options.agent) {
        guides.removePrompts();
        QStringList asked;
        const QString requests = agentRequests(results, &guides, &asked);
        if (asked.isEmpty()) {
            break;
        }
        QList<QByteArray > before;
        for (int i = 0; i < asked.size(); ++i) {
            before << readFile(asked.at(i));
        }
        output.flush();
        console.page(requests
            + QLL("#: Handle every request above, then type q.\n")
        );
        answered = true;
        QStringList changed;
        for (int i = 0; i < asked.size(); ++i) {
            if (readFile(asked.at(i)) != before.at(i)) {
                changed << asked.at(i);
            }
        }
        if (changed.isEmpty()) {
            break;
        }
        results = qMove(runner->run(changed));
        code = qMax(code, logResults(&logger, results, options.check));
    }

    // Removes the agent's files once it quit the tool; a run that asked it
    // nothing keeps them for it to read (see `FormatAgentGuides`).
    if (answered) {
        guides.remove();
    }
    return code;
}
