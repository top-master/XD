// Copyright (C) 2015 The XD Company Ltd.
// SPDX-License-Identifier: Apache-2.0

#include "./project-parser.h"

#include <QtCore/QDir>
#include <QtCore/QFileInfo>
#include <QtCore/QProcess>
#include <QtCore/QRegularExpression>
#include <QtCore/QTemporaryDir>

/*!
    @namespace Project
    @internal

    Learns what a qmake project holds from qmake itself: Parser runs it on the
    project file, and reads the variables it prints.
*/

/*!
    @class Project::Parser
    @internal

    Lists the files a qmake project names, and its include folders, as qmake
    evaluates them. For each project, run() starts `qmake -E <project>` in a
    scratch folder; it prints every variable after evaluation and writes no
    Makefile. run() then reads `SOURCES`, `HEADERS`, `OBJECTIVE_SOURCES`, and
    `INCLUDEPATH`, resolved against the project's folder (`_PRO_FILE_PWD_`).
    A `subdirs` project gives the projects of its `SUBDIRS` instead, which
    run() reads in turn (through `<name>.file` or `<name>.subdir` when the
    project sets them).

    A project lists only its own files: the sources and headers that its
    `STATIC_PLUGINS` bring in (`STATIC_PLUGINS.<id>.sources` and
    `.headers`) are left to the plugin's own project, which lists them in
    `PLUGIN_SOURCES` and `PLUGIN_HEADERS`. Their include folders still count.

    Each list holds a path once, in order, as an absolute path to a file or a
    folder that exists. The arguments a caller passes to qmake (such as
    `-spec <name>`) pick the platform; the files of a scope that this
    evaluation leaves off are not listed.
*/

/*!
    Runs qmake (@a qmake, given @a qmakeArguments before its own) on
    @a projectFile and on each project it holds (see Parser). Returns false,
    with @a error, when qmake cannot run, fails, or names a missing project.
*/
bool Project::Parser::run(const QString & projectFile, const QString & qmake,
                          const QStringList & qmakeArguments, QString *error)
{
    return this->runOne(projectFile, qmake, qmakeArguments, error, 0);
}

/*!
    Runs qmake on @a projectFile, and on the projects of its `SUBDIRS`, up to
    the nesting @a depth of 32 (see run()).
*/
bool Project::Parser::runOne(const QString & projectFile, const QString & qmake,
                             const QStringList & qmakeArguments,
                             QString *error,
                             int depth)
{
    const QString project = QDir::cleanPath(QFileInfo(projectFile).absoluteFilePath());
    if ( ! QFileInfo(project).isFile()) {
        *error = QString(QLL("%1: no such project file")).arg(QDir::toNativeSeparators(project));
        return false;
    }
    if (this->m_projects.contains(project) || depth > 32) {
        return true;
    }
    this->m_projects << project;

    // Runs qmake in a scratch folder, which goes away with the run.
    QTemporaryDir scratch;
    QProcess process;
    process.setWorkingDirectory(scratch.path());
    process.setProcessChannelMode(QProcess::SeparateChannels);
    process.start(qmake, QStringList()
        << qmakeArguments
        << QString(QLL("-E"))
        << project
    );
    if ( ! process.waitForStarted() || ! process.waitForFinished(-1)) {
        *error = QString(QLL("%1: cannot run %2")).arg(
            QDir::toNativeSeparators(project),
            qmake
        );
        return false;
    }
    const QByteArray output = process.readAllStandardOutput();
    if (process.exitStatus() != QProcess::NormalExit
        || process.exitCode() != 0
    ) {
        const QByteArray diagnostics = process.readAllStandardError();
        *error = QString(QLL("%1: qmake failed: %2")).arg(
            QDir::toNativeSeparators(project),
            QString::fromLocal8Bit(diagnostics).trimmed()
        );
        return false;
    }
    const QHash<QString, QStringList > values = this->parse(output);
    QString folder = values.value(QString(QLL("_PRO_FILE_PWD_"))).value(0);
    if (folder.isEmpty()) {
        folder = QFileInfo(project).absolutePath();
    }

    // Reads the projects a subdirs project holds.
    if (values.value(QString(QLL("TEMPLATE"))).value(0) == QLL("subdirs")) {
        const QStringList entries = values.value(QString(QLL("SUBDIRS")));
        for (int i = 0; i < entries.size(); ++i) {
            const QString & entry = entries.at(i);
            QString file = values.value(entry + QLL(".file")).value(0);
            if (file.isEmpty()) {
                QString sub = values.value(entry + QLL(".subdir")).value(0);
                if (sub.isEmpty()) {
                    sub = entry;
                }
                file = sub.endsWith(QLL(".pro"))
                    ? sub : sub + QLatin1Char('/') + QFileInfo(sub).fileName() + QLL(".pro");
            }
            if ( ! this->runOne(
                    Project::Parser::absolute(folder, file),
                    qmake,
                    qmakeArguments,
                    error, depth + 1
                )
            ) {
                return false;
            }
        }
        return true;
    }

    // Lists the files that the project's static plugins bring in, which
    // belong to the plugins' own projects.
    const QRegularExpression pluginFiles(
            QString(QLL("^STATIC_PLUGINS\\.[^.]+\\.(sources|headers)$")));
    QStringList borrowed;
    for (QHash<QString, QStringList >::const_iterator it = values.constBegin();
         it != values.constEnd();
         ++it
    ) {
        if (pluginFiles.match(it.key()).hasMatch()) {
            for (int i = 0; i < it.value().size(); ++i) {
                borrowed << Project::Parser::absolute(
                    folder,
                    it.value().at(i)
                );
            }
        }
    }

    // Reads the files and include folders of any other project, and the
    // `PLUGIN_SOURCES` and `PLUGIN_HEADERS` a static plugin's project lists.
    const char *const fileVariables[] = {
        "SOURCES", "HEADERS", "OBJECTIVE_SOURCES",
        "PLUGIN_SOURCES", "PLUGIN_HEADERS"
    };
    for (size_t v = 0; v < sizeof(fileVariables) / sizeof(fileVariables[0]); ++v) {
        const QStringList names = values.value(QString::fromLatin1(fileVariables[v]));
        for (int i = 0; i < names.size(); ++i) {
            const QString path = Project::Parser::absolute(
                folder,
                names.at(i)
            );
            if (QFileInfo(path).isFile() && ! borrowed.contains(path)
                && ! this->m_files.contains(path)
            ) {
                this->m_files << path;
            }
        }
    }
    const QStringList includes = values.value(QString(QLL("INCLUDEPATH")));
    for (int i = 0; i < includes.size(); ++i) {
        const QString path = Project::Parser::absolute(
            folder,
            includes.at(i)
        );
        if (QFileInfo(path).isDir() && ! path.startsWith(scratch.path())
            && ! this->m_includePaths.contains(path)
        ) {
            this->m_includePaths << path;
        }
    }
    return true;
}

/*!
    Builds the variables that the `qmake -E` @a output prints, one `<name> = <words>`
    line each, as lists of words (see words()).
*/
QHash<QString, QStringList > Project::Parser::parse(
        const QByteArray & output) const
{
    QHash<QString, QStringList > values;
    const QRegularExpression line(QString(QLL("^([A-Za-z_][A-Za-z0-9_.]*) = ?(.*)$")));
    const QStringList lines = QString::fromLocal8Bit(output).split(QLatin1Char('\n'));
    for (int i = 0; i < lines.size(); ++i) {
        QString text = lines.at(i);
        if (text.endsWith(QLatin1Char('\r'))) {
            text.chop(1);
        }
        const QRegularExpressionMatch found = line.match(text);
        if (found.hasMatch()) {
            values.insert(found.captured(1), Project::Parser::words(found.captured(2)));
        }
    }
    return values;
}

/*!
    Builds the words of @a line, split at spaces the way qmake prints them: a
    word in double quotes keeps its spaces, and the quotes go.
*/
QStringList Project::Parser::words(const QString & line)
{
    QStringList result;
    QString word;
    bool hasWord = false;
    bool quoted = false;
    for (int i = 0; i < line.size(); ++i) {
        const QChar c = line.at(i);
        if (c == QLatin1Char('"')) {
            quoted = ! quoted;
            hasWord = true;
        } else if (c.isSpace() && ! quoted) {
            if (hasWord) {
                result << word;
            }
            word.clear();
            hasWord = false;
        } else {
            word += c;
            hasWord = true;
        }
    }
    if (hasWord) {
        result << word;
    }
    return result;
}

/*!
    Builds the absolute, clean form of @a path, a relative one resolved
    against @a folder.
*/
QString Project::Parser::absolute(const QString & folder, const QString & path)
{
    return QDir::cleanPath(QDir(folder).absoluteFilePath(QDir::fromNativeSeparators(path)));
}
