// Copyright (C) 2015 The XD Company Ltd.
// SPDX-License-Identifier: Apache-2.0

#ifndef FORMAT_CONFIG_H
#define FORMAT_CONFIG_H

#include <QtCore/QString>
#include <QtCore/QStringList>
#include <QtCore/QHash>

/*!
    @internal

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
      compiler searches them (see FormatDocument::findInclude());
    * `scheduledFiles`, `fileLineSpecs`, and `lineSpecs`: the run's file set
      and selected lines, for decisions involving another file;
    * `snapshots` and `snapshotManifest`: the original files and the manifest
      that lets worker processes read the same originals.
*/
class FormatConfig
{
public:
    inline FormatConfig() : verbose(false) {}

    QString file;
    QStringList adds;
    QStringList assignments;
    bool verbose;
    QStringList includePaths;
    QStringList scheduledFiles;
    QHash<QString, QStringList > fileLineSpecs;
    QStringList lineSpecs;
    QHash<QString, QString > snapshots;
    QString snapshotManifest;
};

#endif // FORMAT_CONFIG_H
