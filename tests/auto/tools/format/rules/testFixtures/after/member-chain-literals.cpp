// Copyright (C) 2015 The XD Company Ltd.
// SPDX-License-Identifier: Apache-2.0

void report()
{
    log(
        context,
        location,
        "notice",
        String(Text("First fragment gives a brief introduction; "
            "second fragment adds some extra detail; "
            "third fragment completes the message.")).replace(item)
    );
}
