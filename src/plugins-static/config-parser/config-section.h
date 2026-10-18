// Copyright (C) 2015 The XD Company Ltd.
// SPDX-License-Identifier: Apache-2.0

#ifndef CONFIG_SECTION_H
#define CONFIG_SECTION_H

#include "./config-key.h"

#include <QtCore/QHash>
#include <QtCore/QList>
#include <QtCore/QString>
#include <QtCore/QStringList>

namespace Config {

class Section
{
public:
    inline Section() {}
    explicit Section(const QString & name);

    inline bool isNull() const { return m_name.isEmpty(); }
    inline const QString &name() const { return m_name; }
    inline const QList<Key > &keys() const { return m_keys; }
    void addKey(const QString & name, const Matchers & matchers,
                const QString & defaultValue,
                const QHash<QString, QString > & aliases = QHash<QString, QString >());
    void addKey(const QString & name, const QStringList & words, const QString & defaultValue,
                const QHash<QString, QString > & aliases = QHash<QString, QString >());
    void extend(const Section & base, const QStringList & names);

private:
    QString m_name;
    QList<Key > m_keys;
};

} // namespace Config

#endif // CONFIG_SECTION_H
