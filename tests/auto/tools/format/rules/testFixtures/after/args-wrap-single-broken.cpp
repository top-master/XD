// Copyright (C) 2015 The XD Company Ltd.
// SPDX-License-Identifier: Apache-2.0

QByteArray lastField(const QByteArray & record)
{
    {
        {
            const QByteArray field = record.mid(
                record.indexOf(
                    ':',
                    record.indexOf(
                        ':',
                        record.indexOf(':', record.indexOf(':') + 1)
                            + 1
                    ) + 1
                ) + 1
            );
            return field;
        }
    }
}
