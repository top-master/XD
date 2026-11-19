// Copyright (C) 2015 The XD Company Ltd.
// SPDX-License-Identifier: Apache-2.0

#ifndef CONFIG_PARSER_H
#define CONFIG_PARSER_H

#include "./config-key.h"
#include "./config-schema.h"
#include "./config-settings.h"

#include <QtCore/QHash>
#include <QtCore/QByteArray>
#include <QtCore/QList>
#include <QtCore/QPair>
#include <QtCore/QString>
#include <QtCore/QStringList>

QT_BEGIN_NAMESPACE
class QIODevice;
QT_END_NAMESPACE

namespace Config {

class Lines
{
public:
    inline Lines() {}
    inline void append(const QByteArray &line) { m_lines.append(line); }
    void appendCompact(const QByteArray &line);
    inline Lines &operator<<(const QByteArray &line) { this->append(line); return *this; }
    inline void clear() { m_lines.clear(); }
    QByteArray text() const;

private:
    friend class LayeredParser;
    QList<QByteArray > m_lines;
};

class Parser
{
public:
    explicit Parser(const Schema & schema);
    virtual ~Parser();

    bool readFile(const QString & file, QString *error);
    bool readData(
            const QByteArray &data,
            const QString &origin,
            QString *error);
    bool readData(QIODevice *device, const QString &origin, QString *error);
    bool readLevels(const QString & path, const QString & fileName,
                    QString *error);
    bool assign(const QString & assignment, QString *error);
    QString value(const QString & name) const;
    inline bool hasValue(const QString &name) const
    { return m_settings.values.contains(name.toLower()); }
    inline const Schema &schema() const { return m_schema; }
    inline const Settings &settings() const { return m_settings; }
    QList<QPair<QString, QString > > openEntries(const QString & section) const;

protected:
    virtual void store(const QString & name, const QString & section,
                       const QString & entry, const QString & value);
    static void storeInto(Settings *settings, const QString & name,
                          const QString & section, const QString & entry,
                          const QString & value);

    /// Gives the values that count, which every read changes.
    Settings m_settings;

private:
    bool readLine(const QByteArray & line, QString *section,
                  const QString & origin, QString *error);
    bool set(const QString & name, const QString & shown, const QString & value,
             QString *error);
    static bool matchValue(const Key & key, const QString & value,
                           QString *matched);

    Schema m_schema;
};

class LayeredParser : public Parser
{
    typedef Parser super;
public:
    explicit LayeredParser(const Parser &base);

    bool addLines(const Lines &lines, const QString &origin,
                  QString *error);
    bool pushLines(const Lines &lines, const QString &origin,
                   QString *error);
    bool popLines(QString *error);

protected:
    void store(const QString & name, const QString & section,
               const QString & entry, const QString & value) Q_DECL_OVERRIDE;

private:
    bool apply(const Lines &lines, const QString &origin, QString *error);

    /// Lets each layer below the innermost one (the permanent one first,
    /// then each outer scope) take the reads aimed at it; Parser's own
    /// settings act as the innermost layer.
    QList<Settings > m_layers;
    /// Tells the first layer that a read changes (an index into m_layers,
    /// where its size stands for the innermost layer).
    int m_layerBegin;
    /// Tells the layer past the last one that a read changes.
    int m_layerEnd;
};

} // Namespace `Config`.

#endif // CONFIG_PARSER_H
