// Copyright (C) 2015 The XD Company Ltd.
// SPDX-License-Identifier: Apache-2.0

void linkLocalIPv4()
{
    if (addresses.isEmpty()) {
        QSKIP("No IPv4 link local addresses; server-dummy adds one to the loopback only when it runs elevated.");
    }

    runSockets();
}
