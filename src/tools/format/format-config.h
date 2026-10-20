// Copyright (C) 2015 The XD Company Ltd.
// SPDX-License-Identifier: Apache-2.0

#ifndef FORMAT_CONFIG_H
#define FORMAT_CONFIG_H

#include <QtCore/QString>
#include <QtCore/QStringList>

/*!
    \internal

    Carries the settings that the command line gives over the `.formatrc`
    files:

    * `file` (`--config-file`): the one settings file read in place of every
      `.formatrc`, when set;
    * `adds` (`--config-add`): settings files read over the `.formatrc`
      files, each overriding only the keys it sets;
    * `assignments` (`-c` and `--config`): `<section>.<key>=<value>`
      settings, read last;
    * `includePaths` (`-I` and `-i`): the folders, besides a file's own,
      that hold the headers its quoted `#include`s name, the way a
      compiler searches them (see FormatDocument::findInclude()).
*/
class FormatConfig
{
public:
    inline FormatConfig() {}

    QString file;
    QStringList adds;
    QStringList assignments;
    QStringList includePaths;
};

#endif // FORMAT_CONFIG_H
