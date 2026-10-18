// Copyright (C) 2015 The XD Company Ltd.
// SPDX-License-Identifier: Apache-2.0

#include "./config-numbers.h"

#include <QtCore/QStringList>

/*!
    @class Config::Numbers
    @internal

    Matches one whole number or more, joined by `|` (such as `75 | 51`), each
    within a range, and at most a given count of them (1 for a key that takes
    one number); a tool reads them in order, for instance as the runs of a
    task to try one after the other. It keeps them as `<n> | <n>`.
*/

/*!
    Builds the matcher of up to @a maximumCount numbers (any count for 0),
    each from @a minimum to @a maximum.
*/
Config::Numbers::Numbers(int minimum, int maximum, int maximumCount)
    : m_minimum(minimum), m_maximum(maximum), m_maximumCount(maximumCount)
{
}

/*!
    Builds a shared matcher of up to @a maximumCount numbers (any count for
    0), each from @a minimum to @a maximum, for a key's matchers.
*/
QSharedPointer<const Config::ValueMatcher > Config::Numbers::create(
        int minimum, int maximum,
        int maximumCount)
{
    return QSharedPointer<const Config::ValueMatcher >(
            new Config::Numbers(minimum, maximum, maximumCount));
}

/*!
    Builds the numbers of @a value, in order; an empty list for a value with
    a part that is no whole number.
*/
QList<int > Config::Numbers::parse(const QString & value)
{
    QList<int > numbers;
    const QStringList parts = value.split(QLatin1Char('|'));
    for (int i = 0; i < parts.size(); ++i) {
        bool ok = false;
        const int number = parts.at(i).trimmed().toInt(&ok);
        if ( ! ok) {
            return QList<int >();
        }
        numbers << number;
    }
    return numbers;
}

/*!
    Matches @a value when each of its parts is a number within the range,
    and it has no more parts than the count allows.
*/
bool Config::Numbers::match(const QString & value, QString *matched) const
{
    const QList<int > numbers = Config::Numbers::parse(value);
    if (numbers.isEmpty()
        || (this->m_maximumCount > 0 && numbers.size() > this->m_maximumCount)
    ) {
        return false;
    }
    QStringList parts;
    for (int i = 0; i < numbers.size(); ++i) {
        if (numbers.at(i) < this->m_minimum
            || numbers.at(i) > this->m_maximum
        ) {
            return false;
        }
        parts << QString::number(numbers.at(i));
    }
    *matched = parts.join(QString(QLL(" | ")));
    return true;
}

/*!
    Builds the description of the numbers this matcher takes.
*/
QString Config::Numbers::describe() const
{
    if (this->m_maximumCount == 1) {
        return QString(QLL("a number from %1 to %2")).arg(this->m_minimum).arg(this->m_maximum);
    }
    return QString(QLL("numbers from %1 to %2, joined by |")).arg(this->m_minimum)
        .arg(this->m_maximum);
}
