// Copyright (C) 2015 The XD Company Ltd.
// SPDX-License-Identifier: Apache-2.0

#ifndef RULES_TEXT_REPORT_H
#define RULES_TEXT_REPORT_H

#include "../format-named-rule.h"

#include <QtCore/QRegularExpression>
#include <QtCore/QString>

class FormatTextReportRule : public FormatNamedRule
{
    typedef FormatNamedRule super;
public:
    inline FormatTextReportRule(const char *name, const char *summary, const char *pattern,
                                const char *message)
        : super(name, summary), m_pattern(QString::fromUtf8(pattern))
        , m_message(QString::fromLatin1(message))
    {
    }

    void run(FormatDocument *document) const Q_DECL_OVERRIDE;

private:
    QRegularExpression m_pattern;
    QString m_message;
};

#endif // RULES_TEXT_REPORT_H
