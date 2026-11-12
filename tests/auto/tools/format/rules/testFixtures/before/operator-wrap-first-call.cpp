// Copyright (C) 2015 The XD Company Ltd.
// SPDX-License-Identifier: Apache-2.0

void append()
{
    output << Packet(message.convertToInteger(), message.beginsWith(Text("sample")));
}

void appendMore()
{
    output << Packet(message.convertToInteger(), message.beginsWith(Text("sample"))) << finalItem;
}
