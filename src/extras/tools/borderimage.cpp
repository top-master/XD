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

#include "borderimage.h"

#include <QtCore/qdebug.h>
#include <QtGui/qpainter.h>
#include <QtWidgets/qstyle.h>


void BorderImage::inital()
{
    d = new Datas;
}

BorderImage::BorderImage(const QPixmap &pixmap, int cornerW, int cornerH, Qt::Orientation o)
{
    inital();

    fromPixmap(pixmap, cornerW, cornerH, o);
}
BorderImage::BorderImage(const QString &fileName, int cornerW, int cornerH, Qt::Orientation o)
{
    inital();

    fromPixmap(fileName, cornerW, cornerH, o);
}

BorderImage::BorderImage(const char *s, int cornerW, int cornerH, Qt::Orientation o)
{
    inital();

    fromPixmap(s, cornerW, cornerH, o);
}

BorderImage::~BorderImage()
{
    delete d;
}

BorderImage::BorderImage(const BorderImage &other)
{
    inital();
    operator =(other);
}

BorderImage &BorderImage::operator=(const BorderImage &v)
{
    d->pixmap = v.d->pixmap;
    d->images = v.d->images;
    d->orientation = v.d->orientation;
    return *this;
}

void BorderImage::fromPixmap(const QPixmap &pixmap, int cornerW, int cornerH, Qt::Orientation o)
{
    d->pixmap = pixmap;
    d->orientation = o;

    if(cornerH < 0)
        cornerH = cornerW;
    surfaceFromCorner(QSize(cornerW, cornerH), d->pixmap.size());
}
void BorderImage::fromPixmap(const QString &fileName, int cornerW, int cornerH, Qt::Orientation o)
{
    d->pixmap = QPixmap(fileName);
    d->orientation = o;

    if(cornerH < 0)
        cornerH = cornerW;
    surfaceFromCorner(QSize(cornerW, cornerH), d->pixmap.size());
}

void BorderImage::fromPixmap(const char *s, int cornerW, int cornerH, Qt::Orientation o)
{
    d->pixmap = QPixmap(QLatin1String(s));
    d->orientation = o;

    if(cornerH < 0)
        cornerH = cornerW;
    surfaceFromCorner(QSize(cornerW, cornerH), d->pixmap.size());
}

void BorderImage::paint(QPainter *painter, const QPixmap &icon)
{ paint(painter, painter->clipBoundingRect(), icon); }

void BorderImage::paint(QPainter *painter)
{ paint(painter, painter->clipBoundingRect()); }

inline QRect operator +(QRect r, QPoint p) { return r.translated(p); }
inline QRect operator +(QRect r, QSize s)
{ return QRect(QPoint(r.x(), r.y()), QPoint(r.right() + s.width(), r.bottom() + s.height())); }

void BorderImage::paint(QPainter *painterIn, const QRect &dest)
{
    int i; //# buffer
    bool smooth = false;
    QRect rect;

    if(d->orientation == Qt::Horizontal) {
        rect = QRect(0,0,dest.height(),dest.width());
    } else {
        rect = QRect(0,0,dest.width(),dest.height());
    }

    i = minWidth();
    if (rect.width() < i) { rect.setWidth(i); smooth = true; }
    i = minHeight();
    if (rect.height() < i) { rect.setHeight(i); smooth = true; }

    QImage image(rect.width(), rect.height(), QImage::Format_ARGB32_Premultiplied);
    image.fill(0);
    QPainter painter(&image);

//    if(m_orientation == Qt::Horizontal) {
//        QPointF center;
//        center = rect.center() - rect.topLeft();
//        painter.translate(center);
//        painter.rotate(-90);
//        painter.translate(-center);
//        painter.setRenderHint(QPainter::SmoothPixmapTransform);
//    }

#if 1
    // With inline helpers.
    painter.drawPixmap(topLeftTarget(rect), d->pixmap, topLeft());
    painter.drawPixmap(topCenterTarget(rect), d->pixmap, topCenter());
    painter.drawPixmap(topRightTarget(rect), d->pixmap, topRight());

    painter.drawPixmap(centerLeftTarget(rect), d->pixmap, centerLeft());
    painter.drawPixmap(centerTarget(rect), d->pixmap, center());
    painter.drawPixmap(centerRightTarget(rect), d->pixmap, centerRight());

    painter.drawPixmap(bottomLeftTarget(rect), d->pixmap, bottomLeft());
    painter.drawPixmap(bottomCenterTarget(rect), d->pixmap, bottomCenter());
    painter.drawPixmap(bottomRightTarget(rect), d->pixmap, bottomRight());
#else
    // Without inline helpers.
    painter.drawPixmap(rect.x() + topLeft().width(), rect.y(), rect.width() - (topLeft().width()*2), topLeft().height(), topCenter());
    painter.drawPixmap(rect.x() + topLeft().width() + rect.width() - (topLeft().width()*2), rect.y(), topLeft().width(), topLeft().height(), topRight());

    painter.drawPixmap(rect.x(), rect.y() + topLeft().height(), topLeft().width(), rect.height() - (topLeft().height()*2), centerLeft());
    painter.drawPixmap(rect.x() + topLeft().width(), rect.y() + topLeft().height(), rect.width() - (topLeft().width()*2), rect.height() - (topLeft().height()*2), center());
    painter.drawPixmap(rect.x() + topLeft().width() + rect.width() - (topLeft().width()*2), rect.y() + topLeft().height(), topLeft().width(), rect.height() - (topLeft().height()*2), centerRight());

    painter.drawPixmap(rect.x(), rect.y() + topLeft().height() + rect.height() - (topLeft().height()*2), topLeft().width(), topLeft().height(), bottomLeft());
    painter.drawPixmap(rect.x() + topLeft().width(), rect.y() + topLeft().height() + rect.height() - (topLeft().height()*2), rect.width() - (topLeft().width()*2), topLeft().height(), bottomCenter());
    painter.drawPixmap(rect.x() + topLeft().width() + rect.width() - (topLeft().width()*2), rect.y() +  topLeft().height() + rect.height() - (topLeft().height()*2), topLeft().width(), topLeft().height(), bottomRight());
#endif

    painter.end();

    // Intentionally validates late, since
    // the `isActive` may return different result based on time passed
    // (caller should be happy we don't throw error).
    if ( ! painterIn || ! painterIn->isActive()) {
        return;
    }

    painterIn->save();
    if (smooth) {
//        p->setRenderHint(QPainter::Antialiasing);
        painterIn->setRenderHint(QPainter::SmoothPixmapTransform);
//        p->setRenderHint(QPainter::HighQualityAntialiasing);
    }

//    qDebug() << dest << rectOut;

    if(d->orientation == Qt::Horizontal) {
        //
        // Translate and rotate for horizontal;
        // in some frameworks we need to `translate(...)` to center, and
        // that to rotate correctly, but bottom-left seems to be what works for Qt.
        //
        // note: QRect::right() is one less then left() + width()
        // note: QRect::bottom() is one less then top() + height()
        //
        QPointF bottomLeft = QPointF( dest.left()
                                    , dest.top() + dest.height() );
        painterIn->translate(bottomLeft);
        painterIn->rotate(-90);
        // Don't un-translate to draw at right place, else could do:
        // ```
        // painterIn->translate(-bottomLeft);
        // ```
        QRect rectRotated = QRect(0, 0, dest.height(), dest.width());
        painterIn->drawImage(rectRotated, image);
    } else {
        painterIn->drawImage(dest, image);
    }

    painterIn->restore();
}

void BorderImage::paint(QPainter *painter, const QRect &rect, const QPixmap &icon)
{
    paint(painter, rect);
    painter->drawPixmap(QStyle::alignedRect(Qt::LeftToRight, Qt::AlignCenter,
                                            QSize(qMin(rect.width(), icon.width()), qMin(rect.height(), icon.height())), rect), icon);
}

QImage *BorderImage::paintImage(const QSize &size)
{
    QImage *image = new QImage(size.width(), size.height(), QImage::Format_ARGB32_Premultiplied);
    image->fill(0);
    QPainter painter(image);
    paint(&painter, QRect(QPoint(0,0), size));
    painter.end();
    return image;
}

int BorderImage::minLeft() const {return qMin(qMin(topLeft().width(), centerLeft().width()), bottomLeft().width()); }
int BorderImage::minTop() const  {return qMin(qMin(topLeft().height(), topCenter().height()), topRight().height());}
int BorderImage::minRight() const {return qMin(qMin(topRight().width(), centerRight().width()), bottomRight().width());}
int BorderImage::minBottom() const {return qMin(qMin(bottomLeft().height(), bottomCenter().height()), bottomRight().height());}

int BorderImage::minCenterWidth() const {return qMin(qMin(topCenter().width(), center().width()), bottomCenter().width()); }
int BorderImage::minCenterHeight() const {return qMin(qMin(centerLeft().height(), center().height()), centerRight().height()); }

int BorderImage::minWidth() const {
    int w = minLeft() + minRight();
    if(d->isCenterRequired)
        w += minCenterWidth();
    return w;
}

int BorderImage::minHeight() const {
    int h = minTop() + minBottom();
    if(d->isCenterRequired)
        h += minCenterHeight();
    return h;
}


int BorderImage::maxLeft() const {return qMax(qMax(topLeft().width(), centerLeft().width()), bottomLeft().width());}
int BorderImage::maxTop() const {return qMax(qMax(topLeft().height(), topCenter().height()), topRight().height());}
int BorderImage::maxRight() const {return qMax(qMax(topRight().width(), centerRight().width()), bottomRight().width());}
int BorderImage::maxBottom() const {return qMax(qMax(bottomLeft().height(), bottomCenter().height()), bottomRight().height());}

int BorderImage::maxCenterWidth() const {return qMax(qMax(topCenter().width(), center().width()), bottomCenter().width()); }
int BorderImage::maxCenterHeight() const {return qMax(qMax(centerLeft().height(), center().height()), centerRight().height()); }

int BorderImage::maxWidth() const {return maxLeft() + maxRight() + maxCenterWidth();}
int BorderImage::maxHeight() const {return maxTop() + maxBottom() + maxCenterHeight();}

QRect BorderImage::contentRect(const QRect &outer) const
{
    const bool useMargins = !d->margins.isNull();
    QMargins margins(maxLeft(), maxTop(),
                     maxRight(), maxBottom());

    //find minimum size that does not require any stretching
    QSize rectMin((margins.left() + margins.right()),
                  (margins.top() + margins.bottom()));

    if(outer.width() < rectMin.width()) {
        qreal r = (1.0 / rectMin.width()) * outer.width();
        if(useMargins) {
            margins.setLeft(d->margins.left() * r);
            margins.setRight(d->margins.right() * r);
        } else {
            margins.setLeft(margins.left() * r);
            margins.setRight(margins.right() * r);
        }
    } else if(useMargins) {
        margins.setLeft(d->margins.left());
        margins.setRight(d->margins.right());
    }
    if(outer.height() < rectMin.height()) {
        qreal r = (1.0 / rectMin.height()) * outer.height();
        if(useMargins) {
            margins.setTop(d->margins.top() * r);
            margins.setBottom(d->margins.bottom() * r);
        } else {
            margins.setTop(margins.top() * r);
            margins.setBottom(margins.bottom() * r);
        }
    } else if(useMargins) {
        margins.setTop(d->margins.top());
        margins.setBottom(d->margins.bottom());
    }

    return outer.adjusted(margins.left(), margins.top(),
                          -margins.right(), -margins.bottom());
}

QRectF BorderImage::contentRect(const QRectF &outer) const
{
    QRect r = contentRect(outer.toRect());
    return QRectF(r.left(), r.top(), r.width(), r.height());
}

QRect BorderImage::boundingRect(const QRect &content) const
{
    if(d->margins.isNull())
        return content.adjusted(-maxLeft(), -maxTop(),
                                maxRight(), maxBottom());

    return content.adjusted(-d->margins.left(), -d->margins.top(),
                            d->margins.right(), d->margins.bottom());
}

void BorderImage::surfaceFromMargins(const QMargins &v, const QSize &pixmap)
{
    setTopLeft( QRect(0, 0, v.left(), v.top()) );
    setTopCenter( QRect(v.left(), 0, pixmap.width() - v.left() - v.right(), v.top()) );
    setTopRight( QRect(pixmap.width() - v.right(), 0, v.right(), v.top()) );

    setCenterLeft( QRect(0, v.top(), v.left(), pixmap.height() - v.top() - v.bottom()) );
    setCenter( QRect(v.left(), v.top(), pixmap.width() - v.left() - v.right(), pixmap.height() - v.top() - v.bottom()) );
    setCenterRight( QRect(pixmap.width() - v.right(), v.top(), v.right(), pixmap.height() - v.top() - v.bottom()) );

    setBottomLeft( QRect(0, pixmap.height() - v.bottom(), v.left(), v.bottom()) );
    setBottomCenter( QRect(v.left(), pixmap.height() - v.bottom(), pixmap.width() - v.left() - v.right(), v.bottom()) );
    setBottomRight( QRect(pixmap.width() - v.right(), pixmap.height() - v.bottom(), v.right(), v.bottom()) );
}

void BorderImage::surfaceFromCorner(const QSize &corner, const QSize &pixmap)
{
    if(corner.height() <= 0 || corner.height() <= 0) {
        setTopLeft( QRect() );
        setTopCenter( QRect() );
        setTopRight( QRect() );

        setCenterLeft( QRect() );
        setCenter( QRect(0, 0, pixmap.width(), pixmap.height()) );
        setCenterRight( QRect() );

        setBottomLeft( QRect() );
        setBottomCenter( QRect() );
        setBottomRight( QRect() );
    }

    // Adjust for pixmap size.
    setTopLeft( QRect(0, 0, corner.width(), corner.height()) );
    setTopCenter( QRect(topLeft().width(), 0, pixmap.width() - (topLeft().width()*2), topLeft().height()) );
    setTopRight( QRect(topLeft().width() + topCenter().width() , 0, topLeft().width(), topLeft().height()) );

    setCenterLeft( QRect(0, topLeft().height(), topLeft().width(), pixmap.height() - (topLeft().height()*2)) );
    setCenter( QRect(topLeft().width(), topLeft().height(), pixmap.width() - (topLeft().width()*2), pixmap.height() - (topLeft().height()*2)) );
    setCenterRight( QRect(topLeft().width() + pixmap.width() - (topLeft().width()*2), topLeft().height(), topLeft().width(), pixmap.height() - (topLeft().height()*2)) );

    setBottomLeft( QRect(0, topLeft().height() + pixmap.height() - (topLeft().height()*2), topLeft().width(), topLeft().height()) );
    setBottomCenter( QRect(topLeft().width(), topLeft().height() + pixmap.height() - (topLeft().height()*2), pixmap.width() - (topLeft().width()*2), topLeft().height()) );
    setBottomRight( QRect(topLeft().width() + pixmap.width() - (topLeft().width()*2), topLeft().height() + pixmap.height() - (topLeft().height()*2), topLeft().width(), topLeft().height()) );
}

