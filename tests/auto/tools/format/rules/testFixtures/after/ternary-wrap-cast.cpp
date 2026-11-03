// Copyright (C) 2015 The XD Company Ltd.
// SPDX-License-Identifier: Apache-2.0

int pick(int someVeryLongExpressionNameHere, int anotherQuiteLongOperand)
{
    const int value =
        static_cast<int>(someVeryLongExpressionNameHere) + anotherQuiteLongOperand
        ? yesValue : noValue;
    return value;
}
