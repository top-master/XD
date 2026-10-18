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

    /*!
        Tells whether @a value is one this matcher takes; when it is, it gives
        @a matched the value to keep, such as a word in its own spelling.
    */
    virtual bool match(const QString & value, QString *matched) const = 0;

    /*!
        Builds the description of the values this matcher takes, for the
        error of a value no matcher of the key takes.
    */
    virtual QString describe() const = 0;
};

typedef QList<QSharedPointer<const ValueMatcher > > Matchers;

} // namespace Config

#endif // CONFIG_VALUE_MATCHER_H
