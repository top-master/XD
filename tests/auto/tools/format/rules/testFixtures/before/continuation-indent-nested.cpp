// Copyright (C) 2015 The XD Company Ltd.
// SPDX-License-Identifier: Apache-2.0

bool pick(int alpha, int beta, bool gamma)
{
    if ((alpha
        == beta)
        && gamma
    ) {
        return true;
    }
    if ((alpha
        || beta)
        && gamma
    ) {
        return true;
    }
    if (alpha
        == beta
    ) {
        return true;
    }
    return false;
}
