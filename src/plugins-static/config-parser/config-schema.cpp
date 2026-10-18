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
    Finds the section that the setting @a name (`<section>.<key>`, in any
    case) belongs to: the one with the longest name that, with a `.` after
    it, starts @a name; hence a section's own name may hold a `.`, as in
    `[Replace.types]`. A name no section starts gets a null section.
*/
Config::Section Config::Schema::sectionOf(const QString & name) const
{
    Config::Section found;
    for (int i = 0; i < this->m_sections.size(); ++i) {
        const QString prefix = this->m_sections.at(i).name() + QLatin1Char('.');
        if (name.startsWith(prefix, Qt::CaseInsensitive)
            && prefix.size() > found.name().size() + 1
        ) {
            found = this->m_sections.at(i);
        }
    }
    return found;
}

/*!
    Finds the setting @a name (`<section>.<key>`), in any case; a name the
    schema does not list gets a null key (see Config::Key::isNull()), unless
    its section is open (see Config::Section::setOpen()), which takes it
    with the section's open matchers.
*/
Config::Key Config::Schema::key(const QString & name) const
{
    const Config::Section found = this->sectionOf(name);
    const QString lower = name.toLower();
    for (int i = 0; i < found.keys().size(); ++i) {
        if (found.keys().at(i).name == lower) {
            return found.keys().at(i);
        }
    }
    if (found.isOpen() && name.size() > found.name().size() + 1) {
        Config::Key key;
        key.name = lower;
        key.matchers = found.openMatchers();
        key.open = true;
        return key;
    }
    return Config::Key();
}
