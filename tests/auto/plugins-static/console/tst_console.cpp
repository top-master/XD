// Copyright (C) 2015 The XD Company Ltd.
// SPDX-License-Identifier: Apache-2.0

#include "console.h"
#include "logger.h"

#include <QtCore/QBuffer>
#include <QtTest/QtTest>

/// Tests the logger's layout and wrapping, and the console's prompts, on
/// in-memory devices.
class tst_Console : public QObject
{
    Q_OBJECT
    typedef QObject super;
private slots:
    /// Logger.
    void testLogLine_shouldWriteOutput();
    void testLogError_shouldNameTool();
    void testLogDiagnostic_shouldKeepLineAsIs();
    void testLogWarning_shouldPadAndIndent();
    void testLogWarning_shouldBoldOnTerminal();
    void testWrap_shouldBreakBetweenWords_data();
    void testWrap_shouldBreakBetweenWords();

    /// Console.
    void testConfirm_shouldReadYesAndNo_data();
    void testConfirm_shouldReadYesAndNo();
    void testPrompt_shouldReadOneLine();
    void testPromptMulti_shouldJoinBackslashLines();
    void testPage_shouldWaitForQ();
    void testPage_shouldEndWithInput();
};

void tst_Console::testLogLine_shouldWriteOutput()
{
    QBuffer output;
    QBuffer errors;
    output.open(QIODevice::WriteOnly);
    errors.open(QIODevice::WriteOnly);
    Logger logger(QString(QLL("tool")), &output, &errors);
    logger.logLine(QString(QLL("a.cpp: formatted")));
    logger.logRaw("raw");
    qExpect(output.data())->toEqual(QByteArray("a.cpp: formatted\nraw"));
    qExpect(errors.data())->toBeEmpty();
}

void tst_Console::testLogError_shouldNameTool()
{
    QBuffer output;
    QBuffer errors;
    output.open(QIODevice::WriteOnly);
    errors.open(QIODevice::WriteOnly);
    Logger logger(QString(QLL("tool")), &output, &errors);
    logger.logError(QString(QLL("no files given")));
    qExpect(errors.data())->toEqual(QByteArray("tool: no files given\n"));
}

void tst_Console::testLogDiagnostic_shouldKeepLineAsIs()
{
    QBuffer output;
    QBuffer errors;
    output.open(QIODevice::WriteOnly);
    errors.open(QIODevice::WriteOnly);
    Logger logger(QString(QLL("tool")), &output, &errors);
    logger.logDiagnostic(QString(QLL("a.cpp:3: error: bad")));
    qExpect(errors.data())->toEqual(QByteArray("a.cpp:3: error: bad\n"));
}

void tst_Console::testLogWarning_shouldPadAndIndent()
{
    QBuffer output;
    QBuffer errors;
    output.open(QIODevice::WriteOnly);
    errors.open(QIODevice::WriteOnly);
    Logger logger(QString(QLL("tool")), &output, &errors);
    logger.setWidth(40);
    logger.logWarning(QString(QLL("one two three four five six seven eight\nnine")));
    qExpect(errors.data())->toEqual(QByteArray("\nWARNING: one two three four five six\n"
                                                "         seven eight\n"
                                                "         nine\n\n"));
}

void tst_Console::testLogWarning_shouldBoldOnTerminal()
{
    QBuffer output;
    QBuffer errors;
    output.open(QIODevice::WriteOnly);
    errors.open(QIODevice::WriteOnly);
    Logger logger(QString(QLL("tool")), &output, &errors);
    logger.setStyled(true);
    logger.logWarning(QString(QLL("careful")));
    qExpect(errors.data())->toEqual(QByteArray("\n\033[1mWARNING:\033[22m careful\n\n"));
}

void tst_Console::testWrap_shouldBreakBetweenWords_data()
{
    QTest::addColumn<QString >("text");
    QTest::addColumn<int >("width");
    QTest::addColumn<QStringList >("lines");
    QTest::newRow("fits") << QString(QLL("a b c")) << 10
        << (QStringList() << QString(QLL("a b c")));
    QTest::newRow("breaks") << QString(QLL("aaa bbb ccc")) << 7
        << (QStringList() << QString(QLL("aaa bbb")) << QString(QLL("ccc")));
    QTest::newRow("long word alone") << QString(QLL("a longerword b")) << 5
        << (QStringList() << QString(QLL("a")) << QString(QLL("longerword")) << QString(QLL("b")));
    QTest::newRow("newline") << QString(QLL("a\nb")) << 10
        << (QStringList() << QString(QLL("a")) << QString(QLL("b")));
}

void tst_Console::testWrap_shouldBreakBetweenWords()
{
    QFETCH(QString, text);
    QFETCH(int, width);
    QFETCH(QStringList, lines);
    QCOMPARE(Logger::wrap(text, width), lines);
}

void tst_Console::testConfirm_shouldReadYesAndNo_data()
{
    QTest::addColumn<QByteArray >("answer");
    QTest::addColumn<bool >("result");
    QTest::newRow("yes") << QByteArray("y\n") << true;
    QTest::newRow("empty") << QByteArray("\n") << true;
    QTest::newRow("ended input") << QByteArray() << true;
    QTest::newRow("no") << QByteArray("n\n") << false;
    QTest::newRow("No with spaces") << QByteArray("  No \r\n") << false;
}

void tst_Console::testConfirm_shouldReadYesAndNo()
{
    QFETCH(QByteArray, answer);
    QFETCH(bool, result);
    QBuffer input;
    input.setData(answer);
    input.open(QIODevice::ReadOnly);
    QBuffer output;
    output.open(QIODevice::WriteOnly);
    Console console(&input, &output);
    qExpect(console.confirm(QString(QLL("Go on?"))))->toEqual(result);
    qExpect(output.data())->toEqual(QByteArray("Go on? [Y/n] "));
}

void tst_Console::testPrompt_shouldReadOneLine()
{
    QBuffer input;
    input.setData("first\nsecond\n");
    input.open(QIODevice::ReadOnly);
    QBuffer output;
    output.open(QIODevice::WriteOnly);
    Console console(&input, &output);
    qExpect(console.prompt(QString(QLL("Name:"))))->toEqualString(QLL("first"));
    qExpect(console.prompt(QString(QLL("Next:"))))->toEqualString(QLL("second"));
    qExpect(output.data())->toEqual(QByteArray("Name: Next: "));
}

void tst_Console::testPromptMulti_shouldJoinBackslashLines()
{
    QBuffer input;
    input.setData("one \\\ntwo\\\nthree\nafter\n");
    input.open(QIODevice::ReadOnly);
    QBuffer output;
    output.open(QIODevice::WriteOnly);
    Console console(&input, &output);
    qExpect(console.promptMulti(QString(QLL("Text:"))))->toEqualString(QLL("one \ntwo\nthree"));
    qExpect(console.prompt(QString(QLL("Then:"))))->toEqualString(QLL("after"));
}

/// Shows the text, then asks again for each answer that is not `q`, and
/// leaves the line after the `q` unread.
void tst_Console::testPage_shouldWaitForQ()
{
    QBuffer input;
    input.setData("done?\n\n Q \nafter\n");
    input.open(QIODevice::ReadOnly);
    QBuffer output;
    output.open(QIODevice::WriteOnly);
    Console console(&input, &output);
    console.page(QString(QLL("#: a request")));
    qExpect(output.data())->toEqual(QByteArray("#: a request\n: (type q to continue) "
                                               ": (type q to continue) : (type q to continue) "));
    qExpect(console.prompt(QString(QLL("Next:"))))->toEqualString(QLL("after"));
}

void tst_Console::testPage_shouldEndWithInput()
{
    QBuffer input;
    input.setData("not yet\n");
    input.open(QIODevice::ReadOnly);
    QBuffer output;
    output.open(QIODevice::WriteOnly);
    Console console(&input, &output);
    console.page(QString(QLL("#: a request\n")));
    qExpect(output.data())->toEqual(QByteArray("#: a request\n: (type q to continue) "
                                               ": (type q to continue) "));
}

QTEST_APPLESS_MAIN(tst_Console)
#include "tst_console.moc"
