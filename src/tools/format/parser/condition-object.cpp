// Copyright (C) 2015 The XD Company Ltd.
// SPDX-License-Identifier: Apache-2.0

#include "./condition-object.h"

#include "./condition-context.h"

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
      the tool breaks its list.

    FormatConditionContext::registerObjects() adds it to the global scope of
    Config::Scopes once, as the tool starts. A name the place cannot answer
    makes the condition miss a value; hence its setting's default decides.
*/

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
