// Copyright (C) 2015 The XD Company Ltd.
// SPDX-License-Identifier: Apache-2.0

void chooseActiveConfig()
{
    serviceConfig = QNetworkConfigurationManager()
        .configurationFromIdentifier(QLL("dummy-service-network"));
}

void chainedCalls()
{
    value = source.first().second()
        .configurationFromIdentifier(QLL("dummy-service-network"));
    const QString shortValue = source.first().second();
}
