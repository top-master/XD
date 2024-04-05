/****************************************************************************
**
** Copyright (C) 2015 The XD Company Ltd.
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing/
**
** This file is part of the test suite of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL21$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see http://www.qt.io/terms-conditions. For further
** information use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file. Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** As a special exception, The Qt Company gives you certain additional
** rights. These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef BACKEND_SERVICE_H
#define BACKEND_SERVICE_H

#include <QtCore/qglobal.h>
#if QT_HAS_XD(5, 6, 5)
#  include <QtRemote/QRemoteObject>
#else
// Minimal compile need.
#  include <QtCore/QObject>
#  define Q_REMOTE Q_OBJECT
#endif

namespace my_lib {

class BackendService : public QObject {
    Q_REMOTE

public slots:
    inline bool login() Q_DEFAULT(false);

    QString statusText() const {
        return m_didLogin ? QLL("is-logged-in") : QLL("is-not-logged-in");
    }


    // Below `Q_DEFAULT` is for `status()`, just testing distance,
    // which causes compile-time error if wrong (sat for `QByteArray`).
    Q_DEFAULT(QLL("did-timeout")) QByteArray statusData() const {
        return m_didLogin ? "is-logged-in" : "is-not-logged-in";
    }

    Q_DEFAULT("data-did-timeout")


    // No such thing as `private` in QRemote, but if really needed you could do:
private slots:
    inline QString mySecretMethod() {
        if (qobject_cast<QObjectRemote *>(this->sender())) {
            return QLL("Permission denied");
        }
        return QLL("My secret value");
    }

private:
    bool m_didLogin;
};

inline bool BackendService::login() { m_didLogin = true; return true; }

} // namespace my_lib

#endif // BACKEND_SERVICE_H
