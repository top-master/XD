/****************************************************************************
**
** Copyright (C) 2015 The XD Company Ltd.
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing/
**
** This file is part of the QtWidgets module of the Qt Toolkit.
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

#ifndef QPROGRESSBAR_P_H
#define QPROGRESSBAR_P_H

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

#include "qprogressbar.h"
#include <private/qwidget_p.h>


#ifndef QT_NO_PROGRESSBAR

QT_BEGIN_NAMESPACE

class QDialog;
class QPushButton;
class QProgressBarPrivate : public QWidgetPrivate
{
    Q_DECLARE_PUBLIC(QProgressBar)

public:
    QProgressBarPrivate();

    static inline QProgressBarPrivate *get(QProgressBar *o) { return o->d_func(); }
    static inline const QProgressBarPrivate *get(const QProgressBar *o) { return o->d_func(); }


    void init();
    void initDefaultFormat();
    inline void resetLayoutItemMargins();

    int minimum;
    int maximum;
    int value;
    Qt::Alignment alignment;
    uint textVisible : 1;
    uint defaultFormat: 1;
    int lastPaintedValue;
    Qt::Orientation orientation;
    bool invertedAppearance;
    QProgressBar::Direction textDirection;
    QString format;
    inline int bound(int val) const { return qMax(minimum-1, qMin(maximum, val)); }
    bool repaintRequired() const;
};

QT_END_NAMESPACE

#endif // QT_NO_PROGRESSBAR

#endif // QPROGRESSBAR_P_H
