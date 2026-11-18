// Copyright (C) 2015 The XD Company Ltd.
// SPDX-License-Identifier: Apache-2.0

#include "./config-choice.h"

#include "./config-escape.h"
#include "./config-scopes.h"

#include <QtCore/QRegExp>

/*!
    @class Config::Choice
    @internal

    Validates a choice between words selected by conditional expressions.
    Resolution evaluates only the condition and the branch it selects. A
    branch may spell its word in double quotes too, with the escapes of a
    C++ string literal (see Config::Escape). A choice without words takes any
    such text, and a bare name that nothing defines (no function of
    Config::Scopes, and no name of the condition), such as
    `hasCompiler("win32-msvc2010") ? Q_DECL_OVERRIDE_IL : override`; a name
    with signs, such as `win32-msvc2010`, needs the quotes.
*/
Config::Choice::Choice(const QStringList &names, const QStringList &values)
    : m_names(names), m_values(values)
{
}

/*!
    Tells whether @a text is a bare name that nothing defines: neither a
    function of Config::Scopes, nor one of @a names, `true`, or `false`, in
    any case.
*/
static bool isUndefinedName(const QString &text, const QStringList &names)
{
    const QRegExp name(QString(QLL("(::)?[A-Za-z_][A-Za-z0-9_]*(::[A-Za-z_][A-Za-z0-9_]*)*")));
    if ( ! name.exactMatch(text)) {
        return false;
    }
    // Takes a name of another case as the name it differs from, just like
    // the condition's compile does.
    const QString canonical = Config::Condition::canonicalName(text);
    const QStringList known = QStringList(names)
        << QString(QLL("true")) << QString(QLL("false"));
    for (int i = 0; i < known.size(); ++i) {
        if (Config::Condition::canonicalName(known.at(i)).compare(
                canonical,
                Qt::CaseInsensitive
            ) == 0
        ) {
            return false;
        }
    }
    return Config::Scopes::find(canonical).isNull();
}

/*!
    Builds in @a error why the bare branch @a expression is no text: a sign
    that follows a name nothing defines, as in `win32-1`, which reads as
    math on that name. Leaves @a error alone for any other branch.
*/
static void explainSigns(const QString &expression, const QStringList &names,
                         QString *error)
{
    static const char *const verbs[][2] = {
        { "-", "subtract" }, { "+", "add" }, { "*", "multiply by" },
        { "/", "divide by" }
    };
    for (int i = 1; i < expression.size(); ++i) {
        for (int v = 0; v < 4; ++v) {
            if (expression.at(i) != QLatin1Char(verbs[v][0][0])) {
                continue;
            }
            const QString left = expression.left(i).trimmed();
            const QString right = expression.mid(i + 1).trimmed();
            if (isUndefinedName(left, names)) {
                *error = QString(QLL("can't %1 `%2` since `%3` is undefined."))
                    .arg(QLatin1String(verbs[v][1]), right, left);
            }
            return;
        }
    }
}

/*!
    Builds in @a word the branch @a expression picks among @a values: one of
    them, in any case, written bare or in double quotes (with its escapes
    resolved). When @a values is empty, it takes any text in double quotes,
    and any bare name that nothing defines (see isUndefinedName(), which
    @a names is the condition's own names for). Returns false for anything
    else, such as a condition.
*/
static bool branchWord(const QString &expression, const QStringList &values,
                       const QStringList &names, QString *word)
{
    QString text = expression;
    const bool isQuoted = expression.size() >= 2
        && expression.startsWith(QLatin1Char('"'))
        && expression.endsWith(QLatin1Char('"'));
    QString reason;
    if (isQuoted
        && ! Config::Escape::decode(
            expression.mid(1, expression.size() - 2),
            &text,
            &reason
        )
    ) {
        return false;
    }
    for (int i = 0; i < values.size(); ++i) {
        if (values.at(i).compare(text, Qt::CaseInsensitive) == 0) {
            *word = values.at(i);
            return true;
        }
    }
    if (values.isEmpty() && (isQuoted || isUndefinedName(text, names))) {
        *word = text;
        return true;
    }
    return false;
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
    bool quoted = false;
    for (int i = 0; i < text.size(); ++i) {
        const QChar c = text.at(i);

        // Skips a string, whose `(`, `?` or `:` is no sign of the choice.
        if (c == QLatin1Char('"')) {
            quoted = ! quoted;
            continue;
        }
        if (quoted) {
            if (c == QLatin1Char('\\')) {
                ++i;
            }
            continue;
        }
        // Skips a `::` of a name, such as `Foo::bar`, which no choice splits.
        if (c == QLatin1Char(':') && i + 1 < text.size()
            && text.at(i + 1) == QLatin1Char(':')
        ) {
            ++i;
            continue;
        }
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
                        const QStringList &values,
                        Config::Condition::NameMismatch mismatch,
                        QString *error, bool *mismatched)
{
    const QString expression = stripChoiceParens(text);
    QString word;
    if (branchWord(expression, values, names, &word)) {
        return true;
    }
    QString condition;
    QString yes;
    QString no;
    QStringList program;
    if ( ! splitChoice(expression, &condition, &yes, &no)) {
        if (values.isEmpty()) {
            explainSigns(expression, names, error);
        }
        return false;
    }
    return Config::Condition::compile(
        condition,
        names,
        &program,
        error,
        false,
        mismatch,
        mismatched
    )
        && choiceValid(yes, names, values, mismatch, error, mismatched)
        && choiceValid(no, names, values, mismatch, error, mismatched);
}

/*!
    Validates @a value and preserves it in @a matched.
*/
bool Config::Choice::match(const QString &value, QString *matched) const
{
    QString error;
    bool mismatched = false;
    if ( ! choiceValid(
            value,
            this->m_names,
            this->m_values,
            Condition::NameMismatchIgnore,
            &error,
            &mismatched
        )
    ) {
        return false;
    }
    *matched = value.trimmed();
    return true;
}

/*!
    Builds why the choice does not take @a value, such as a bare branch
    with signs after a name nothing defines, or a condition that does not
    compile; empty when it takes it, or no reason is known.
*/
QString Config::Choice::explain(const QString & value) const
{
    QString error;
    bool mismatched = false;
    if (choiceValid(
            value,
            this->m_names,
            this->m_values,
            Condition::NameMismatchIgnore,
            &error,
            &mismatched
        )
    ) {
        return QString();
    }
    return error;
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
    *mismatched = false;
    return choiceValid(
        expression,
        names,
        values,
        mismatch,
        error,
        mismatched
    );
}

/*!
    Resolves @a expression with @a context and stores its selected word in
    @a result (see branchWord()). Reports an invalid expression in @a error.
*/
bool Config::Choice::resolve(const QString &expression, const QStringList &names,
                             const QStringList &values, const Evaluator::Context *context,
                             QString *result, QString *error)
{
    const QString text = stripChoiceParens(expression);
    if (branchWord(text, values, names, result)) {
        return true;
    }
    QString condition;
    QString yes;
    QString no;
    QStringList program;
    QString reason;
    const bool isChoice = splitChoice(text, &condition, &yes, &no);
    if ( ! isChoice && values.isEmpty()) {
        explainSigns(text, names, &reason);
    }
    if ( ! isChoice
        || ! Condition::compile(
            condition,
            names,
            &program,
            &reason,
            false,
            Condition::NameMismatchIgnore
        )
    ) {
        // Keeps what the condition's compile found, after the choice.
        *error = QString(QLL("invalid conditional choice: %1")).arg(text);
        if ( ! reason.isEmpty()) {
            *error += QLL(": ") + reason;
        }
        return false;
    }
    bool ok = false;
    const bool selected = Condition::evaluate(program, QSet<QString >(),
                                               context, &ok);
    if ( ! ok) {
        *error = QString(QLL("cannot evaluate condition: %1")).arg(condition);
        return false;
    }
    return resolve(selected ? yes : no, names, values, context, result, error);
}
