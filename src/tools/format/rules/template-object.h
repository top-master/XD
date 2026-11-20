// Copyright (C) 2015 The XD Company Ltd.
// SPDX-License-Identifier: Apache-2.0

#ifndef RULES_TEMPLATE_OBJECT_H
#define RULES_TEMPLATE_OBJECT_H

#include "config-object.h"
#include "config-scopes.h"

#include <QtCore/QString>

class FormatArgsArray;

class FormatTemplateObject : public Config::Object
{
    Q_OBJECT
    typedef Config::Object super;
public slots:
    FormatArgsArray *args() const;
    bool isLiteral(const QVariant & value) const;
public:
    QVariant dynamicValue(const QByteArray & name) const Q_DECL_OVERRIDE;
    static QSharedPointer<Config::Scopes::Scope > argumentScope(const QString & text);
};

#endif // RULES_TEMPLATE_OBJECT_H
