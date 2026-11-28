// Copyright (C) 2015 The XD Company Ltd.
// SPDX-License-Identifier: Apache-2.0

#ifndef PARSER_COMMENT_LAYOUT_H
#define PARSER_COMMENT_LAYOUT_H

#include "../format-comment.h"

#include <QtCore/QByteArray>
#include <QtCore/QList>

class FormatDocument;

struct AttachedComment
{
    enum Placement { OwnLine, EndOfLine, Inline };
    enum Role { Leading, Trailing };

    inline AttachedComment()
        : anchorToken(0), placement(Inline), role(Leading)
    {
    }

    FormatComment comment;
    int anchorToken;
    Placement placement;
    Role role;
};

QList<AttachedComment > attachComments(
        const FormatDocument *document,
        int offset,
        int length,
        int firstToken,
        int tokenCount);

QByteArray restoreCommentsInReplacement(
        const FormatDocument *document,
        int offset,
        int length,
        const QByteArray & requested);

#endif // PARSER_COMMENT_LAYOUT_H
