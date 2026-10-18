// Copyright (C) 2015 The XD Company Ltd.
// SPDX-License-Identifier: Apache-2.0

#ifndef CONFIG_OBJECT_H
#define CONFIG_OBJECT_H

#include "./config-evaluator-context.h"

#include <QtCore/QObject>
#include <QtCore/QVariant>

namespace Config {

class Object : public QObject
{
    Q_OBJECT
    typedef QObject super;
public:
    explicit Object(QObject *parent = Q_NULLPTR);
    ~Object();

    const Evaluator::Context *context() const;
    virtual QVariant dynamicValue(const QByteArray & name) const;
    static const Evaluator::Context *setContext(
        const Evaluator::Context *context
    );
};

} // Namespace `Config`.

#endif // CONFIG_OBJECT_H
