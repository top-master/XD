// Copyright (C) 2015 The XD Company Ltd.
// SPDX-License-Identifier: Apache-2.0

#ifndef CONFIG_CHOICE_H
#define CONFIG_CHOICE_H

#include "./config-condition.h"

namespace Config {

class Choice : public ValueMatcher
{
    typedef ValueMatcher super;
public:
    Choice(const QStringList &names, const QStringList &values);
    bool match(const QString &value, QString *matched) const Q_DECL_OVERRIDE;
    QString describe() const Q_DECL_OVERRIDE;
    static bool resolve(const QString &expression, const QStringList &names,
                        const QStringList &values, const Condition::Context *context,
                        QString *result, QString *error);

private:
    QStringList m_names;
    QStringList m_values;
};

} // Namespace `Config`.

#endif // CONFIG_CHOICE_H
