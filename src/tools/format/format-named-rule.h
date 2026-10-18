// Copyright (C) 2015 The XD Company Ltd.
// SPDX-License-Identifier: Apache-2.0

#ifndef FORMAT_NAMED_RULE_H
#define FORMAT_NAMED_RULE_H

#include "./format-rule.h"

#include <QtCore/QByteArray>
#include <QtCore/QString>

class FormatNamedRule : public FormatRule
{
    typedef FormatRule super;
public:
    FormatNamedRule(const char *name, const char *summary);

    QByteArray name() const Q_DECL_OVERRIDE;
    QString summary() const Q_DECL_OVERRIDE;

private:
    QByteArray m_name;
    QString m_summary;
};

#endif // FORMAT_NAMED_RULE_H
