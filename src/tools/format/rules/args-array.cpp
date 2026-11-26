// Copyright (C) 2015 The XD Company Ltd.
// SPDX-License-Identifier: Apache-2.0

#include "./args-array.h"

/*!
    @class FormatArgsArray
    @internal

    Offers the arguments of a call that `[Replace.types]` turns to its call
    template, as `args`: each argument as the source writes it, with the
    names of Config::ArrayObj, such as
    `args[0]` and `args.join(", ")`.
*/

/*!
    Builds the array of @a arguments, a child of @a parent.
*/
FormatArgsArray::FormatArgsArray(const QStringList & arguments, QObject *parent)
    : super(arguments, parent)
{
}
