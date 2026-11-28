// Copyright (C) 2015 The XD Company Ltd.
// SPDX-License-Identifier: Apache-2.0

#ifndef PARSER_SETTINGS_OBJECT_H
#define PARSER_SETTINGS_OBJECT_H

#include "config-object.h"

#include <QtCore/QStringList>
#include <QtCore/QVariant>

class FormatConditionContext;

class FormatPercentResolver
{
public:

    static QVariant resolve(const QStringList & program,
                            const FormatConditionContext *place,
                            int percent);
};

#endif // PARSER_SETTINGS_OBJECT_H
