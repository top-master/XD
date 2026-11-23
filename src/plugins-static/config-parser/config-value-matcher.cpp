// Copyright (C) 2015 The XD Company Ltd.
// SPDX-License-Identifier: Apache-2.0

#include "./config-value-matcher.h"
#include "./config-evaluator-context.h"

/*!
    @class Config::ValueMatcher
    @internal

    Decides which values a setting takes: a key lists its matchers,
    and a value counts when one of them matches it.
    A tool subclasses it for a value form of its own;
    Config::Word and Config::AnySpace cover the common ones.
*/

/*!
    Destroys the matcher.
*/
Config::ValueMatcher::~ValueMatcher()
{
}

bool Config::ValueMatcher::matchLiteral(const QString & value, QString *matched) const
{
    return this->match(value, matched);
}

/*!
    Builds why this matcher does not take @a value, for the error of a value
    no matcher of the key takes, such as an undefined name a sign follows;
    empty when the description of the values it takes says enough (see
    describe()).
*/
QString Config::ValueMatcher::explain(const QString &) const
{
    return QString();
}

bool Config::ValueMatcher::allowsCode() const
{
    return true;
}

namespace {

class NoCode : public Config::ValueMatcher
{
public:
    bool match(const QString &, QString *) const Q_DECL_OVERRIDE { return false; }
    QString describe() const Q_DECL_OVERRIDE { return QString(); }
    bool allowsCode() const Q_DECL_OVERRIDE { return false; }
};

} // namespace

/*!
    Builds a marker that restricts a setting to literal values.
*/
QSharedPointer<const Config::ValueMatcher > Config::forbidCode()
{
    return QSharedPointer<const Config::ValueMatcher >(new NoCode);
}

QStringList Config::ValueMatcher::enumKeys() const
{
    return Evaluator::CompileContext::anyEnumKeys();
}

/*!
    Converts @a resolved, what a key's compiled program resolves to (see
    Config::Condition::resolve()), into the value of the type the key takes.
    An invalid value leaves the result to the next matcher of the key.
*/
QVariant Config::ValueMatcher::convert(const QVariant & resolved) const
{
    if (resolved.type() != QVariant::String
        && resolved.type() != QVariant::Bool
        && resolved.type() != QVariant::Int
        && resolved.type() != QVariant::UInt
        && resolved.type() != QVariant::LongLong
        && resolved.type() != QVariant::ULongLong
        && resolved.type() != QVariant::Double
    ) {
        return QVariant();
    }
    QString matched;
    return this->matchLiteral(resolved.toString(), &matched) ? QVariant(matched) : QVariant();
}

/*!
    Builds the value a setting exposes to another condition.
*/
QVariant Config::ValueMatcher::settingValue(
        const QString & value, const Evaluator::Context *) const
{
    return value;
}

QSharedPointer<const Config::ValueMatcher > Config::Boolean::create()
{
    return QSharedPointer<const Config::ValueMatcher >(new Config::Boolean);
}

bool Config::Boolean::match(const QString & value, QString *matched) const
{
    if (value.compare(QLL("true"), Qt::CaseInsensitive) == 0) {
        *matched = QString(QLL("true"));
        return true;
    }
    if (value.compare(QLL("false"), Qt::CaseInsensitive) == 0) {
        *matched = QString(QLL("false"));
        return true;
    }
    *matched = QString();
    return false;
}

QString Config::Boolean::describe() const
{
    return QString(QLL("true or false"));
}

QVariant Config::Boolean::convert(const QVariant & resolved) const
{
    if (resolved.type() == QVariant::Bool) {
        return resolved;
    }
    if (resolved.type() == QVariant::Double) {
        return QVariant(resolved.toDouble() != 0);
    }
    QString matched;
    if (resolved.type() == QVariant::String && this->match(resolved.toString(), &matched)) {
        return QVariant(matched == QLL("true"));
    }
    return QVariant();
}

QVariant Config::Boolean::settingValue(
        const QString & value, const Evaluator::Context *) const
{
    return this->convert(value);
}
