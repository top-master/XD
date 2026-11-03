// Copyright (C) 2015 The XD Company Ltd.
// SPDX-License-Identifier: Apache-2.0

void chooseActiveConfig()
{
    serviceConfig = QNetworkConfigurationManager().configurationFromIdentifier(
        QLL("dummy-service-network"));
}
