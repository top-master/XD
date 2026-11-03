// Copyright (C) 2015 The XD Company Ltd.
// SPDX-License-Identifier: Apache-2.0

void run(Document *document, int open, int close)
{
    const QByteArray text = sourceText(
        document,
        open + 1,
        close - 1
    );
}
