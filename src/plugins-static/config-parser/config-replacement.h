// Copyright (C) 2015 The XD Company Ltd.
// SPDX-License-Identifier: Apache-2.0

#ifndef CONFIG_REPLACEMENT_H
#define CONFIG_REPLACEMENT_H

#include <QtCore/QList>
#include <QtCore/QRegularExpression>
#include <QtCore/QString>
#include <QtCore/QStringList>

namespace Config {

class Replacement
{
public:
    inline Replacement() {}

    static bool parse(const QString & text, int groupCount,
                      Replacement *replacement);

    QString apply(const QRegularExpression & pattern,
                  const QString & subject) const;

private:
    QStringList m_literals;
    QList<int > m_groups;
};

} // namespace Config

#endif // CONFIG_REPLACEMENT_H
