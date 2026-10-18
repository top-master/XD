// Copyright (C) 2015 The XD Company Ltd.
// SPDX-License-Identifier: Apache-2.0

#ifndef RULES_COMMENT_WIDTH_H
#define RULES_COMMENT_WIDTH_H

#include "../format-settings.h"
#include "../lang/comment-line.h"
#include "./comment.h"

#include <QtCore/QByteArray>
#include <QtCore/QList>
#include <QtCore/QString>
#include <QtCore/QStringList>

class FormatCommentWidthRule : public FormatCommentRule
{
    typedef FormatCommentRule super;
public:
    inline FormatCommentWidthRule()
        : super("comment-width",
                "Wraps comment lines past the column limit at the break nearest the wrap column.")
    {
    }

    static QStringList wrap(const QString & text, int firstWidth, int nextWidth, int column,
                            const FormatSettings & settings);
    static bool fits(const QString & line, const FormatSettings & settings);

protected:
    void runGroup(FormatDocument *document,
                  const QList<int > & group) const Q_DECL_OVERRIDE;

private:
    bool rewrap(FormatDocument *document, const QList<FormatCommentLine > & lines,
                int first, int count, const QByteArray & nextPrefix) const;
};

#endif // RULES_COMMENT_WIDTH_H
