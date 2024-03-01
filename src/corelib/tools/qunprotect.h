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

#ifndef QUNPROTECT_H
#define QUNPROTECT_H

#include <QtCore/qglobal.h>

QT_BEGIN_NAMESPACE

/// Helper to gain access to given @tparam T class's protected base-class.
template <typename T, typename TBase>
class QUnprotect : public T {
public:

    /// Casts from sub-class of type @tparam T to its base-class @tparam TBase.
    static Q_ALWAYS_INLINE TBase *cast(T *t) {
        return static_cast<TBase *>(t);
    }

};

QT_END_NAMESPACE

#endif // QUNPROTECT_H
