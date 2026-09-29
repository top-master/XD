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

#ifndef QOBJECT_LAZY_H
#define QOBJECT_LAZY_H

#include <QtCore/qobjectdecor.h>


#ifndef QT_NO_QOBJECT

QT_BEGIN_NAMESPACE

/*!
A QObjectDecor that lazily BECOMES its decoratee.

On load it swaps the decor owner's private for the decoratee's, so the owner
dissolves into the loaded object (identity merge). This is the behaviour a
lazy loader needs -- the owner is only a placeholder until the real object
arrives, after which it forwards AND reads as that object at the private level.

The plain @ref QObjectDecor base does not swap: it keeps its own private and
only forwards through the decoratee, which is what a replaceable decorator
(swap the decoratee at run time) needs instead.
*/
class Q_CORE_EXPORT QObjectLazy : public QObjectDecor {
    typedef QObjectDecor super;
public:
    inline QObjectLazy()
        : super()
    {}

protected:
    void postDecorLoad(QObject *loaded) Q_THROWS( QRequirementErrorType::Usage ) Q_DECL_OVERRIDE;
};

QT_END_NAMESPACE

#endif // QT_NO_QOBJECT

#endif // QOBJECT_LAZY_H
