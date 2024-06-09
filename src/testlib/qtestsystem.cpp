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

#include "qtestsystem.h"

#include <QtCore/qthreadfunc.h>


QT_BEGIN_NAMESPACE


#ifndef QT_NO_THREAD
bool QTest::qWaitForThread(QThread *thread, int ms) {
    QElapsedTimer timer;
    timer.start();

    bool success = false;
    int timeLeft = 0;
    while ( ! (success = thread->isFinished()) && (timeLeft = timer.timeLeft(ms)) != 0) {
        QCoreApplication::processEvents(QEventLoop::AllEvents, Q_MAX(timeLeft, 30));
        QCoreApplication::sendPostedEvents(Q_NULLPTR, QEvent::DeferredDelete);
        QTest::qSleep(30);
    }

    if (success) {
        // Ensures really finished (since native-thread remains little longer).
        timer.restart();
        while ( ! thread->wait(30) && timer.timeLeft(3000)) {
            QCoreApplication::processEvents(QEventLoop::AllEvents, 100);
            QCoreApplication::sendPostedEvents(Q_NULLPTR, QEvent::DeferredDelete);
        }
    }

    return success;
}

bool QTest::qWaitForAsync(QRunnable *task, int ms) Q_THROWS(?)
{
    QThreadFunc *thread = new QThreadFunc();
    Q_CHECK_PTR(thread);
    thread->setCallback(task);
    thread->setAutoDelete(false);

    defer {
        thread->requestInterruption();
        if (qApp) {
            // Waits for finish.
            QElapsedTimer timer;
            timer.start();
            while ( ! thread->wait(30) && timer.timeLeft(3000)) {
                QCoreApplication::processEvents(QEventLoop::AllEvents, 100);
                QCoreApplication::sendPostedEvents(Q_NULLPTR, QEvent::DeferredDelete);
            }
            // Calls said destructor.
            thread->deleteLater();
        } else {
            delete thread;
        }
    };

    thread->setObjectName(QLL("QTest::qWaitForAsync"));
    thread->start();

    const bool success = qWaitForThread(thread, ms);
    std::exception_ptr error = thread->lastError();
    if ( ! (error == Q_NULLPTR)) { // MSVC may not support `!=` operator.
        std::rethrow_exception(error);
    }

    return success;
}

#endif // QT_NO_THREAD

QT_END_NAMESPACE
