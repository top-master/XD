// Copyright (C) 2015 The XD Company Ltd.
// SPDX-License-Identifier: Apache-2.0

#ifndef PARSER_CONDITION_OBJECT_H
#define PARSER_CONDITION_OBJECT_H

#include "config-object.h"

#include <QtCore/QVariant>

class FormatConditionObject : public Config::Object
{
    Q_OBJECT
    typedef Config::Object super;
    Q_PROPERTY(QVariant spaceRemaining READ spaceRemaining)
    Q_PROPERTY(QVariant hasMoreArgs READ hasMoreArgs)
    Q_PROPERTY(QVariant isWrapping READ isWrapping)
public:
    QVariant spaceRemaining() const;
    QVariant hasMoreArgs() const;
    QVariant isWrapping() const;

public slots:
    QVariant argCount(int depth, int argument = 0);
    QVariant hasStd(const QString & standard);
};

#endif // PARSER_CONDITION_OBJECT_H
