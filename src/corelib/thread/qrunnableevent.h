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

#ifndef QRUNNABLE_EVENT_H
#define QRUNNABLE_EVENT_H

#ifndef QT_NO_STD_FUNCTION
#  include <QtCore/qrunnablefunc.h>
#else
#  include <QtCore/qrunnable.h>
#endif

#include <QtCore/qcoreevent.h>


QT_BEGIN_NAMESPACE

class Q_CORE_EXPORT QRunnableEvent : public QEvent
{
    typedef QEvent super;
public:
    Q_ALWAYS_INLINE QRunnableEvent() : super(QEvent::Runnable) {}

    Q_ALWAYS_INLINE explicit QRunnableEvent(QRunnable *callback)
        : super(QEvent::Runnable)
    {
        this->d = Q_PTR_CAST(QEventPrivate *, callback);
    }

    virtual ~QRunnableEvent();

    Q_ALWAYS_INLINE void run() {
        if (this->isAccepted()) {
            this->ignore();
            QRunnable *callback = Q_PTR_CAST(QRunnable *, this->d);
            if (callback) {
                this->d = Q_NULLPTR;
                callback->run();
                if (callback->autoDelete()) {
                    delete callback;
                }
            }
        }
    }

private:
    Q_DISABLE_COPY(QRunnableEvent)
};

QT_END_NAMESPACE

#endif
