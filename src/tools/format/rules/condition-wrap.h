// Copyright (C) 2015 The XD Company Ltd.
// SPDX-License-Identifier: Apache-2.0

#ifndef RULES_CONDITION_WRAP_H
#define RULES_CONDITION_WRAP_H

#include "../format-named-rule.h"

#include <QtCore/QByteArray>
#include <QtCore/QList>

class FormatConditionWrapRule : public FormatNamedRule
{
    typedef FormatNamedRule super;
public:
    inline FormatConditionWrapRule()
        : super("condition-wrap",
                "Breaks a long condition before each && and ||, by nesting level.")
    {
    }

    void run(FormatDocument *document) const Q_DECL_OVERRIDE;

private:
    QList<int > operators(const FormatDocument *document, int open,
                          int close) const;
    QByteArray group(
            const FormatDocument *document, int open, int close, int indent,
            int column, bool isCondition) const;
    QByteArray atom(
            const FormatDocument *document, int first, int last, int indent,
            int column, bool *isComplex) const;
};

#endif // RULES_CONDITION_WRAP_H
