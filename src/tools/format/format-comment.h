// Copyright (C) 2015 The XD Company Ltd.
// SPDX-License-Identifier: Apache-2.0

#ifndef FORMAT_COMMENT_H
#define FORMAT_COMMENT_H

class FormatComment
{
public:
    enum Kind { Line, LineDoc, Block, BlockDoc };

    inline FormatComment()
        : offset(0), length(0), kind(Line), trailing(false)
    {
    }

    inline bool isDoc() const { return kind == LineDoc || kind == BlockDoc; }
    inline bool isLine() const { return kind == Line || kind == LineDoc; }

    int offset;
    int length;
    Kind kind;
    bool trailing;
};

#endif // FORMAT_COMMENT_H
