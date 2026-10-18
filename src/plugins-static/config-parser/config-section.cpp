// Copyright (C) 2015 The XD Company Ltd.
// SPDX-License-Identifier: Apache-2.0

#include "./config-section.h"

#include "./config-word.h"

/*!
    @class Config::Section
    @internal

    Lists the settings of one section of a tool's settings files, such as the
    `[Types]` of `[Types] refSuffix = " "`; a settings file may hold any
    number of sections. Each key's name is `<section>.<key>`, in lower case.

    A null section (see isNull()), one without a name, stands for a section
    that the schema does not list.
*/

/*!
    Builds the empty section @a name (in any case), for addKey() to fill.
*/
Config::Section::Section(const QString & name)
    : m_name(name)
{
}

/*!
    Adds the setting @a name (the key within the section, in any case),
    which takes each value one of @a matchers matches, and starts as
    @a defaultValue; @a aliases maps another spelling of a value to the
    value it stands for.
*/
void Config::Section::addKey(
        const QString & name, const Config::Matchers & matchers,
        const QString & defaultValue, const QHash<QString, QString > & aliases)
{
    Config::Key key;
    key.name = (this->m_name + QLatin1Char('.') + name).toLower();
    key.matchers = matchers;
    key.defaultValue = defaultValue;
    QHash<QString, QString >::const_iterator alias = aliases.constBegin();
    for (; alias != aliases.constEnd(); ++alias) {
        key.aliases.insert(alias.key().toLower(), alias.value());
    }
    this->m_keys.append(key);
}

/*!
    Extends the keys @a names (in any case) of the section @a base into this
    section: each gets the same matchers, default, and aliases, and follows
    the base key's value until a file or an assignment overrides it here
    (see Config::Key::extends). Hence a section can be the override of
    another one for just those keys.
*/
void Config::Section::extend(const Config::Section & base,
                             const QStringList & names)
{
    for (int n = 0; n < names.size(); ++n) {
        const QString baseName = (base.name() + QLatin1Char('.') + names.at(n)).toLower();
        for (int k = 0; k < base.keys().size(); ++k) {
            Config::Key key = base.keys().at(k);
            if (key.name != baseName) {
                continue;
            }
            key.name = (this->m_name + QLatin1Char('.') + names.at(n)).toLower();
            key.extends = baseName;
            this->m_keys.append(key);
        }
    }
}

/*!
    Adds the setting @a name that takes one of the fixed @a words (in any
    case) as its values; see the overload that takes matchers.
*/
void Config::Section::addKey(const QString & name, const QStringList & words,
                             const QString & defaultValue,
                             const QHash<QString, QString > & aliases)
{
    Config::Matchers matchers;
    for (int i = 0; i < words.size(); ++i) {
        matchers << Config::Word::create(words.at(i));
    }
    this->addKey(name, matchers, defaultValue, aliases);
}
