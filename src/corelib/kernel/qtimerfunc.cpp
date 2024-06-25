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

#include "qtimerfunc.h"

#ifndef QT_NO_QOBJECT

#include <QtCore/private/qobject_p.h>

QTimerFunc::~QTimerFunc() {
    if (m_info.timerId > 0) {
        killTimer(m_info.timerId);
    }
}

void QTimerFunc::timerEvent(QTimerEvent *) {
    // Prevents repeat if callback triggers `processEvents(...)`.
    if (m_info.timerId > 0) {
        killTimer(m_info.timerId);
    }
    m_info.timerId = -1;

    if (m_callback) {
        (m_callback)();
    }

    if (m_isSingleShot) {
        // Same as `deleteLater` logic, without waste of posting a new event.
        qDeleteInEventHandler(this);
        return;
    }

    m_info.timerId = this->startTimer(m_info.interval, m_info.timerType);
}

#endif // QT_NO_QOBJECT
