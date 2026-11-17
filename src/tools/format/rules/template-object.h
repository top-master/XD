// Copyright (C) 2015 The XD Company Ltd.
// SPDX-License-Identifier: Apache-2.0

#ifndef RULES_TEMPLATE_OBJECT_H
#define RULES_TEMPLATE_OBJECT_H

#include "config-object.h"

class FormatArgsArray;

class FormatTemplateObject : public Config::Object
{
    Q_OBJECT
    typedef Config::Object super;
public slots:
    FormatArgsArray *args() const;
};

#endif // RULES_TEMPLATE_OBJECT_H
