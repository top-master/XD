// Copyright (C) 2015 The XD Company Ltd.
// SPDX-License-Identifier: Apache-2.0

#ifndef PARSER_COUNT_OBJECT_H
#define PARSER_COUNT_OBJECT_H

#include "config-object.h"

#include <QtCore/QVariant>

class FormatCountObject : public Config::Object
{
    Q_OBJECT
    typedef Config::Object super;
    Q_PROPERTY(QVariant value READ value)
public:

    QVariant value() const;
};

#endif // PARSER_COUNT_OBJECT_H
