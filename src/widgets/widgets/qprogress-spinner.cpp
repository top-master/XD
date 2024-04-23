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

#include "qprogress-spinner.h"

#include <QtGui/QPainter>


QProgressSpinner::QProgressSpinner(QWidget *parent)
    : super(parent)
{
    m.color = super::palette().color(QPalette::Normal, QPalette::WindowText);
    this->setFocusPolicy(Qt::NoFocus);
    this->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    this->setLineCount(QProgressSpinner::DefaultLineCount);
}

void QProgressSpinner::setColor(const QColor &color)
{
    m.color = color;
    // Update happens later, but to be sure, is last line.
    update();
}

void QProgressSpinner::setFrameDuration(int duration)
{
    QMutexLocker _(&m.mutex);
    if (this->isAnimating()) {
        m.timer.stop();
        m.duration = duration;
        m.timer.start(m.duration, this);
    } else {
        m.duration = duration;
    }
}

void QProgressSpinner::setVisibleIfStopped(bool state)
{
    if (state != m.isVisibleIfStopped) {
        m.isVisibleIfStopped = state;
        update();
    }
}

QSize QProgressSpinner::sizeHint() const
{
    return QSize(QProgressSpinner::DefaultSize,
                 QProgressSpinner::DefaultSize);
}

int QProgressSpinner::heightForWidth(int widthArg) const
{
    return widthArg;
}

void QProgressSpinner::render(QPainter *p, const QRectF &dest,
                              int lineCount,
                              int angle, const QColor &color)
{
    const int shortDimension = qMin(dest.width(), dest.height());
    if (shortDimension < 3) {
        return;
    }
    const int halfWidth = shortDimension / 2.0f;
    Q_ASSERT(p);

    int lineHeight = (shortDimension * 0.40) - 2;
    int lineWidth  = shortDimension > 40
            ? lineHeight * 0.21
            : lineHeight * 0.40;
    qreal lineWidthHalf = lineWidth / 2.0;
    // Plus one to keep smooth-ness (leave space for border anti-aliasing).
    QRectF lineRect( - lineWidthHalf, - halfWidth + 1,
                    lineWidth, lineHeight);

    // Initial rotation.

    p->save();
    p->translate(dest.center());
    p->rotate(angle);
    p->setPen(Qt::NoPen);

    qreal degreePerLine = 360 / lineCount;

    QColor colorAlpha = color;
    for (int i = 1; i <= lineCount; ++i) {
        // Starts almost invisible, ends at given `angle` fully visible.
        colorAlpha.setAlphaF(i / (lineCount * 1.0f));
        p->setBrush(colorAlpha);
        // Intentionally skips initial `angle` (since we need that fully visible).
        p->rotate(degreePerLine);
        p->drawRoundedRect(lineRect, lineWidthHalf, lineWidthHalf);
    }

    p->restore();
}

void QProgressSpinner::startAnimation()
{
    m.angle = 0;

    if ( ! m.timer.isActive()) {
        m.timer.start(m.duration, this);
    }
}

void QProgressSpinner::stopAnimation()
{
    QMutexLocker _(&m.mutex);
    if (m.timer.isActive()) {
        m.timer.stop();
    }

    if ( ! m.isVisibleIfStopped) {
        update();
    }
}

void QProgressSpinner::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event)
    if ( ! isAnimating()
      && ! m.isVisibleIfStopped
    ) {
        return;
    }

    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);

    render(&p, rect(), m.lineCount, m.angle, m.color);
}

void QProgressSpinner::timerEvent(QTimerEvent *event)
{
    Q_UNUSED(event)
    m.angle = (m.angle + m.degreePerLine) % 360;

    update();
}
