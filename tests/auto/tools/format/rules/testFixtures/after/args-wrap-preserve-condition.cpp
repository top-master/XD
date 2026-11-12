// Copyright (C) 2015 The XD Company Ltd.
// SPDX-License-Identifier: Apache-2.0

void prepare()
{
    if (ready()) {
        Spacing spacing = Settings::toSpacing(
            hasAlternate ? alternate : ordinaryValue,
            ordinaryValue
        );
    }
}
