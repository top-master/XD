// Copyright (C) 2015 The XD Company Ltd.
// SPDX-License-Identifier: Apache-2.0

void run(Document *document, int open, int close)
{
    if (open > 0
        && close > open
    ) {
        apply(document,
              text);
    }
    const int total = open
        + close;
    const int side = open > close
        ? open
        : close;
    const QString message = QString(QLL("first part of the message, "
                                        "second part of it"));
    document->replace(document->lineStart(line), current.size(),
        indentation + suffix + prefix
    );
}
