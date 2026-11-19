// Copyright (C) 2015 The XD Company Ltd.
// SPDX-License-Identifier: Apache-2.0

#include "./template-object.h"

#include "./args-array.h"
#include "../parser/condition-context.h"

#include "config-cache.h"

/*!
    @class FormatTemplateObject
    @internal

    Offers `args` to the call templates of `[Replace.types]` (see
    FormatReplaceTypesRule): the arguments of the call that a template turns,
    as a FormatArgsArray, which a template reaches on with `args[<index>]`
    or `args.<name>(...)`. FormatConditionContext::registerObjects() adds it
    to the global scope of Config::Scopes once, as the tool starts.
*/

/*!
    Builds the arguments of the call that the template in question turns, or
    null where the place is no such call.
*/
FormatArgsArray *FormatTemplateObject::args() const
{
    Config::Cache::skip();
    const FormatConditionContext *format =
        static_cast<const FormatConditionContext * >(this->context());
    if ( ! format) {
        return Q_NULLPTR;
    }
    return format->callArguments();
}
