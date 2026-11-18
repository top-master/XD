// Copyright (C) 2015 The XD Company Ltd.
// SPDX-License-Identifier: Apache-2.0

#include "./config-scanner.h"

#include <QtCore/QDir>
#include <QtCore/QFileInfo>

/*!
    @class Config::Scanner
    @internal

    Finds a project's settings files level by level, the way git reads its
    `.gitignore` files: the project's root is the folder of the closest
    `.git`, and a file in each folder from that root down to a path's folder
    applies, a deeper one over the ones above it.
*/

/*!
    Builds the root of the project that @a path belongs to: the folder that
    holds the closest `.git` (a folder, or a file as in a worktree) at or
    above it, or else the folder @a path names or sits in.
*/
QString Config::Scanner::findRoot(const QString & path)
{
    const QFileInfo info(path);
    const QString start = QDir::cleanPath(info.isDir() ? info.absoluteFilePath()
                                                      : info.absolutePath());
    QDir dir(start);
    for (;;) {
        if (QFileInfo(dir.filePath(QString(QLL(".git")))).exists()) {
            return QDir::cleanPath(dir.absolutePath());
        }
        if ( ! dir.cdUp()) {
            return start;
        }
    }
}

/*!
    Lists the folders from @a root down to the folder that holds @a path, or
    down to @a path itself when it is a folder, in that order, both ends
    included; @a path outside of @a root has only @a root.
*/
QStringList Config::Scanner::levelFolders(
        const QString & root, const QString & path)
{
    QStringList folders;
    folders << root;
    const QString absolute = QDir::cleanPath(
        QFileInfo(path).absoluteFilePath()
    );

    // Takes a root that already ends in `/`, such as `/` or `C:/`, as it is.
    const QString base = root.endsWith(QLatin1Char('/'))
        ? root : root + QLatin1Char('/');
    if ( ! absolute.startsWith(base)) {
        return folders;
    }
    const QStringList parts = absolute
        .mid(base.size())
        .split(QLatin1Char('/'), QString::SkipEmptyParts);

    // Ends at a folder path itself, and at the folder that holds a file.
    const int count = QFileInfo(absolute).isDir() ? parts.size()
                                                  : parts.size() - 1;
    QString current = base;
    for (int i = 0; i < count; ++i) {
        current += parts.at(i);
        folders << current;
        current += QLatin1Char('/');
    }
    return folders;
}

/*!
    Lists the files named @a fileName that apply to @a path: the one in each
    folder from its project's root down to the folder that holds it, that
    exist, in that order.
*/
QStringList Config::Scanner::levelFiles(
        const QString & path, const QString & fileName)
{
    QStringList files;
    const QString root = Config::Scanner::findRoot(path);
    const QStringList folders = Config::Scanner::levelFolders(root, path);
    for (int i = 0; i < folders.size(); ++i) {
        const QString file = folders.at(i) + QLatin1Char('/') + fileName;
        if (QFileInfo(file).isFile()) {
            files << file;
        }
    }
    return files;
}
