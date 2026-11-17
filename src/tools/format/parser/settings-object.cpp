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
    FormatConditionContext names, else nothing; the property's stored value
    is the key's default, as the schema spells it.
*/

namespace {

/*!
    Lists, for each thread, the keys whose condition the thread evaluates
    right now; hence a key that its own condition reaches, directly or
    through another key, answers nothing instead of looping.
*/
Q_GLOBAL_STATIC(QThreadStorage<QSet<QByteArray > >, busyKeys)

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
    setting that gives a number.
*/
QStringList FormatSettingsObject::keys()
{
    return QStringList() << QString(QLL("wrapMax"))
        << QString(QLL("wrapArgsOver")) << QString(QLL("wrapConditionsOver"))
        << QString(QLL("joinArgs"));
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
    qreal value = 0;
    const bool ok = Config::Condition::value(
        *program,
        QSet<QString >(),
        format,
        &value
    );
    busy->remove(name);
    return ok ? QVariant(value) : QVariant();
}
