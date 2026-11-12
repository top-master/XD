// Copyright (C) 2015 The XD Company Ltd.
// SPDX-License-Identifier: Apache-2.0

bool matches()
{
    return acceptsToken(next)
        || ((next == DOT || next == ARROW) && hasJoinedExpression(document, before))
        || document->token(before) == EQ;
}

bool alternate()
{
    return cachedResult || findAlternateRecord(firstCandidate, secondCandidate);
}

bool validate()
{
    return isAvailable && validateExtraordinarilyLongArguments(firstExcessivelyLongCandidateName, secondExcessivelyLongCandidateName);
}
