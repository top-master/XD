// Copyright (C) 2015 The XD Company Ltd.
// SPDX-License-Identifier: Apache-2.0

#include "./condition-object.h"

#include "./condition-context.h"
#include "./settings-object.h"

/*!
    @class FormatConditionObject
    @internal

    Offers the names that the tool's settings may use in a condition (see
    Config::Condition) at every place, each answered by the
    FormatConditionContext of the place in question (see
    Config::Object::context()):

    * `argCount(<depth>)` and `argCount(<depth>, <argument>)`: the
      arguments of the calls at that depth (see
      FormatConditionContext::argumentCount());
    * `spaceRemaining`: the share of `[Structure] wrapMax` that is left on
      the line;
    * `hasMoreArgs`: another argument follows the one in question;
    * `isWrapping`: the argument in question gets a line of its own, since
      the tool breaks its list;
    * `hasStd("<standard>")`: the project targets that C++ standard, such
      as `hasStd("C++11")` (see `[Lint] standards`);
    * `hasCompiler("<spec>")`: the project builds with that compiler, named
      by its qmake spec, such as `hasCompiler("win32-msvc2010")` (see
      `[Code] compilers`);
    * `is(<kind>)`: the list at the place is of that kind: `array`, a brace
      initializer list; `call`, a call's arguments; or `condition`, the
      condition of an `if` or a `while` (see FormatScopeConditions::targetIs());
    * `cfg.<key>`: the number that the `[Structure]` setting `<key>` gives
      at the place, such as `cfg.wrapArgsOver` (see FormatSettingsObject).

    FormatConditionContext::registerObjects() adds it to the global scope of
    Config::Scopes once, as the tool starts. A name the place cannot answer
    makes the condition miss a value; hence its setting's default decides.
*/

/*!
    Builds the object of the expressions of settings, with the settings
    object that `cfg` answers as its child.
*/
FormatConditionObject::FormatConditionObject()
    : m_settings(new FormatSettingsObject(this))
{
}

/*!
    Builds the settings of the place, whose keys a condition reads as
    `cfg.<key>` (see FormatSettingsObject).
*/
QObject *FormatConditionObject::cfg() const
{
    return this->m_settings;
}

/*!
    Builds the share of `[Structure] wrapMax`, from 0 to 100, that is left on
    the line, or an invalid value where the place has no line to measure.
*/
QVariant FormatConditionObject::spaceRemaining() const
{
    const FormatConditionContext *format =
        static_cast<const FormatConditionContext * >(this->context());
    qreal percent = 0;
    if ( ! format || ! format->spaceRemaining(&percent)) {
        return QVariant();
    }
    return percent;
}

/*!
    Builds whether another argument follows the one in question, or an
    invalid value where the place is no argument.
*/
QVariant FormatConditionObject::hasMoreArgs() const
{
    const FormatConditionContext *format =
        static_cast<const FormatConditionContext * >(this->context());
    bool value = false;
    if ( ! format || ! format->hasMoreArgs(&value)) {
        return QVariant();
    }
    return value;
}

/*!
    Builds whether the argument in question gets a line of its own, or an
    invalid value where the place is no argument.
*/
QVariant FormatConditionObject::isWrapping() const
{
    const FormatConditionContext *format =
        static_cast<const FormatConditionContext * >(this->context());
    bool value = false;
    if ( ! format || ! format->isWrapping(&value)) {
        return QVariant();
    }
    return value;
}

/*!
    Builds the number of arguments that the calls at @a depth take, of the
    argument @a argument (one-based, or zero for every argument), or an
    invalid value where the place has no argument list.
*/
QVariant FormatConditionObject::argCount(int depth, int argument)
{
    const FormatConditionContext *format =
        static_cast<const FormatConditionContext * >(this->context());
    qreal count = 0;
    if ( ! format || ! format->argumentCount(depth, argument, &count)) {
        return QVariant();
    }
    return count;
}

/*!
    Builds whether the project targets the C++ standard @a standard, such as
    `C++11`, in any case, or an invalid value where the place does not know
    the project's standards.
*/
QVariant FormatConditionObject::hasStd(const QString & standard)
{
    return this->ask(&FormatConditionContext::hasStandard, standard);
}

/*!
    Builds whether the project builds with the compiler @a compiler, named by
    its qmake spec, such as `win32-msvc2010`, in any case, or an invalid
    value where the place does not know the project's compilers.
*/
QVariant FormatConditionObject::hasCompiler(const QString & compiler)
{
    return this->ask(&FormatConditionContext::hasCompiler, compiler);
}

/*!
    Builds whether the list at the place is of the kind @a kind, such as
    `array` (see FormatScopeConditions::targetIs()), or an invalid value
    where the place is no list, or the kind is unknown.
*/
QVariant FormatConditionObject::is(const QString & kind)
{
    return this->ask(&FormatConditionContext::isTarget, kind);
}

/*!
    Builds the answer of the context at the place to @a question about
    @a name, or an invalid value where the place has no such context, or
    the context does not know the answer.
*/
QVariant FormatConditionObject::ask(
        Question question,
        const QString & name) const
{
    const FormatConditionContext *format =
        static_cast<const FormatConditionContext * >(this->context());
    bool value = false;
    if ( ! format || ! (format->*question)(name, &value)) {
        return QVariant();
    }
    return value;
}
