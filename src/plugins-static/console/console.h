// Copyright (C) 2015 The XD Company Ltd.
// SPDX-License-Identifier: Apache-2.0

#ifndef CONSOLE_H
#define CONSOLE_H

#include <QtCore/QIODevice>
#include <QtCore/QString>

class Console
{
public:
    Console(QIODevice *input, QIODevice *output);

    bool confirm(const QString & message);
    QString prompt(const QString & message);
    QString promptMulti(const QString & message);
    void setTerminal(int inputFd, bool styled);
    void page(const QString & text);

private:
    QString readLine(bool *atEnd);
    void drawPagerLine(const QString & typed);
    bool pageOnTerminal();

    QIODevice *m_input;
    QIODevice *m_output;
    int m_terminalFd;
    bool m_styled;
};

#endif // CONSOLE_H
