// Copyright (C) 2015 The XD Company Ltd.
// SPDX-License-Identifier: Apache-2.0

#ifndef RULES_PATTERN_REPORT_H
#define RULES_PATTERN_REPORT_H

#include "./comment.h"

#include <QtCore/QList>
#include <QtCore/QRegularExpression>
#include <QtCore/QString>

class FormatPatternReportRule : public FormatCommentRule
{
    typedef FormatCommentRule super;
public:
    inline FormatPatternReportRule(const char *name, const char *summary, const char *pattern,
                                   const char *message, bool docOnly = false)
        : super(name, summary)
        , m_pattern(QString::fromLatin1(pattern), QRegularExpression::CaseInsensitiveOption)
        , m_message(QString::fromLatin1(message)), m_docOnly(docOnly)
    {
    }

protected:
    void runGroup(FormatDocument *document,
                  const QList<int > & group) const Q_DECL_OVERRIDE;

private:
    QRegularExpression m_pattern;
    QString m_message;
    bool m_docOnly;
};

#endif // RULES_PATTERN_REPORT_H
