// Copyright (C) 2015 The XD Company Ltd.
// SPDX-License-Identifier: Apache-2.0

void writeAll()
{
    const QString file = this->writeFile(
        QString(QLL("rewrite/a.cpp")),
        "void f()\n{\n    // The tool waits for the server to answer\n"
        "    // each request in turn.\n    g(first,\n      second);\n}\n"
    );
}
