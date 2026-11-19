// Copyright (C) 2015 The XD Company Ltd.
// SPDX-License-Identifier: Apache-2.0

#include "./count-object.h"

#include "./condition-context.h"

#include "config-cache.h"

/*!
    @class FormatCountObject
    @internal

    Offers `value` to the conditions of a count setting (`wrapArgsOver`,
    `wrapConditionsOver` and `joinArgs`) alone: the count the setting is
    about, such as the arguments of the list in question (see
    FormatConditionContext::pushCountScope()).
*/

/*!
    Builds the count the setting in question is about, or an invalid value
    where the place has none.
*/
QVariant FormatCountObject::value() const
{
    Config::Cache::skip();
    const FormatConditionContext *format =
        static_cast<const FormatConditionContext * >(this->context());
    qreal count = 0;
    if ( ! format || ! format->value(&count)) {
        return QVariant();
    }
    return count;
}
