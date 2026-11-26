// Copyright (C) 2015 The XD Company Ltd.
// SPDX-License-Identifier: Apache-2.0

#ifndef RULES_DOC_TAG_SIGN_H
#define RULES_DOC_TAG_SIGN_H

#include "./comment.h"

#include <QtCore/QByteArray>
#include <QtCore/QList>

class FormatDocTagSignRule : public FormatCommentRule
{
    typedef FormatCommentRule super;
public:
    inline FormatDocTagSignRule()
        : super("doc-tag-sign",
                "Writes the commands of doc-comments with one sign.")
    {
    }

protected:
    void runGroup(FormatDocument *document,
                  const QList<int > & group) const Q_DECL_OVERRIDE;

private:
    QList<int > tags(const FormatDocument *document, int comment) const;
    char fileSign(const FormatDocument *document, char fallback) const;
};

#endif // RULES_DOC_TAG_SIGN_H
