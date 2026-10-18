// Copyright (C) 2015 The XD Company Ltd.
// SPDX-License-Identifier: Apache-2.0

#ifndef CONFIG_KEY_H
#define CONFIG_KEY_H

#include "./config-value-matcher.h"

#include <QtCore/QHash>
#include <QtCore/QString>

namespace Config {

/*!
    \internal

    Describes one setting a tool supports: its name (`<section>.<key>`, in
    lower case), the matchers of the values it takes (see
    Config::ValueMatcher), the value it has when nothing sets it, and the other
    spellings of a value (such as `none` for an empty one).

    A key that extends another one (see Config::Section::extend()) names it
    in `extends`: until a file or an assignment overrides the key itself, its
    value is the extended key's current value.

    A null key (see isNull()), one without a name, stands for a setting that
    the schema does not list.
*/
class Key
{
public:
    inline Key() {}

    inline bool isNull() const { return name.isEmpty(); }

    QString name;
    Matchers matchers;
    QString defaultValue;
    QHash<QString, QString > aliases;
    /// Lets the key follow the key it extends, `<section>.<key>` in lower
    /// case, until it is overridden; empty for a key of its own.
    QString extends;
};

} // namespace Config

#endif // CONFIG_KEY_H
