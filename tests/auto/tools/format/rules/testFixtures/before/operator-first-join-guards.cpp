// Copyright (C) 2015 The XD Company Ltd.
// SPDX-License-Identifier: Apache-2.0

Widget::Widget(int a,
               int b
)
    : m_a(a)
{
    if (isReady(a,
            b
        )
    )
        ++count;
    const int total = sum(a,
        b
    )
        + 1;
}
