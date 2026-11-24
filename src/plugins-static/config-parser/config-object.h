// Copyright (C) 2015 The XD Company Ltd.
// SPDX-License-Identifier: Apache-2.0

#ifndef CONFIG_OBJECT_H
#define CONFIG_OBJECT_H

#include "./config-evaluator-context.h"
#include "./config-schema.h"

#include <QtCore/QObject>
#include <QtCore/QSharedPointer>
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
    virtual bool hasDynamicValue(const QByteArray & name) const;
    static const Evaluator::Context *setContext(
        const Evaluator::Context *context
    );

};

class CfgObject : public Object
{
public:
    explicit CfgObject(const QRef<const Schema> & schema);
    QVariant dynamicValue(const QByteArray & name) const Q_DECL_OVERRIDE;
    bool hasDynamicValue(const QByteArray & name) const Q_DECL_OVERRIDE;

private:
    QRef<const Schema> m_schema;
    QString m_section;
};

class GlobalObject : public Object
{
    Q_OBJECT
    Q_PROPERTY(QObject *cfg READ cfg)
public:
    explicit GlobalObject(const QRef<const Schema> & schema);
    QObject *cfg() const;

private:
    Object *m_settings;
};

} // Namespace `Config`.

#endif // CONFIG_OBJECT_H
