// Copyright (C) 2015 The XD Company Ltd.
// SPDX-License-Identifier: Apache-2.0

#ifndef CONFIG_SCHEMA_H
#define CONFIG_SCHEMA_H

#include "./config-section.h"
#include "./config-evaluator-context.h"

#include <QtCore/QList>
#include <QtCore/QString>
#include <QtCore/qglobal.h>

namespace Config {

class Schema
{
public:
    inline Schema() {}
    virtual ~Schema() {}
    inline explicit Schema(const Evaluator::CompileContext &context)
        : m_context(context) {}

    void addSection(const Section & section);
    inline void setNames(const QStringList & names)
    {
        m_names = names;
        m_context = Evaluator::CompileContext(names);
    }
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

    /*!
        Answers the program of the default of @a name, when it is code (see
        Config::Code), which it compiles over the context it took (see the
        constructor): the one whose names the conditions of the defaults name,
        and whose words, when it takes any, a choice may pick (see
        Config::Choice).
    */
    QStringList programFor(const QString & name) const;

private:
    Q_DISABLE_COPY(Schema)
    QList<Section > m_sections;
    QStringList m_names;
    /// The context that the schema's defaults compile over (see
    /// programFor()).
    Evaluator::CompileContext m_context;
};

} // Namespace `Config`.

#endif // CONFIG_SCHEMA_H
