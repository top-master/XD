/****************************************************************************
**
** Copyright (C) 2026 The XD Company Ltd.
**
** This file is part of the test suite of the XD Toolkit.
**
** $QT_BEGIN_LICENSE:APACHE2$
**
** Licensed under the Apache License, Version 2.0 (the "License");
** you may not use this file except in compliance with the License.
** You may obtain a copy of the License at
**
**     http://www.apache.org/licenses/LICENSE-2.0
**
** Unless required by applicable law or agreed to in writing, software
** distributed under the License is distributed on an "AS IS" BASIS,
** WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
** See the License for the specific language governing permissions and
** limitations under the License.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include <QtCore/QDir>
#include <QtCore/QFileInfo>
#include <QtCore/QLibraryInfo>
#include <QtTest/QtTest>
#include <QtTest/qtestexpectation.h>

class tst_QLibraryInfo : public QObject
{
    Q_OBJECT

private slots:
    inline void binaryPath_shouldBeQtCoreFolderIfNameEmpty() {
        // Dummy.
        const QString path = QLibraryInfo::binaryPath();
        if (path.isNull()) {
            QSKIP("The platform cannot tell where QtCore was loaded from.");
        }

        // Actual test.
        qExpect(QFileInfo(path).isAbsolute())->toBeTruthy();
        qExpect(coreFileIn(path))->Not->toBeEmpty()
                ->withContext("Should hold the QtCore binary.");
    }

    inline void binaryPath_shouldFindLoadedLibraryByBareName() {
        // Dummy.
        const QString path = QLibraryInfo::binaryPath();
        if (path.isNull()) {
            QSKIP("The platform cannot tell where QtCore was loaded from.");
        }
        // With QtCore's bare name, without its platform prefix and suffix.
        const QString name = QLatin1String(QT_CORE_LIBRARY_NAME);

        // Actual test.
        qExpect(QLibraryInfo::binaryPath(name))->toEqual(path)
                ->withContext(qPrintable(name));
    }

    inline void binaryPath_shouldFindLoadedLibraryByFullPath() {
        // Dummy.
        const QString path = QLibraryInfo::binaryPath();
        if (path.isNull()) {
            QSKIP("The platform cannot tell where QtCore was loaded from.");
        }
        const QString filePath = QDir(path).filePath(coreFileIn(path));

        // Actual test.
        qExpect(QLibraryInfo::binaryPath(filePath))->toEqual(path)
                ->withContext(qPrintable(filePath));
    }

    inline void binaryPath_shouldBeNullIfNotLoaded() {
        // Actual test.
        qExpect(QLibraryInfo::binaryPath(QLatin1String("NoSuchXdLibrary")).isNull())->toBeTruthy();
    }

public:
    /// Returns the file name of this build's QtCore binary in @p folder, or an
    /// empty string when there is none.
    static inline QString coreFileIn(const QString &folder) {
        const QLatin1String name(QT_CORE_LIBRARY_NAME);
#if defined(Q_OS_WIN)
        const QString filter = name + QLatin1String(".dll");
#elif defined(Q_OS_DARWIN)
        const QString filter = QLatin1String("lib") + name + QLatin1String("*.dylib");
#else
        const QString filter = QLatin1String("lib") + name + QLatin1String(".so*");
#endif
        const QStringList found = QDir(folder).entryList(QStringList(filter), QDir::Files);
        return found.isEmpty() ? QString() : found.first();
    }
};

QTEST_MAIN(tst_QLibraryInfo)

#include "tst_qlibraryinfo.moc"
