// Copyright (C) 2015 The XD Company Ltd.
// SPDX-License-Identifier: Apache-2.0

#ifndef RULES_ARGS_ARRAY_H
#define RULES_ARGS_ARRAY_H

#include "config-array-obj.h"

class FormatArgsArray : public Config::ArrayObj
{
    Q_OBJECT
    typedef Config::ArrayObj super;
public:
    explicit FormatArgsArray(
            const QStringList & arguments,
            QObject *parent = Q_NULLPTR);
};

#endif // RULES_ARGS_ARRAY_H
