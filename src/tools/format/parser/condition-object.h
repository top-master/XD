// Copyright (C) 2015 The XD Company Ltd.
// SPDX-License-Identifier: Apache-2.0

#ifndef PARSER_CONDITION_OBJECT_H
#define PARSER_CONDITION_OBJECT_H

#include "config-object.h"

#include <QtCore/QVariant>

class FormatConditionContext;
class FormatSettingsObject;

class FormatConditionObject : public Config::Object
{
    Q_OBJECT
    typedef Config::Object super;
    Q_PROPERTY(QVariant spaceRemaining READ spaceRemaining)
    Q_PROPERTY(QVariant hasMoreArgs READ hasMoreArgs)
    Q_PROPERTY(QVariant isWrapping READ isWrapping)
    Q_PROPERTY(QObject *cfg READ cfg)
public:
    FormatConditionObject();

    QObject *cfg() const;
    QVariant spaceRemaining() const;
    QVariant hasMoreArgs() const;
    QVariant isWrapping() const;

public slots:
    QVariant argCount(int depth, int argument = 0);
    QVariant hasStd(const QString & standard);
    QVariant hasCompiler(const QString & compiler);
    QVariant is(const QString & kind);

private:
    typedef bool (FormatConditionContext::*Question)(
            const QString & name, bool *value) const;

    QVariant ask(Question question, const QString & name) const;

    FormatSettingsObject *m_settings;
};

#endif // PARSER_CONDITION_OBJECT_H
