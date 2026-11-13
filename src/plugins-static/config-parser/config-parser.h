// Copyright (C) 2015 The XD Company Ltd.
// SPDX-License-Identifier: Apache-2.0

#ifndef CONFIG_PARSER_H
#define CONFIG_PARSER_H

#include "./config-key.h"
#include "./config-schema.h"

#include <QtCore/QHash>
#include <QtCore/QByteArray>
#include <QtCore/QList>
#include <QtCore/QString>

namespace Config {

class Parser
{
public:
    explicit Parser(const Schema & schema);

    bool readFile(const QString & file, QString *error);
    bool readData(const QByteArray &data, const QString &origin, QString *error);
    bool readLevels(const QString & path, const QString & fileName,
                    QString *error);
    bool assign(const QString & assignment, QString *error);
    QString value(const QString & name) const;
    inline const Schema &schema() const { return m_schema; }

private:
    bool set(const QString & name, const QString & shown, const QString & value,
             QString *error);
    static bool matchValue(const Key & key, const QString & value,
                           QString *matched);

    Schema m_schema;
    QHash<QString, QString > m_values;
};

class LayeredParser : public Parser
{
public:
    explicit LayeredParser(const Parser &base);

    bool addLines(const QList<QByteArray > &lines, const QString &origin,
                  QString *error);
    bool pushLines(const QList<QByteArray > &lines, const QString &origin,
                   QString *error);
    bool popLines(QString *error);

private:
    struct Layer
    {
        QList<QByteArray > lines;
        QString origin;
    };
    bool rebuild(QString *error);
    static bool apply(Parser *parser, const Layer &layer, QString *error);

    Parser m_base;
    QList<Layer > m_permanent;
    QList<Layer > m_scoped;
};

} // Namespace `Config`.

#endif // CONFIG_PARSER_H
