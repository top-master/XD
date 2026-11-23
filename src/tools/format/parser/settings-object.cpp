// Copyright (C) 2015 The XD Company Ltd.
// SPDX-License-Identifier: Apache-2.0

#include "./settings-object.h"

#include "../format-file-config.h"
#include "./condition-context.h"

#include "config-condition.h"
#include "config-schema.h"


/*!
    @class FormatPercentResolver
    @internal

    Resolves a condition against the formatting context at a percentage.
*/

namespace {

/*!
    Answers a condition of percents at a place as @a place does, with
    @a percent as the place's `spaceUsed` (100 less its `spaceRemaining`);
    hence the condition can be asked at every share of the place.
*/
class PercentContext : public FormatConditionContext
{
    typedef FormatConditionContext super;
public:
    inline PercentContext(const FormatConditionContext *place, qreal percent)
        : m_place(place), m_percent(percent)
    {
    }

    inline QStringList onNames() const Q_DECL_OVERRIDE_IL
    { return m_place->names(); }

    inline bool predicate(const QString &name,
                          bool *value) const Q_DECL_OVERRIDE_IL
    {
        return m_place->predicate(name, value);
    }

    inline bool value(qreal *value) const Q_DECL_OVERRIDE_IL
    { return m_place->value(value); }

    inline bool argumentCount(int depth, int argument, qreal *count) const Q_DECL_OVERRIDE_IL
    { return m_place->argumentCount(depth, argument, count); }

    inline bool hasMoreArgs(bool *value) const Q_DECL_OVERRIDE_IL
    { return m_place->hasMoreArgs(value); }

    inline bool isWrapping(bool *value) const Q_DECL_OVERRIDE_IL
    { return m_place->isWrapping(value); }

    inline FormatArgsArray *callArguments() const Q_DECL_OVERRIDE_IL
    { return m_place->callArguments(); }

    inline bool hasStandard(const QString & standard, bool *value) const Q_DECL_OVERRIDE_IL
    { return m_place->hasStandard(standard, value); }

    inline bool hasCompiler(const QString & compiler, bool *value) const Q_DECL_OVERRIDE_IL
    { return m_place->hasCompiler(compiler, value); }

    inline bool spaceRemaining(qreal *percent) const Q_DECL_OVERRIDE_IL
    {
        *percent = 100 - m_percent;
        return true;
    }

    inline bool isTarget(const QString & kind,
                         bool *value) const Q_DECL_OVERRIDE_IL
    {
        return m_place->isTarget(kind, value);
    }

    inline const FormatFileConfig *fileConfig() const Q_DECL_OVERRIDE_IL
    {
        return m_place->fileConfig();
    }

private:
    const FormatConditionContext *m_place;
    qreal m_percent;
};

} // namespace

/*!
    Resolves @a program at @a place with @a percent as the space used.
    Builds an invalid value where the condition misses a value.
*/
QVariant FormatPercentResolver::resolve(
        const QStringList & program, const FormatConditionContext *place, int percent)
{
    const PercentContext context(place, percent);
    const QVariant holds = Config::Condition::resolve(program, QSet<QString >(), &context);
    return context.lastResolvePassed() ? holds : QVariant();
}
