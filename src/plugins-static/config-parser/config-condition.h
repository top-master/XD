// Copyright (C) 2015 The XD Company Ltd.
// SPDX-License-Identifier: Apache-2.0

#ifndef CONFIG_CONDITION_H
#define CONFIG_CONDITION_H

#include "./config-value-matcher.h"

#include <QtCore/QSet>
#include <QtCore/QStringList>

namespace Config {

class Condition : public ValueMatcher
{
    typedef ValueMatcher super;
public:
    explicit Condition(const QStringList & names, bool takesPercents = false);
    static QSharedPointer<const ValueMatcher > create(
            const QStringList & names, bool takesPercents = false);
    bool match(const QString & value, QString *matched) const Q_DECL_OVERRIDE;
    QString describe() const Q_DECL_OVERRIDE;

    static bool compile(const QString & text,
                        const QStringList & names,
                        QStringList *program,
                        QString *error, bool takesPercents = false);
    static bool evaluate(const QStringList & program,
                         const QSet<QString > & holding);
    static bool evaluate(const QStringList & program,
                         const QSet<QString > & holding, bool *ok);
    static bool evaluate(const QStringList & program,
                         const QSet<QString > & holding, qreal percent,
                         bool *ok = Q_NULLPTR);

private:
    static bool run(const QStringList & program, const QSet<QString > & holding,
                    const qreal *percent, bool *ok);

    QStringList m_names;
    bool m_takesPercents;
};

} // namespace Config

#endif // CONFIG_CONDITION_H
