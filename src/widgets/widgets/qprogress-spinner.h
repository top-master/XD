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

#ifndef EXTRAS_PROGRESS_SPINNER_H
#define EXTRAS_PROGRESS_SPINNER_H

#include <QtCore/qbasictimer.h>
#include <QtCore/qmutex.h>
#include <QtGui/qcolor.h>
#include <QtWidgets/qwidget.h>


class Q_WIDGETS_EXPORT QProgressSpinner : public QWidget
{
    Q_OBJECT
    typedef QWidget super;
public:
    QProgressSpinner(QWidget* parent = Q_NULLPTR);

    inline int lineCount() const;
    /// @warning alters @ref degreePerFrame result.
    inline void setLineCount(int v);

    inline int degreePerFrame() const;
    /// @warning alters @ref lineCount result.
    inline void setDegreePerFrame(int v);

    inline const QColor &color() const;
    void setColor(const QColor &color);

    inline int frameDuration() const;
    void setFrameDuration(int duration);

    inline bool isAnimating() const;

    inline bool isVisibleIfStopped() const;
    void setVisibleIfStopped(bool state);

    QSize sizeHint() const Q_DECL_OVERRIDE;
    int heightForWidth(int widthArg) const Q_DECL_OVERRIDE;

    /// By default "angle = (angle+30) % 360;" every "72" mili-seconds.
    static void render(QPainter *, const QRectF &,
                       int lineCount, int angle = 0,
                       const QColor &color = QColor(Qt::black));

public slots:
    void startAnimation();

    void stopAnimation();

    inline void setAnimationEnabled(bool v = true);

protected:
    void paintEvent(QPaintEvent *event) Q_DECL_OVERRIDE;
    void timerEvent(QTimerEvent *event) Q_DECL_OVERRIDE;

private:
    enum {
        DefaultSize = 48,
        DefaultLineCount = 12,
    };

    struct Data {
        inline Data()
            : isVisibleIfStopped(false)
            , angle(0)
            , duration(72)
        {}

        bool isVisibleIfStopped;

        int angle;
        int duration;
        int degreePerLine;
        int lineCount;

        QBasicTimer timer;
        QColor color;
        QMutex mutex;
    } m;
};

#ifndef Q_MOC_RUN
inline int QProgressSpinner::lineCount() const { return m.lineCount; }

inline void QProgressSpinner::setLineCount(int v)
{
    if (v <= 1) {
        v = 2;
    }
    m.lineCount = v;
    m.degreePerLine = 360 / v;
}

inline int QProgressSpinner::degreePerFrame() const { return m.degreePerLine; }

inline void QProgressSpinner::setDegreePerFrame(int v)
{
    m.degreePerLine = v;
    m.lineCount = 360 / v;
}

inline const QColor &QProgressSpinner::color() const { return m.color; }

inline int QProgressSpinner::frameDuration() const { return m.duration; }

inline bool QProgressSpinner::isVisibleIfStopped() const
{
    return m.isVisibleIfStopped;
}

inline bool QProgressSpinner::isAnimating() const
{
    return m.timer.isActive();
}
inline void QProgressSpinner::setAnimationEnabled(bool v)
{
    return v ? startAnimation() : stopAnimation();
}
#endif //Q_MOC_RUN

#endif // EXTRAS_PROGRESS_SPINNER_H
