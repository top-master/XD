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
    Numbers(qint64 minimum, qint64 maximum, int maximumCount);

    static QSharedPointer<const ValueMatcher > create(
            qint64 minimum, qint64 maximum,
            int maximumCount = 0);
    static QList<qint64 > parse(const QString & value);

    bool match(const QString & value, QString *matched) const Q_DECL_OVERRIDE;
    QString describe() const Q_DECL_OVERRIDE;
    QVariant convert(const QVariant & resolved) const Q_DECL_OVERRIDE;

private:
    QString range() const;

    qint64 m_minimum;
    qint64 m_maximum;
    int m_maximumCount;
};

} // Namespace `Config`.

#endif // CONFIG_NUMBERS_H
