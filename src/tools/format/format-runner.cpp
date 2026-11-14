// Copyright (C) 2015 The XD Company Ltd.
// SPDX-License-Identifier: Apache-2.0

#include "./format-runner.h"
#include "./format-process-runner.h"
#include "./format-thread-runner.h"

#include <QtCore/QDir>
#include <QtCore/QFile>
#include <QtCore/QFileInfo>
#include <QtCore/QSaveFile>
#include <QtCore/QTemporaryDir>
#include <QtCore/QDataStream>

#if defined(Q_OS_UNIX)
#  include <unistd.h>
#elif defined(Q_OS_WIN) && !defined(Q_OS_WINRT)
#  include <QtCore/qt_windows.h>
#endif

#ifndef QT_NO_THREAD
#  include <QtCore/QThread>
#endif

/*!
    @class FormatRunner
    @internal

    Formats a list of files with one Formatter, and collects their results in
    the order of the list.

    The base class formats the files one after the other, in this thread;
    its subclasses spread them over threads (FormatThreadRunner) or over
    child processes (FormatProcessRunner), each compiled only where its
    Qt feature exists (QT_NO_THREAD and QT_NO_PROCESS unset). The actual
    formatting stays in Formatter; hence every runner gives the same result.
*/

/*!
    Builds the runner that formats with @a formatter as @a options say.
*/
FormatRunner::FormatRunner(const Formatter *formatter, const FormatOptions & options)
    : m_formatter(formatter), m_options(options), m_listener(0), m_prepared(false)
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
    Builds the runner of @a kind (one of kinds(), or empty for defaultKind()),
    or returns null for a kind this build lacks.
*/
FormatRunner *FormatRunner::create(
        const QString & kind, const Formatter *formatter,
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
    Formats @a files one after the other, telling the listener of each one as
    it finishes (see tellFinished()), and returns their results in that order.
*/
QList<FormatFileResult > FormatRunner::run(const QStringList & files)
{
    QString error;
    if ( ! this->prepareRun(files, &error)) {
        return this->runFailure(files, error);
    }
    QList<FormatFileResult > results;
    for (int i = 0; i < files.size(); ++i) {
        results.append(this->formatFile(files.at(i)));
        this->tellFinished(results.last());
    }
    return results;
}

/*!
    Makes @a copy of @a original as a hard link when possible, else copies
    the bytes. FormatRunner writes through QSaveFile, which replaces the
    original directory entry and leaves the linked content intact.
*/
static bool snapshotFile(const QString &original, const QString &copy)
{
    if ( ! QFileInfo(original).isSymLink()) {
#if defined(Q_OS_UNIX)
        const QByteArray sourceName = QFile::encodeName(original);
        const QByteArray copyName = QFile::encodeName(copy);
        if (::link(sourceName.constData(), copyName.constData()) == 0) {
            return true;
        }
#elif defined(Q_OS_WIN) && !defined(Q_OS_WINRT)
        const QString sourceName = QDir::toNativeSeparators(original);
        const QString copyName = QDir::toNativeSeparators(copy);
        if (CreateHardLinkW(reinterpret_cast<LPCWSTR>(copyName.utf16()),
                            reinterpret_cast<LPCWSTR>(sourceName.utf16()), Q_NULLPTR)) {
            return true;
        }
#endif
    }
    return QFile::copy(original, copy);
}

bool FormatRunner::prepareRun(const QStringList &files, QString *error)
{
    FormatConfig &config = this->m_options.config;
    if (this->m_snapshotDirectory) {
        this->m_snapshotDirectory.reset();
        config.snapshots.clear();
        config.snapshotManifest.clear();
        config.scheduledFiles.clear();
    } else if (this->m_prepared && config.snapshotManifest.isEmpty()) {
        config.scheduledFiles.clear();
    }
    this->m_prepared = true;
    if ( ! config.snapshots.isEmpty() || ! config.snapshotManifest.isEmpty()) {
        return true;
    }
    if (config.scheduledFiles.isEmpty()) {
        config.scheduledFiles = files;
    }
    config.fileLineSpecs = this->m_options.fileLineSpecs;
    config.lineSpecs = this->m_options.lineSpecs;
    bool checksSuper = false;
    const QList<const FormatRule * > rules = this->m_formatter->selectedRules();
    for (int i = 0; i < rules.size(); ++i) {
        checksSuper = checksSuper || rules.at(i)->name() == "super-typedef";
    }
    if ( ! checksSuper || config.scheduledFiles.size() < 2) {
        return true;
    }
    this->m_snapshotDirectory.reset(new QTemporaryDir);
    if ( ! this->m_snapshotDirectory->isValid()) {
        *error = QString(QLL("cannot create a temporary directory for formatting snapshots"));
        return false;
    }
    for (int i = 0; i < config.scheduledFiles.size(); ++i) {
        const QString original = QDir::cleanPath(QFileInfo(config.scheduledFiles.at(i)).absoluteFilePath());
        const QString copy = this->m_snapshotDirectory->path() + QLatin1Char('/')
            + QString::number(i);
        QFile input(original);
        if ( ! input.open(QIODevice::ReadOnly)) {
            continue;
        }
        input.close();
        if ( ! snapshotFile(original, copy)) {
            *error = QString(QLL("cannot snapshot %1")).arg(original);
            return false;
        }
        config.snapshots.insert(original, copy);
    }
    config.snapshotManifest = this->m_snapshotDirectory->path() + QLL("/manifest");
    QFile manifest(config.snapshotManifest);
    if ( ! manifest.open(QIODevice::WriteOnly)) {
        *error = QString(QLL("cannot write the formatting snapshot manifest"));
        return false;
    }
    QDataStream output(&manifest);
    output << config.scheduledFiles << config.snapshots << config.fileLineSpecs << config.lineSpecs;
    if (output.status() != QDataStream::Ok) {
        *error = QString(QLL("cannot write the formatting snapshot manifest"));
        return false;
    }
    return true;
}

QList<FormatFileResult > FormatRunner::runFailure(const QStringList &files,
                                                  const QString &error) const
{
    QList<FormatFileResult > results;
    for (int i = 0; i < files.size(); ++i) {
        FormatFileResult result;
        result.path = files.at(i);
        result.error = error;
        results << result;
        this->tellFinished(result);
    }
    return results;
}

/*!
    Hands @a result, the result of one file, to the listener set with
    setListener(), when there is one. Each runner calls it once per file, in
    the order of the files, from one thread at a time; hence a listener
    needs no lock of its own.
*/
void FormatRunner::tellFinished(const FormatFileResult & result) const
{
    if (this->m_listener) {
        this->m_listener->fileFinished(result);
    }
}

/*!
    @class FormatFileListener
    @internal

    Learns of each file's result as soon as a FormatRunner finished the file
    (see FormatRunner::tellFinished()), before run() returns them all; hence
    a long run can show its progress.
*/

/*!
    Destroys the listener.
*/
FormatFileListener::~FormatFileListener()
{
}

/*!
    Formats the file at @a path, and writes the result back unless the
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

    // Formats it, on the changed lines when `--git` lists the file.
    const QString key = QDir::cleanPath(QFileInfo(path).absoluteFilePath());
    const FormatResult formatted = this->m_formatter->format(
        text,
        path,
        this->m_options.fileLineSpecs.value(key, this->m_options.lineSpecs),
        this->m_options.config,
        this->m_options.agent
    );
    result.changed = formatted.changed;
    result.issues = formatted.issues;
    result.requests = formatted.requests;
    result.warnings = formatted.warnings;
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
        result.error = QString(QLL("cannot write: %1"))
            .arg(output.errorString());
    }
    return result;
}

/*!
    Builds how many jobs may run at once for @a fileCount files: the
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
