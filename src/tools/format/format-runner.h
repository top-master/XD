// Copyright (C) 2015 The XD Company Ltd.
// SPDX-License-Identifier: Apache-2.0

#ifndef FORMAT_RUNNER_H
#define FORMAT_RUNNER_H

#include "./format-file-result.h"
#include "./format-options.h"
#include "./formatter.h"

#include <QtCore/QList>
#include <QtCore/QString>
#include <QtCore/QStringList>
#include <QtCore/QScopedPointer>

class QTemporaryDir;

class FormatFileListener
{
public:
    virtual ~FormatFileListener();
    virtual void fileFinished(const FormatFileResult & result) = 0;
};

class FormatRunner
{
public:
    FormatRunner(const Formatter *formatter, const FormatOptions & options);
    virtual ~FormatRunner();

    static QStringList kinds();
    static QString defaultKind();
    static FormatRunner *create(
            const QString & kind, const Formatter *formatter,
            const FormatOptions & options);

    virtual QList<FormatFileResult > run(const QStringList & files);
    FormatFileResult formatFile(const QString & path) const;
    inline void setListener(FormatFileListener *listener) { m_listener = listener; }
    void tellFinished(const FormatFileResult & result) const;

protected:
    int jobCount(int fileCount) const;
    bool prepareRun(const QStringList &files, QString *error);
    QList<FormatFileResult > runFailure(const QStringList &files,
                                        const QString &error) const;

    const Formatter *m_formatter;
    FormatOptions m_options;
    FormatFileListener *m_listener;
    QScopedPointer<QTemporaryDir > m_snapshotDirectory;
    bool m_prepared;
};

#endif // FORMAT_RUNNER_H
