// Copyright (C) 2015 The XD Company Ltd.
// SPDX-License-Identifier: Apache-2.0

#ifndef CONFIG_CONDITION_H
#define CONFIG_CONDITION_H

#include "./config-evaluator-context.h"
#include "./config-scopes.h"
#include "./config-value-matcher.h"

#include <QtCore/QSet>
#include <QtCore/QHash>
#include <QtCore/QList>
#include <QtCore/QStringList>
#include <QtCore/QVariant>

namespace Config {

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
    bool convert(const QVariant & answer,
                 QVariant *value) const Q_DECL_OVERRIDE;

    static bool compile(const QString & text,
                        const QStringList & names,
                        QStringList *program,
                        QString *error, bool takesPercents = false,
                        NameMismatch mismatch = NameMismatchWarn,
                        bool *mismatched = Q_NULLPTR,
                        const QStringList *words = Q_NULLPTR);
    static bool answer(const QStringList & program,
                       const QSet<QString > & holding,
                       const Evaluator::Context *context, QVariant *result);
    static bool evaluate(const QStringList & program,
                         const QSet<QString > & holding);
    static bool evaluate(const QStringList & program,
                         const QSet<QString > & holding, bool *answered);
    static bool evaluate(const QStringList & program,
                         const QSet<QString > & holding,
                         const Evaluator::Context *context,
                         bool *answered = Q_NULLPTR);
    static bool value(const QStringList & program,
                      const QSet<QString > & holding,
                      const Evaluator::Context *context, qreal *result);
    static QString canonicalName(const QString & name);
    static NameMismatch nameMismatch(const QString & value);

private:
    static bool checkPercents(const QStringList & program,
                              bool takesPercents, QString *error);
    static bool answerWith(const QStringList & program,
                           const QSet<QString > & holding,
                           const Evaluator::Context *context, QVariant *result);
    static bool holds(const QStringList & program,
                      const QSet<QString > & holding,
                      const Evaluator::Context *context, bool *answered);

    QStringList m_names;
    bool m_takesPercents;
    Scopes::Setup m_setup;
    Result m_result;
};

} // Namespace `Config`.

#endif // CONFIG_CONDITION_H
