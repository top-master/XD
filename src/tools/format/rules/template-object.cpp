// Copyright (C) 2015 The XD Company Ltd.
// SPDX-License-Identifier: Apache-2.0

#include "./template-object.h"

#include "./args-array.h"
#include "../parser/condition-context.h"

#include "config-cache.h"

#include <QtCore/QRegExp>

/*!
    @class FormatTemplateObject
    @internal

    Offers `args` to the call templates of `[Replace.types]`: the arguments
    of the call that a template turns, as a FormatArgsArray, which a template
    reaches on with `args[<index>]`
    or `args.<name>(...)`. FormatConditionContext::registerObjects() adds it
    to the global scope of Config::Scopes once, as the tool starts. The
    `isLiteral()` slot checks a call argument for string literals.
*/

/*!
    Builds the arguments of the call that the template in question turns, or
    null where the place is no such call.
*/
FormatArgsArray *FormatTemplateObject::args() const
{
    Config::Cache::skip();
    const FormatConditionContext *format =
        static_cast<const FormatConditionContext * >(this->context());
    if ( ! format) {
        return Q_NULLPTR;
    }
    return format->callArguments();
}

/*!
    Answers the positional argument registered as @a name for this call.
*/
QVariant FormatTemplateObject::dynamicValue(const QByteArray & name) const
{
    bool valid = false;
    const int index = name.mid(1).toInt(&valid) - 1;
    FormatArgsArray *arguments = this->args();
    if ( ! valid || index < 0 || ! arguments
        || index >= arguments->items().size()
        || arguments->items().at(index).isNull()
    ) {
        return QVariant();
    }
    return arguments->items().at(index);
}

/*!
    Registers the positional names used by @a text in a thread-local scope.
*/
QSharedPointer<Config::Scopes::Scope > FormatTemplateObject::argumentScope(
        const QString & text)
{
    const QSharedPointer<Config::Scopes::Scope > scope = Config::Scopes::push();
    FormatTemplateObject *object = new FormatTemplateObject;
    QRegExp argument(QString(QLL("\\$[1-9][0-9]*")));
    int position = 0;
    while ((position = argument.indexIn(text, position)) >= 0) {
        const QByteArray name = argument.cap(0).toLatin1();
        object->setProperty(name.constData(), true);
        position += argument.matchedLength();
    }
    scope->add(object);
    return scope;
}

/*!
    Tells whether @a value starts with a double quote.
*/
bool FormatTemplateObject::isLiteral(const QVariant & value) const
{
    return value.toString().startsWith(QLatin1Char('"'));
}
