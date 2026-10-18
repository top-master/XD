// Copyright (C) 2015 The XD Company Ltd.
// SPDX-License-Identifier: Apache-2.0

#ifndef CONFIG_NUMBERS_H
#define CONFIG_NUMBERS_H

#include "./config-value-matcher.h"

#include <QtCore/QList>
#include <QtCore/QString>

namespace Config {

class Numbers : public ValueMatcher
{
    typedef ValueMatcher super;
public:
    Numbers(int minimum, int maximum, int maximumCount);

    static QSharedPointer<const ValueMatcher > create(int minimum, int maximum,
                                                      int maximumCount = 0);
    static QList<int > parse(const QString & value);

    bool match(const QString & value, QString *matched) const Q_DECL_OVERRIDE;
    QString describe() const Q_DECL_OVERRIDE;

private:
    int m_minimum;
    int m_maximum;
    int m_maximumCount;
};

} // namespace Config

#endif // CONFIG_NUMBERS_H
