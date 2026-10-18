// Copyright (C) 2015 The XD Company Ltd.
// SPDX-License-Identifier: Apache-2.0

#ifndef CONFIG_WORD_H
#define CONFIG_WORD_H

#include "./config-value-matcher.h"

namespace Config {

class Word : public ValueMatcher
{
    typedef ValueMatcher super;
public:
    explicit Word(const QString & word);

    static QSharedPointer<const ValueMatcher > create(const QString & word);

    bool match(const QString & value, QString *matched) const Q_DECL_OVERRIDE;
    QString describe() const Q_DECL_OVERRIDE;

private:
    QString m_word;
};

} // namespace Config

#endif // CONFIG_WORD_H
