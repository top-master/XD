// Copyright (C) 2015 The XD Company Ltd.
// SPDX-License-Identifier: Apache-2.0

#ifndef CONFIG_VALUE_MATCHER_H
#define CONFIG_VALUE_MATCHER_H

#include <QtCore/QList>
#include <QtCore/QSharedPointer>
#include <QtCore/QVariant>
#include <QtCore/QString>
#include <QtCore/QStringList>

namespace Config {

namespace Evaluator { class Context; }

class ValueMatcher
{
public:
    virtual ~ValueMatcher();

    /*!
        Tells whether @a value is one this matcher takes; when it is, it gives
        @a matched the value to keep, such as a word in its own spelling.
    */
    virtual bool match(const QString & value, QString *matched) const = 0;
    virtual bool matchLiteral(const QString & value, QString *matched) const;

    /*!
        Builds the description of the values this matcher takes, for the
        error of a value no matcher of the key takes.
    */
    virtual QString describe() const = 0;

    virtual QString explain(const QString & value) const;
    virtual bool allowsCode() const;

    /*!
        Lists the words this matcher takes, which a value of the key may be
        on its own (see Config::Choice); a matcher that takes no word lists
        none, and the key's value is an expression or a value as it reads.
    */
    virtual QStringList enumKeys() const;

    /*!
        Tells whether @a program, which a key's condition compiled (see
        Config::Condition::compile()), uses a percent term, as in
        `spaceUsed > 40%`, which a matcher of words or of a yes-or-no
        rejects.
    */
    static bool usesPercents(const QStringList & program);

    virtual QVariant convert(const QVariant & resolved) const;
    virtual QVariant settingValue(const QString & value, const Evaluator::Context *context) const;
};

typedef QList<QSharedPointer<const ValueMatcher > > Matchers;

QSharedPointer<const ValueMatcher > forbidCode();

class Boolean : public ValueMatcher
{
public:
    static QSharedPointer<const ValueMatcher > create();
    bool match(const QString & value, QString *matched) const Q_DECL_OVERRIDE;
    QString describe() const Q_DECL_OVERRIDE;
    QVariant convert(const QVariant & resolved) const Q_DECL_OVERRIDE;
    QVariant settingValue(const QString & value, const Evaluator::Context *context) const Q_DECL_OVERRIDE;
};

} // Namespace `Config`.

#endif // CONFIG_VALUE_MATCHER_H
