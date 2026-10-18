// Copyright (C) 2015 The XD Company Ltd.
// SPDX-License-Identifier: Apache-2.0

#ifndef CONFIG_SCANNER_H
#define CONFIG_SCANNER_H

#include <QtCore/QString>
#include <QtCore/QStringList>

namespace Config {

class Scanner
{
public:
    static QString findRoot(const QString & path);
    static QStringList levelFolders(const QString & root, const QString & path);
    static QStringList levelFiles(const QString & path, const QString & fileName);
};

} // namespace Config

#endif // CONFIG_SCANNER_H
