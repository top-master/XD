// Copyright (C) 2015 The XD Company Ltd.
// SPDX-License-Identifier: Apache-2.0

#ifndef PARSER_CONDITION_CONTEXT_H
#define PARSER_CONDITION_CONTEXT_H

#include "config-scopes.h"

#include <QtCore/QSharedPointer>

class FormatArgsArray;
class FormatFileConfig;

class FormatConditionContext : public Config::Evaluator::Context
{
    typedef Config::Evaluator::Context super;
public:
    virtual bool value(qreal *value) const;
    virtual bool argumentCount(int depth, int argument, qreal *count) const;
    virtual bool spaceRemaining(qreal *percent) const;
    virtual bool hasMoreArgs(bool *value) const;
    virtual bool isWrapping(bool *value) const;
    virtual FormatArgsArray *callArguments() const;
    virtual bool hasStandard(const QString & standard, bool *value) const;
    virtual bool hasCompiler(const QString & compiler, bool *value) const;
    virtual const FormatFileConfig *fileConfig() const;
    static void registerObjects();
    static void setGlobalObjects(Config::Scopes::Scope *scope);
    static void setCountObjects(Config::Scopes::Scope *scope);
    static QSharedPointer<Config::Scopes::Scope > pushCountScope();
};

#endif // PARSER_CONDITION_CONTEXT_H
