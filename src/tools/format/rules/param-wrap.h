// Copyright (C) 2015 The XD Company Ltd.
// SPDX-License-Identifier: Apache-2.0

#ifndef RULES_PARAM_WRAP_H
#define RULES_PARAM_WRAP_H

#include "../format-named-rule.h"

#include <QtCore/QByteArray>
#include <QtCore/QList>

class FormatScopeConditions;

class FormatParamWrapRule : public FormatNamedRule
{
    typedef FormatNamedRule super;
public:
    inline FormatParamWrapRule()
        : super("param-wrap",
                "Wraps the parameters of a declaration line past the column limit.")
    {
    }

    void run(FormatDocument *document) const Q_DECL_OVERRIDE;

private:
    void regroup(FormatDocument *document,
                 FormatScopeConditions *conditions) const;
    QList<QByteArray > groupedLines(
            const QByteArray & prefix,
            const QList<QList<QByteArray > > & groups,
            const QByteArray & suffix, bool breaksAfterOpen,
            const QByteArray & breakPad, int limit,
            bool *fits) const;
    bool isDeclarationParen(const FormatDocument *document, int paren) const;
    QList<QByteArray > parameters(
            const FormatDocument *document, int open,
            int close) const;
    QList<QByteArray > fill(const QByteArray & first, const QByteArray & pad,
                            const QList<QByteArray > & items,
                            const QByteArray & suffix,
                            int limit) const;
};

#endif // RULES_PARAM_WRAP_H
