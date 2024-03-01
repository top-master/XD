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

#ifndef QITEMDELEGATE_P_H
#define QITEMDELEGATE_P_H


#ifndef QT_NO_ITEMVIEWS
#include "qitemdelegate.h"
#include <qtextlayout.h>
#include <qpainter.h>
#include <qitemeditorfactory.h>
#include <private/qobject_p.h>
#include <private/qabstractitemdelegate_p.h>


QT_BEGIN_NAMESPACE

class QItemDelegatePrivate : public QAbstractItemDelegatePrivate
{
    Q_DECLARE_PUBLIC(QItemDelegate)

public:
    QItemDelegatePrivate() : f(0), clipPainting(true) {}

    static inline QItemDelegatePrivate *get(const QItemDelegate *o) { return const_cast<QItemDelegate *>(o)->d_func(); }


    inline const QItemEditorFactory *editorFactory() const
        { return f ? f : QItemEditorFactory::defaultFactory(); }

    inline QIcon::Mode iconMode(QStyle::State state) const
        {
            if (state & QStyle::State_Enabled) {
                return (state & QStyle::State_Selected)
                    ? QIcon::Selected
                    : QIcon::Normal;
            }
            return QIcon::Disabled;
        }

    inline QIcon::State iconState(QStyle::State state) const
        { return state & QStyle::State_Open ? QIcon::On : QIcon::Off; }

    // TRACE/widgets #license; no move to header.
    static QString replaceNewLine(QString text);

    QString valueToText(const QVariant &value, const QStyleOptionViewItem &option) const;

    QItemEditorFactory *f;
    bool clipPainting;

    QRect textLayoutBounds(const QStyleOptionViewItem &options) const;
    QSizeF doTextLayout(int lineWidth) const;
    mutable QTextLayout textLayout;
    mutable QTextOption textOption;

    const QWidget *widget(const QStyleOptionViewItem &option) const
    {
        return option.widget;
    }

    // ### temporary hack until we have QStandardItemDelegate
    mutable struct Icon {
        QIcon icon;
        QIcon::Mode mode;
        QIcon::State state;
    } tmp;
};

QT_END_NAMESPACE

#endif // QT_NO_ITEMVIEWS

#endif //QITEMDELEGATE_P_H
