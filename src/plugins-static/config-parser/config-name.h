// Copyright (C) 2015 The XD Company Ltd.
// SPDX-License-Identifier: Apache-2.0

#ifndef CONFIG_NAME_H
#define CONFIG_NAME_H

#include "./config-value-matcher.h"

namespace Config {

class Name : public ValueMatcher
{
    typedef ValueMatcher super;
public:
    inline explicit Name(bool takesCall = false)
        : m_takesCall(takesCall)
    {
    }

    static QSharedPointer<const ValueMatcher > create(bool takesCall = false);

    bool match(const QString & value, QString *matched) const Q_DECL_OVERRIDE;
    QString describe() const Q_DECL_OVERRIDE;

private:
    bool m_takesCall;
};

} // Namespace `Config`.

#endif // CONFIG_NAME_H
