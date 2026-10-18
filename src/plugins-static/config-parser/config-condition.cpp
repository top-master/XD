// Copyright (C) 2015 The XD Company Ltd.
// SPDX-License-Identifier: Apache-2.0

#include "./config-condition.h"

/*!
    \class Config::Condition
    \internal

    Matches a condition over a fixed set of names, such as
    `is-base-member | already-existing | (is-outline & is-header)`: names
    joined by `|` (or) and `&` (and, which binds closer), with parentheses
    for grouping. A name is any of the names the schema gives, in any case;
    spaces around names and signs do not count.

    A condition that takes percents also takes terms such as `40%` (from 0%
    to 100%), as in `(40% & is-non-header) | (30% & is-header)`. The schema
    only knows that such a term needs a value: each place that evaluates the
    condition sets the value, as a share of a whole the setting names (for
    instance a column, as a share of the widest column), and a term holds
    when that share is past the term's.

    compile() turns such a text into a program, and evaluate() runs that
    program against the names that hold at the place in question; hence a
    tool parses a setting once, and evaluates it as often as it needs.
*/

/*!
    Builds the matcher of a condition over the names \a names.
*/
Config::Condition::Condition(const QStringList & names, bool takesPercents)
    : m_names(names), m_takesPercents(takesPercents)
{
}

/*!
    Builds a key's shared matcher of a condition over the names \a names,
    which also takes percent terms when \a takesPercents is set.
*/
QSharedPointer<const Config::ValueMatcher > Config::Condition::create(
        const QStringList & names, bool takesPercents)
{
    return QSharedPointer<const Config::ValueMatcher >(
            new Config::Condition(names, takesPercents));
}

/*!
    Matches \a value when it is a condition over the names; \a matched gets
    it with each name in the schema's spelling, and with a space on each
    side of every sign.
*/
bool Config::Condition::match(const QString & value, QString *matched) const
{
    QStringList program;
    QString error;
    if ( ! Config::Condition::compile(value, this->m_names, &program, &error,
                                      this->m_takesPercents)
    ) {
        return false;
    }

    // Spells the condition the way the schema does.
    QString text;
    for (int i = 0; i < value.size(); ++i) {
        const QChar c = value.at(i);
        if (c == QLatin1Char('|') || c == QLatin1Char('&')) {
            text += QLatin1Char(' ') + c + QLatin1Char(' ');
        } else if ( ! c.isSpace()) {
            text += c.toLower();
        }
    }
    *matched = text.simplified().replace(QLL("( "), QLL("(")).replace(QLL(" )"), QLL(")"));
    return true;
}

/*!
    Builds the description of the values the matcher takes: a condition of
    its names, and of percents when it takes them, joined by `|` and `&`.
*/
QString Config::Condition::describe() const
{
    const QString names = this->m_names.join(QString(QLL(", ")));
    if (this->m_takesPercents) {
        return QString(QLL("a condition of %1, and percents from 0% to 100%, joined by | and &"))
            .arg(names);
    }
    return QString(QLL("a condition of %1, joined by | and &")).arg(names);
}

/*!
    Compiles the condition \a text over the names \a names, and over percent
    terms when \a takesPercents is set, into \a program, a list of operands
    and signs in the order evaluate() runs them (each sign after its
    operands); returns false, with \a error, for a text that is no such
    condition, or that uses a name outside \a names.
*/
bool Config::Condition::compile(const QString & text, const QStringList & names,
                                QStringList *program, QString *error, bool takesPercents)
{
    program->clear();
    QStringList signs;
    bool expectOperand = true;
    int i = 0;
    while (i < text.size()) {
        const QChar c = text.at(i);
        if (c.isSpace()) {
            ++i;
            continue;
        }

        // Reads a name.
        if (c.isLetterOrNumber()
            || c == QLatin1Char('-')
            || c == QLatin1Char('_')
        ) {
            int end = i;
            while (end < text.size() && (text.at(end).isLetterOrNumber()
                || text.at(end) == QLatin1Char('-')
                || text.at(end) == QLatin1Char('_'))
            ) {
                ++end;
            }
            QString name = text.mid(i, end - i).toLower();
            bool isNumber = false;
            const int percent = name.toInt(&isNumber);
            if (isNumber
                && end < text.size()
                && text.at(end) == QLatin1Char('%')
            ) {
                name += QLatin1Char('%');
                ++end;
            }
            if ( ! expectOperand) {
                *error = QString(QLL("a sign is missing before %1")).arg(name);
                return false;
            }
            if (name.endsWith(QLatin1Char('%'))) {
                if ( ! takesPercents) {
                    *error = QString(QLL("the key takes no percent, as in %1")).arg(name);
                    return false;
                }
                if (percent < 0 || percent > 100) {
                    *error = QString(QLL("%1 is not from 0% to 100%")).arg(name);
                    return false;
                }
            } else if ( ! names.contains(name)) {
                *error = QString(QLL("unknown name %1")).arg(name);
                return false;
            }
            *program << name;
            expectOperand = false;
            i = end;
            continue;
        }

        // Reads a sign or a parenthesis; `&` binds closer than `|`.
        if (c == QLatin1Char('(')) {
            if ( ! expectOperand) {
                *error = QString(QLL("a sign is missing before ("));
                return false;
            }
            signs << QString(c);
        } else if (c == QLatin1Char(')')) {
            if (expectOperand) {
                *error = QString(QLL("a name is missing before )"));
                return false;
            }
            while ( ! signs.isEmpty() && signs.last() != QLL("(")) {
                *program << signs.takeLast();
            }
            if (signs.isEmpty()) {
                *error = QString(QLL("a ( is missing"));
                return false;
            }
            signs.removeLast();
        } else if (c == QLatin1Char('|') || c == QLatin1Char('&')) {
            if (expectOperand) {
                *error = QString(QLL("a name is missing before %1")).arg(c);
                return false;
            }
            while ( ! signs.isEmpty() && signs.last() != QLL("(")
                && (signs.last() == QLL("&") || c == QLatin1Char('|'))
            ) {
                *program << signs.takeLast();
            }
            signs << QString(c);
            expectOperand = true;
        } else {
            *error = QString(QLL("unknown sign %1")).arg(c);
            return false;
        }
        ++i;
    }
    if (expectOperand) {
        *error = QString(QLL("a name is missing at the end"));
        return false;
    }
    while ( ! signs.isEmpty()) {
        if (signs.last() == QLL("(")) {
            *error = QString(QLL("a ) is missing"));
            return false;
        }
        *program << signs.takeLast();
    }
    return true;
}

/*!
    Tells whether the condition that compile() made \a program of holds,
    given the names \a holding that hold; a percent term, which needs a
    value, logs a warning and makes the condition fail.
*/
bool Config::Condition::evaluate(const QStringList & program,
                                 const QSet<QString > & holding)
{
    return Config::Condition::run(program, holding, Q_NULLPTR, Q_NULLPTR);
}

/*!
    Tells, like the evaluate() above, whether \a program holds given the
    names \a holding; \a ok tells whether every term had what it needs.
*/
bool Config::Condition::evaluate(const QStringList & program,
                                 const QSet<QString > & holding, bool *ok)
{
    return Config::Condition::run(program, holding, Q_NULLPTR, ok);
}

/*!
    Tells whether the condition that compile() made \a program of holds,
    given the names \a holding that hold, and the value \a percent (from 0
    to 100, or past it) that its percent terms compare with: a term such as
    `40%` holds when \a percent is more than 40. \a ok, when given, tells
    whether every term had what it needs.
*/
bool Config::Condition::evaluate(const QStringList & program,
                                 const QSet<QString > & holding, qreal percent,
                                 bool *ok)
{
    return Config::Condition::run(program, holding, &percent, ok);
}

/*!
    Runs \a program (see evaluate()) with the names \a holding, and with the
    value \a percent when it is not null; without that value, a percent term
    logs a warning, clears \a ok when given, and makes the condition fail.
*/
bool Config::Condition::run(const QStringList & program, const QSet<QString > & holding,
                            const qreal *percent, bool *ok)
{
    if (ok) {
        *ok = true;
    }
    QList<bool > stack;
    for (int i = 0; i < program.size(); ++i) {
        const QString & item = program.at(i);
        if ((item == QLL("|") || item == QLL("&")) && stack.size() >= 2) {
            const bool right = stack.takeLast();
            const bool left = stack.takeLast();
            stack << (item == QLL("|") ? left || right : left && right);
        } else if (item.endsWith(QLatin1Char('%'))) {
            if ( ! percent) {
                qWarning("Config::Condition: no value is set for the term %s",
                         qPrintable(item));
                if (ok) {
                    *ok = false;
                }
                return false;
            }
            stack << (*percent > item.left(item.size() - 1).toInt());
        } else {
            stack << holding.contains(item);
        }
    }
    return ! stack.isEmpty() && stack.last();
}
