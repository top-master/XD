/****************************************************************************
**
** Copyright (C) 2015 The XD Company Ltd.
**
** This file is part of the QtRemote module of the XD Toolkit.
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

#ifndef QT_REMOTE_LOGGING_H
#define QT_REMOTE_LOGGING_H

#include "./qremote-config.h"

#include <QtCore/qatomicflags.h>

QT_BEGIN_NAMESPACE

namespace QRemote {

enum LogTypes {
    WarnGlobalInstance = 0x00000001
};

QT_REMOTE_EXPORT extern QBasicAtomicFlags<LogTypes> logSkip;

Q_ALWAYS_INLINE void warnGlobalInstance(const char *where) {
    if (QRemote::logSkip.append(QRemote::WarnGlobalInstance)) {
        qAssertWarning(where, "Failed to find QRemoteUser's global instance"
                       " (ensure to construct the default instance first,"
                       " and destruct it only after it's no longer relied on).");
    }
}

} // namespace QRemote

QT_END_NAMESPACE

#endif // QT_REMOTE_LOGGING_H
