/****************************************************************************
**
** Copyright (C) 2015 The XD Company Ltd.
**
** This file is part of the QtExtras module of the XD Toolkit.
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

#ifndef EXTRAS_BORDERIMAGE_H
#define EXTRAS_BORDERIMAGE_H

#include <QtCore/qstring.h>
#include <QtCore/qrect.h>
#include <QtCore/qmargins.h>
#include <QtCore/qmutex.h>
#include <QtCore/qvarlengtharray.h>
#include <QtGui/qpixmap.h>
//#include <QtWidgets/QStyleOptionGraphicsItem>


QT_BEGIN_NAMESPACE
class QPainter;
QT_END_NAMESPACE

/// A border image is an image that is composed of nine parts:
///
/// top left (corner), top center, top right (corner),
/// center left, center, center right,
/// bottom left (corner), bottom center, and bottom right (corner).
///
/// When a border of a certain size is required, the corner parts are used as is,
/// and the top, right, bottom, and left parts are
/// stretched or repeated to produce a border with the desired size.
///
class BorderImage
{
protected:
    void inital();

public:
    inline BorderImage();
    BorderImage(const QPixmap &pixmap, int cornerW, int cornerH = -1, Qt::Orientation o = Qt::Vertical);
    BorderImage(const QString &fileName, int cornerW, int cornerH = -1, Qt::Orientation o = Qt::Vertical);
    BorderImage(const char *s, int cornerW, int cornerH = -1, Qt::Orientation o = Qt::Vertical);
    ~BorderImage();

    BorderImage(const BorderImage &other);
    BorderImage &operator=(const BorderImage &);

    void fromPixmap(const QPixmap &pixmap, int cornerW, int cornerH = -1, Qt::Orientation o = Qt::Vertical);
    void fromPixmap(const QString &fileName, int cornerW, int cornerH = -1, Qt::Orientation o = Qt::Vertical);
    void fromPixmap(const char* s, int cornerW, int cornerH = -1, Qt::Orientation o = Qt::Vertical);

    inline QPixmap *pixmap() { return &d->pixmap; }
    inline void setPixmap(const QPixmap &v) { d->pixmap = v; }

    void paint(QPainter *painter, const QRect &rect);
    inline void paint(QPainter *painter, const QRectF &rect) { paint(painter, rect.toRect()); }
    void paint(QPainter *painter);

    void paint(QPainter *painter, const QRect &rect, const QPixmap &icon);
    inline void paint(QPainter *painter, const QRectF &rect, const QPixmap &icon) { paint(painter, rect.toRect(), icon); }
    void paint(QPainter *painter, const QPixmap &icon);

    /// Caller should take ownership for returned pointer (`delete` it).
    QImage *paintImage(const QSize &size);

    friend class ThumbViewTheme;

    int minLeft() const;
    int minTop() const;
    int minRight() const;
    int minBottom() const;

    int minCenterWidth() const;
    int minCenterHeight() const;

    int minWidth() const;
    int minHeight() const;

    int maxLeft() const;
    int maxTop() const;
    int maxRight() const;
    int maxBottom() const;

    int maxCenterWidth() const;
    int maxCenterHeight() const;

    int maxWidth() const;
    int maxHeight() const;

    inline void setOrientation(Qt::Orientation o) { d->orientation = o; }
    inline Qt::Orientation orientation() { return d->orientation; }

    inline void setMargins(const QMargins &v) { d->margins = v; }
    inline void setMargins(int left, int top, int right, int bottom) { d->margins.setLeft(left); d->margins.setTop(top); d->margins.setRight(right); d->margins.setBottom(bottom); }
    inline void setMargins(int v) { d->margins.setLeft(v); d->margins.setTop(v); d->margins.setRight(v); d->margins.setBottom(v); }
    inline QMargins *margins() { return &d->margins; }

    QRect contentRect(const QRect &outer) const;
    QRectF contentRect(const QRectF &outer) const;

    QRect boundingRect(const QRect &content) const;

    inline bool isCenterRequired() const { return d->isCenterRequired; }
    void setCenterRequired(bool v) { d->isCenterRequired = v; }

    inline void surfaceFromMargins() { surfaceFromMargins(d->margins, d->pixmap.size()); }
    void surfaceFromMargins(const QMargins &v, const QSize &pixmap);

    void surfaceFromCorner(const QSize &corner, const QSize &pixmap);
    inline void surfaceFromCorner(int corner, const QSize &pixmap) { surfaceFromCorner(QSize(corner, corner), pixmap); }
    inline void surfaceFromCorner(int corner) { surfaceFromCorner(QSize(corner, corner), d->pixmap.size()); }

protected:
    enum LayoutPosition {
        lp_topLeft = 0,
        lp_topCenter = 1,
        lp_topRight = 2,
        lp_centerLeft = 3,
        lp_center = 4,
        lp_centerRight = 5,
        lp_bottomLeft = 6,
        lp_bottomCenter = 7,
        lp_bottomRight = 8,
        lp_Count
    };

    struct Datas {
        inline Datas()
            : images(BorderImage::lp_Count)
            , isCenterRequired(false)
        {}

        QPixmap pixmap;
        QVarLengthArray<QRect, lp_Count> images;
        Qt::Orientation orientation;
        QMargins margins;
        bool isCenterRequired;
    };
    Datas *d;

    inline QRect topLeft()      const { return d->images.at(lp_topLeft); }
    inline QRect topCenter()    const { return d->images.at(lp_topCenter); }
    inline QRect topRight()     const { return d->images.at(lp_topRight); }
    inline QRect centerLeft()   const { return d->images.at(lp_centerLeft); }
    inline QRect center()       const { return d->images.at(lp_center); }
    inline QRect centerRight()  const { return d->images.at(lp_centerRight); }
    inline QRect bottomLeft()   const { return d->images.at(lp_bottomLeft); }
    inline QRect bottomCenter() const { return d->images.at(lp_bottomCenter); }
    inline QRect bottomRight()  const { return d->images.at(lp_bottomRight); }

    inline void setTopLeft(const QRect &v)      { d->images.replace(lp_topLeft, v);      }
    inline void setTopCenter(const QRect &v)    { d->images.replace(lp_topCenter, v);    }
    inline void setTopRight(const QRect &v)     { d->images.replace(lp_topRight, v);     }
    inline void setCenterLeft(const QRect &v)   { d->images.replace(lp_centerLeft, v);   }
    inline void setCenter(const QRect &v)       { d->images.replace(lp_center, v);       }
    inline void setCenterRight(const QRect &v)  { d->images.replace(lp_centerRight, v);  }
    inline void setBottomLeft(const QRect &v)   { d->images.replace(lp_bottomLeft, v);   }
    inline void setBottomCenter(const QRect &v) { d->images.replace(lp_bottomCenter, v); }
    inline void setBottomRight(const QRect &v)  { d->images.replace(lp_bottomRight, v);  }

    inline QRect topLeftTarget(const QRect &rect)      { return QRect(rect.x(), rect.y(), topLeft().width(), topLeft().height()); }
    inline QRect topCenterTarget(const QRect &rect)    { return QRect(rect.x() + topLeft().width(), rect.y(),
                                                                      rect.width() - topLeft().width() - topRight().width(), topCenter().height()); }
    inline QRect topRightTarget(const QRect &rect)     { return QRect(rect.x() + topLeft().width() + rect.width() - (topLeft().width()*2), rect.y(), topLeft().width(), topLeft().height()); }
    inline QRect centerLeftTarget(const QRect &rect)   { return QRect(rect.x(), rect.y() + topLeft().height(),
                                                                      centerLeft().width(), rect.height() - topLeft().height() - bottomLeft().height()); }
    inline QRect centerTarget(const QRect &rect)       { return QRect(rect.x() + topLeft().width(), rect.y() + topLeft().height(),
                                                                      rect.width() - topLeft().width() - topRight().width(), rect.height() - topLeft().height() - bottomLeft().height()); }
    inline QRect centerRightTarget(const QRect &rect)  { return QRect(rect.x() + rect.width() - topRight().width(), rect.y() + topRight().height(),
                                                                      centerRight().width(), rect.height() - topRight().height() - bottomRight().height()); }
    inline QRect bottomLeftTarget(const QRect &rect)   { return QRect(rect.x(), rect.y() + topLeft().height() + rect.height() - (topLeft().height()*2), topLeft().width(), topLeft().height()); }
    inline QRect bottomCenterTarget(const QRect &rect) { return QRect(rect.x() + bottomLeft().width(), rect.y() + rect.height() - bottomLeft().height(),
                                                                      rect.width() - bottomLeft().width() - bottomRight().width(), bottomCenter().height()); }
    inline QRect bottomRightTarget(const QRect &rect)  { return QRect(rect.x() + topLeft().width() + rect.width() - (topLeft().width()*2), rect.y() +  topLeft().height() + rect.height() - (topLeft().height()*2), topLeft().width(), topLeft().height()); }
};

BorderImage::BorderImage() { inital(); }

#endif // EXTRAS_BORDERIMAGE_H
