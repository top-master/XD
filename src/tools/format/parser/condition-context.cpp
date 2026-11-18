// Copyright (C) 2015 The XD Company Ltd.
// SPDX-License-Identifier: Apache-2.0

#include "./condition-context.h"

#include "../format-file-config.h"
#include "./condition-object.h"
#include "./count-object.h"
#include "../rules/template-object.h"

/*!
    @class FormatConditionContext
    @internal

    Answers the names that the tool's settings may use in a condition (see
    Config::Condition and FormatConditionObject), at the place where a rule
    evaluates it: each place that evaluates a condition passes a subclass
    that answers what holds there, and an answer it lacks makes the
    condition miss a value; hence its setting's default decides.

    `value`, in a count setting only, comes from FormatCountObject (see
    pushCountScope()).
*/

/*!
    Builds in @a value the count that the setting in question is about;
    returns false when the place has none.
*/
bool FormatConditionContext::value(qreal *) const
{
    return false;
}

/*!
    Builds in @a count the number of arguments that the calls at @a depth
    take, of the argument @a argument (one-based, or zero for every
    argument); returns false when the place has no argument list.
*/
bool FormatConditionContext::argumentCount(int, int, qreal *) const
{
    return false;
}

/*!
    Builds in @a percent the share of `[Structure] wrapMax`, from 0 to 100,
    that is left on the line after the place; returns false when the place
    has no line to measure.
*/
bool FormatConditionContext::spaceRemaining(qreal *) const
{
    return false;
}

/*!
    Tells in @a value whether another argument follows the one in question;
    returns false when the place is no argument.
*/
bool FormatConditionContext::hasMoreArgs(bool *) const
{
    return false;
}

/*!
    Tells in @a value whether the argument in question gets a line of
    its own because the tool breaks its list; returns false when the
    place is no argument.
*/
bool FormatConditionContext::isWrapping(bool *) const
{
    return false;
}

/*!
    Tells in @a value whether the project targets the C++ standard
    @a standard, such as `C++11` (see `[Lint] standards`); returns false
    when the place does not know the project's standards.
*/
bool FormatConditionContext::hasStandard(const QString &, bool *) const
{
    return false;
}

/*!
    Tells in @a value whether the project builds with the compiler
    @a compiler, such as `win32-msvc2010`, in any case (see `[Code]
    compilers`), from the settings of the place (see fileConfig()); returns
    false when the place names no settings.
*/
bool FormatConditionContext::hasCompiler(const QString & compiler, bool *value) const
{
    const FormatFileConfig *config = this->fileConfig();
    if ( ! config) {
        return false;
    }
    *value = config->compilers.contains(compiler, Qt::CaseInsensitive);
    return true;
}

/*!
    Builds the settings that hold at the place, which `cfg` answers from
    (see FormatSettingsObject), or null when the place names none.
*/
const FormatFileConfig *FormatConditionContext::fileConfig() const
{
    return Q_NULLPTR;
}

/*!
    Builds the arguments of the call that a call template turns, as written
    (see FormatTemplateObject), or null when the place is no such call.
*/
FormatArgsArray *FormatConditionContext::callArguments() const
{
    return Q_NULLPTR;
}

/*!
    Lets the global scope of Config::Scopes get, at its first use, the
    objects whose names the tool's conditions use at every place (see
    setGlobalObjects()).
*/
void FormatConditionContext::registerObjects()
{
    Config::Scopes::addGlobalSetup(&FormatConditionContext::setGlobalObjects);
}

/*!
    Adds the objects whose names the tool's conditions use at every place to
    @a scope, the global one; every context the tool passes to a condition
    is a FormatConditionContext, hence each name reaches the answer of the
    place in question.
*/
void FormatConditionContext::setGlobalObjects(Config::Scopes::Scope *scope)
{
    scope->add(new FormatConditionObject);
    scope->add(new FormatTemplateObject);
}

/*!
    Pushes the scope of a count setting (`wrapArgsOver`,
    `wrapConditionsOver` and `joinArgs`), whose conditions alone may say
    `value` (see value()); a caller holds it while it compiles or evaluates
    such a setting, and `value` goes with the last pointer to it.
*/
QSharedPointer<Config::Scopes::Scope > FormatConditionContext::pushCountScope()
{
    const QSharedPointer<Config::Scopes::Scope > scope =
        Config::Scopes::push();
    FormatConditionContext::setCountObjects(scope.data());
    return scope;
}

/*!
    Adds the object of a count setting (see pushCountScope()) to @a scope,
    for the schema's matcher of such a setting.
*/
void FormatConditionContext::setCountObjects(Config::Scopes::Scope *scope)
{
    scope->add(new FormatCountObject);
}

namespace {

/*!
    Registers the objects before any setting is read.
*/
static void registerFormatObjects()
{
    FormatConditionContext::registerObjects();
}

} // namespace

Q_CONSTRUCTOR_FUNCTION(registerFormatObjects)
