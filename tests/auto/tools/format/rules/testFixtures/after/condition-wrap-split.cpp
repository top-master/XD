// Copyright (C) 2015 The XD Company Ltd.
// SPDX-License-Identifier: Apache-2.0

void poll(bool hasData, bool isBuffered, int socketType, Engine *engine)
{
    if (!hasData
        && !isBuffered
        && socketType == UdpSocketType
        && engine->isOpen()
        && engine->lastError() == ConnectionRefusedError
    ) {
        engine->reset();
    }
}
