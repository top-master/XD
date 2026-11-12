// Copyright (C) 2015 The XD Company Ltd.
// SPDX-License-Identifier: Apache-2.0

void announce()
{
    publish("note", Message(Text("An invented opening with several details; "
            "another fictional sentence adds information; "
            "the last sentence completes the message.")).substitute(value));
}
