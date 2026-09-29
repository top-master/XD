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

#ifndef QEVENTDISPATCHER_DECOR_H
#define QEVENTDISPATCHER_DECOR_H

#include <QtCore/qeventdispatcherlazy.h>
#include <QtCore/qpointer.h>

QT_BEGIN_NAMESPACE

/*!
A run-time replaceable decorator around an event dispatcher.

It loads the system default on first use exactly as @ref QEventDispatcherLazy
does, but, unlike that lazy loader (which dissolves into its decoratee's
private), it keeps its own identity through the plain @ref QObjectDecor load,
and forwards every call to the delegate it holds. @ref replace swaps that
delegate at run time and returns the previous one, and @ref restore swaps the
system default back, so a test can install this once as the thread's
dispatcher, swap in an overriding dispatcher for the duration of a case, then
restore the default -- without touching Qt's own setEventDispatcher workflow
(whose default stays @ref QEventDispatcherLazy).
*/
class Q_CORE_EXPORT QEventDispatcherDecor : public QEventDispatcherLazy
{
    Q_OBJECT
    typedef QEventDispatcherLazy super;
    typedef QEventDispatcherDecor Self;
public:
    explicit QEventDispatcherDecor(QObject *parent = Q_NULLPTR);
    ~QEventDispatcherDecor();

    QAbstractEventDispatcher *replace(QAbstractEventDispatcher *newValue);
    QAbstractEventDispatcher *restore();

    inline QPointer<QAbstractEventDispatcher> systemDefault() const {
        return loadedDefault;
    }

protected:
    void postDecorLoad(QObject *loaded) Q_THROWS( QRequirementErrorType::Usage ) Q_DECL_OVERRIDE;
    void decorListener(PreDecorContext *ctx) Q_DECL_OVERRIDE;

private:
    Q_DISABLE_COPY(QEventDispatcherDecor);

    /// The system default loaded on first use; only this class sets it.
    QPointer<QAbstractEventDispatcher> loadedDefault;
};

QT_END_NAMESPACE

#endif // QEVENTDISPATCHER_DECOR_H
