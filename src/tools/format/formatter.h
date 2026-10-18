// Copyright (C) 2015 The XD Company Ltd.
// SPDX-License-Identifier: Apache-2.0

#ifndef FORMATTER_H
#define FORMATTER_H

#include "./format-config.h"
#include "./format-result.h"
#include "./format-rule.h"

#include <QtCore/QByteArray>
#include <QtCore/QList>
#include <QtCore/QString>
#include <QtCore/QStringList>

class Formatter
{
public:
    Formatter();
    ~Formatter();

    inline const QList<const FormatRule * > &selectedRules() const { return m_selected; }
    QList<const FormatRule * > rules() const;
    bool selectRules(const QStringList & names, QString *error);
    FormatResult format(const QByteArray & text, const QString & path,
                        const QStringList & lineSpecs,
                        const FormatConfig & config = FormatConfig(),
                        bool agentMode = false) const;

private:
    Q_DISABLE_COPY(Formatter)

    QList<FormatRule * > m_rules;
    QList<const FormatRule * > m_selected;
};

#endif // FORMATTER_H
