// Copyright (C) 2015 The XD Company Ltd.
// SPDX-License-Identifier: Apache-2.0

#ifndef RULES_PROSE_H
#define RULES_PROSE_H

#include "./comment.h"

#include <QtCore/QList>
#include <QtCore/QString>

class FormatProseRule : public FormatCommentRule
{
    typedef FormatCommentRule super;
public:
    inline FormatProseRule(const char *name, const char *summary,
                           QString (*map)(const QString &, const void *))
        : super(name, summary), m_map(map)
    {
    }

protected:
    void runGroup(FormatDocument *document,
                  const QList<int > & group) const Q_DECL_OVERRIDE;

private:
    QString (*m_map)(const QString &, const void *);
};

#endif // RULES_PROSE_H
