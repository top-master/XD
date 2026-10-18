// Copyright (C) 2015 The XD Company Ltd.
// SPDX-License-Identifier: Apache-2.0

#include "./config-schema.h"

/*!
    @class Config::Schema
    @internal

    Lists the sections, and their settings, that a tool supports, which
    Config::Parser accepts; any other section or key, and any value a key does
    not take, is an error.
*/

/*!
    Adds @a section, with its keys, to the schema.
*/
void Config::Schema::addSection(const Config::Section & section)
{
    this->m_sections.append(section);
}

/*!
    Finds the section @a name, in any case; a name the schema does not list
    gets a null section (see Config::Section::isNull()).
*/
Config::Section Config::Schema::section(const QString & name) const
{
    for (int i = 0; i < this->m_sections.size(); ++i) {
        if (this->m_sections.at(i).name().compare(
                name,
                Qt::CaseInsensitive
            ) == 0
        ) {
            return this->m_sections.at(i);
        }
    }
    return Config::Section();
}

/*!
    Finds the setting @a name (`<section>.<key>`), in any case; a name the
    schema does not list gets a null key (see Config::Key::isNull()).
*/
Config::Key Config::Schema::key(const QString & name) const
{
    const int dot = name.indexOf(QLatin1Char('.'));
    const Config::Section found = this->section(name.left(dot));
    const QString lower = name.toLower();
    for (int i = 0; i < found.keys().size(); ++i) {
        if (found.keys().at(i).name == lower) {
            return found.keys().at(i);
        }
    }
    return Config::Key();
}
