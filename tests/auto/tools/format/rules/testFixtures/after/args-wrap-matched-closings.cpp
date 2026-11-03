// Copyright (C) 2015 The XD Company Ltd.
// SPDX-License-Identifier: Apache-2.0

void certificate()
{
    build(
        QLL("subjectAltName=DNS:") + QtNetworkSettings::serverLocalName()
            + QLatin1Char('.') + QtNetworkSettings::serverDomainName()
    );
    build(
        QLL("subjectAltName=DNS:*.wildcard.dev.")
            + QtNetworkSettings::serverDomainName(),
        &wildcardCa()
    );
    build(
        values[index]
            + remainder
    );
    const Values values = Values{
        Value{first}
            + remainder
    };
}
