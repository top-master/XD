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

#ifndef QFORMATTER_H
#define QFORMATTER_H

#include <QtCore/qstringnoquote.h>
#include "./qlocale_p.h"

QT_BEGIN_NAMESPACE

// Renders a pointer the way printf's "%p" does (QString::asprintf("%p", p)) -- "0x" followed by
// lowercase hex -- for the cases where a value must be streamed into qDebug()/QDebug and the "%p"
// spelling isn't available.
Q_ALWAYS_INLINE QStringNoQuote qFormatPtr(const void *pointer)
{
    return QStringNoQuote(QLocaleData::classic()->formatULongLong(reinterpret_cast<quintptr>(pointer), -1, 16, -1, QLocaleData::Alternate));
}

// Renders an unsigned 64-bit integer in base 10, matching printf's "%llu", for the same streaming cases.
Q_ALWAYS_INLINE QStringNoQuote qFormatULongLong(quint64 value)
{
    return QStringNoQuote(QLocaleData::classic()->formatULongLong(value, -1, 10, -1, 0));
}

// Short alias for qFormatULongLong(); the body is copied rather than forwarded so it stays a single
// inlined expression.
Q_ALWAYS_INLINE QStringNoQuote qFormatULL(quint64 value)
{
    return QStringNoQuote(QLocaleData::classic()->formatULongLong(value, -1, 10, -1, 0));
}

QT_END_NAMESPACE

#endif // QFORMATTER_H
