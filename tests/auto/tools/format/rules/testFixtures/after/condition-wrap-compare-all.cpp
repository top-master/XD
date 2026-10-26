// Copyright (C) 2015 The XD Company Ltd.
// SPDX-License-Identifier: Apache-2.0

bool isActive(const Item *item)
{
    for (int i = 0; i < 1; ++i) {
        {
            if ((item->value()->state & ItemConfiguration::Active)
                    == ItemConfiguration::Active
            ) {
                return true;
            }
        }
    }
    return false;
}
