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

#ifndef QRUNNABLE_FUNC_H
#define QRUNNABLE_FUNC_H

#include <QtCore/qrunnable.h>
#include <QtCore/qfunction.h>

QT_BEGIN_NAMESPACE

class Q_CORE_EXPORT QRunnableFunc : public QRunnable
{
    QFunction<void () > data;
public:
    inline QRunnableFunc() {}

    template <typename T>
    Q_ALWAYS_INLINE Q_IMPLICIT QRunnableFunc(const T &callback)
        : data(callback)
    {
    }

    virtual ~QRunnableFunc();

    void run() Q_DECL_OVERRIDE;

    Q_ALWAYS_INLINE void operator =(const std::function<void()> &callback) Q_DECL_NOEXCEPT
    {
        this->data = callback;
    }

};

QT_END_NAMESPACE

#endif
