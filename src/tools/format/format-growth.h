// Copyright (C) 2015 The XD Company Ltd.
// SPDX-License-Identifier: Apache-2.0

#ifndef FORMAT_GROWTH_H
#define FORMAT_GROWTH_H

/*!
    \internal

    Names a change of a line's length that a rule queues: the token at
    \a index of the document, the column it starts at, and the characters
    \a delta it gains (or, when negative, loses) there.
*/
class FormatGrowth
{
public:
    inline FormatGrowth() : index(0), column(0), delta(0) {}
    inline FormatGrowth(int index, int column, int delta)
        : index(index), column(column), delta(delta)
    {
    }

    int index;
    int column;
    int delta;
};

#endif // FORMAT_GROWTH_H
