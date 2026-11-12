// Copyright (C) 2015 The XD Company Ltd.
// SPDX-License-Identifier: Apache-2.0

void choose()
{
    if (first) accept();
    else if (second) defer();
    else reject();
    if (ready) { start(); }
    else { stop(); }
    if (cached) { useCache(); }
    // Keep this explanation with the alternative.
    else { fetch(); }
}
