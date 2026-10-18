// Copyright (C) 2015 The XD Company Ltd.
// SPDX-License-Identifier: Apache-2.0

#ifndef CONFIG_VALUE_MATCHER_H
#define CONFIG_VALUE_MATCHER_H

#include <QtCore/QList>
#include <QtCore/QSharedPointer>
#include <QtCore/QString>

namespace Config {

class ValueMatcher
{
public:
    virtual ~ValueMatcher();

    virtual bool match(const QString & value, QString *matched) const = 0;
    virtual QString describe() const = 0;
};

typedef QList<QSharedPointer<const ValueMatcher > > Matchers;

} // namespace Config

#endif // CONFIG_VALUE_MATCHER_H
