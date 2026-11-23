// Copyright (C) 2015 The XD Company Ltd.
// SPDX-License-Identifier: Apache-2.0

#ifndef CONFIG_SCHEMA_H
#define CONFIG_SCHEMA_H

#include "./config-section.h"
#include "./config-evaluator-context.h"

#include <QtCore/QList>
#include <QtCore/QString>

namespace Config {

class Schema
{
public:
    void addSection(const Section & section);
    inline void setNames(const QStringList & names) { m_names = names; }
    Section section(const QString & name) const;
    Section sectionOf(const QString & name) const;
    inline const QList<Section > &sections() const { return m_sections; }
    Key key(const QString & name) const;
    bool isExpression(const QString & name, const QString & text,
                      const Evaluator::CompileContext *context) const;
    QStringList compile(const QString & name, const QString & text,
                        const Evaluator::CompileContext *context) const;
    QVariant eval(const QString & name, const QString & text,
                  const Evaluator::Context *context) const;
    QVariant reference(const QString & name, const QString & text,
                       const Evaluator::Context *context) const;
    QVariant resolve(const QString & name, const QStringList & program,
                     const Evaluator::Context *context) const;
    QVariant convert(const QString & name, const QVariant & resolved) const;

private:
    QList<Section > m_sections;
    QStringList m_names;
};

} // Namespace `Config`.

#endif // CONFIG_SCHEMA_H
