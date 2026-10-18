// Copyright (C) 2015 The XD Company Ltd.
// SPDX-License-Identifier: Apache-2.0

#ifndef LANG_COMMENT_LINE_H
#define LANG_COMMENT_LINE_H

#include <QtCore/QByteArray>

/*!
    \internal

    Names one line of a comment's content: where its text starts in the
    document, and the text itself, without the slashes and stars that open,
    decorate, and close the comment, and without the spaces around them.
*/
class FormatCommentLine
{
public:
    inline FormatCommentLine() : offset(0) {}
    inline FormatCommentLine(int offset, const QByteArray & text)
        : offset(offset), text(text)
    {
    }

    int offset;
    QByteArray text;
};

#endif // LANG_COMMENT_LINE_H
