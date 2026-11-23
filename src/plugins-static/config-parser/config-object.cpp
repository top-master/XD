// Copyright (C) 2015 The XD Company Ltd.
// SPDX-License-Identifier: Apache-2.0

#include "./config-object.h"

#include "./config-types.h"
#include "./config-schema.h"
#include "./config-condition.h"
#include "./config-settings.h"
#include "./config-cache.h"

#include <QtCore/qfunction.h>
#include <QtCore/QThreadStorage>
#include <QtCore/QSet>

/*!
    @class Config::Object
    @internal

    Offers names to the expressions of settings (see Config::Condition): a
    tool adds such an object to a scope of Config::Scopes, and each public
    slot and each property that a subclass declares becomes a name an
    expression may call, such as `argCount(1)` or `spaceRemaining`.

    Only what Config::Object and its subclasses declare counts: nothing of
    QObject itself, such as `deleteLater()` or `objectName`, whatever QObject
    may add later. No expression emits a signal, but a slot may emit one of
    its own when its work needs it.

    A slot or property answers for the place an expression is evaluated at,
    which context() gives while the call runs. One that has no answer there
    returns an invalid QVariant; hence the expression misses a value. One
    that returns a pointer to another Config::Object, such as an array, lets
    the expression go on with that object's names (see Config::Types).

    A dynamic property, one that `QObject::setProperty()` adds without a
    declaration, is a name too: dynamicValue() answers it, which gives the
    stored value unless a subclass answers for the place instead.
*/

namespace {

/*!
    Tells a thread which context the call it runs answers for (see
    Config::Object::context()).
*/
class CurrentContext
{
public:
    inline CurrentContext() : context(Q_NULLPTR) {}

    const Config::Evaluator::Context *context;
};

/*!
    Gives each thread the context its running call answers for; hence
    threads that format files at once see each its own place.
*/
Q_GLOBAL_STATIC(QThreadStorage<CurrentContext >, currentContexts)
Q_GLOBAL_STATIC(QThreadStorage<QSet<QString > >, settingsInProgress)

} // namespace

/*!
    Builds an object of the expressions of settings, a child of @a parent.
*/
Config::Object::Object(QObject *parent)
    : QObject(parent)
{
    Config::Types::track(this);
}

/*!
    Destroys the object, which from then on no expression reaches.
*/
Config::Object::~Object()
{
    Config::Types::untrack(this);
}

/*!
    Builds the context of the place where the expression in question runs,
    while one of the object's slots or properties answers it; outside of such
    a call, it builds null.
*/
const Config::Evaluator::Context *Config::Object::context() const
{
    return currentContexts()->localData().context;
}

/*!
    Builds the stored value of the dynamic property @a name.
*/
QVariant Config::Object::dynamicValue(const QByteArray & name) const
{
    return this->property(name.constData());
}

bool Config::Object::hasDynamicValue(const QByteArray & name) const
{
    return ! this->dynamicValue(name).isNull();
}

/*!
    Sets @a context as the one context() gives in this thread, and returns
    the one it gave before; Config::Scopes sets it around each call, and
    puts the earlier one back after it.
*/
const Config::Evaluator::Context *Config::Object::setContext(
        const Config::Evaluator::Context *context)
{
    CurrentContext &current = currentContexts()->localData();
    const Config::Evaluator::Context *previous = current.context;
    current.context = context;
    return previous;
}

/*!
    Builds a settings object whose properties come from @a schema.
*/
Config::CfgObject::CfgObject(const Config::Schema & schema)
    : m_schema(schema), m_section(QLL("structure"))
{
}

bool Config::CfgObject::hasDynamicValue(const QByteArray & name) const
{
    return ! m_schema.key(m_section + QLatin1Char('.')
        + QString::fromLatin1(name)).isNull();
}

/*!
    Resolves @a name from the effective settings of the current place.
*/
QVariant Config::CfgObject::dynamicValue(const QByteArray & name) const
{
    Config::Cache::skip();
    const Config::Evaluator::Context *place = this->context();
    const Config::Settings *settings = place ? place->settings() : Q_NULLPTR;
    if ( ! settings) {
        return QVariant();
    }
    const QString keyName = m_section + QLatin1Char('.') + QString::fromLatin1(name).toLower();
    const Config::Key key = m_schema.key(keyName);
    const QString value = settings->value(m_schema, keyName);
    QSet<QString > *active = &(settingsInProgress()->localData());
    if (active->contains(keyName)) {
        return QVariant();
    }
    active->insert(keyName);
    Q_DEFER { active->remove(keyName); };
    if (key.referenceThreshold) {
        return m_schema.reference(keyName, value, place);
    }
    for (int i = 0; i < key.matchers.size(); ++i) {
        QString matched;
        const Config::ValueMatcher *matcher = key.matchers.at(i).data();
        if (key.allowsCode() ? matcher->match(value, &matched)
                             : matcher->matchLiteral(value, &matched)) {
            const QVariant resolved = matcher->settingValue(matched, place);
            if ( ! resolved.isValid()) {
                return QVariant();
            }
            const QVariant converted = m_schema.convert(keyName, resolved);
            if (converted.isValid()) {
                if (converted.type() == QVariant::LongLong || converted.type() == QVariant::Int) {
                    return converted.toDouble();
                }
                return converted;
            }
            return resolved;
        }
    }
    return m_schema.reference(keyName, value, place);
}

/*!
    Offers @a settings to conditions as `cfg`.
*/
Config::GlobalObject::GlobalObject(const Config::Schema & schema)
    : m_settings(new Config::CfgObject(schema))
{
    m_settings->setParent(this);
}

QObject *Config::GlobalObject::cfg() const
{
    return m_settings;
}
