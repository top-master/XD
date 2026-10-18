// Copyright (C) 2015 The XD Company Ltd.
// SPDX-License-Identifier: Apache-2.0

#ifndef CONFIG_REGEXP_H
#define CONFIG_REGEXP_H

#include "./config-replacement.h"
#include "./config-value-matcher.h"

#include <QtCore/QRegularExpression>
#include <QtCore/QString>

namespace Config {

class RegExp : public ValueMatcher
{
    typedef ValueMatcher super;
public:
    static QSharedPointer<const ValueMatcher > create();
    static bool split(const QString & value, QString *head, QString *fallback,
                      bool *hasFallback);
    static bool parse(const QString & value, QRegularExpression *pattern,
                      Replacement *replacement);

    bool match(const QString & value, QString *matched) const Q_DECL_OVERRIDE;
    QString describe() const Q_DECL_OVERRIDE;
};

} // namespace Config

#endif // CONFIG_REGEXP_H
