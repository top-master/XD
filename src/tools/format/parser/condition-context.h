// Copyright (C) 2015 The XD Company Ltd.
// SPDX-License-Identifier: Apache-2.0

#ifndef PARSER_CONDITION_CONTEXT_H
#define PARSER_CONDITION_CONTEXT_H

#include "config-scopes.h"

#include <QtCore/QSharedPointer>

class FormatArgsArray;
class FormatFileConfig;
class FormatDocument;

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
    virtual bool isTarget(const QString & kind, bool *value) const;
    virtual const FormatFileConfig *fileConfig() const;
    const Config::Settings *settings() const Q_DECL_OVERRIDE;
    QVariant resolveAtPercent(
            const QStringList & program, int percent) const Q_DECL_OVERRIDE;
    static void registerObjects();
    static void setGlobalObjects(Config::Scopes::Scope *scope);
    static void setCountObjects(Config::Scopes::Scope *scope);
    static QSharedPointer<Config::Scopes::Scope > pushCountScope();
};

class FormatReplaceTypesContext : public FormatConditionContext
{
    typedef FormatConditionContext super;
public:
    inline FormatReplaceTypesContext(
            const FormatDocument *document = Q_NULLPTR,
            int token = -1,
            const FormatFileConfig *config = Q_NULLPTR)
        : m_document(document), m_token(token), m_config(config) {}

    QStringList onNames() const Q_DECL_OVERRIDE;
    bool predicate(const QString & name, bool *value) const Q_DECL_OVERRIDE;
    bool argumentCount(int depth,
                       int argument,
                       qreal *count) const Q_DECL_OVERRIDE;
    bool spaceRemaining(qreal *percent) const Q_DECL_OVERRIDE;
    bool isTarget(const QString & kind, bool *value) const Q_DECL_OVERRIDE;
    inline const FormatFileConfig *fileConfig() const Q_DECL_OVERRIDE_IL
    { return this->m_config; }

private:
    const FormatDocument *m_document;
    int m_token;
    const FormatFileConfig *m_config;
};

#endif // PARSER_CONDITION_CONTEXT_H
