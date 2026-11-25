// Copyright (C) 2015 The XD Company Ltd.
// SPDX-License-Identifier: Apache-2.0

#include "./config-choice.h"
#include "./config-escape.h"
#include "./config-name.h"


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

QStringList Config::Choice::enumKeys() const
{
    return this->m_values.isEmpty() ? Evaluator::CompileContext::anyEnumKeys()
                                    : this->m_values;
}

/*!
    Validates @a value and preserves it in @a matched.
*/
bool Config::Choice::match(const QString &value, QString *matched) const
{
    const Evaluator::CompileContext context(this->m_names, &this->m_values);
    const QStringList program = Condition::compile(value, &context);
    if ( ! context.lastCompiled()
        || Config::ValueMatcher::usesPercents(program)
    ) {
        *matched = QString();
        return false;
    }
    *matched = value.trimmed();
    return true;
}

bool Config::Choice::matchLiteral(const QString & value, QString *matched) const
{
    *matched = QString();
    const QString text = value.trimmed();
    QString literal;
    QString error;
    if (Config::Escape::isQuoted(text)) {
        if ( ! Config::Escape::unquote(text, &literal, &error)) {
            return false;
        }
    } else if (this->m_values.isEmpty()) {
        if ( ! Config::Name().match(text, &literal)) {
            return false;
        }
    } else {
        literal = text;
    }
    bool number = false;
    const double numeric = literal.toDouble(&number);
    const QVariant converted = this->convert(number ? QVariant(numeric)
                                                    : QVariant(literal));
    if ( ! converted.isValid()) {
        return false;
    }
    *matched = text;
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
    const Evaluator::CompileContext context(this->m_names, &this->m_values);
    Condition::compile(value, &context);
    if (context.lastCompiled()) {
        return QString();
    }
    return context.lastErrorForCompile();
}

/*!
    Converts @a resolved, what the choice resolves to (see resolve()), to a
    listed word in its own spelling, any text when no word is listed, or a
    number as a qreal. Builds an invalid value for any other result, such as
    the 0.5 that `50%` reads as, since a choice of words takes no percent.
*/
QVariant Config::Choice::convert(const QVariant & resolved) const
{
    if (resolved.type() == QVariant::Double) {
        const qreal number = resolved.toDouble();
        if (number != qreal(qint64(number))) {
            return QVariant();
        }
        return resolved;
    }
    const QString word = resolved.toString();
    for (int i = 0; i < this->m_values.size(); ++i) {
        if (this->m_values.at(i).compare(word, Qt::CaseInsensitive) == 0) {
            return this->m_values.at(i);
        }
    }
    if (resolved.type() != QVariant::String
        || ! this->m_values.isEmpty()
    ) {
        return QVariant();
    }
    return word;
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
    Resolves a choice referenced by another setting.
*/
QVariant Config::Choice::settingValue(
        const QString & value, const Evaluator::Context *context) const
{
    const Evaluator::CompileContext compiler(m_names, &m_values, context->mismatchMode());
    const QStringList program = Config::Condition::compile(value, &compiler);
    if ( ! compiler.lastCompiled()
        || Config::ValueMatcher::usesPercents(program)
    ) {
        return QVariant();
    }
    const QVariant resolved = Config::Condition::resolve(program, QSet<QString >(), context);
    return context->lastResolvePassed() ? this->convert(resolved) : QVariant();
}

/*!
    Resolves @a expression at @a context: a word as a QString or a number
    as a qreal. The context records whether resolution passed and any error.
*/
QVariant Config::Choice::resolve(const QString & expression,
                                  const Evaluator::Context *context)
{
    Q_ASSERT(context);
    const QString text = expression.trimmed();
    Evaluator::MismatchScope scope(context, NameMismatchIgnore);
    const QStringList program = Condition::compile(text, context);
    scope.close();
    if ( ! context->lastCompiled()) {
        context->m_lastResolvePassed = false;
        context->m_lastError = QString(QLL("invalid conditional choice: %1")).arg(text);
        if ( ! context->lastErrorForCompile().isEmpty()) {
            context->m_lastError += QLL(": ") + context->lastErrorForCompile();
        }
        return QVariant();
    }
    const QVariant result = Condition::resolve(program, QSet<QString >(), context);
    if ( ! context->lastResolvePassed()) {
        context->m_lastError = QString(QLL("cannot evaluate condition: %1")).arg(text);
    }
    return result;
}
