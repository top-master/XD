// Copyright (C) 2015 The XD Company Ltd.
// SPDX-License-Identifier: Apache-2.0

#include "./config-choice.h"

/*!
    @class Config::Choice
    @internal

    Validates a choice between words selected by conditional expressions.
    Resolution evaluates only the condition and the branch it selects.
*/
Config::Choice::Choice(const QStringList &names, const QStringList &values)
    : m_names(names), m_values(values)
{
}

static QString stripChoiceParens(const QString &expression)
{
    QString text = expression.trimmed();
    while (text.startsWith(QLatin1Char('(')) && text.endsWith(QLatin1Char(')'))) {
        int depth = 0;
        bool encloses = true;
        for (int i = 0; i < text.size(); ++i) {
            if (text.at(i) == QLatin1Char('(')) {
                ++depth;
            } else if (text.at(i) == QLatin1Char(')')) {
                --depth;
                if (depth == 0 && i != text.size() - 1) {
                    encloses = false;
                    break;
                }
            }
        }
        if ( ! encloses || depth != 0) {
            break;
        }
        text = text.mid(1, text.size() - 2).trimmed();
    }
    return text;
}

static bool splitChoice(const QString &text, QString *condition,
                        QString *yes, QString *no)
{
    int parentheses = 0;
    int question = -1;
    int nested = 0;
    for (int i = 0; i < text.size(); ++i) {
        const QChar c = text.at(i);
        if (c == QLatin1Char('(')) {
            ++parentheses;
        } else if (c == QLatin1Char(')')) {
            if (--parentheses < 0) {
                return false;
            }
        } else if (parentheses == 0 && c == QLatin1Char('?')) {
            if (question < 0) {
                question = i;
            } else {
                ++nested;
            }
        } else if (parentheses == 0 && c == QLatin1Char(':') && question >= 0) {
            if (nested > 0) {
                --nested;
            } else {
                *condition = text.left(question).trimmed();
                *yes = text.mid(question + 1, i - question - 1).trimmed();
                *no = text.mid(i + 1).trimmed();
                return ! condition->isEmpty() && ! yes->isEmpty() && ! no->isEmpty();
            }
        }
    }
    return false;
}

static bool choiceValid(const QString &text, const QStringList &names,
                        const QStringList &values)
{
    const QString expression = stripChoiceParens(text);
    if (values.contains(expression, Qt::CaseInsensitive)) {
        return true;
    }
    QString condition;
    QString yes;
    QString no;
    QStringList program;
    QString error;
    return splitChoice(expression, &condition, &yes, &no)
        && Config::Condition::compile(condition, names, &program, &error)
        && choiceValid(yes, names, values)
        && choiceValid(no, names, values);
}

/*!
    Validates @a value and preserves it in @a matched.
*/
bool Config::Choice::match(const QString &value, QString *matched) const
{
    if ( ! choiceValid(value, this->m_names, this->m_values)) {
        return false;
    }
    *matched = value.trimmed();
    return true;
}

/*!
    Describes the accepted words and conditional choices.
*/
QString Config::Choice::describe() const
{
    return QString(QLL("a choice of %1, optionally selected by a condition"))
        .arg(this->m_values.join(QString(QLL(", "))));
}

/*!
    Resolves @a expression with @a context and stores its selected word in
    @a result. Reports an invalid expression in @a error.
*/
bool Config::Choice::resolve(const QString &expression, const QStringList &names,
                             const QStringList &values, const Condition::Context *context,
                             QString *result, QString *error)
{
    const QString text = stripChoiceParens(expression);
    for (int i = 0; i < values.size(); ++i) {
        if (values.at(i).compare(text, Qt::CaseInsensitive) == 0) {
            *result = values.at(i);
            return true;
        }
    }
    QString condition;
    QString yes;
    QString no;
    QStringList program;
    if ( ! splitChoice(text, &condition, &yes, &no)
        || ! Condition::compile(condition, names, &program, error)
    ) {
        *error = QString(QLL("invalid conditional choice: %1")).arg(text);
        return false;
    }
    bool ok = false;
    const bool selected = Condition::evaluate(program, QSet<QString >(),
                                               QHash<QString, qreal >(), context, &ok);
    if ( ! ok) {
        *error = QString(QLL("cannot evaluate condition: %1")).arg(condition);
        return false;
    }
    return resolve(selected ? yes : no, names, values, context, result, error);
}
