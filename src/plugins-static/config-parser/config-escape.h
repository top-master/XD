// Copyright (C) 2015 The XD Company Ltd.
// SPDX-License-Identifier: Apache-2.0

#ifndef CONFIG_ESCAPE_H
#define CONFIG_ESCAPE_H

#include <QtCore/QString>

namespace Config {

class Escape
{
public:
    static bool decode(const QString & text, QString *decoded, QString *error);
    static bool simple(char name, char *value);
    static void appendCodePoint(QString *text, uint codePoint);
};

} // Namespace `Config`.

#endif // CONFIG_ESCAPE_H
