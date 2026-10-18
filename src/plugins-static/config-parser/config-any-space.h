// Copyright (C) 2015 The XD Company Ltd.
// SPDX-License-Identifier: Apache-2.0

#ifndef CONFIG_ANY_SPACE_H
#define CONFIG_ANY_SPACE_H

#include "./config-value-matcher.h"

namespace Config {

class AnySpace : public ValueMatcher
{
    typedef ValueMatcher super;
public:
    static QSharedPointer<const ValueMatcher > create();

    bool match(const QString & value, QString *matched) const Q_DECL_OVERRIDE;
    QString describe() const Q_DECL_OVERRIDE;
};

} // namespace Config

#endif // CONFIG_ANY_SPACE_H
