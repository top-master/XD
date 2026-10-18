// Copyright (C) 2015 The XD Company Ltd.
// SPDX-License-Identifier: Apache-2.0

#ifndef CONFIG_SCHEMA_H
#define CONFIG_SCHEMA_H

#include "./config-section.h"

#include <QtCore/QList>
#include <QtCore/QString>

namespace Config {

class Schema
{
public:
    void addSection(const Section & section);
    Section section(const QString & name) const;
    inline const QList<Section > &sections() const { return m_sections; }
    Key key(const QString & name) const;

private:
    QList<Section > m_sections;
};

} // namespace Config

#endif // CONFIG_SCHEMA_H
