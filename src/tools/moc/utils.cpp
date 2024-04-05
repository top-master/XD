/****************************************************************************
**
** Copyright (C) 2015 The XD Company Ltd.
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing/
**
** This file is part of the tools applications of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL21$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see http://www.qt.io/terms-conditions. For further
** information use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file. Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** As a special exception, The Qt Company gives you certain additional
** rights. These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "utils.h"
#include <QFile>
#include <QFileInfo>
#include <QDir>

#ifdef Q_OS_WIN
#include "qt_windows.h"
#endif

#define MOC_NO_TOKEN
#include "moc.h"

inline QString invalidRoot() {
    QString s = QString::fromUtf8((const char *)__FILE__);
    s.chop(9); //size of "utils.cpp" is 9
    return s;
}
inline QString validRoot() {
    QString binDir;
#ifdef Q_OS_WIN
    {
        wchar_t buffer[MAX_PATH+1];
        if(::GetModuleFileName(0, buffer, MAX_PATH)) {
            buffer[MAX_PATH] = 0;
            QString moduleFileName = QString::fromWCharArray(buffer);
            QFileInfo fi(moduleFileName);
            binDir = fi.absolutePath();
        }
    }
#endif
    return QDir::toNativeSeparators( binDir + QLatin1String("/../src/tools/moc"));
}

QString xdTranslatePath(const QString &filePath)
{
    static const QString rootInvalid = invalidRoot();
    if(filePath.startsWith(rootInvalid)) {
        static const QString root = validRoot();
        return root + filePath.mid(rootInvalid.length() - 1);
    }
    return filePath;
}



QString moc_mark_impl()
{
    // TRACE/moc path: input file's path is NOT encoded with `QFile::encodeName` #2,
    // but passing never-encoded path to `QFile::decodeName` should be harmless, and
    // is more future compatible.
    return QFile::decodeName(Moc::filename);
}


void errorAt(const char *file, const char *msg, int line)
{
    if (msg)
        qWarning(ErrorFormatStringByOption("moc: Error: %s"),
                 file, line, msg);
    else
        qWarning(ErrorFormatStringByOption("moc: Unknown error"),
                 file, line);
    Moc::onExitStatic(EXIT_FAILURE);
}


/**
 * Similar to combinePath(), but accepts file-paths as input.
 *
 * @param targetFile Path of Parent file, which the Child file will target.
 * @param referingFile Path of Child file which will contain returned link, hence
 * refering from Child file to Parent file.
 *
 * @returns Encoded path to @p targetFile file, if written inside @p referingFile file.
 *
 * @see QFile::encodName for what "encode" means.
 */
QByteArray linkToParent(const QString &targetFile, const QString &referingFile)
{
    const QDir pwd = QDir::current();
    QFileInfo refererInfo(pwd, referingFile);
    // Path may already be relative to referer, or even absolute.
    QFileInfo targetInfo(refererInfo.dir(), targetFile);
    if ( ! targetInfo.exists()) {
        // Fallback to relative to PWD.
        targetInfo = QFileInfo(pwd, targetFile);
    }
    int numCommonComponents = 0;

    QStringList inSplitted = targetInfo.absolutePath().split(QLatin1Char('/'));
    QStringList outSplitted = refererInfo.absolutePath().split(QLatin1Char('/'));
    while (!inSplitted.isEmpty() && !outSplitted.isEmpty() &&
            inSplitted.first() == outSplitted.first()) {
        inSplitted.removeFirst();
        outSplitted.removeFirst();
        numCommonComponents++;
    }

    if (numCommonComponents < 2) {
        // The paths don't have the same drive,
        // or they don't have the same root directory,
        // hence use an absolute file-path.
        const QByteArray &resultEncoded = QFile::encodeName(targetInfo.absoluteFilePath());
#if defined(QT_DEBUG) && 0
        if ( ! targetInfo.exists()) {
            errorAt(qPrintable(refererInfo.absoluteFilePath()),
                    qPrintable(QLL("Failed to find file: ") + targetFile));
        }
#endif
        return resultEncoded;
    }

    // The paths have something in common, u
    // hence create a file-path relative to the refering file.
    int distance = outSplitted.count();
    while (distance > 0) {
        inSplitted.prepend(QLatin1String(".."));
        --distance;
    }
    inSplitted.append(targetInfo.fileName());

    const QString &result = inSplitted.join(QLatin1String("/"));
    const QByteArray &resultEncoded = QFile::encodeName(result);
#ifdef Q_OS_WIN
    // Support MAX_PATH limit (which some compilers yet use).
    if (refererInfo.dir().absolutePath().length() + resultEncoded.length() + 1 >= 260) {
        return QFile::encodeName(targetInfo.absoluteFilePath());
    }
#endif

    return resultEncoded;
}

QByteArray linkToParent(const QByteArray &targetFile, const QByteArray &referingFile)
{
    return linkToParent(QFile::decodeName(targetFile), QFile::decodeName(referingFile));
}

QByteArray linkFromDir(const char *target, const QDir &dir)
{
    QString pathIn = QFile::decodeName(target);
    QFileInfo inFileInfo(QDir::current(), pathIn);
    pathIn = inFileInfo.dir().canonicalPath();

    QString path = dir.relativeFilePath(pathIn);
    xd("linkFromDir" << pathIn << path);
    return QFile::encodeName(path);
}

QByteArray linkFromDir(const char *target, const char *currentDir)
{
    const QString &path = QFile::decodeName(currentDir);
    xd("linkFromDir A" << path);
    return linkFromDir(target, QDir(path));
}
