// Copyright (C) 2015 The XD Company Ltd.
// SPDX-License-Identifier: Apache-2.0

#include "./format-runner.h"
#include "./format-process-runner.h"
#include "./format-thread-runner.h"

#include <QtCore/QFile>
#include <QtCore/QSaveFile>

#ifndef QT_NO_THREAD
#  include <QtCore/QThread>
#endif

/*!
    \class FormatRunner
    \internal

    Formats a list of files with one Formatter, and collects their results in
    the order of the list.

    The base class formats the files one after the other, in this thread;
    its subclasses spread them over threads (FormatThreadRunner) or over
    child processes (FormatProcessRunner), each compiled only where its
    Qt feature exists (QT_NO_THREAD and QT_NO_PROCESS unset). The actual
    formatting stays in Formatter; hence every runner gives the same result.
*/

/*!
    Builds the runner that formats with \a formatter as \a options say.
*/
FormatRunner::FormatRunner(const Formatter *formatter, const FormatOptions & options)
    : m_formatter(formatter), m_options(options)
{
}

/*!
    Destroys the runner.
*/
FormatRunner::~FormatRunner()
{
}

/*!
    Lists the runner kinds that this build of the tool has: `serial` always,
    then `thread` and `process` when their Qt features exist.
*/
QStringList FormatRunner::kinds()
{
    QStringList kinds;
    kinds << QString(QLL("serial"));
#ifndef QT_NO_THREAD
    kinds << QString(QLL("thread"));
#endif
#ifndef QT_NO_PROCESS
    kinds << QString(QLL("process"));
#endif
    return kinds;
}

/*!
    Names the runner kind to use when none is asked for: `thread` where
    threads exist (a thread starts far faster than a process, on every
    platform), else `process`, else `serial`.
*/
QString FormatRunner::defaultKind()
{
#ifndef QT_NO_THREAD
    return QString(QLL("thread"));
#elif !defined(QT_NO_PROCESS)
    return QString(QLL("process"));
#else
    return QString(QLL("serial"));
#endif
}

/*!
    Builds the runner of \a kind (one of kinds(), or empty for defaultKind()),
    or returns null for a kind this build lacks.
*/
FormatRunner *FormatRunner::create(const QString & kind, const Formatter *formatter,
                                   const FormatOptions & options)
{
    const QString name = kind.isEmpty()
        ? FormatRunner::defaultKind() : kind;
    if (name == QLL("serial")) {
        return new FormatRunner(formatter, options);
    }
#ifndef QT_NO_THREAD
    if (name == QLL("thread")) {
        return new FormatThreadRunner(formatter, options);
    }
#endif
#ifndef QT_NO_PROCESS
    if (name == QLL("process")) {
        return new FormatProcessRunner(formatter, options);
    }
#endif
    return Q_NULLPTR;
}

/*!
    Formats \a files one after the other, and returns
    their results in the same order.
*/
QList<FormatFileResult > FormatRunner::run(const QStringList & files)
{
    QList<FormatFileResult > results;
    for (int i = 0; i < files.size(); ++i) {
        results.append(this->formatFile(files.at(i)));
    }
    return results;
}

/*!
    Formats the file at \a path, and writes the result back unless the
    options only check it.
*/
FormatFileResult FormatRunner::formatFile(const QString & path) const
{
    FormatFileResult result;
    result.path = path;

    // Reads the file.
    QFile file(path);
    if ( ! file.open(QIODevice::ReadOnly)) {
        result.error = QString(QLL("cannot read: %1")).arg(file.errorString());
        return result;
    }
    const QByteArray text = file.readAll();
    file.close();

    // Formats it.
    const FormatResult formatted = this->m_formatter->format(
        text,
        path,
        this->m_options.lineSpecs,
        this->m_options.config,
        this->m_options.agent
    );
    result.changed = formatted.changed;
    result.issues = formatted.issues;
    result.requests = formatted.requests;
    result.error = formatted.error;
    if ( ! result.error.isEmpty()
        || ! formatted.changed
        || this->m_options.check
    ) {
        return result;
    }

    // Writes it back.
    QSaveFile output(path);
    if ( ! output.open(QIODevice::WriteOnly)
        || output.write(formatted.text) != formatted.text.size()
        || ! output.commit()
    ) {
        result.error = QString(QLL("cannot write: %1")).arg(output.errorString());
    }
    return result;
}

/*!
    Builds how many jobs may run at once for \a fileCount files: the
    options' count, else one per CPU core, but never more than the files.
*/
int FormatRunner::jobCount(int fileCount) const
{
    int jobs = this->m_options.jobs;
#ifndef QT_NO_THREAD
    if (jobs <= 0) {
        jobs = QThread::idealThreadCount();
    }
#endif
    return qBound(1, jobs, qMax(fileCount, 1));
}
