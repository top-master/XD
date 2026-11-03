// Copyright (C) 2015 The XD Company Ltd.
// SPDX-License-Identifier: Apache-2.0

QByteArray lastWord(const QByteArray & text)
{
    {
        {
            const QByteArray word = text.mid(
                text.indexOf(
                    ':',
                    text.indexOf(
                        ':',
                        text.indexOf(':', text.indexOf(':') + 1) + 1
                    ) + 1
                ) + 1
            );
            return word;
        }
    }
}
