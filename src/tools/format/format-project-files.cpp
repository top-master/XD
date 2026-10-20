// Copyright (C) 2015 The XD Company Ltd.
// SPDX-License-Identifier: Apache-2.0

#include "./format-project-files.h"

#include <QtCore/QMutexLocker>

/*!
    \class FormatProjectFiles
    \internal

    Hands the files of the `--project` readings to the formatting, batch by
    batch, while later readings still add to them: each spec's qmake run adds
    its files, and each batch takes the ones no batch took yet. A file that
    several specs list is formatted once.
*/

/*!
    Adds each of \a files that no reading added yet, and each of
    \a includePaths not known yet, then wakes a takeUnparsed() that waits.
*/
void FormatProjectFiles::add(const QStringList & files,
                             const QStringList & includePaths)
{
    QMutexLocker locker(&this->m_mutex);
    for (int i = 0; i < files.size(); ++i) {
        if ( ! this->m_isParsed.contains(files.at(i))) {
            this->m_isParsed.insert(files.at(i), false);
            this->m_order << files.at(i);
        }
    }
    for (int i = 0; i < includePaths.size(); ++i) {
        if ( ! this->m_includePaths.contains(includePaths.at(i))) {
            this->m_includePaths << includePaths.at(i);
        }
    }
    this->m_changed.wakeAll();
}

/*!
    Marks the reading done, then wakes a takeUnparsed() that waits.
*/
void FormatProjectFiles::finish()
{
    QMutexLocker locker(&this->m_mutex);
    this->m_finished = true;
    this->m_changed.wakeAll();
}

/*!
    Builds the list of the files that no batch took yet, in the order they
    arrived, and marks them taken. With \a wait, it waits for files while
    none are left and the reading goes on; hence an empty list then means
    that the reading ended.
*/
QStringList FormatProjectFiles::takeUnparsed(bool wait)
{
    QMutexLocker locker(&this->m_mutex);
    QStringList taken;
    for (;;) {
        for (int i = 0; i < this->m_order.size(); ++i) {
            const QString & path = this->m_order.at(i);
            if ( ! this->m_isParsed.value(path)) {
                this->m_isParsed.insert(path, true);
                taken << path;
            }
        }
        if ( ! taken.isEmpty() || ! wait || this->m_finished) {
            return taken;
        }
        this->m_changed.wait(&this->m_mutex);
    }
}

/*!
    Builds the list of the include folders that the readings found so far.
*/
QStringList FormatProjectFiles::includePaths()
{
    QMutexLocker locker(&this->m_mutex);
    return this->m_includePaths;
}

/*!
    Tells whether the reading ended, hence no more files arrive.
*/
bool FormatProjectFiles::isFinished()
{
    QMutexLocker locker(&this->m_mutex);
    return this->m_finished;
}
