/****************************************************************************
**
** Copyright (C) 2015 The XD Company Ltd.
**
** This file is part of the QtCore module of the XD Toolkit.
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

#ifndef QMATH_EXTRAS_H
#define QMATH_EXTRAS_H

#include "./qmath.h"


QT_BEGIN_NAMESPACE

template <typename T1, typename T2, typename T3>
inline qreal qFrameToProgressF(T1 frameStart, T2 durationFrames, T3 currentFrame) {
    return 1.0/durationFrames * (currentFrame-frameStart);
}
template <typename T1, typename T2, typename T3>
inline int qFrameToProgress(T1 frameStart, T2 durationFrames, T3 currentFrame) {
    return 255.0/durationFrames * (currentFrame-frameStart);
}
template <typename T1, typename T2, typename T3>
inline qreal qProgressToValueF(T1 valueStart, T2 valueEnd, T3 progress) {
    return qreal(valueEnd-valueStart)/1.0 * progress + valueStart;
}
template <typename T1, typename T2, typename T3>
inline int qProgressToValue(T1 valueStart, T2 valueEnd, T3 progress) {
    return qreal(valueEnd-valueStart)/1.0 * progress + valueStart;
}

/// Returns same as qFrameToProgressF(...) but limited from 0.0 to 1.0
template <typename T1, typename T2, typename T3>
inline qreal qFrameToPercentF(T1 frameStart, T2 durationFrames, T3 currentFrame) {
    if(currentFrame < frameStart)
        return 0.0;
    else if(currentFrame >= frameStart+durationFrames)
        return 1.0;
    return 1.0/durationFrames * (currentFrame-frameStart);
}

/// Returns same as qFrameToProgress(...) but limited from 0 to 255.
template <typename T1, typename T2, typename T3>
inline int qFrameToPercent(T1 frameStart, T2 durationFrames, T3 currentFrame) {
    if(currentFrame < frameStart)
        return 0;
    else if(currentFrame >= frameStart+durationFrames)
        return 255;
    return 255.0/durationFrames * (currentFrame-frameStart);
}

template <typename T1, typename T2, typename T3, typename T4, typename T5>
inline qreal qValueForFrameF(T1 valueStart, T2 valueEnd, T3 frameStart, T4 durationFrames, T5 currentFrame) {
    return qreal(valueEnd-valueStart)/durationFrames * (currentFrame-frameStart) + valueStart;
}
template <typename T1, typename T2, typename T3, typename T4, typename T5>
inline int qValueForFrame(T1 valueStart, T2 valueEnd, T3 frameStart, T4 durationFrames, T5 currentFrame) {
    return qreal(valueEnd-valueStart)/durationFrames * (currentFrame-frameStart) + valueStart;
}

template <typename T1, typename T2, typename T3, typename T4, typename T5>
inline qreal qBoundToFrameF(T1 valueMin, T2 valueMax, T3 frameStart, T4 durationFrames, T5 currentFrame) {
    if(currentFrame < frameStart)
        return valueMin;
    else if(currentFrame >= frameStart+durationFrames)
        return valueMax;
    return qValueForFrameF(valueMin, valueMax, frameStart, durationFrames, currentFrame);
}
template <typename T1, typename T2, typename T3, typename T4, typename T5>
inline int qBoundToFrame(T1 valueMin, T2 valueMax, T3 frameStart, T4 durationFrames, T5 currentFrame) {
    if(currentFrame < frameStart)
        return valueMin;
    else if(currentFrame >= frameStart+durationFrames)
        return valueMax;
    return qValueForFrame(valueMin, valueMax, frameStart, durationFrames, currentFrame);
}

QT_END_NAMESPACE

#endif // QMATH_EXTRAS_H
