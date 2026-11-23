// Copyright (C) 2015 The XD Company Ltd.
// SPDX-License-Identifier: Apache-2.0

#ifndef CONFIG_SETTINGS_H
#define CONFIG_SETTINGS_H

#include <QtCore/QHash>
#include <QtCore/QList>
#include <QtCore/QPair>
#include <QtCore/QString>

namespace Config {

class Schema;

/*!
    @class Config::Settings
    @internal

    Holds the values that a Config::Parser read, and nothing of how it read
    them; hence a copy is cheap (both members are implicitly shared), and a
    caller keeps one as a layer, such as the settings before a scope of a
    Config::LayeredParser, to change a copy of it, or to go back to it.
*/
class Settings
{
public:
    QString value(const Schema & schema, const QString & name) const;

    /*!
        Gives each setting (`<section>.<key>`, lower-cased) the value that
        the latest file or assignment set.
    */
    QHash<QString, QString > values;

    /*!
        Gives each open section (lower-cased, see Config::Section::setOpen())
        its keys, with their values, in the order they came, a key given
        twice included.
    */
    QHash<QString, QList<QPair<QString, QString > > > openEntries;
};

} // Namespace `Config`.

#endif // CONFIG_SETTINGS_H
