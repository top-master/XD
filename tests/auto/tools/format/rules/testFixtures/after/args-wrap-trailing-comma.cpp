// Copyright (C) 2015 The XD Company Ltd.
// SPDX-License-Identifier: Apache-2.0

int rankOf(Token token)
{
    static const Token ranks[][4] = {
        { OR, OR, OR, OR },
        { STAR, SLASH, PERCENT, PERCENT },
    };
    for (int r = 0; r < 2; ++r) {
        if (ranks[r][0] == token) {
            return r;
        }
    }
    return -1;
}
