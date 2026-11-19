// Copyright (C) 2015 The XD Company Ltd.
// SPDX-License-Identifier: Apache-2.0

#include "./config-choice.h"


/*!
    @class Config::Choice
    @internal

    Validates a choice between words selected by conditional expressions,
    such as `hasStd("C++11") ? warn : ignore`, which Config::Condition
    compiles as a whole (see Condition::compile()); resolution runs
    only the condition and the branch it selects. A branch may spell its
    word in double quotes too, with the escapes of a C++ string literal
    (see Config::Escape), and may give a number instead, such as `2` or
    `cfg.joinArgs`. A choice without words takes any text in double quotes,
    and a bare name that nothing defines (no function of Config::Scopes, and
    no name of the condition), such as
    `hasCompiler("win32-msvc2010") ? Q_DECL_OVERRIDE_IL : override`; a name
    with signs, such as `win32-msvc2010`, needs the quotes.
*/
Config::Choice::Choice(const QStringList &names, const QStringList &values)
    : m_names(names), m_values(values)
{
}

/*!
    Validates @a value and preserves it in @a matched.
*/
bool Config::Choice::match(const QString &value, QString *matched) const
{
    QString error;
    bool mismatched = false;
    if ( ! Config::Choice::check(value, this->m_names, this->m_values,
                                 Condition::NameMismatchIgnore, &error,
                                 &mismatched)
    ) {
        return false;
    }
    *matched = value.trimmed();
    return true;
}

/*!
    Builds why the choice does not take @a value, such as a bare branch
    with signs after a name nothing defines, or a condition that does not
    compile (see Condition::compile()); empty when it takes it, or no
    reason is known.
*/
QString Config::Choice::explain(const QString & value) const
{
    QString error;
    bool mismatched = false;
    if (Config::Choice::check(value, this->m_names, this->m_values,
                              Condition::NameMismatchIgnore, &error,
                              &mismatched)
    ) {
        return QString();
    }
    return error;
}

/*!
    Converts @a answer, what the choice gives (see answer()), into @a value:
    a word the choice lists, in the list's spelling, or any text when it
    lists none, as a QString, or a number as a qreal. Returns false for any
    other answer, such as a word it does not list.
*/
bool Config::Choice::convert(const QVariant & answer, QVariant *value) const
{
    if (answer.type() == QVariant::Double) {
        *value = answer;
        return true;
    }
    const QString word = answer.toString();
    for (int i = 0; i < this->m_values.size(); ++i) {
        if (this->m_values.at(i).compare(word, Qt::CaseInsensitive) == 0) {
            *value = this->m_values.at(i);
            return true;
        }
    }
    if (answer.type() != QVariant::String || ! this->m_values.isEmpty()) {
        return false;
    }
    *value = word;
    return true;
}

/*!
    Describes the accepted words and conditional choices.
*/
QString Config::Choice::describe() const
{
    if (this->m_values.isEmpty()) {
        return QString(QLL("a text in double quotes, optionally selected by a "
            "condition"));
    }
    return QString(QLL("a choice of %1, optionally selected by a condition"))
        .arg(this->m_values.join(QString(QLL(", "))));
}

/*!
    Checks the conditions of @a expression, a choice over @a names and
    @a values, and handles a name that differs from a known one in case
    alone as @a mismatch says (see Condition::NameMismatch); returns false,
    with @a error, for a choice that does not compile, and sets
    @a mismatched when a name mismatch failed it.
*/
bool Config::Choice::check(const QString &expression, const QStringList &names,
                           const QStringList &values,
                           Condition::NameMismatch mismatch, QString *error,
                           bool *mismatched)
{
    QStringList program;
    return Condition::compile(expression, names, &program, error, false,
                              mismatch, mismatched, &values);
}

/*!
    Resolves @a expression with @a context and stores its selected word in
    @a result as text, a number as its digits (see answer()). Reports an
    invalid expression in @a error.
*/
bool Config::Choice::resolve(const QString &expression, const QStringList &names,
                             const QStringList &values, const Evaluator::Context *context,
                             QString *result, QString *error)
{
    QVariant answer;
    if ( ! Config::Choice::answer(expression, names, values, context, &answer,
                                  error)
    ) {
        return false;
    }
    *result = answer.toString();
    return true;
}

/*!
    Resolves @a expression with @a context into @a result: the word it picks
    as a QString, or the number it gives as a qreal (see
    Condition::answer()); hence the caller tells the two apart by the type.
    Reports an invalid expression in @a error.
*/
bool Config::Choice::answer(const QString &expression, const QStringList &names,
                            const QStringList &values,
                            const Evaluator::Context *context,
                            QVariant *result, QString *error)
{
    const QString text = expression.trimmed();
    QStringList program;
    QString reason;
    if ( ! Condition::compile(
        text,
        names,
        &program,
        &reason,
        false,
        Condition::NameMismatchIgnore,
        Q_NULLPTR,
        &values
    )) {
        *error = QString(QLL("invalid conditional choice: %1")).arg(text);
        if ( ! reason.isEmpty()) {
            *error += QLL(": ") + reason;
        }
        return false;
    }
    if ( ! Condition::answer(program, QSet<QString >(), context, result)) {
        *error = QString(QLL("cannot evaluate condition: %1")).arg(text);
        return false;
    }
    return true;
}
