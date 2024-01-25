/****************************************************************************
**
** Copyright (C) 2015 The XD Company Ltd.
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing/
**
** This file is part of the QtCore module of the Qt Toolkit.
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

#include "udebug.h"

#include <QtCore/qstacktrace.h>
#include <QtCore/qdir.h>

#include <stdio.h> //defines FILENAME_MAX
#include <stdarg.h>

#ifdef Q_OS_WIN
    #include <qt_windows.h>
    #include <DbgHelp.h>
#endif //Q_OS_WIN

static bool uDebugAt = false;
static quint32 uDebugCount = 0;

bool xdAtEnabled() {
    return uDebugCount > 0;
}
bool setXdAtEnabled(bool enabled) {
    bool tmp = uDebugAt;
    uDebugAt = enabled;

    if(enabled)
        uDebugCount++;
    else if(uDebugCount > 0)
        uDebugCount--;

    return tmp != enabled;
}

static UDebug::LogFilterFunc globalLogFilter;

// Ensures Debug-Logs are not used unless Debugger is attached.
static void MsgHandler(QtMsgType t, const char* str) {
    Q_UNUSED(t)

    if ( ! globalLogFilter(t, str)) {
        return;
    }

    // TRACE/qmake BugFix 16: replaced Debug-Logs with Console-Prints,
    // because maybe there is a Debug-Listener that does not act so fast, or,
    // maybe it has crashed and is under debug, in which case,
    // any debug print from this/any program will block this/that program,
    // until said Listener runs normally again or is closed.
#if defined(QT_DEBUG) && defined(Q_OS_WIN) && 0
    if(IsDebuggerPresent()) {
        qWinMsgHandler(t, str);
        return;
    }
#endif
    fprintf(stderr, "%s\n", str);

    // TRACE/qmake #logs; manually flush #1.
    fflush(stderr);
}

void UDebug::init(const LogFilterFunc &logFilter)
{
    // Ensures our logs are not malformed.
    qunsetenv("QT_MESSAGE_PATTERN");

    globalLogFilter = logFilter;
    qInstallMsgHandler(MsgHandler);

#ifdef QMAKE_WATCH
    QStackTrace::prepare();
    QStackTrace::setPathTranslatorDefault(&xdTranslatePath);
#endif
}

#define UDEBUG_NO_STACK 1

void UDebug::printStack(const QString &msg, int skipLatest)
{
#ifndef QMAKE_WATCH
    Q_UNUSED(skipLatest)
    if (msg.size() > 0) {
        stream->buffer += QLatin1String("WARNING: ");
        stream->buffer += msg;
        if(stream->buffer.endsWith(QLatin1Char(' ')))
            stream->buffer.chop(1);
    }
#else // QMAKE_WATCH
#  ifdef Q_OS_WIN
    // Plus one to skip self.
    ++skipLatest;

    QStackTrace trace = QStackTrace::capture(skipLatest);
    if (msg.size() > 0) {
        if (trace.next()) {
            this->printLocation(trace.filePath(), trace.fileLineNumber());
            this->stream->buffer += msg;
        } else {
            stream->buffer += QLatin1String("WARNING: ");
            stream->buffer += msg;
        }
        if(stream->buffer.endsWith(QLatin1Char(' ')))
            stream->buffer.chop(1);
    }

#    if !UDEBUG_NO_STACK
    if (msg.size() > 0) {
        // Separates message-log from stack-trace.
        this->stream->buffer += '\n';
    }

    const QChar separatpr = QDir::separator();
    while (trace.next()) {
        const QString &symbolName = trace.symbolName();
        if (symbolName == QLL("main"))
            break;

        this->stream->buffer += QString::asprintf("%i: %s - 0x%08X "
                , trace.depth(), qPrintable(symbolName), trace.symbolAddress()
            );
        const QString &filePath = trace.filePath();
        if ( ! filePath.isEmpty()) {
            int pos = filePath.lastIndexOf(separatpr);
            if (pos > 0) {
                this->stream->buffer += xdTranslatePath(filePath.mid(pos + 1));
                this->stream->buffer += " : ";
                this->stream->buffer += QString::number(trace.fileLineNumber(), 10);
            }
        }
        this->stream->buffer += '\n';
    }
#    endif //UDEBUG_NO_STACK
#  endif //Q_OS_WIN
#endif //QMAKE_WATCH
}

UDebug &UDebug::warn(const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    const QString s = QString::vasprintf(fmt, ap);
    va_end(ap);

    this->stream->buffer.append(QLL("WARNING: "));
    this->stream->buffer.append(s);
    return *this;
}
