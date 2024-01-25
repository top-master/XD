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

#ifndef EXTRAS_UDEBUG_H
#define EXTRAS_UDEBUG_H

#include <QtCore/qdebug.h>
#include <QtCore/qfunction.h>

class UDebug : public QDebug {
public:
    inline UDebug()
        : QDebug(QtDebugMsg)
    {}

    inline UDebug &printLocation(const QString &file, int line);

    typedef QFunction<bool (*) (QtMsgType, const char *)> LogFilterFunc;
    static void init(const LogFilterFunc &logFilter);
    static inline void init() { init([&] (QtMsgType, const char *) { return true; }); }
    void printStack(const QString &msg, int skipLatest = 0);
    UDebug &warn(const char *format, ...)  Q_ATTRIBUTE_FORMAT_PRINTF(2, 3);
};

inline UDebug &UDebug::printLocation(const QString &file, int line) {
    QString &out = stream->buffer;
    // Ensure enough memory.
    const QLatin1String prefix("WARNING: ");
    const QString &lineStr = QString::number(line);
    out.reserve(out.size() + prefix.size() + file.size() + 1 + lineStr.size() + 2);
    // Join.
    out += prefix;
    out += file;
    out += QLatin1Char(':');
    out += lineStr;
    out += QLatin1Literal(": ");
    return *this;
}

QString xdTranslatePath(const QString &filePath); //defined in qmake/main.cpp

#define xd(text) (UDebug().printLocation(xdTranslatePath(QString::fromLocal8Bit((const char *)__FILE__)), (int)__LINE__) << text)

bool xdAtEnabled();
bool setXdAtEnabled(bool enabled = true);

#define xdStart(condition, text) if(setXdAtEnabled(condition)) xd(text << "{").setEnabled(xdAtEnabled())
#define xdAt(text) xd("    " << text).setEnabled(xdAtEnabled())
#define xdEnd(text) if(setXdAtEnabled(false)) xd("}" << text)

#endif // EXTRAS_UDEBUG_H
