// Copyright (C) 2015 The XD Company Ltd.
// SPDX-License-Identifier: Apache-2.0

void pick(bool ready)
{
    if (ready
        && typeNameIsVeryVeryLongInThisCase == QNetworkConfiguration::Active
    ) {
        use();
    }
    if (ready
        && typeNameIsVeryVeryLongInThisCase
            == QNetworkConfiguration::UserChoiceValueThatIsLonger
    ) {
        use();
    }
}
