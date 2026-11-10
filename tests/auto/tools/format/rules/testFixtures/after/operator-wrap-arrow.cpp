// Copyright (C) 2015 The XD Company Ltd.
// SPDX-License-Identifier: Apache-2.0

void chooseActiveConfig()
{
    serviceConfig = engineManager
        ->sessionManager()
        ->configurationFromIdentifier(QLL("dummy-service-network"));
}
