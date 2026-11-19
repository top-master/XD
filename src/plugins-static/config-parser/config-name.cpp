// Copyright (C) 2015 The XD Company Ltd.
// SPDX-License-Identifier: Apache-2.0

#include "./config-name.h"

#include <QtCore/QRegExp>

/*!
    @class Config::Name
    @internal

    Matches a C++ name, such as `QLatin1String` or `::Qt::CaseSensitive`: one
    identifier or more joined by `::`, which may start with `::` to name the
    global scope; a `*` in it stands for any part of a name, such as
    `dispose*`. It keeps the name as written, without the spaces around it.

    One that takes a call matches a call of such a name as well, with any
    arguments in its parentheses, such as `QLL($1)` (see the template of
    `[Replace.types]` in the format tool's README).
*/

/*!
    Builds a shared matcher of a name, for a key's matchers.
*/
QSharedPointer<const Config::ValueMatcher > Config::Name::create(bool takesCall)
{
    return QSharedPointer<const Config::ValueMatcher >(new Config::Name(takesCall));
}

/*!
    Matches @a value when it is a C++ name.
*/
bool Config::Name::match(const QString & value, QString *matched) const
{
    QString pattern = QString(QLL("(::)?[A-Za-z_*][A-Za-z0-9_*]*(::[A-Za-z_*][A-Za-z0-9_*]*)*"));
    if (this->m_takesCall) {
        pattern += QLL("(\\(.*\\))?");
    }
    const QRegExp name(pattern);
    if ( ! name.exactMatch(value.trimmed())) {
        *matched = QString();
        return false;
    }
    *matched = value.trimmed();
    return true;
}

/*!
    Builds the description of a name, for an error.
*/
QString Config::Name::describe() const
{
    if (this->m_takesCall) {
        return QString(QLL("a C++ name, such as ::QLL, or a call of one, such as QLL($1)"));
    }
    return QString(QLL("a C++ name, such as ::QLatin1String"));
}
