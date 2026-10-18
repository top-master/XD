// Copyright (C) 2015 The XD Company Ltd.
// SPDX-License-Identifier: Apache-2.0

#ifndef CONFIG_PARSER_H
#define CONFIG_PARSER_H

#include "./config-key.h"
#include "./config-schema.h"

#include <QtCore/QHash>
#include <QtCore/QString>

namespace Config {

class Parser
{
public:
    explicit Parser(const Schema & schema);

    bool readFile(const QString & file, QString *error);
    bool readLevels(const QString & path, const QString & fileName, QString *error);
    bool assign(const QString & assignment, QString *error);
    QString value(const QString & name) const;

private:
    bool set(const QString & name, const QString & shown, const QString & value,
             QString *error);
    static bool matchValue(const Key & key, const QString & value, QString *matched);

    Schema m_schema;
    QHash<QString, QString > m_values;
};

} // namespace Config

#endif // CONFIG_PARSER_H
