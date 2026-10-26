// Copyright (C) 2015 The XD Company Ltd.
// SPDX-License-Identifier: Apache-2.0

int kindOf(const Config & publicConfig)
{
    if (publicConfig.isValid()) {
        return 0;
    } else if (publicConfig.type() == QNetworkConfiguration::ServiceNetwork) {
        return 1;
    }
    if (publicConfig.isValid()
        && publicConfig.type() == Network::Service
    ) {
        return 2;
    }
    return 3;
}
