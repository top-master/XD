// Copyright (C) 2015 The XD Company Ltd.
// SPDX-License-Identifier: Apache-2.0

#include "./format-process-runner.h"

#ifndef QT_NO_PROCESS

#include <QtCore/QCoreApplication>
#include <QtCore/QProcess>

/*!
    \class FormatProcessRunner
    \internal

    Formats the files of a run in up to jobCount() child processes of the
    tool itself, each given a slice of the files with the `--worker` option,
    which makes it print its results in the form that encode() writes.
*/

/*!
    Builds the runner that formats with \a formatter as \a options say.
*/
FormatProcessRunner::FormatProcessRunner(const Formatter *formatter, const FormatOptions & options)
    : super(formatter, options)
{
}

/*!
    Formats \a files in child processes, and returns their results in the
    order of \a files; a file whose child process failed
    gets that failure as its error.
*/
QList<FormatFileResult > FormatProcessRunner::run(const QStringList & files)
{
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
        process->start(QCoreApplication::applicationFilePath(),
                       this->workerArguments() + slices.at(i));
        processes.append(process);
    }

    // Gathers the children's results.
    QList<FormatFileResult > gathered;
    for (int i = 0; i < processes.size(); ++i) {
        QProcess *process = processes.at(i);
        const bool finished = process->waitForFinished(-1);
        const QList<QByteArray > lines = process->readAllStandardOutput().split('\n');
        for (int l = 0; l < lines.size(); ++l) {
            FormatProcessRunner::decode(lines.at(l), &gathered);
        }
        if ( ! finished || process->exitStatus() != QProcess::NormalExit) {
            const QStringList & slice = slices.at(i);
            for (int f = 0; f < slice.size(); ++f) {
                FormatFileResult failed;
                failed.path = slice.at(f);
                failed.error = QString(QLL("worker failed: %1")).arg(process->errorString());
                gathered.append(failed);
            }
        }
    }
    qDeleteAll(processes);

    // Puts the results in the order of the files.
    QList<FormatFileResult > results;
    for (int i = 0; i < files.size(); ++i) {
        FormatFileResult result;
        result.path = files.at(i);
        result.error = QString(QLL("worker gave no result"));
        for (int g = 0; g < gathered.size(); ++g) {
            if (gathered.at(g).path == files.at(i)) {
                result = gathered.at(g);
                break;
            }
        }
        results.append(result);
    }
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
    const FormatConfig & config = this->m_options.config;
    if ( ! config.file.isEmpty()) {
        arguments << QString(QLL("--config-file")) << config.file;
    }
    for (int i = 0; i < config.adds.size(); ++i) {
        arguments << QString(QLL("--config-add")) << config.adds.at(i);
    }
    for (int i = 0; i < config.assignments.size(); ++i) {
        arguments << QString(QLL("--config")) << config.assignments.at(i);
    }
    arguments << QString(QLL("--"));
    return arguments;
}

/*!
    Writes \a result as lines a parent's decode() reads back: a `file` line,
    then one `issue` line per issue, each field separated by a tab and each
    text in percent-encoding.
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
    return text;
}

/*!
    Reads one \a line that encode() wrote into \a results: a `file` line
    appends a result, and an `issue` line adds to the last result of its
    file; returns false for a line of neither kind.
*/
bool FormatProcessRunner::decode(const QByteArray & line,
                                 QList<FormatFileResult > *results)
{
    const QList<QByteArray > fields = line.split('\t');
    if (fields.size() == 4 && fields.at(0) == "file") {
        FormatFileResult result;
        result.path = QString::fromUtf8(QByteArray::fromPercentEncoding(fields.at(1)));
        result.changed = fields.at(2) == "1";
        result.error = QString::fromUtf8(QByteArray::fromPercentEncoding(fields.at(3)));
        results->append(result);
        return true;
    }
    if ((fields.size() == 6 || fields.size() == 7)
        && (fields.at(0) == "issue" || fields.at(0) == "request")
    ) {
        const QString path = QString::fromUtf8(QByteArray::fromPercentEncoding(fields.at(1)));
        for (int i = results->size() - 1; i >= 0; --i) {
            if (results->at(i).path != path) {
                continue;
            }
            const FormatIssue issue(
                fields.at(2).toInt(),
                QByteArray::fromPercentEncoding(fields.at(4)),
                QString::fromUtf8(QByteArray::fromPercentEncoding(fields.at(5))),
                fields.at(3).toInt(),
                fields.size() == 7 ? fields.at(6).toInt() : 0
            );
            if (fields.at(0) == "issue") {
                (*results)[i].issues.append(issue);
            } else {
                (*results)[i].requests.append(issue);
            }
            return true;
        }
    }
    return false;
}

/*!
    Builds the line that carries \a issue of the file \a path (already
    percent-encoded) as a \a kind line (`issue` or `request`): its kind,
    path, first and last line, rule, message, and column limit.
*/
QByteArray FormatProcessRunner::encodeIssue(const char *kind, const QByteArray & path,
                                            const FormatIssue & issue)
{
    return QByteArray(kind) + '\t' + path + '\t' + QByteArray::number(issue.line) + '\t'
        + QByteArray::number(issue.endLine) + '\t' + issue.rule.toPercentEncoding() + '\t'
        + issue.message.toUtf8().toPercentEncoding() + '\t'
        + QByteArray::number(issue.columnLimit) + '\n';
}

#endif // QT_NO_PROCESS
