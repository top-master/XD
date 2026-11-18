// Copyright (C) 2015 The XD Company Ltd.
// SPDX-License-Identifier: Apache-2.0

#ifndef PARSER_SETTINGS_OBJECT_H
#define PARSER_SETTINGS_OBJECT_H

#include "config-object.h"

#include <QtCore/QStringList>
#include <QtCore/QVariant>

class FormatConditionContext;

class FormatSettingsObject : public Config::Object
{
    Q_OBJECT
    typedef Config::Object super;
public:
    explicit FormatSettingsObject(QObject *parent = Q_NULLPTR);

    static QStringList keys();
    QVariant dynamicValue(const QByteArray & name) const Q_DECL_OVERRIDE;

private:
    static QVariant threshold(
            const QByteArray & name,
            const QStringList & program,
            const FormatConditionContext *place);
};

#endif // PARSER_SETTINGS_OBJECT_H
