// Copyright (C) 2015 The XD Company Ltd.
// SPDX-License-Identifier: Apache-2.0

#ifndef CONFIG_SCOPES_H
#define CONFIG_SCOPES_H

#include "./config-evaluator-context.h"
#include "./config-object.h"

#include <QtCore/QList>
#include <QtCore/QSharedPointer>
#include <QtCore/QString>
#include <QtCore/QStringList>
#include <QtCore/QVariant>

namespace Config {

namespace Scopes {

class Member
{
public:
    inline Member()
        : object(Q_NULLPTR), isProperty(false)
        , minimumArguments(0), maximumArguments(0)
    {
    }

    inline bool isNull() const { return ! object; }

    Object *object;
    QString name;
    bool isProperty;
    int minimumArguments;
    int maximumArguments;
};

class Scope
{
public:
    void add(Object *object);
    Member find(const QString & name) const;
    QStringList names() const;

private:
    QList<QSharedPointer<Object > > m_objects;
};

typedef void (*Setup)(Scope *scope);

Scope *global();
void addGlobalSetup(Setup setup);
QSharedPointer<Scope > push();
void popUntil(int scope);
int count();
Member find(const QString & name);
Member member(Object *object, const QString & name);
QStringList names();
bool answer(const Member & member, const Evaluator::Context *context,
            const QVariantList & arguments, QVariant *result);
bool call(const Member & member, const Evaluator::Context *context,
          const QVariantList & arguments, qreal *result);
bool invoke(Object *object, const QString & name,
            const QVariantList & arguments, QVariant *result);

} // Namespace `Scopes`.

} // Namespace `Config`.

#endif // CONFIG_SCOPES_H
