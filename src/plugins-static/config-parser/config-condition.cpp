// Copyright (C) 2015 The XD Company Ltd.
// SPDX-License-Identifier: Apache-2.0

#include "./config-condition.h"

namespace {

static QString canonicalName(QString name)
{
    name = name.toLower();
    name.remove(QLatin1Char('_'));
    name.remove(QLatin1Char('-'));
    return name;
}

static bool isPredicateName(const QString &name)
{
    const QString lower = name.toLower();
    return lower.startsWith(QLL("is-")) || lower.startsWith(QLL("has-"))
        || lower.startsWith(QLL("already-"));
}

class ExpressionCompiler
{
public:
    ExpressionCompiler(const QString &text, const QStringList &names)
        : m_text(text), m_at(0), m_spellings(names)
    {
        for (int i = 0; i < names.size(); ++i) {
            m_names.insert(canonicalName(names.at(i)));
        }
        m_names << QString(QLL("value")) << QString(QLL("spaceremaining"));
    }

    bool compile(QStringList *program, QString *error)
    {
        m_program << QString(QLL("@expression"));
        if ( ! expression(0) || ! peek().isEmpty()) {
            *error = QString(QLL("invalid condition near '%1'"))
                .arg(m_text.mid(m_at, 20));
            return false;
        }
        *program = m_program;
        return true;
    }

private:
    QString peek()
    {
        while (m_at < m_text.size() && m_text.at(m_at).isSpace()) {
            ++m_at;
        }
        if (m_at == m_text.size()) {
            return QString();
        }
        const QChar c = m_text.at(m_at);
        if (c.isLetter() || c == QLatin1Char('_')) {
            // Preserve established hyphenated names, while leaving any
            // following minus sign to the expression parser.
            QString known;
            for (int i = 0; i < m_spellings.size(); ++i) {
                const QString name = m_spellings.at(i);
                const int end = m_at + name.size();
                if (isPredicateName(name) && name.size() > known.size()
                    && m_text.mid(m_at, name.size()).compare(
                        name,
                        Qt::CaseInsensitive
                    ) == 0
                    && (end == m_text.size() || ( ! m_text.at(end).isLetterOrNumber()
                        && m_text.at(end) != QLatin1Char('_')))
                ) {
                    known = m_text.mid(m_at, name.size());
                }
            }
            if ( ! known.isEmpty()) {
                return known;
            }
            int end = m_at + 1;
            while (end < m_text.size() && (m_text.at(end).isLetterOrNumber()
                    || m_text.at(end) == QLatin1Char('_'))
            ) {
                ++end;
            }
            const QString first = m_text.mid(m_at, end - m_at).toLower();
            if ((first == QLL("is") || first == QLL("has"))
                && end < m_text.size() && m_text.at(end) == QLatin1Char('-')
                && end + 1 < m_text.size() && m_text.at(end + 1).isLetter()
            ) {
                ++end;
                while (end < m_text.size() && (m_text.at(end).isLetterOrNumber()
                        || m_text.at(end) == QLatin1Char('_')
                        || m_text.at(end) == QLatin1Char('-'))
                ) {
                    ++end;
                }
            }
            return m_text.mid(m_at, end - m_at);
        }
        if (c.isDigit() || (c == QLatin1Char('.') && m_at + 1 < m_text.size()
                && m_text.at(m_at + 1).isDigit())
        ) {
            int end = m_at + 1;
            while (end < m_text.size() && (m_text.at(end).isDigit()
                    || m_text.at(end) == QLatin1Char('.'))
            ) {
                ++end;
            }
            if (end < m_text.size()
                && m_text.at(end) == QLatin1Char('%')
            ) {
                ++end;
            }
            return m_text.mid(m_at, end - m_at);
        }
        const QString pair = m_text.mid(m_at, 2);
        if (pair == QLL("&&") || pair == QLL("||") || pair == QLL("<=")
            || pair == QLL(">=") || pair == QLL("==") || pair == QLL("!=")
        ) {
            return pair;
        }
        return QString(c);
    }

    bool take(const QString &token)
    {
        if (peek() != token) {
            return false;
        }
        m_at += token.size();
        return true;
    }

    int precedence(const QString &token) const
    {
        if (token == QLL("||") || token == QLL("|")) {
            return 1;
        }
        if (token == QLL("&&") || token == QLL("&")) {
            return 2;
        }
        if (token == QLL("==") || token == QLL("!=")) {
            return 3;
        }
        if (token == QLL("<") || token == QLL(">")
            || token == QLL("<=") || token == QLL(">=")
        ) {
            return 4;
        }
        if (token == QLL("+") || token == QLL("-")) {
            return 5;
        }
        if (token == QLL("*") || token == QLL("/")) {
            return 6;
        }
        return 0;
    }

    bool expression(int minRank)
    {
        const QString token = peek();
        if (token == QLL("!") || token == QLL("-") || token == QLL("+")) {
            take(token);
            if ( ! expression(7)) {
                return false;
            }
            if (token != QLL("+")) {
                m_program << (token == QLL("-")
                    ? QString(QLL("op:neg")) : QString(QLL("op:!")));
            }
        } else if (take(QString(QLL("(")))) {
            if ( ! expression(0) || ! take(QString(QLL(")")))) {
                return false;
            }
        } else {
            if (token.isEmpty()) {
                return false;
            }
            take(token);
            bool isNumber = false;
            QString number = token;
            if (number.endsWith(QLatin1Char('%'))) {
                number.chop(1);
            }
            number.toDouble(&isNumber);
            if (isNumber) {
                m_program << (QString(token.endsWith(QLatin1Char('%'))
                    ? QLL("p:") : QLL("n:")) + number);
            } else {
                const QString name = canonicalName(token);
                if (name == QLL("argcount")) {
                    if ( ! take(QString(QLL("("))) || ! expression(0)) {
                        return false;
                    }
                    int count = 1;
                    if (take(QString(QLL(",")))) {
                        if ( ! expression(0)) {
                            return false;
                        }
                        ++count;
                    }
                    if ( ! take(QString(QLL(")")))) {
                        return false;
                    }
                    m_program << QString(QLL("f:argcount:%1")).arg(count);
                } else if (name == QLL("true") || name == QLL("false")
                    || m_names.contains(name)
                ) {
                    m_program << (QString(QLL("v:")) + name);
                } else {
                    return false;
                }
            }
        }
        while (true) {
            const QString op = peek();
            const int rank = precedence(op);
            if (rank == 0 || rank <= minRank) {
                break;
            }
            take(op);
            if ( ! expression(rank)) {
                return false;
            }
            m_program << (QString(QLL("op:")) + op);
        }
        return true;
    }

    QString m_text;
    int m_at;
    QSet<QString > m_names;
    QStringList m_spellings;
    QStringList m_program;
};

} // namespace

/*!
    @class Config::Condition
    @internal

    Matches a condition over a fixed set of names, such as
    `is-base-member | already-existing | (is-outline & is-header)`: names
    joined by `|` or `||` (or) and `&` or `&&` (and, which binds closer),
    with `!` for negation and parentheses for grouping. A name is any of
    the names the schema gives, in any case;
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
    Builds the matcher of a condition over the names @a names.
*/
Config::Condition::Condition(const QStringList & names, bool takesPercents)
    : m_names(names), m_takesPercents(takesPercents)
{
}

/*!
    Builds a key's shared matcher of a condition over the names @a names,
    which also takes percent terms when @a takesPercents is set.
*/
QSharedPointer<const Config::ValueMatcher > Config::Condition::create(
        const QStringList & names, bool takesPercents)
{
    return QSharedPointer<const Config::ValueMatcher >(
            new Config::Condition(names, takesPercents));
}

/*!
    Matches @a value when it is a condition over the names; @a matched gets
    it with each name in the schema's spelling, and with a space on each
    side of every sign.
*/
bool Config::Condition::match(const QString & value, QString *matched) const
{
    QString normalized = value;
    normalized.replace(QLL("&&"), QLL("&"));
    normalized.replace(QLL("||"), QLL("|"));
    QStringList program;
    QString error;
    if ( ! Config::Condition::compile(
            normalized, this->m_names, &program, &error,
            this->m_takesPercents
        )
    ) {
        return false;
    }

    // Spells the condition the way the schema does.
    QString text;
    for (int i = 0; i < normalized.size(); ++i) {
        const QChar c = normalized.at(i);
        if (c == QLatin1Char('|') || c == QLatin1Char('&')) {
            text += QLatin1Char(' ') + c + QLatin1Char(' ');
        } else if ( ! c.isSpace()) {
            text += c.toLower();
        }
    }
    *matched = text.simplified().replace(QLL("( "), QLL("(")).replace(
        QLL(" )"),
        QLL(")")
    );
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
    Compiles the condition @a text over the names @a names, and over percent
    terms when @a takesPercents is set, into @a program, a list of operands
    and signs in the order evaluate() runs them (each sign after its
    operands); returns false, with @a error, for a text that is no such
    condition, or that uses a name outside @a names.
*/
bool Config::Condition::compile(const QString & input, const QStringList & names,
                                QStringList *program,
                                QString *error,
                                bool takesPercents)
{
    QString minusCheck = input;
    for (int i = 0; i < names.size(); ++i) {
        if (isPredicateName(names.at(i))) {
            minusCheck.replace(
                names.at(i),
                canonicalName(names.at(i)),
                Qt::CaseInsensitive
            );
        }
    }
    if (input.contains(QLL("argcount"), Qt::CaseInsensitive)
        || input.contains(QLL("value"), Qt::CaseInsensitive)
        || input.contains(QLL("spaceremaining"), Qt::CaseInsensitive)
        || input.contains(QLL("space_remaining"), Qt::CaseInsensitive)
        || input.contains(QLatin1Char('<')) || input.contains(QLatin1Char('>'))
        || input.contains(QLatin1Char('=')) || input.contains(QLatin1Char('+'))
        || input.contains(QLatin1Char('*')) || input.contains(QLatin1Char('/'))
        || minusCheck.contains(QLatin1Char('-'))
    ) {
        ExpressionCompiler compiler(input, names);
        return compiler.compile(program, error);
    }
    QString text = input;
    text.replace(QLL("&&"), QLL("&"));
    text.replace(QLL("||"), QLL("|"));
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
            for (int n = 0; n < names.size(); ++n) {
                if (canonicalName(name) == canonicalName(names.at(n))) {
                    name = names.at(n).toLower();
                    break;
                }
            }
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
            } else if (name != QLL("true") && name != QLL("false")
                       && ! names.contains(name)) {
                *error = QString(QLL("unknown name %1")).arg(name);
                return false;
            }
            *program << name;
            expectOperand = false;
            i = end;
            continue;
        }

        // Reads a sign or a parenthesis; `&` binds closer than `|`.
        if (c == QLatin1Char('!')) {
            if ( ! expectOperand) {
                *error = QString(QLL("a sign is missing before !"));
                return false;
            }
            signs << QString(c);
        } else if (c == QLatin1Char('(')) {
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
                && (signs.last() == QLL("!") || signs.last() == QLL("&")
                    || c == QLatin1Char('|'))
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
    Tells whether the condition that compile() made @a program of holds,
    given the names @a holding that hold; a percent term, which needs a
    value, logs a warning and makes the condition fail.
*/
bool Config::Condition::evaluate(
        const QStringList & program,
        const QSet<QString > & holding)
{
    return Config::Condition::run(program, holding, Q_NULLPTR, Q_NULLPTR);
}

/*!
    Tells, like the evaluate() above, whether @a program holds given the
    names @a holding; @a ok tells whether every term had what it needs.
*/
bool Config::Condition::evaluate(
        const QStringList & program,
        const QSet<QString > & holding, bool *ok)
{
    return Config::Condition::run(program, holding, Q_NULLPTR, ok);
}

/*!
    Tells whether the condition that compile() made @a program of holds,
    given the names @a holding that hold, and the value @a percent (from 0
    to 100, or past it) that its percent terms compare with: a term such as
    `40%` holds when @a percent is more than 40. @a ok, when given, tells
    whether every term had what it needs.
*/
bool Config::Condition::evaluate(
        const QStringList & program,
        const QSet<QString > & holding, qreal percent,
        bool *ok)
{
    return Config::Condition::run(program, holding, &percent, ok);
}

/*!
    Runs @a program (see evaluate()) with the names @a holding, and with the
    value @a percent when it is not null; without that value, a percent term
    logs a warning, clears @a ok when given, and makes the condition fail.
*/
bool Config::Condition::run(
        const QStringList & program, const QSet<QString > & holding,
        const qreal *percent, bool *ok)
{
    if ( ! program.isEmpty() && program.first() == QLL("@expression")) {
        QHash<QString, qreal > values;
        if (percent) {
            values.insert(QString(QLL("spaceremaining")), 100 - *percent);
            values.insert(QString(QLL("percent")), *percent);
        }
        return Config::Condition::evaluate(
            program,
            holding,
            values,
            Q_NULLPTR,
            ok
        );
    }
    if (ok) {
        *ok = true;
    }
    QList<bool > stack;
    for (int i = 0; i < program.size(); ++i) {
        const QString & item = program.at(i);
        if (item == QLL("!") && ! stack.isEmpty()) {
            const bool operand = stack.takeLast();
            stack << ! operand;
        } else if ((item == QLL("|") || item == QLL("&")) && stack.size() >= 2) {
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
        } else if (item == QLL("true")) {
            stack << true;
        } else if (item == QLL("false")) {
            stack << false;
        } else {
            stack << holding.contains(item);
        }
    }
    return ! stack.isEmpty() && stack.last();
}

/*!
    Evaluates a numeric condition with the variables and function context of
    the target. Sets @a ok to false when a required value is unavailable.
*/
bool Config::Condition::evaluate(
        const QStringList &program, const QSet<QString > &holding,
        const QHash<QString, qreal > &values, const Context *context, bool *ok)
{
    if (ok) {
        *ok = true;
    }
    if (program.isEmpty() || program.first() != QLL("@expression")) {
        return Config::Condition::run(program, holding, Q_NULLPTR, ok);
    }
    QSet<QString > names;
    for (QSet<QString >::const_iterator i = holding.constBegin(); i != holding.constEnd(); ++i) {
        names.insert(canonicalName(*i));
    }
    QHash<QString, qreal > normalized;
    for (QHash<QString, qreal >::const_iterator i = values.constBegin(); i != values.constEnd(); ++i) {
        normalized.insert(canonicalName(i.key()), i.value());
    }
    struct Operand {
        qreal number;
        bool percent;
        Operand(qreal value = 0, bool isPercent = false)
            : number(value), percent(isPercent) {}
    };
    QList<Operand > stack;
    for (int i = 1; i < program.size(); ++i) {
        const QString item = program.at(i);
        if (item.startsWith(QLL("n:")) || item.startsWith(QLL("p:"))) {
            stack
                << Operand(item.mid(2).toDouble(), item.startsWith(QLL("p:")));
        } else if (item.startsWith(QLL("v:"))) {
            const QString name = item.mid(2);
            if (name == QLL("true")) {
                stack << Operand(1);
            } else if (name == QLL("false")) {
                stack << Operand(0);
            } else if (normalized.contains(name)) {
                stack << Operand(normalized.value(name));
            } else if (name == QLL("value")
                || name == QLL("spaceremaining")
            ) {
                if (ok) {
                    *ok = false;
                }
                return false;
            } else {
                stack << Operand(names.contains(name) ? 1 : 0);
            }
        } else if (item.startsWith(QLL("f:argcount:"))) {
            const int count = item.mid(11).toInt();
            if ( ! context
                || stack.size() < count
                || count < 1
                || count > 2
            ) {
                if (ok) {
                    *ok = false;
                }
                return false;
            }
            const int argument = count == 2 ? int(stack.takeLast().number) : 0;
            const int depth = int(stack.takeLast().number);
            stack << Operand(context->argumentCount(depth, argument));
        } else if (item.startsWith(QLL("op:"))) {
            const QString op = item.mid(3);
            if (stack.isEmpty()) {
                if (ok) {
                    *ok = false;
                }
                return false;
            }
            Operand right = stack.takeLast();
            if (op == QLL("!")) {
                if (right.percent
                    && ! normalized.contains(QString(QLL("percent")))
                ) {
                    if (ok) {
                        *ok = false;
                    }
                    return false;
                }
                const bool value = right.percent
                    ? normalized.value(QString(QLL("percent"))) > right.number
                    : right.number != 0;
                stack << Operand( ! value);
                continue;
            }
            if (op == QLL("neg")) {
                stack << Operand(-right.number, right.percent);
                continue;
            }
            if (stack.isEmpty()) {
                if (ok) {
                    *ok = false;
                }
                return false;
            }
            Operand left = stack.takeLast();
            if (op == QLL("&")
                || op == QLL("&&")
                || op == QLL("|")
                || op == QLL("||")
            ) {
                if ((left.percent || right.percent)
                    && ! normalized.contains(QString(QLL("percent")))
                ) {
                    if (ok) {
                        *ok = false;
                    }
                    return false;
                }
                const qreal percent = normalized.value(QString(QLL("percent")));
                const bool l = left.percent ? percent > left.number
                                            : left.number != 0;
                const bool r = right.percent ? percent > right.number
                                             : right.number != 0;
                stack
                    << Operand(op == QLL("&") || op == QLL("&&") ? l && r
                                                                 : l || r
                    );
            } else if (op == QLL("+")) {
                stack << Operand(left.number + right.number);
            } else if (op == QLL("-")) {
                stack << Operand(left.number - right.number);
            } else if (op == QLL("*")) {
                stack << Operand(left.number * right.number);
            } else if (op == QLL("/") && right.number != 0) {
                stack << Operand(left.number / right.number);
            } else if (op == QLL("<")) {
                stack << Operand(left.number < right.number);
            } else if (op == QLL("<=")) {
                stack << Operand(left.number <= right.number);
            } else if (op == QLL(">")) {
                stack << Operand(left.number > right.number);
            } else if (op == QLL(">=")) {
                stack << Operand(left.number >= right.number);
            } else if (op == QLL("==")) {
                stack << Operand(left.number == right.number);
            } else if (op == QLL("!=")) {
                stack << Operand(left.number != right.number);
            } else {
                if (ok) {
                    *ok = false;
                }
                return false;
            }
        } else {
            if (ok) {
                *ok = false;
            }
            return false;
        }
    }
    if (stack.size() != 1) {
        if (ok) {
            *ok = false;
        }
        return false;
    }
    if (stack.first().percent) {
        if ( ! normalized.contains(QString(QLL("percent")))) {
            if (ok) {
                *ok = false;
            }
            return false;
        }
        return normalized.value(QString(QLL("percent"))) > stack.first().number;
    }
    return stack.first().number != 0;
}
