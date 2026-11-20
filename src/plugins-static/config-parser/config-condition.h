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

enum EvalFlag {
    FailedToCompile = 0x01,
    Fallback = 0x02,
    FailedToResolve = 0x04,
    FailedToFallback = 0x08,
    Resolved = 0x10
};
Q_DECLARE_FLAGS(EvalRes, EvalFlag)
Q_DECLARE_OPERATORS_FOR_FLAGS(EvalRes)

class Condition : public ValueMatcher
{
    typedef ValueMatcher super;
public:
    enum NameMismatch {
        NameMismatchIgnore,
        NameMismatchWarn,
        NameMismatchError,
        NameMismatchCritical
    };

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
    bool convert(const QVariant & resolved,
                 QVariant *value) const Q_DECL_OVERRIDE;

    static bool compile(const QString & text,
                        const QStringList & names,
                        QStringList *program,
                        QString *error, bool takesPercents = false,
                        NameMismatch mismatch = NameMismatchWarn,
                        bool *mismatched = Q_NULLPTR,
                        const QStringList *words = Q_NULLPTR);
    static bool resolve(const QStringList & program,
                        const QSet<QString > & holding,
                        const Evaluator::Context *context, QVariant *result);
    static bool resolve(const QStringList & program,
                        const QSet<QString > & holding,
                        const Evaluator::Context *context = Q_NULLPTR,
                        bool *resolved = Q_NULLPTR);
    static bool resolve(const QStringList & program,
                        const QSet<QString > & holding, bool *resolved);
    static EvalRes eval(
            const QString & text,
            const Evaluator::Context *context, QVariant *result,
            QString *error, const ValueMatcher *fallback = Q_NULLPTR);
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
Q_ALWAYS_INLINE bool Condition::resolve(
        const QStringList & program, const QSet<QString > & holding,
        const Evaluator::Context *context, bool *resolved)
{
    QVariant value;
    const bool ok = resolve(program, holding, context, &value)
        && value.type() == QVariant::Double;
    if (resolved) {
        *resolved = ok;
    }
    return ok && value.toDouble() != 0;
}

Q_ALWAYS_INLINE bool Condition::resolve(
        const QStringList & program, const QSet<QString > & holding,
        bool *resolved)
{
    return resolve(program, holding, Q_NULLPTR, resolved);
}

Q_ALWAYS_INLINE bool Condition::value(
        const QStringList & program, const QSet<QString > & holding,
        const Evaluator::Context *context, qreal *number)
{
    QVariant result;
    const bool isNumber = resolve(program, holding, context, &result)
        && result.type() == QVariant::Double;
    *number = isNumber ? result.toDouble() : 0;
    return isNumber;
}
#endif // Q_MOC_RUN

} // Namespace `Config`.

#endif // CONFIG_CONDITION_H
