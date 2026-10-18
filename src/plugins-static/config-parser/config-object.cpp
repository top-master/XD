// Copyright (C) 2015 The XD Company Ltd.
// SPDX-License-Identifier: Apache-2.0

#include "./config-object.h"

#include "./config-types.h"

#include <QtCore/QThreadStorage>

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
