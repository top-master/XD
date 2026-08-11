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

#include "qstacktrace_p.h"

#include <QtCore/qmutex.h>
#include <QtCore/qdir.h>
#include <QtCore/qexception.h>
#ifndef QT_NO_REGULAREXPRESSION
#  include <QtCore/qregularexpression.h>
#elif !defined(QT_NO_REGEXP)
#  include <QtCore/qregexp.h>
#endif


QT_BEGIN_NAMESPACE

static QString defaultPathTranslator(const QString &path)
{
    return path;
}

class QStackTraceGlobals {
public:
    inline QStackTraceGlobals()
        : isPrepared(false)
        , pathTranslator(defaultPathTranslator)
    {}

    QBasicMutex lock;
    volatile bool isPrepared;
    QStackTrace::PathTranslator pathTranslator;

#ifdef Q_OS_WIN

#endif

};

Q_GLOBAL_STATIC(QStackTraceGlobals, qstackTraceGlobals)


QStackTrace::QStackTrace()
    : d_ptr(new QStackTracePrivate())
{
}

bool QStackTrace::prepare()
{
    QStackTraceGlobals *globals = qstackTraceGlobals();
    if ( ! globals) {
        return false;
    }
    QMutexLocker _(&globals->lock);
    if (globals->isPrepared) {
        return true;
    }

#ifdef Q_OS_WIN
    HANDLE process = ::GetCurrentProcess();
    // Gets App's directory as search-path.
    wchar_t path[FILENAME_MAX];
    DWORD size = ::GetModuleFileNameW(NULL, path, sizeof(path)/sizeof(path[0]));
    if (size > 0) {
        while (size > 0 && path[size-1] != '\\')
            --size;
        path[size] = 0;
    }
    // Prepare debug help.
    DWORD options = ::SymGetOptions();
    options |= SYMOPT_LOAD_LINES;
    ::SymSetOptions(options);
    ::SymInitializeW( process, path, TRUE );
#endif // Q_OS_WIN

    globals->isPrepared = true;
    return true;
}

QStackTrace QStackTrace::capture(quint32 skipLatest)
{
    QStackTrace result;
    QStackTracePrivate *d = result.d_func();
    if ( ! d) {
        qThrowOutOfMemory();
    }
    QStackTraceGlobals *globals = qstackTraceGlobals();
    d->pathTranslator = globals->pathTranslator;

    if ( ! globals->isPrepared && ! QStackTrace::prepare()) {
        return result;
    }

    // Plus one to skip self.
    ++skipLatest;

#ifdef Q_OS_WIN
    d->process = GetCurrentProcess();
    d->frameCount = CaptureStackBackTrace( 0, 100, d->stack, NULL );

    // Validate.
    if (skipLatest >= d->frameCount)
        skipLatest = d->frameCount - 1;
    if (skipLatest >= 0) {
        d->beginIndex = skipLatest;
        d->endIndex = d->frameCount;
    }
#endif // Q_OS_WIN

    // Start by invalid index.
    d->currentIndex = d->beginIndex - 1;
    d->isOpen = true;

    return result;
}

bool QStackTrace::next()
{
    Q_D(QStackTrace);

    // Validate index.
    if (d->currentIndex < d->endIndex) {
        ++d->currentIndex;
    }
    if (d->currentIndex >= d->endIndex) {
        return false;
    }

#ifdef Q_OS_WIN
    // Reserves memory for line (if not already).
    if (d->line == Q_NULLPTR) {
        d->line = reinterpret_cast<IMAGEHLP_LINE64 *>(calloc( sizeof(IMAGEHLP_LINE64), 1 ));
    } else {
        ::memset(d->line, 0, sizeof(IMAGEHLP_LINE64));
    }
    d->line->SizeOfStruct = sizeof(IMAGEHLP_LINE64);

    DWORD dwDisplacement = 0;
    // Source-code file-names should not contain any unicode.
    // Use the *64 address API so a 64-bit pointer is not truncated to DWORD
    // (matches the DWORD64 SymFromAddr call in qstacktrace_p.h).
    SymGetLineFromAddr64(d->process, reinterpret_cast<DWORD64>(d->stack[d->currentIndex]), &dwDisplacement, d->line);

    // If not first try, check symbol.
    if (d->currentIndex - d->beginIndex > 0) {
        d->loadSymbol();
    }

    return true;
#else
    return false;
#endif
}

bool QStackTrace::restart()
{
    Q_D(QStackTrace);
    if (d->isOpen) {
        // Start by invalid index.
        d->currentIndex = d->beginIndex - 1;
        return true;
    }
    return false;
}

#if !defined(QT_NO_REGULAREXPRESSION) || !defined(QT_NO_REGEXP)
bool QStackTrace::skip(const QString &symbolPattern)
{
    Q_D(QStackTrace);

    // Ensures ready (moves past invalid index).
    if (d->currentIndex < d->beginIndex) {
        if ( ! this->next()) {
            return false;
        }
    }

    bool foundAnyMatch = false;
#if !defined(QT_NO_REGULAREXPRESSION) && 0
    QRegularExpression regExp(symbolPattern);
#else
    QRegExp regExp(symbolPattern);
#endif
    do {
        QString symbol = qMove(this->symbolName());
#if 0
        qDebug() << symbol << filePath() << fileLineNumber() << symbolAddress();
#endif
#if !defined(QT_NO_REGULAREXPRESSION) && 0
        if (regExp.match(symbol).hasMatch())
#else
        if (regExp.indexIn(symbol) > 0)
#endif
        {
            // Skips all symbol matches (we want caller of symbol).
            foundAnyMatch = true;
            continue;
        } else if (foundAnyMatch) {
            return true;
        }
    } while (this->next());

    // Reached end of stack, hence fallback.
    if (foundAnyMatch) {
        d->currentIndex = d->endIndex - 1;
    }

    return foundAnyMatch;
}
#endif // !QT_NO_REGULAREXPRESSION

bool QStackTrace::skipAddress(qptrdiff address)
{
    Q_D(QStackTrace);

    // Ensures ready (moves past invalid index).
    if (d->currentIndex < d->beginIndex &&  ! this->next()) {
        return false;
    }

    bool foundAnyMatch = false;
    do {
        const qptrdiff &symbol = this->symbolAddress();
        if (address == symbol) {
            // Skips symbol match (we want caller of symbol).
            foundAnyMatch = true;
            if (this->next()) {
                return true;
            } else {
                break;
            }
        }
    } while (this->next());

    // Reached end of stack, hence fallback.
    if (foundAnyMatch) {
        d->currentIndex = d->endIndex - 1;
    }

    return foundAnyMatch;
}

QString QStackTrace::filePath() const
{
#ifdef Q_OS_WIN
    const Q_D(QStackTrace);
    if (d->line && d->line->FileName) {
        const char *rawFilePath = d->line->FileName;
        const QString &filePath = QDir::toNativeSeparators(QString::fromLocal8Bit(rawFilePath));

        return d->pathTranslator(filePath);
    }
#endif

    return QString();
}

int QStackTrace::fileLineNumber() const
{
#ifdef Q_OS_WIN
    const Q_D(QStackTrace);
    if (d->line && d->line->FileName) {
        return static_cast<int>(d->line->LineNumber);
    }
#endif

    return 0;
}

QString QStackTrace::symbolName() const
{
#ifdef Q_OS_WIN
    const Q_D(QStackTrace);
    if (d->ensureSymbol()) {
        const char *rawName = d->symbol->Name;
        return QString::fromLocal8Bit(rawName);
    }
#endif

    return QString();
}

qptrdiff QStackTrace::symbolAddress() const
{
#ifdef Q_OS_WIN
    const Q_D(QStackTrace);
    if (d->ensureSymbol()) {
        return qptrdiff(d->symbol->Address);
    }
#endif

    Q_STATIC_ASSERT(Q_NULLPTR == 0);
    return qptrdiff(0);
}

int QStackTrace::currentIndex() const
{
#ifdef Q_OS_WIN
    const Q_D(QStackTrace);
    return d->currentIndex;
#else
    return 0;
#endif
}

int QStackTrace::depth() const
{
#ifdef Q_OS_WIN
    const Q_D(QStackTrace);
    return qMax(int(d->frameCount - d->currentIndex - 1), 0);
#else
    return 0;
#endif
}

QStackTrace::PathTranslator QStackTrace::pathTranslator() const
{
    const Q_D(QStackTrace);
    return d->pathTranslator;
}

void QStackTrace::setPathTranslator(const QStackTrace::PathTranslator &f)
{
    Q_D(QStackTrace);
    d->pathTranslator = f;
}

QStackTrace::PathTranslator QStackTrace::setPathTranslatorDefault(const QStackTrace::PathTranslator &f)
{
    QStackTraceGlobals *globals = qstackTraceGlobals();

    PathTranslator old = globals->pathTranslator;
    globals->pathTranslator = f;
    return old;
}

QString QStackTrace::toString(const QString &msg, int depthLimit, bool skipSymbolOnly)
{
    QString out;
    const QLatin1String lineFade = QLL(QT_NEW_LINE);
    const QLatin1String prefix("WARNING: ");
    int depth = 1;
    if (depthLimit <= 0) {
        depthLimit = (std::numeric_limits<int>::max)();
    }
    // Ensure enough memory.
    out.reserve(1024 * 1024); // 1 MiB.

    while (this->next()) {
        QString file = qMove(this->filePath());
        int line = this->fileLineNumber();
        // Maybe skip if address is missing.
        if (skipSymbolOnly
                && (line <= 0 || file.isEmpty()))
        {
            continue;
        }
        const QString &lineStr = QString::number(line);

        // Join.
        out += prefix;
        out += file;
        out += QLatin1Char(':');
        out += lineStr;

        // Message.
        out += QLatin1Literal(": ");
        if (depth == 1 && ! msg.isEmpty()) {
            out += msg;
        } else {
            out += QLatin1Char('#');
            out += QString::number(depth);
            out += QChar::Space;
            out += this->symbolName();
        }
        out += lineFade;

        if (depth >= depthLimit) {
            break;
        }
        ++depth;
    }
    out.chop(lineFade.size());
    // Releases unused memory while keeping 512 bytes for caller.
    out.squeeze(out.size() + 512);

    return out;
}

QT_END_NAMESPACE
