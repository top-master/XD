// Copyright (C) 2015 The XD Company Ltd.
// SPDX-License-Identifier: Apache-2.0

#ifndef FORMAT_OPTIONS_H
#define FORMAT_OPTIONS_H

#include "./format-config.h"

#include <QtCore/QStringList>

/*!
    \internal

    Carries what the command line asks of every file: the git-blame style
    line ranges, the rules to run, the settings over the `.formatrc` files
    (see FormatConfig), whether to only check, whether an agent reads the
    requests the rules leave (`--ai`), and how many jobs may run at once (0
    picks one per CPU core).
*/
class FormatOptions
{
public:
    inline FormatOptions() : check(false), agent(false), jobs(0) {}

    QStringList lineSpecs;
    QStringList rules;
    FormatConfig config;
    bool check;
    bool agent;
    int jobs;
};

#endif // FORMAT_OPTIONS_H
