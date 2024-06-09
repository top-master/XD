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


QT_BEGIN_NAMESPACE

#ifndef QT_NO_THREAD

QThreadFunc::QThreadFunc()
    : m_callback(Q_NULLPTR)
    , m_autoDelete(false)
    , m_keepLooping(false)
#ifndef QT_NO_EXCEPTIONS
    , m_lastError(Q_NULLPTR)
#endif
{
    QObjectData::get(this)->receiveChildEvents = false;

    // First of all, connect things we want to happen in calling-thread
    // (just to be sure, else `moveToThread(...)` takes effect later).
    QObject::connect(this, &QThread::finished, this, &QThreadFunc::onFinished);
    // Below needs to occur before `start()`, else things connected to
    // this class's signals get triggered from calling-thread
    // (and never happen if said calling-thread is busy, or has no event-loop).
    this->moveToThread(this);
}

QThreadFunc::~QThreadFunc() {

    if (QThread::currentThread() != this) {
        // Allows below to wait for itself.
        if (qApp) {
            this->moveToThread(qApp->thread());
        }
        // Waits for self.
        this->wait(3000);
    } else {
        Q_ASSERT_X(false, "QThreadFunc", "Thread-manager should not be deleted by own thread.");
    }

    QRunnable *runnable = m_callback;
    if (runnable && runnable->autoDelete()) {
        delete runnable;
    }
}

void QThreadFunc::run() {
    Q_DEFER {
        // Allows destructor to wait for self.
        if (qApp) {
            this->moveToThread(qApp->thread());
        }
    };

    if (this->isInterruptionRequested()) {
        return;
    }

    QT_TRY {
        if (m_callback) {
            m_callback->run();
        }

        if (m_keepLooping) {
            this->exec();
        }
    } QT_CATCHES(...,
        m_lastError = std::current_exception();
    )
}

void QThreadFunc::onFinished() {
    Q_ASSERT(QThread::currentThread() != this);
    if (m_autoDelete) {
        this->deleteLater();
    }
}


#endif // QT_NO_THREAD


QT_END_NAMESPACE
