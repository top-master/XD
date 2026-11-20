// Copyright (C) 2015 The XD Company Ltd.
// SPDX-License-Identifier: Apache-2.0

#ifndef CONFIG_CONDITION_H
#define CONFIG_CONDITION_H

#include "./config-evaluator-context.h"
#include "./config-scopes.h"
#include "./config-value-matcher.h"

#include <QtCore/QSet>
#include <QtCore/QFlags>
#include <QtCore/QHash>
#include <QtCore/QList>
#include <QtCore/QStringList>
#include <QtCore/QVariant>

namespace Config {

class Condition : public ValueMatcher
{
    typedef ValueMatcher super;
public:
    enum Result {
        ResultHolds,
        ResultNumber
    };

    explicit Condition(const QStringList & names, bool takesPercents = false,
                       Scopes::Setup setup = Q_NULLPTR,
                       Result result = ResultHolds);
    static QSharedPointer<const ValueMatcher > create(
            const QStringList & names, bool takesPercents = false,
            Scopes::Setup setup = Q_NULLPTR, Result result = ResultHolds);
    bool match(const QString & value, QString *matched) const Q_DECL_OVERRIDE;
    QString describe() const Q_DECL_OVERRIDE;
    QVariant convert(const QVariant & resolved) const Q_DECL_OVERRIDE;

    static QStringList compile(const QString & text,
                        const Evaluator::CompileContext *context,
                        bool takesPercents = false);
    static QVariant resolve(const QStringList & program,
                            const QSet<QString > & holding,
                            const Evaluator::Context *context);
    static QVariant eval(const QString & text,
                         const Evaluator::Context *context,
                         const ValueMatcher *fallback = Q_NULLPTR);
    static bool value(const QStringList & program,
                      const QSet<QString > & holding,
                      const Evaluator::Context *context, qreal *number);
    static QString canonicalName(const QString & name);
    static NameMismatch nameMismatch(const QString & value);

private:
    static bool checkPercents(const QStringList & program,
                              bool takesPercents, QString *error);

    QStringList m_names;
    bool m_takesPercents;
    Scopes::Setup m_setup;
    Result m_result;
};

#ifndef Q_MOC_RUN
Q_ALWAYS_INLINE bool Condition::value(
        const QStringList & program, const QSet<QString > & holding,
        const Evaluator::Context *context, qreal *number)
{
    const Evaluator::Context empty(false);
    if ( ! context) {
        context = &empty;
    }
    const QVariant result = resolve(program, holding, context);
    const bool isNumber = context->lastResolvePassed()
        && result.type() == QVariant::Double;
    *number = isNumber ? result.toDouble() : 0;
    return isNumber;
}
#endif // Q_MOC_RUN

} // Namespace `Config`.

#endif // CONFIG_CONDITION_H
