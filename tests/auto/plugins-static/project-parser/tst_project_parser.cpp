// Copyright (C) 2015 The XD Company Ltd.
// SPDX-License-Identifier: Apache-2.0

#include "project-parser.h"

#include <QtCore/QDir>
#include <QtCore/QFile>
#include <QtCore/QFileInfo>
#include <QtCore/QTemporaryDir>
#include <QtTest/QtTest>

/// Tests the qmake project reader: the words of qmake's variable dump, and a
/// real qmake run over a subdirs project and the projects it holds.
class tst_ProjectParser : public QObject
{
    Q_OBJECT
    typedef QObject super;
private slots:
    /// Words.
    void testWords_shouldSplitAtSpaces();
    void testWords_shouldKeepQuotedSpaces();
    void testWords_shouldKeepEmptyQuotedWord();

    /// Variable dump.
    void testParse_shouldReadEachVariable();
    void testParse_shouldSkipOtherLines();

    /// Runs of qmake.
    void testRun_shouldListFilesOfEverySubproject();
    void testRun_shouldListIncludeFolders();
    void testRun_shouldSkipMissingFiles();
    void testRun_shouldLeaveStaticPluginFilesToThePlugin();
    void testRun_shouldLeaveFilesOfBuiltStaticPluginOut();
    void testRun_shouldListFilesOfStaticPluginProject();
    void testRun_shouldRejectMissingProject();
    void testRun_shouldReportQmakeThatCannotRun();

private:
    QString writeFile(const QString & relative, const QByteArray & text) const;
    QString qmake() const;
    QString absolute(const QString & relative) const;

    QTemporaryDir m_dir;
};

/*!
    Writes \a text into the file at \a relative under the test's folder,
    making its folders, and builds its absolute path.
*/
QString tst_ProjectParser::writeFile(const QString & relative,
                                     const QByteArray & text) const
{
    const QString path = this->absolute(relative);
    QDir().mkpath(QFileInfo(path).absolutePath());
    QFile file(path);
    if (file.open(QIODevice::WriteOnly)) {
        file.write(text);
    }
    return path;
}

/*!
    Builds the path of the qmake that built the test.
*/
QString tst_ProjectParser::qmake() const
{
#if defined(Q_OS_WIN)
    return QString::fromLocal8Bit(TEST_QMAKE_BINS) + QLL("/qmake.exe");
#else
    return QString::fromLocal8Bit(TEST_QMAKE_BINS) + QLL("/qmake");
#endif
}

/*!
    Builds the clean absolute path of \a relative under the test's folder.
*/
QString tst_ProjectParser::absolute(const QString & relative) const
{
    return QDir::cleanPath(this->m_dir.path() + QLatin1Char('/') + relative);
}

void tst_ProjectParser::testWords_shouldSplitAtSpaces()
{
    qExpect(Project::Parser::words(QString(QLL(" a.cpp  b.cpp\tc.cpp "))))
        ->toEqual(QStringList() << QString(QLL("a.cpp")) << QString(QLL("b.cpp"))
                                << QString(QLL("c.cpp")));
}

void tst_ProjectParser::testWords_shouldKeepQuotedSpaces()
{
    qExpect(Project::Parser::words(QString(QLL("\"sp ace/x y.cpp\" z.cpp"))))
        ->toEqual(QStringList() << QString(QLL("sp ace/x y.cpp")) << QString(QLL("z.cpp")));
}

void tst_ProjectParser::testWords_shouldKeepEmptyQuotedWord()
{
    qExpect(Project::Parser::words(QString(QLL("a \"\" b"))))
        ->toEqual(QStringList() << QString(QLL("a")) << QString() << QString(QLL("b")));
}

void tst_ProjectParser::testParse_shouldReadEachVariable()
{
    Project::Parser parser;
    const QHash<QString, QStringList > values = parser.parse(
        "TEMPLATE = subdirs\nSUBDIRS = app lib\nb.subdir = \"sp ace\"\nEMPTY = \r\n");
    qExpect(values.value(QString(QLL("TEMPLATE"))))->toEqual(QStringList() << QString(QLL("subdirs")));
    qExpect(values.value(QString(QLL("SUBDIRS"))))
        ->toEqual(QStringList() << QString(QLL("app")) << QString(QLL("lib")));
    qExpect(values.value(QString(QLL("b.subdir"))))->toEqual(QStringList() << QString(QLL("sp ace")));
    qExpect(values.contains(QString(QLL("EMPTY"))))->toBeTruthy();
    qExpect(values.value(QString(QLL("EMPTY"))))->toBeEmpty();
}

void tst_ProjectParser::testParse_shouldSkipOtherLines()
{
    Project::Parser parser;
    const QHash<QString, QStringList > values = parser.parse(
        "Project MESSAGE: a = b\nnot a variable\nSOURCES = a.cpp\n");
    qExpect(values.size())->toEqual(1);
    qExpect(values.value(QString(QLL("SOURCES"))))->toEqual(QStringList() << QString(QLL("a.cpp")));
}

/// Reads a subdirs project whose entries reach their projects in each way:
/// a plain folder (`app/app.pro`), a `.file`, and a `.subdir` with a space.
void tst_ProjectParser::testRun_shouldListFilesOfEverySubproject()
{
    this->writeFile(QString(QLL("run/app/main.cpp")), "int main() { return 0; }\n");
    this->writeFile(QString(QLL("run/app/app.h")), "int app;\n");
    this->writeFile(QString(QLL("run/app/app.pro")), "SOURCES = main.cpp\nHEADERS = app.h\n");
    this->writeFile(QString(QLL("run/inner/one.cpp")), "int one;\n");
    this->writeFile(QString(QLL("run/inner/one.pro")), "SOURCES = one.cpp\n");
    this->writeFile(QString(QLL("run/sp ace/x y.cpp")), "int x;\n");
    this->writeFile(QString(QLL("run/sp ace/sp ace.pro")), "SOURCES = \"x y.cpp\"\n");
    const QString top = this->writeFile(QString(QLL("run/top.pro")),
        "TEMPLATE = subdirs\nSUBDIRS = app a b\na.file = inner/one.pro\n"
        "b.subdir = \"sp ace\"\n");
    Project::Parser parser;
    QString error;
    qExpect(parser.run(top, this->qmake(), QStringList(), &error))->toBeTruthy();
    qExpect(error)->toBeEmpty();
    qExpect(parser.files())->toEqual(QStringList()
        << this->absolute(QString(QLL("run/app/main.cpp")))
        << this->absolute(QString(QLL("run/app/app.h")))
        << this->absolute(QString(QLL("run/inner/one.cpp")))
        << this->absolute(QString(QLL("run/sp ace/x y.cpp"))));
    qExpect(parser.projects().size())->toEqual(4);
}

void tst_ProjectParser::testRun_shouldListIncludeFolders()
{
    QDir().mkpath(this->absolute(QString(QLL("inc/include"))));
    this->writeFile(QString(QLL("inc/a.cpp")), "int a;\n");
    const QString project = this->writeFile(QString(QLL("inc/inc.pro")),
        "SOURCES = a.cpp\nINCLUDEPATH += include missing\n");
    Project::Parser parser;
    QString error;
    qExpect(parser.run(project, this->qmake(), QStringList(), &error))->toBeTruthy();
    const QStringList folders = parser.includePaths();
    qExpect(folders.contains(this->absolute(QString(QLL("inc/include")))))->toBeTruthy();
    qExpect(folders.contains(this->absolute(QString(QLL("inc/missing")))))->toBeFalsy();
}

/// Lists only the files that exist, since a generated or missing source has
/// nothing to format.
void tst_ProjectParser::testRun_shouldSkipMissingFiles()
{
    this->writeFile(QString(QLL("gone/here.cpp")), "int here;\n");
    const QString project = this->writeFile(QString(QLL("gone/gone.pro")),
                                            "SOURCES = here.cpp away.cpp\n");
    Project::Parser parser;
    QString error;
    qExpect(parser.run(project, this->qmake(), QStringList(), &error))->toBeTruthy();
    qExpect(parser.files())
        ->toEqual(QStringList() << this->absolute(QString(QLL("gone/here.cpp"))));
}

/// Lists a project's own files alone: a source that the project's
/// `STATIC_PLUGINS` bring in belongs to the plugin's own project.
void tst_ProjectParser::testRun_shouldLeaveStaticPluginFilesToThePlugin()
{
    this->writeFile(QString(QLL("own/main.cpp")), "int main() { return 0; }\n");
    this->writeFile(QString(QLL("own/plugin/p.cpp")), "int p;\n");
    this->writeFile(QString(QLL("own/plugin/p.h")), "int p();\n");
    const QString project = this->writeFile(QString(QLL("own/own.pro")),
        "STATIC_PLUGINS.fake.sources = $$PWD/plugin/p.cpp\n"
        "STATIC_PLUGINS.fake.headers = $$PWD/plugin/p.h\n"
        "SOURCES = main.cpp plugin/p.cpp\nHEADERS = plugin/p.h\nINCLUDEPATH = plugin\n");
    Project::Parser parser;
    QString error;
    const bool ran = parser.run(project, this->qmake(), QStringList(), &error);
    qExpect(error)->toBeEmpty();
    qExpect(ran)->toBeTruthy();
    qExpect(parser.files())
        ->toEqual(QStringList() << this->absolute(QString(QLL("own/main.cpp"))));
    qExpect(parser.includePaths().contains(this->absolute(QString(QLL("own/plugin")))))
        ->toBeTruthy();
}

/// Reads a project that links a static plugin this Qt built: the files that
/// `static_plugins.prf` brings in stay out of the list.
void tst_ProjectParser::testRun_shouldLeaveFilesOfBuiltStaticPluginOut()
{
    this->writeFile(QString(QLL("linked/main.cpp")), "int main() { return 0; }\n");
    const QString project = this->writeFile(QString(QLL("linked/linked.pro")),
        "STATIC_PLUGINS += console\nSOURCES = main.cpp\n");
    Project::Parser parser;
    QString error;
    const bool ran = parser.run(project, this->qmake(), QStringList(), &error);
    qExpect(error)->toBeEmpty();
    qExpect(ran)->toBeTruthy();
    qExpect(parser.files())
        ->toEqual(QStringList() << this->absolute(QString(QLL("linked/main.cpp"))));
}

void tst_ProjectParser::testRun_shouldListFilesOfStaticPluginProject()
{
    this->writeFile(QString(QLL("plugin/p.cpp")), "int p;\n");
    this->writeFile(QString(QLL("plugin/p.h")), "int p();\n");
    const QString project = this->writeFile(QString(QLL("plugin/plugin.pro")),
        "TEMPLATE = aux\nPLUGIN_HEADERS = p.h\nPLUGIN_SOURCES = p.cpp\n");
    Project::Parser parser;
    QString error;
    const bool ran = parser.run(project, this->qmake(), QStringList(), &error);
    qExpect(error)->toBeEmpty();
    qExpect(ran)->toBeTruthy();
    qExpect(parser.files())->toEqual(QStringList()
        << this->absolute(QString(QLL("plugin/p.cpp")))
        << this->absolute(QString(QLL("plugin/p.h"))));
}

void tst_ProjectParser::testRun_shouldRejectMissingProject()
{
    Project::Parser parser;
    QString error;
    const QString project = this->absolute(QString(QLL("none/none.pro")));
    qExpect(parser.run(project, this->qmake(), QStringList(), &error))->toBeFalsy();
    qExpect(error)->toContain(QString(QLL("no such project file")));
}

void tst_ProjectParser::testRun_shouldReportQmakeThatCannotRun()
{
    const QString project = this->writeFile(QString(QLL("bad/bad.pro")), "SOURCES = a.cpp\n");
    Project::Parser parser;
    QString error;
    const QString qmake = this->absolute(QString(QLL("no-such-qmake")));
    qExpect(parser.run(project, qmake, QStringList(), &error))->toBeFalsy();
    qExpect(error)->toContain(QString(QLL("cannot run ")) + qmake);
}

QTEST_GUILESS_MAIN(tst_ProjectParser)
#include "tst_project_parser.moc"
