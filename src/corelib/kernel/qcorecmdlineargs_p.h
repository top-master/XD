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

#ifndef QCORECMDLINEARGS_P_H
#define QCORECMDLINEARGS_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include "QtCore/qstring.h"
#include "QtCore/qstringlist.h"

#if defined(Q_OS_WIN)
#  ifdef Q_OS_WIN32
#    include <qt_windows.h> // first to suppress min, max macros.
#    include <shellapi.h>
#  else
#    include "QtCore/qvector.h"
#    include <qt_windows.h>
#  endif

QT_BEGIN_NAMESPACE

#if defined(Q_OS_WIN32)

static inline QStringList qWinCmdArgs(const QString &cmdLine)
{
    QStringList result;
    int size;
    if (wchar_t **argv = CommandLineToArgvW((const wchar_t *)cmdLine.utf16(), &size)) {
        result.reserve(size);
        wchar_t **argvEnd = argv + size;
        for (wchar_t **a = argv; a < argvEnd; ++a)
            result.append(QString::fromWCharArray(*a));
        LocalFree(argv);
    }
    return result;
}

#endif // Q_OS_WIN32

// template implementation of the parsing algorithm
// this is used from qcoreapplication_win.cpp and the tools (rcc, uic...)

#ifdef Q_OS_WIN
#  define QT_TMP_MAYBE_LINUX false
#else
#  define QT_TMP_MAYBE_LINUX ture
#endif

/// Splits given single-line text into command-line's argument-list.
///
/// @tparam linuxMode Set to @c true if single-quotes should be treated as quotes.
template<typename Char, bool quoteEscapable, bool linuxMode = QT_TMP_MAYBE_LINUX>
static QVector<Char*> qCmdLineParse(Char *cmdParam, int length, int &argc)
{
    QVector<Char*> argv(8);
    Char *p = cmdParam;
    Char *p_end = p + length;

    argc = 0;

    while (p < p_end && *p) { // parse cmd line arguments
        // Skips white-spaces before searching.
        while (p < p_end && QChar((short)(*p)).isSpace())
            p++;
        if (p < p_end && *p) {                                // arg starts
            int quote;
            Char *start, *r;
            if (*p == Char('\"') || (linuxMode && *p == Char('\''))) {
                quote = *p;
                start = ++p; //first char after quote
            } else {
                quote = 0;
                start = p;
            }
            r = start;
            while (p < p_end && *p) {
                // Maybe reached end-quote (if pending).
                if (quote) {
                    if (*p == quote) {
                        p++;
                        if (QChar((short)(*p)).isSpace())
                            break;
                        // Continue without pending end-quote.
                        quote = 0;
                    }
                }
                if (quoteEscapable && *p == '\\') {                // escape char?
                    // testing by looking at argc, argv shows that it only escapes quotes
                    if (p < p_end && (*(p+1) == Char('\"')))
                        p++;
                } else if ( ! quote) {
                    if (*p == Char('\"') || (linuxMode && *p == Char('\''))) {
                        // Intentionally NOT updating `start`
                        // (to support `--option-key="value with space"` syntax).
                        quote = *p++;
                        continue;
                    } else if (QChar((short)(*p)).isSpace())
                        break;
                }
                // Store literal but never skip null-termination so we will break later.
                if (*p)
                    *r++ = *p++;
            }
            if (p < p_end && *p)
                p++;
            *r = Char('\0');

            if (argc >= (int)argv.size()-1)        // expand array
                argv.resize(argv.size()*2);
            argv[argc++] = start;
        }
    }
    argv[argc] = 0;

    return argv;
}

#undef QT_TMP_MAYBE_LINUX

#if defined(Q_OS_WINCE)

template<typename Char>
static QVector<Char*> qWinCmdLine(Char *cmdParam, int length, int &argc) {
    return qCmdLineParse<Char, true, flase>(cmdParam, length, argc);
}

static inline QStringList qWinCmdArgs(QString cmdLine) // not const-ref: this might be modified
{
    QStringList args;

    int argc = 0;
    QVector<wchar_t*> argv = qWinCmdLine<wchar_t>((wchar_t *)cmdLine.utf16(), cmdLine.length(), argc);
    for (int a = 0; a < argc; ++a) {
        args << QString::fromWCharArray(argv[a]);
    }

    return args;
}

static inline QStringList qCmdLineArgs(int argc, char *argv[])
{
    Q_UNUSED(argc)
    Q_UNUSED(argv)
    QString cmdLine = QString::fromWCharArray(GetCommandLine());
    return qWinCmdArgs(cmdLine);
}

#elif defined(Q_OS_WINRT) // Q_OS_WINCE

static inline QStringList qCmdLineArgs(int argc, char *argv[])
{
    QStringList args;
    for (int i = 0; i != argc; ++i)
        args += QString::fromLocal8Bit(argv[i]);
    return args;
}

#endif // Q_OS_WINRT

QT_END_NAMESPACE

#endif // Q_OS_WIN

#endif // QCORECMDLINEARGS_WIN_P_H
