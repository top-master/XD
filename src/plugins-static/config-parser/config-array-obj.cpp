// Copyright (C) 2015 The XD Company Ltd.
// SPDX-License-Identifier: Apache-2.0

#include "./config-array-obj.h"

/*!
    @class Config::ArrayObj
    @internal

    Offers a list of texts to expressions, such as the arguments of a call
    that a tool rewrites: `size`, `at(<index>)` (zero-based), and
    `join(<separator>)`. A tool may extend it with names of its own.
*/

/*!
    Builds the array of @a items, a child of @a parent.
*/
Config::ArrayObj::ArrayObj(const QStringList & items, QObject *parent)
    : super(parent), m_items(items)
{
}

/*!
    Builds the items joined by @a separator.
*/
QString Config::ArrayObj::join(const QString & separator) const
{
    return this->m_items.join(separator);
}

/*!
    Builds the item at @a index (zero-based), or an invalid value for an
    index past the items.
*/
QVariant Config::ArrayObj::at(int index) const
{
    if (index < 0 || index >= this->m_items.size()) {
        return QVariant();
    }
    return this->m_items.at(index);
}
