// Copyright (C) 2015 The XD Company Ltd.
// SPDX-License-Identifier: Apache-2.0

#include "./config-schema.h"
#include "./config-cache.h"
#include "./config-condition.h"
#include "./config-numbers.h"
#include "./config-regexp.h"

/*!
    @class Config::Schema
    @internal

    Lists the sections, and their settings, that a tool supports, which
    Config::Parser accepts; any other section or key, and any value a key does
    not take, is an error.
*/

/*!
    Adds @a section, with its keys, to the schema.
*/
void Config::Schema::addSection(const Config::Section & section)
{
    this->m_sections.append(section);
}

/*!
    Finds the section @a name, in any case; a name the schema does not list
    gets a null section (see Config::Section::isNull()).
*/
Config::Section Config::Schema::section(const QString & name) const
{
    for (int i = 0; i < this->m_sections.size(); ++i) {
        if (this->m_sections.at(i).name().compare(
                name,
                Qt::CaseInsensitive
            ) == 0
        ) {
            return this->m_sections.at(i);
        }
    }
    return Config::Section();
}

/*!
    Finds the section that the setting @a name (`<section>.<key>`, in any
    case) belongs to: the one with the longest name that, with a `.` after
    it, starts @a name; hence a section's own name may hold a `.`, as in
    `[Replace.types]`. A name no section starts gets a null section.
*/
Config::Section Config::Schema::sectionOf(const QString & name) const
{
    Config::Section found;
    for (int i = 0; i < this->m_sections.size(); ++i) {
        const QString prefix = this->m_sections.at(i).name() + QLatin1Char('.');
        if (name.startsWith(prefix, Qt::CaseInsensitive)
            && prefix.size() > found.name().size() + 1
        ) {
            found = this->m_sections.at(i);
        }
    }
    return found;
}

/*!
    Finds the setting @a name (`<section>.<key>`), in any case; a name the
    schema does not list gets a null key (see Config::Key::isNull()), unless
    its section is open (see Config::Section::setOpen()), which takes it
    with the section's open matchers.
*/
Config::Key Config::Schema::key(const QString & name) const
{
    const Config::Section found = this->sectionOf(name);
    const QString lower = name.toLower();
    for (int i = 0; i < found.keys().size(); ++i) {
        if (found.keys().at(i).name == lower) {
            return found.keys().at(i);
        }
    }
    if (found.isOpen() && name.size() > found.name().size() + 1) {
        Config::Key key;
        key.name = lower;
        key.matchers = found.openMatchers();
        key.open = true;
        return key;
    }
    return Config::Key();
}

/*!
    Tells whether @a text for @a name is a quoted value, an expression, or
    a context-dependent value, using the names at @a context.
*/
bool Config::Schema::isExpression(
        const QString & name, const QString & text,
        const Evaluator::CompileContext *context) const
{
    const Config::Key field = this->key(name);
    if ( ! field.allowsCode()) {
        return false;
    }
    if (Config::Numbers::parse(text).size() > 1) {
        for (int i = 0; i < field.matchers.size(); ++i) {
            QString matched;
            if (field.matchers.at(i)->matchLiteral(text, &matched)) {
                return false;
            }
        }
    }
    const QStringList program = this->compile(name, text, context);
    if ( ! context->lastCompiled()) {
        return false;
    }
    if (text.trimmed().startsWith(QLatin1Char('"'))) {
        return true;
    }
    for (int i = 1; i < program.size(); ++i) {
        const QString item = program.at(i);
        if (item.startsWith(QLL("r:")) || item.startsWith(QLL("f:"))
            || item.startsWith(QLL("op:")) || item.startsWith(QLL("jelse:"))
            || (item.startsWith(QLL("v:")) && item != QLL("v:true")
                && item != QLL("v:false"))
        ) {
            return true;
        }
    }
    return false;
}

/*!
    Compiles @a text for @a name, using its matchers' choice words and the
    names at @a context.
*/
QStringList Config::Schema::compile(
        const QString & name, const QString & text,
        const Evaluator::CompileContext *context) const
{
    Q_ASSERT(context);
    const Config::Key found = this->key(name);
    if ( ! found.allowsCode()) {
        context->m_lastCompile = CompileRes();
        context->m_lastCE = QString(QLL("code is forbidden for %1")).arg(name);
        return QStringList();
    }
    QSharedPointer<Scopes::Scope > scope;
    if (found.setup) {
        scope = Scopes::push();
        found.setup(scope.data());
    }
    QStringList words = found.aliases.keys();
    for (int i = 0; i < found.matchers.size(); ++i) {
        words += found.matchers.at(i)->enumKeys();
    }
    const Evaluator::CompileContext compiler(
        context->names() + m_names,
        words.isEmpty() ? Q_NULLPTR : &words, context->mismatchMode()
    );
    const QStringList program = Condition::compile(text, &compiler, true);
    context->m_lastCompile = compiler.m_lastCompile;
    context->m_lastCE = compiler.m_lastCE;
    return program;
}

/*!
    Compiles and resolves the setting @a name's expression @a text at
    @a context, converting the result as the setting's matchers require.
*/
QVariant Config::Schema::eval(
        const QString & name, const QString & text,
        const Evaluator::Context *context) const
{
    Q_ASSERT(context);
    const QStringList program = this->compile(name, text, context);
    if ( ! context->lastCompiled()) {
        context->m_lastResolvePassed = false;
        context->m_lastError = context->lastErrorForCompile();
        return QVariant();
    }
    return this->resolve(name, program, context);
}

/*!
    Resolves @a text when another setting references @a name at @a context.
*/
QVariant Config::Schema::reference(
        const QString & name, const QString & text,
        const Evaluator::Context *context) const
{
    if ( ! this->key(name).referenceThreshold) {
        return this->eval(name, text, context);
    }
    const QStringList program = this->compile(name, text, context);
    if ( ! context->lastCompiled()) {
        context->markResolveFailed(context->lastErrorForCompile());
        return QVariant();
    }
    const CacheScope scope(name);
    Q_UNUSED(scope);
    for (int percent = 0; percent <= 100; ++percent) {
        const QVariant holds = context->resolveAtPercent(program, percent);
        if ( ! holds.isValid()) {
            context->markResolveFailed(QString(QLL("cannot resolve setting %1")).arg(name));
            return QVariant();
        }
        if (holds.toBool()) {
            context->markResolved();
            return QVariant(QString(QString::number(percent - 1) + QLatin1Char('%')));
        }
    }
    context->markResolved();
    return QString(QLL("100%"));
}

/*!
    Resolves the setting @a name's compiled @a program at @a context and
    converts its result. The context records whether resolution and
    conversion passed.
*/
QVariant Config::Schema::resolve(
        const QString & name, const QStringList & program,
        const Evaluator::Context *context) const
{
    Q_ASSERT(context);
    const Config::Key found = this->key(name);
    if (found.isNull()) {
        context->m_lastResolvePassed = false;
        context->m_lastError = QString(QLL("unknown setting %1")).arg(name);
        return QVariant();
    }
    QSharedPointer<Scopes::Scope > scope;
    if (found.setup) {
        scope = Scopes::push();
        found.setup(scope.data());
    }
    const CacheScope cacheScope(found.name);
    Q_UNUSED(cacheScope);
    const QVariant resolved = Condition::resolve(program, QSet<QString >(), context);
    if ( ! context->lastResolvePassed()) {
        return QVariant();
    }
    const QVariant value = this->convert(name, resolved);
    if ( ! value.isValid()) {
        context->m_lastResolvePassed = false;
        context->m_lastError = QString(QLL("cannot convert setting %1")).arg(name);
    }
    return value;
}

/*!
    Converts @a resolved, what a compiled program of the setting @a name
    resolves to (see Config::Condition::resolve()),
    into the value of the type that setting takes: the first of
    its matchers that takes such a result converts it
    (see Config::ValueMatcher::convert()). Builds an invalid value when none
    does, or the schema does not list the setting.
*/
QVariant Config::Schema::convert(
        const QString & name, const QVariant & resolved) const
{
    const Config::Key found = this->key(name);
    QVariant canonical = resolved;
    if (canonical.type() == QVariant::String) {
        const QHash<QString, QString >::const_iterator alias =
            found.aliases.constFind(canonical.toString().toLower());
        if (alias != found.aliases.constEnd()) {
            canonical = *alias;
        }
        QString head;
        QString fallback;
        bool hasFallback = false;
        if (Config::RegExp::split(canonical.toString(), &head, &fallback, &hasFallback)
            && hasFallback
        ) {
            QString nestedHead;
            QString nestedFallback;
            bool nested = false;
            if (Config::RegExp::split(fallback, &nestedHead, &nestedFallback, &nested)) {
                return QVariant();
            }
            const QVariant convertedHead = this->convert(name, head);
            const QVariant convertedFallback = this->convert(name, fallback);
            if ( ! convertedHead.isValid() || ! convertedFallback.isValid()) {
                return QVariant();
            }
            const QString tail = convertedFallback.toString();
            return convertedHead.toString() + QLL(" |")
                + (tail.isEmpty() ? QString() : QLatin1Char(' ') + tail);
        }
    }
    for (int i = 0; i < found.matchers.size(); ++i) {
        const QVariant value = found.matchers.at(i)->convert(canonical);
        if (value.isValid()) {
            return value;
        }
    }
    return QVariant();
}
