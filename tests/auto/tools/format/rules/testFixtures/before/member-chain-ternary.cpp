// Copyright (C) 2015 The XD Company Ltd.
// SPDX-License-Identifier: Apache-2.0

void report()
{
    log(
        "notice",
        String(Text("A fictional description of the %1 and %2 fields")) .replace(mode == "short-format" ? Text("compact-value") : Text("extended-value")) .replace(
            fieldName()
        )
    );
}
