// Copyright (C) 2015 The XD Company Ltd.
// SPDX-License-Identifier: Apache-2.0

bool process(bool *success)
{
    if (invalid()) { if (success) *success = false; return false; }
    if (retry()) { if (enabled()) retryWork(); finish(); }
    return true;
}
