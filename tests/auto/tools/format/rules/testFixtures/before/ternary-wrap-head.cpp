// Copyright (C) 2015 The XD Company Ltd.
// SPDX-License-Identifier: Apache-2.0

void pick(int type, const Item & configuration)
{
    QHash<QString, QNetworkConfigurationPrivatePointer > &table = type == QNetworkConfiguration::UserChoice ? userChoiceConfigurations : snapConfigurations;
    QHash<QString, QNetworkConfigurationPrivatePointer > &other = typeNameIsVeryVeryLongInThisCase == QNetworkConfiguration::UserChoice ? userChoiceConfigurations : snapConfigurations;
    SomethingShort &shortTable = typeNameIsVeryVeryLongInThisCase == QNetworkConfiguration::UserChoice ? userChoiceConfigurations : snapConfigurations;
    const bool usable = configuration.isValid() || configuration.state() == Discovered ? true : fallbackCheck(configuration);
}
