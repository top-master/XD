// Copyright (C) 2015 The XD Company Ltd.
// SPDX-License-Identifier: Apache-2.0

#ifndef FORMAT_PROJECT_FILES_H
#define FORMAT_PROJECT_FILES_H

#include <QtCore/QHash>
#include <QtCore/QMutex>
#include <QtCore/QString>
#include <QtCore/QStringList>
#include <QtCore/QWaitCondition>

class FormatProjectFiles
{
public:
    inline FormatProjectFiles() : m_finished(false) {}

    void add(const QStringList & files, const QStringList & includePaths);
    void finish();
    QStringList takeUnparsed(bool wait);
    QStringList includePaths();
    bool isFinished();

private:
    /// Guards every other member, which the reading thread and the
    /// formatting one both change.
    QMutex m_mutex;
    /// Wakes a takeUnparsed() that waits, once files arrive or reading ends.
    QWaitCondition m_changed;
    /// Maps each file's absolute path to whether a batch took it already.
    QHash<QString, bool > m_isParsed;
    /// Orders the batches: the files in the order they arrived.
    QStringList m_order;
    /// Collects the include folders of every reading, each once.
    QStringList m_includePaths;
    /// Tells takeUnparsed() that no more files may arrive.
    bool m_finished;
};

#endif // FORMAT_PROJECT_FILES_H
