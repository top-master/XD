/****************************************************************************
**
** Copyright (C) 2015 The XD Company Ltd.
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing/
**
** This file is part of the tools applications of the Qt Toolkit.
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

#ifndef Q_MOC_CONFIG_H
#define Q_MOC_CONFIG_H

#include <QtCore/qglobal.h>

#ifndef QT_MOC_NO_REMOTE
#  define QT_REMOTE 1
#else
#  define QT_REMOTE 0
#endif

#ifndef QT_MOC_MACRO_EXPAND
// TRACE/moc performance: maybe should disable macro expand support by default,
// by setting below to zero, since moc run's duration gets around 200% or 1400%,
// for exsample the duration for QtCore expands from "30 sec" to "7 min",
// that means 14 times slower.
#  define QT_MOC_MACRO_EXPAND 1
#elif (QT_MOC_MACRO_EXPAND) != 0 && (QT_MOC_MACRO_EXPAND) != 1
#  undef QT_MOC_MACRO_EXPAND
#  define QT_MOC_MACRO_EXPAND 1
#endif

#ifdef MOC_NO_TOKEN
#  define MOC_TOKEN_SCOPE(x)
#else
#  define MOC_TOKEN_SCOPE(x) x
#endif

#endif // Q_MOC_CONFIG_H
