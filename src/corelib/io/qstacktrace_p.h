/****************************************************************************
**
** Copyright (C) 2015 The XD Company Ltd.
**
** This file is part of the QtCore module of the XD Toolkit.
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

#ifndef Q_STACK_TRACE_P_H
#define Q_STACK_TRACE_P_H

#include "qstacktrace.h"

#include <QtCore/qcoreapplication.h>

#if defined(Q_OS_WIN)
#  include <qt_windows.h>
//#  define DBGHELP_TRANSLATE_TCHAR // Forces Unicode version of DbgHelp functions.
//#  pragma comment( lib, "dbghelp.lib" )
#  include <DbgHelp.h>
#endif


QT_BEGIN_NAMESPACE

class QStackTracePrivate {
public:
    inline QStackTracePrivate()
    {
        clear();
    }

    inline ~QStackTracePrivate()
    {
        close();
    }

    static QStackTracePrivate *get(QStackTrace *ptr) { return ptr->d_func(); }
    static const QStackTracePrivate *get(const QStackTrace *ptr) { return ptr->d_func(); }

    inline void clear()
    {
        isOpen = false;
        beginIndex = currentIndex = endIndex = 0;

#ifdef Q_OS_WIN
        process = INVALID_HANDLE_VALUE;
        frameCount = 0;
        line = Q_NULLPTR;
        symbol = Q_NULLPTR;
#endif
    }

    inline void close()
    {
#ifdef Q_OS_WIN
        if (process != INVALID_HANDLE_VALUE)
            ::CloseHandle(process);
        // Dellocate buffers.
        ::free(line);
        ::free(symbol);
#endif
        isOpen = false;
    }

    inline void reset()
    {
        close();
        clear();
    }

    inline bool isValid() const {
        return currentIndex >= beginIndex && currentIndex < endIndex;
    }

#ifdef Q_OS_WIN
    inline bool ensureSymbol() const;
    inline bool loadSymbol();
#endif

public:
    bool isOpen;
    int beginIndex;
    int currentIndex;
    int endIndex;

    QStackTrace::PathTranslator pathTranslator;

#ifdef Q_OS_WIN
    HANDLE process;
    void *stack[ 100 ];
    unsigned short frameCount;


    IMAGEHLP_LINE *line;
    SYMBOL_INFO *symbol;
#endif

};

#ifdef Q_OS_WIN
/// WARNING: don't call from inside of QStackTrace::next().
inline bool QStackTracePrivate::ensureSymbol() const {
    return this->symbol
            ? qstrlen(this->symbol->Name) > 0
            : this->isValid() && const_cast<QStackTracePrivate *>(this)->loadSymbol();
}

/// WARNING: ensure isValid() before calling.
inline bool QStackTracePrivate::loadSymbol() {
    QStackTracePrivate *d = this;

    // Reserve memory for a single symbol (if not already).
    if (d->symbol == Q_NULLPTR) {
        d->symbol = reinterpret_cast<SYMBOL_INFO *>(calloc( sizeof(SYMBOL_INFO) + 256 * sizeof(char), 1 ));
    } else {
        ::memset(d->symbol, 0, sizeof(SYMBOL_INFO));
    }

    if (d->symbol) {
        d->symbol->MaxNameLen   = 255;
        d->symbol->SizeOfStruct = sizeof( SYMBOL_INFO );
        SymFromAddr( d->process, reinterpret_cast<DWORD64>( d->stack[d->currentIndex] ), 0, d->symbol);

        if (d->symbol && strcmp(d->symbol->Name, "main") == 0) {
            // Mark as end of stack.
            d->endIndex = d->currentIndex + 1;
        }

        return qstrlen(d->symbol->Name) > 0;
    }

    return false;
}
#endif

QT_END_NAMESPACE

#endif // Q_STACK_TRACE_P_H
