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

#ifndef QSTRINGNOQUOTE_H
#define QSTRINGNOQUOTE_H

#include <QtCore/qstring.h>

QT_BEGIN_NAMESPACE

// A QString that QDebug prints WITHOUT the surrounding quotes, so a formatted token (a pointer address,
// an integer) reads like its printf counterpart -- 0x1f4, 500 -- rather than a quoted "0x1f4". Its
// QDebug operator<< suppresses quoting for just this value and restores the stream's prior quote state
// -- a following QString still quotes when quoting was on, and an already-unquoted stream stays
// unquoted -- and it is a plain QString in every other respect. QDebug declares that operator<< as a
// member ahead of its QString overload (so it out-resolves it on every compiler); this header carries
// the operator's out-of-line body, hence streaming a QStringNoQuote needs this header included.
class Q_CORE_EXPORT QStringNoQuote : public QString
{
public:
    Q_ALWAYS_INLINE QStringNoQuote() {}
    Q_ALWAYS_INLINE QStringNoQuote(const QString &other) : QString(other) {}
#ifdef Q_COMPILER_RVALUE_REFS
    Q_ALWAYS_INLINE QStringNoQuote(QString &&other) Q_DECL_NOEXCEPT : QString(qMove(other)) {}
#endif
};

QT_END_NAMESPACE

#include <QtCore/qdebug.h>

QT_BEGIN_NAMESPACE

#ifndef QT_NO_DEBUG_STREAM
Q_ALWAYS_INLINE QDebug &QDebug::operator<<(const QStringNoQuote & t)
{
    if (this->autoInsertQuotes()) {
        this->noquote();
        *this << static_cast<const QString &>(t);
        this->quote();
    } else {
        *this << static_cast<const QString &>(t);
    }
    return *this;
}
#endif

QT_END_NAMESPACE

#endif // QSTRINGNOQUOTE_H
