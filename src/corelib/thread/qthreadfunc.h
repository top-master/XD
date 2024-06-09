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

#ifndef QTHREAD_FUNC_H
#define QTHREAD_FUNC_H

#include <QtCore/qthread.h>
#include <QtCore/qrunnablefunc.h>
#include <QtCore/qexception.h>


QT_BEGIN_NAMESPACE

#ifndef QT_NO_THREAD
/// Same as QThread, but optimized for Lambda instead of overridding @ref QThread::run.
///
/// @warning Sets `QObjectData::receiveChildEvents` to `false`.
class Q_CORE_EXPORT QThreadFunc : public QThread
{
    Q_OBJECT
public:
    /// @warning Forbids setting @ref parent in constructor, since
    /// this moves to itself as thread, which is required because
    /// otherwise signals would be emitted from the thread which constructs this.
    /// Even later @ref setParent calls need to first move parent to this thread.
    QThreadFunc();

    ~QThreadFunc();

    inline QRunnable *callback() const { return m_callback; }

    inline void setCallback(QRunnable *newValue) {
        QRunnable *oldValue = m_callback;
        m_callback = newValue;
        if (oldValue && oldValue->autoDelete()) {
            delete oldValue;
        }
    }

    inline void setCallback(const  QFunction<void () > &newValue)
    { return setCallback(new QRunnableFunc(newValue)); }

    /// Whether to `delete` this class once the thread's event-loop finishes.
    ///
    /// Default's @c false.
    inline bool isAutoDeletable() const { return m_autoDelete; }
    /// Setter for @ref isAutoDeletable.
    inline void setAutoDelete(bool enabled) { m_autoDelete = enabled; }

    /// Whether to keep Event-loop untill user manually calls `quit()` method.
    ///
    /// Default's @c false.
    inline bool keepLooping() const { return m_keepLooping; }
    /// Setter for @ref keepLooping.
    inline void setKeepLooping(bool enabled) { m_keepLooping = enabled; }

#ifndef QT_NO_EXCEPTIONS
    inline const std::exception_ptr &lastError() const { return m_lastError; }
#endif // QT_NO_EXCEPTIONS

protected:
    void run() Q_DECL_OVERRIDE;

private Q_SLOTS:
    void onFinished();

private:
    QRunnable *m_callback;
    bool m_autoDelete;
    bool m_keepLooping;

#ifndef QT_NO_EXCEPTIONS
    std::exception_ptr m_lastError;
#endif // QT_NO_EXCEPTIONS

};

#endif // QT_NO_THREAD

#endif // QTHREAD_FUNC_H
