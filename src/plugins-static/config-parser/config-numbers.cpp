// Copyright (C) 2015 The XD Company Ltd.
// SPDX-License-Identifier: Apache-2.0

#include "./config-numbers.h"

#include <QtCore/QStringList>

#include <limits>

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
Config::Numbers::Numbers(qint64 minimum, qint64 maximum, int maximumCount)
    : m_minimum(minimum), m_maximum(maximum), m_maximumCount(maximumCount)
{
}

/*!
    Builds a shared matcher of up to @a maximumCount numbers (any count for
    0), each from @a minimum to @a maximum, for a key's matchers.
*/
QSharedPointer<const Config::ValueMatcher > Config::Numbers::create(
        qint64 minimum, qint64 maximum,
        int maximumCount)
{
    return QSharedPointer<const Config::ValueMatcher >(
            new Config::Numbers(minimum, maximum, maximumCount));
}

/*!
    Builds the numbers of @a value, in order; an empty list for a value with
    a part that is no whole number.
*/
QList<qint64 > Config::Numbers::parse(const QString & value)
{
    QList<qint64 > numbers;
    const QStringList parts = value.split(QLatin1Char('|'));
    for (int i = 0; i < parts.size(); ++i) {
        bool ok = false;
        const qint64 number = parts.at(i).trimmed().toLongLong(&ok);
        if ( ! ok) {
            return QList<qint64 >();
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
    const QList<qint64 > numbers = Config::Numbers::parse(value);
    if (numbers.isEmpty()
        || (this->m_maximumCount > 0 && numbers.size() > this->m_maximumCount)
    ) {
        *matched = QString();
        return false;
    }
    QStringList parts;
    for (int i = 0; i < numbers.size(); ++i) {
        if (numbers.at(i) < this->m_minimum
            || numbers.at(i) > this->m_maximum
        ) {
            *matched = QString();
            return false;
        }
        parts << QString::number(numbers.at(i));
    }
    *matched = parts.join(QString(QLL(" | ")));
    return true;
}

/*!
    Converts @a resolved, the number a key's program resolves to, such as
    the math `1 + 7 - 3`, into a whole number. Builds an invalid value for a
    word or a number outside the range this matcher takes.
*/
QVariant Config::Numbers::convert(const QVariant & resolved) const
{
    if (resolved.type() != QVariant::Double) {
        return QVariant();
    }
    const qreal number = resolved.toDouble();
    if (number < qreal(this->m_minimum) || number > qreal(this->m_maximum)) {
        return QVariant();
    }
    return QVariant(qint64(number));
}

/*!
    Builds the description of the numbers this matcher takes.
*/
QString Config::Numbers::describe() const
{
    if (this->m_maximumCount == 1) {
        return QString(QLL("a number %1")).arg(this->range());
    }
    if (this->m_maximumCount > 1) {
        return QString(QLL("up to %1 numbers %2, joined by |"))
            .arg(this->m_maximumCount)
            .arg(this->range());
    }
    return QString(QLL("numbers %1, joined by |")).arg(this->range());
}

/*!
    Builds the range of the numbers this matcher takes, as describe() names
    it: `from <minimum> to <maximum>`, or `of <minimum> or more` when only
    the size of a qint64 limits them.
*/
QString Config::Numbers::range() const
{
    if (this->m_maximum == std::numeric_limits<qint64 >::max()) {
        return QString(QLL("of %1 or more")).arg(this->m_minimum);
    }
    return QString(QLL("from %1 to %2")).arg(this->m_minimum)
        .arg(this->m_maximum);
}
