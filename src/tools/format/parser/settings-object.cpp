// Copyright (C) 2015 The XD Company Ltd.
// SPDX-License-Identifier: Apache-2.0

#include "./settings-object.h"

#include "../format-file-config.h"
#include "./condition-context.h"

#include "config-condition.h"
#include "config-schema.h"

#include <QtCore/QSet>
#include <QtCore/QThreadStorage>

/*!
    @class FormatSettingsObject
    @internal

    Offers `cfg`, the settings of the place where a condition is evaluated,
    to every condition of the tool (see FormatConditionObject): each key of
    keys() is a dynamic property, such as `cfg.wrapArgsOver`; hence a setting
    may follow another one, as in `wrapConditionsOver = cfg.wrapArgsOver`.

    A key answers the number its setting gives at the place (see
    dynamicValue()), from the settings that the place's
    FormatConditionContext names, else nothing; a key of percents, such as
    `wrapArgsAt`, answers the `spaceUsed` past which it holds there (`40%`,
    for one). Each property stores the default the schema spells for its key.
*/

namespace {

/*!
    Lists, for each thread, the keys whose condition the thread evaluates
    right now; hence a key that its own condition reaches, directly or
    through another key, answers nothing instead of looping.
*/
Q_GLOBAL_STATIC(QThreadStorage<QSet<QByteArray > >, busyKeys)

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

    inline QStringList names() const Q_DECL_OVERRIDE_IL
    { return m_place->names(); }

    inline bool predicate(const QString &name,
                          bool *value) const Q_DECL_OVERRIDE_IL
    {
        return m_place->predicate(name, value);
    }

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
    Builds the settings object, a child of @a parent, with a dynamic property
    for each key of keys(), whose stored value is that key's default.
*/
FormatSettingsObject::FormatSettingsObject(QObject *parent)
    : super(parent)
{
    const Config::Schema &schema = FormatFileConfig::schema();
    const QStringList names = FormatSettingsObject::keys();
    for (int i = 0; i < names.size(); ++i) {
        const QString key = QString(QLL("structure.")) + names.at(i);
        this->setProperty(
            names.at(i).toLatin1().constData(),
            schema.key(key).defaultValue
        );
    }
}

/*!
    Builds the `[Structure]` keys that `cfg` offers: each of them names a
    setting that gives a number, or a percent.
*/
QStringList FormatSettingsObject::keys()
{
    return QStringList() << QString(QLL("wrapMax"))
        << QString(QLL("wrapArgsOver")) << QString(QLL("wrapConditionsOver"))
        << QString(QLL("joinArgs")) << QString(QLL("wrapChainAt"))
        << QString(QLL("wrapArgsAt")) << QString(QLL("wrapSignatureAt"));
}

/*!
    Builds the number that the setting @a name gives at the place in
    question: the setting's number, else what its condition gives there,
    evaluated with the same place, whose `value` stays the count that place
    is about. Builds an invalid value where the place names no settings, the
    condition misses a value, or the key is already being evaluated.
*/
QVariant FormatSettingsObject::dynamicValue(const QByteArray & name) const
{
    const FormatConditionContext *format =
        static_cast<const FormatConditionContext * >(this->context());
    const FormatFileConfig *fileConfig = format ? format->fileConfig()
                                                : Q_NULLPTR;
    if ( ! fileConfig) {
        return QVariant();
    }
    if (name == "wrapMax") {
        return fileConfig->wrapMax;
    }
    if (name == "wrapChainAt") {
        return FormatSettingsObject::threshold(
            name, fileConfig->wrapChainAt, format
        );
    }
    if (name == "wrapArgsAt") {
        return FormatSettingsObject::threshold(
            name, fileConfig->wrapArgsAt, format
        );
    }
    if (name == "wrapSignatureAt") {
        return FormatSettingsObject::threshold(
            name, fileConfig->wrapSignatureAt, format
        );
    }

    // Picks the number and the condition of a count setting.
    int count = 0;
    const QStringList *program = Q_NULLPTR;
    if (name == "wrapArgsOver") {
        count = fileConfig->wrapArgsOver;
        program = &fileConfig->wrapArgsOverCondition;
    } else if (name == "wrapConditionsOver") {
        count = fileConfig->wrapConditionsOver;
        program = &fileConfig->wrapConditionsOverCondition;
    } else if (name == "joinArgs") {
        count = fileConfig->joinArgs;
        program = &fileConfig->joinArgsCondition;
    } else {
        return super::dynamicValue(name);
    }
    if (program->isEmpty()) {
        return count;
    }

    // Evaluates the condition, unless this thread already does.
    QSet<QByteArray > *busy = &(busyKeys()->localData());
    if (busy->contains(name)) {
        return QVariant();
    }
    busy->insert(name);
    const QSharedPointer<Config::Scopes::Scope > scope =
        FormatConditionContext::pushCountScope();
    QVariant value;
    const bool ok = FormatFileConfig::resolve(
        QLL("structure.") + QString::fromLatin1(name),
        *program,
        format,
        &value
    );
    busy->remove(name);
    return ok ? QVariant(value.toDouble()) : QVariant();
}

/*!
    Builds the `spaceUsed` past which the condition @a program of the key
    @a name holds at the place that @a place answers, such as `40%`: one
    less than the smallest whole percent, from 0 to 100, at which it holds,
    else `100%`. Builds an invalid value where the condition misses a
    value, or the key is already being evaluated (see dynamicValue()).
*/
QVariant FormatSettingsObject::threshold(
        const QByteArray & name, const QStringList & program,
        const FormatConditionContext *place)
{
    QSet<QByteArray > *busy = &(busyKeys()->localData());
    if (busy->contains(name)) {
        return QVariant();
    }
    busy->insert(name);
    int holdsFrom = 101;
    bool ok = true;
    for (int percent = 0; percent <= 100 && ok; ++percent) {
        const PercentContext context(place, percent);
        QVariant holds;
        ok = FormatFileConfig::resolve(
            QLL("structure.") + QString::fromLatin1(name),
            program,
            &context,
            &holds
        );
        if (ok && holds.toBool()) {
            holdsFrom = percent;
            break;
        }
    }
    busy->remove(name);
    if ( ! ok) {
        return QVariant();
    }
    return QString(
        QString::number(qMin(holdsFrom - 1, 100)) + QLatin1Char('%')
    );
}
