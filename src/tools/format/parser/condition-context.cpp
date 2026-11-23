// Copyright (C) 2015 The XD Company Ltd.
// SPDX-License-Identifier: Apache-2.0

#include "./condition-context.h"

#include "../format-file-config.h"
#include "../format-document.h"
#include "./condition-object.h"
#include "./settings-object.h"
#include "./count-object.h"
#include "../rules/template-object.h"
#include "config-cache.h"

/*!
    Lists the predicates available to replacement conditions.
*/
QStringList FormatReplaceTypesContext::onNames() const
{
    return QStringList() << QString(QLL("isNew"));
}

/*!
    Answers whether the current line is selected.
*/
bool FormatReplaceTypesContext::predicate(
        const QString & name, bool *value) const
{
    if (name.compare(QLL("isNew"), Qt::CaseInsensitive) != 0
        || ! this->m_document
    ) {
        return false;
    }
    Config::Cache::skip();
    *value = this->m_document->isLineSelected(this->m_line);
    return true;
}

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
    Tells in @a value whether the list @a list of @a config holds @a name,
    in any case; returns false without @a config.
*/
static bool listsName(const FormatFileConfig *config,
                      QStringList FormatFileConfig::*list,
                      const QString & name, bool *value)
{
    if ( ! config) {
        return false;
    }
    *value = (config->*list).contains(name, Qt::CaseInsensitive);
    return true;
}

/*!
    Tells in @a value whether the project targets the C++ standard
    @a standard, such as `C++11`, in any case (see `[Lint] standards`), from
    the settings of the place (see fileConfig()); returns false when the
    place names no settings.
*/
bool FormatConditionContext::hasStandard(
        const QString & standard, bool *value) const
{
    return listsName(
        this->fileConfig(),
        &FormatFileConfig::standards,
        standard,
        value
    );
}

/*!
    Tells in @a value whether the project builds with the compiler
    @a compiler, such as `win32-msvc2010`, in any case (see `[Code]
    compilers`), from the settings of the place (see fileConfig()); returns
    false when the place names no settings.
*/
bool FormatConditionContext::hasCompiler(
        const QString & compiler, bool *value) const
{
    return listsName(
        this->fileConfig(),
        &FormatFileConfig::compilers,
        compiler,
        value
    );
}

/*!
    Tells in @a value whether the list at the place is of the kind @a kind
    (see FormatScopeConditions::targetIs()), asked each time a condition
    calls `is()`; returns false when the place is no list, or the kind is
    unknown.
*/
bool FormatConditionContext::isTarget(const QString &, bool *) const
{
    return false;
}

/*!
    Builds the settings that hold at the place, which `cfg` answers from
    (see Config::CfgObject), or null when the place names none.
*/
const FormatFileConfig *FormatConditionContext::fileConfig() const
{
    return Q_NULLPTR;
}

/*!
    Builds the effective settings at the place of the condition.
*/
const Config::Settings *FormatConditionContext::settings() const
{
    const FormatFileConfig *config = this->fileConfig();
    return config ? &config->settings : Q_NULLPTR;
}

QVariant FormatConditionContext::resolveAtPercent(
        const QStringList & program, int percent) const
{
    return FormatPercentResolver::resolve(program, this, percent);
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
    scope->add(new Config::GlobalObject(FormatFileConfig::schema()));
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
