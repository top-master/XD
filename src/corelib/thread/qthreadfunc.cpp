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

#include "./qthreadfunc.h"

#include <QtCore/private/qobject_p.h>

QT_BEGIN_NAMESPACE

#ifndef QT_NO_THREAD

QThreadFunc::QThreadFunc()
    : m_callback(Q_NULLPTR)
    , m_autoDelete(false)
    , m_keepLooping(false)
{
}

QThreadFunc::~QThreadFunc() {
    requireDeleteSafe();

    QRunnable *runnable = m_callback;
    if (runnable && runnable->autoDelete()) {
        delete runnable;
    }
}

bool QThreadFunc::preRun() {
    if (m_callback) {
        m_callback->run();
    }

    return m_keepLooping;
}

void QThreadFunc::onFinished() {
    super::onFinished();
    if (m_autoDelete) {
        this->deleteSafe();
    }
}


#endif // QT_NO_THREAD


QT_END_NAMESPACE
