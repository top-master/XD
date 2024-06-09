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

#ifndef Q_STACK_TRACE_H
#define Q_STACK_TRACE_H

#include <QtCore/qglobal.h>
#include <QtCore/qnamespace.h>
#include <QtCore/qsharedpointer.h>
#include <QtCore/qfunction.h>
#include <QtCore/qdebug.h>

class QStackTracePrivate;

/// TODO: support Linux and MacOS platforms.
class Q_CORE_EXPORT QStackTrace {
public:
    QStackTrace();

    static bool prepare();
    static QStackTrace capture(quint32 skipLatest = 0);

    bool next();

    /// Skips stack-trace entries until given @p symbolPattern is reached, and
    /// even once reached, continues skipping until there's NOT any match left.
    ///
    /// @warning For MSVC's release builds with `force_debug_info`, the trace may
    /// end at first symbol that's part of `std` library, unless `std` got inlined.
    bool skip(const QString &symbolPattern);
    inline bool skip(const char *symbolPattern) { return this->skip(QString::fromLocal8Bit(symbolPattern)); }
    bool skipAddress(qptrdiff address);

    QString filePath() const;
    int fileLineNumber() const;

    QString symbolName() const;
    qptrdiff symbolAddress() const;

    int currentIndex() const;
    int depth() const;

    typedef QFunction<QString (const QString &)> PathTranslator;
    PathTranslator pathTranslator() const;
    void setPathTranslator(const PathTranslator &f);
    static PathTranslator setPathTranslatorDefault(const PathTranslator &f);

private:
    Q_DECLARE_PRIVATE_D(d_ptr.data(), QStackTrace)

    QSharedPointer<QStackTracePrivate> d_ptr;
};

#endif // Q_STACK_TRACE_H
