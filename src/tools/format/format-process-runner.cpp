// Copyright (C) 2015 The XD Company Ltd.
// SPDX-License-Identifier: Apache-2.0

#include "./format-process-runner.h"

#ifndef QT_NO_PROCESS

#include <QtCore/QCoreApplication>
#include <QtCore/QProcess>

/*!
    @class FormatProcessRunner
    @internal

    Formats the files of a run in up to jobCount() child processes of the
    tool itself, each given a slice of the files with the `--worker` option,
    which makes it print its results in the form that encode() writes.
*/

/*!
    Builds the runner that formats with @a formatter as @a options say.
*/
FormatProcessRunner::FormatProcessRunner(const Formatter *formatter, const FormatOptions & options)
    : super(formatter, options)
{
}

/*!
    Formats @a files in child processes, tells the listener of each result as
    the children write them (see tellFinished()), and returns the results in
    the order of @a files; a file whose child process failed gets that
    failure as its error.
*/
QList<FormatFileResult > FormatProcessRunner::run(const QStringList & files)
{
    QString error;
    if ( ! this->prepareRun(files, &error)) {
        return this->runFailure(files, error);
    }
    // Starts no child for no files, since a child given none fails as a
    // command line without paths.
    if (files.isEmpty()) {
        return QList<FormatFileResult >();
    }
    const int jobs = this->jobCount(files.size());

    // Starts one child per slice of the files.
    QList<QProcess * > processes;
    QList<QStringList > slices;
    for (int i = 0; i < jobs; ++i) {
        slices.append(QStringList());
    }
    for (int i = 0; i < files.size(); ++i) {
        slices[i % jobs].append(files.at(i));
    }
    for (int i = 0; i < jobs; ++i) {
        QProcess *process = new QProcess;
        process->setProcessChannelMode(QProcess::ForwardedErrorChannel);
        process->start(
            QCoreApplication::applicationFilePath(),
            this->workerArguments() + slices.at(i)
        );
        processes.append(process);
    }

    // Gathers each child's results as the child writes them, and tells the
    // listener of each result in the order of the files, once the files
    // before it are done.
    QList<FormatFileResult > results;
    QList<bool > done;
    for (int i = 0; i < files.size(); ++i) {
        FormatFileResult result;
        result.path = files.at(i);
        result.error = QString(QLL("worker gave no result"));
        results.append(result);
        done.append(false);
    }
    QList<QList<FormatFileResult > > partial;
    QList<bool > ended;
    for (int i = 0; i < processes.size(); ++i) {
        partial.append(QList<FormatFileResult >());
        ended.append(false);
    }
    int told = 0;
    int running = processes.size();
    while (running > 0) {
        for (int i = 0; i < processes.size(); ++i) {
            if (ended.at(i)) {
                continue;
            }
            QProcess *process = processes.at(i);
            if ( ! process->canReadLine()) {
                process->waitForReadyRead(100);
            }
            const bool finished = process->state() == QProcess::NotRunning;

            // Reads the complete lines, and all that is left of
            // a child that ended.
            while (process->canReadLine()
                || (finished
                    && process->bytesAvailable() > 0)
            ) {
                QByteArray line = process->readLine();
                if (line.endsWith('\n')) {
                    line.chop(1);
                }
                QString path;
                if ( ! FormatProcessRunner::decodeEnd(line, &path)) {
                    FormatProcessRunner::decode(line, &partial[i]);
                    continue;
                }
                for (int p = 0; p < partial.at(i).size(); ++p) {
                    if (partial.at(i).at(p).path != path) {
                        continue;
                    }
                    const int index = files.indexOf(path);
                    if (index >= 0 && ! done.at(index)) {
                        results[index] = partial.at(i).at(p);
                        done[index] = true;
                    }
                    partial[i].removeAt(p);
                    break;
                }
            }

            // Ends the child: a file it gave no result for gets its failure.
            if (finished) {
                ended[i] = true;
                --running;
                const bool failed = process->exitStatus() != QProcess::NormalExit
                    || process->error() == QProcess::FailedToStart;
                const QStringList & slice = slices.at(i);
                for (int f = 0; f < slice.size(); ++f) {
                    const int index = files.indexOf(slice.at(f));
                    if (index < 0 || done.at(index)) {
                        continue;
                    }
                    if (failed) {
                        results[index].error = QString(QLL("worker failed: %1"))
                            .arg(process->errorString());
                    }
                    done[index] = true;
                }
            }
            while (told < files.size() && done.at(told)) {
                this->tellFinished(results.at(told));
                ++told;
            }
        }
    }
    qDeleteAll(processes);
    return results;
}

/*!
    Builds the arguments that make a child format its files as the options
    say, with the serial runner, and print its results for decode().
*/
QStringList FormatProcessRunner::workerArguments() const
{
    QStringList arguments;
    arguments << QString(QLL("--worker")) << QString(QLL("--runner=serial"));
    for (int i = 0; i < this->m_options.lineSpecs.size(); ++i) {
        arguments << QString(QLL("-L")) << this->m_options.lineSpecs.at(i);
    }
    if ( ! this->m_options.git.isEmpty() && this->m_options.config.snapshotManifest.isEmpty()) {
        arguments << QString(QLL("--git")) << this->m_options.git;
    }
    if ( ! this->m_options.rules.isEmpty()) {
        arguments << QString(QLL("--rules="))
            + this->m_options.rules.join(QLatin1Char(','));
    }
    if (this->m_options.check) {
        arguments << QString(QLL("--check"));
    }
    if (this->m_options.agent) {
        arguments << QString(QLL("--ai"));
    }
    if (this->m_options.config.verbose) {
        arguments << QString(QLL("--verbose"));
    }
    const FormatConfig & config = this->m_options.config;
    if ( ! config.snapshotManifest.isEmpty()) {
        arguments << QString(QLL("--snapshot-manifest")) << config.snapshotManifest;
    }
    if ( ! config.file.isEmpty()) {
        arguments << QString(QLL("--config-file")) << config.file;
    }
    for (int i = 0; i < config.adds.size(); ++i) {
        arguments << QString(QLL("--config-add")) << config.adds.at(i);
    }
    for (int i = 0; i < config.assignments.size(); ++i) {
        arguments << QString(QLL("--config")) << config.assignments.at(i);
    }
    for (int i = 0; i < config.includePaths.size(); ++i) {
        arguments << QString(QLL("-I")) << config.includePaths.at(i);
    }
    arguments << QString(QLL("--"));
    return arguments;
}

/*!
    Writes @a result as lines a parent's decode() reads back: a `file` line,
    then one `issue` line per issue and one `request` line per request, and
    last an `end` line that tells the result is complete (see decodeEnd());
    each field is separated by a tab, and each text is in percent-encoding.
*/
QByteArray FormatProcessRunner::encode(const FormatFileResult & result)
{
    const QByteArray path = result.path.toUtf8().toPercentEncoding();
    QByteArray text = "file\t" + path + '\t' + (result.changed ? '1' : '0') + '\t'
        + result.error.toUtf8().toPercentEncoding() + '\n';
    for (int i = 0; i < result.issues.size(); ++i) {
        text += FormatProcessRunner::encodeIssue(
            "issue",
            path,
            result.issues.at(i)
        );
    }
    for (int i = 0; i < result.requests.size(); ++i) {
        text += FormatProcessRunner::encodeIssue(
            "request",
            path,
            result.requests.at(i)
        );
    }
    for (int i = 0; i < result.warnings.size(); ++i) {
        text += FormatProcessRunner::encodeIssue(
            "warning",
            path,
            result.warnings.at(i)
        );
    }
    text += "end\t" + path + '\n';
    return text;
}

/*!
    Tells whether @a line is the `end` line of a result that encode() wrote,
    and if so, sets @a path to the result's file.
*/
bool FormatProcessRunner::decodeEnd(const QByteArray & line, QString *path)
{
    const QList<QByteArray > fields = line.split('\t');
    if (fields.size() != 2 || fields.at(0) != "end") {
        return false;
    }
    *path = QString::fromUtf8(QByteArray::fromPercentEncoding(fields.at(1)));
    return true;
}

/*!
    Reads one @a line that encode() wrote into @a results: a `file` line
    appends a result, and an `issue` line adds to the last result of its
    file; returns false for a line of neither kind.
*/
bool FormatProcessRunner::decode(
        const QByteArray & line,
        QList<FormatFileResult > *results)
{
    const QList<QByteArray > fields = line.split('\t');
    if (fields.size() == 4 && fields.at(0) == "file") {
        FormatFileResult result;
        result.path = QString::fromUtf8(
            QByteArray::fromPercentEncoding(fields.at(1))
        );
        result.changed = fields.at(2) == "1";
        result.error = QString::fromUtf8(
            QByteArray::fromPercentEncoding(fields.at(3))
        );
        results->append(result);
        return true;
    }
    if ((fields.size() == 6 || fields.size() == 7)
        && (fields.at(0) == "issue" || fields.at(0) == "request"
            || fields.at(0) == "warning")
    ) {
        const QString path = QString::fromUtf8(
            QByteArray::fromPercentEncoding(fields.at(1))
        );
        for (int i = results->size() - 1; i >= 0; --i) {
            if (results->at(i).path != path) {
                continue;
            }
            const FormatIssue issue(
                fields.at(2).toInt(),
                QByteArray::fromPercentEncoding(fields.at(4)),
                QString::fromUtf8(
                    QByteArray::fromPercentEncoding(fields.at(5))
                ),
                fields.at(3).toInt(),
                fields.size() == 7 ? fields.at(6).toInt() : 0
            );
            if (fields.at(0) == "issue") {
                (*results)[i].issues.append(issue);
            } else if (fields.at(0) == "request") {
                (*results)[i].requests.append(issue);
            } else {
                (*results)[i].warnings.append(issue);
            }
            return true;
        }
    }
    return false;
}

/*!
    Builds the line that carries @a issue of the file @a path (already
    percent-encoded) as a @a kind line (`issue` or `request`): its kind,
    path, first and last line, rule, message, and column limit.
*/
QByteArray FormatProcessRunner::encodeIssue(
        const char *kind, const QByteArray & path,
        const FormatIssue & issue)
{
    return QByteArray(kind) + '\t' + path + '\t' + QByteArray::number(issue.line) + '\t'
        + QByteArray::number(issue.endLine) + '\t' + issue.rule.toPercentEncoding() + '\t'
        + issue.message.toUtf8().toPercentEncoding() + '\t'
        + QByteArray::number(issue.columnLimit) + '\n';
}

#endif // QT_NO_PROCESS
