// Copyright (C) 2015 The XD Company Ltd.
// SPDX-License-Identifier: Apache-2.0

#include "./config-regexp.h"

#include <QtCore/QStringList>

/*!
    @class Config::RegExp
    @internal

    Matches a replacement by regular expression, written as
    `regexp/<pattern>/<replacement>/`: a tool that reads such a setting runs
    the replacement on the text that the setting governs, instead of a fixed
    value. A `\/` stands for a `/` inside the pattern or the replacement; the
    replacement names the whole match as `\0` and the pattern's groups as
    `\1` to `\99` (see Config::Replacement).

    A `| <value>` after it names the value that applies where the pattern
    matches nothing, such as `regexp/<pattern>/<replacement>/ | keep-as-is`;
    split() parts the two, and Config::Parser checks the fallback against the
    key's other matchers.
*/

/*!
    Builds a shared matcher of `regexp/<pattern>/<replacement>/` values, for
    the matchers of a key.
*/
QSharedPointer<const Config::ValueMatcher > Config::RegExp::create()
{
    return QSharedPointer<const Config::ValueMatcher >(new Config::RegExp);
}

/*!
    Splits @a value, a `regexp/<pattern>/<replacement>/` that may go on with
    `| <fallback>`, into @a head (the part up to the closing `/`) and
    @a fallback (the trimmed value after the `|`, which may be empty), and
    whether there is a `|` into @a hasFallback; returns false for a
    value of any other form.
*/
bool Config::RegExp::split(
        const QString & value, QString *head, QString *fallback,
        bool *hasFallback)
{
    const QString prefix = QString(QLL("regexp/"));
    if ( ! value.startsWith(prefix)) {
        return false;
    }

    // Finds the closing `/`, the second one that no backslash escapes; a
    // backslash takes the character after it along, a backslash included.
    int slashes = 0;
    int end = -1;
    for (int i = prefix.size(); i < value.size(); ++i) {
        if (value.at(i) == QLatin1Char('\\') && i + 1 < value.size()) {
            ++i;
        } else if (value.at(i) == QLatin1Char('/') && ++slashes == 2) {
            end = i + 1;
            break;
        }
    }
    if (end < 0) {
        return false;
    }
    const QString rest = value.mid(end).trimmed();
    if ( ! rest.isEmpty() && ! rest.startsWith(QLatin1Char('|'))) {
        return false;
    }
    *head = value.left(end);
    *fallback = rest.mid(1).trimmed();
    *hasFallback = ! rest.isEmpty();
    return true;
}

/*!
    Parses the pattern and the replacement of @a value (see split(); any
    fallback is left out) into @a pattern and @a replacement; returns false
    for a value of another form, for a pattern that is no valid regular
    expression, or for a replacement that names a group the pattern does not
    have (see Config::Replacement).
*/
bool Config::RegExp::parse(const QString & value, QRegularExpression *pattern,
                           Config::Replacement *replacement)
{
    QString head;
    QString fallback;
    bool hasFallback = false;
    if ( ! Config::RegExp::split(value, &head, &fallback, &hasFallback)) {
        return false;
    }

    // Parts the pattern from the replacement at the first `/` that no
    // backslash escapes, and turns each `\/` back into a `/`; any other
    // backslash pair stays, for the pattern or the replacement to read.
    QStringList parts;
    QString part;
    for (int i = int(qstrlen("regexp/")); i < head.size() - 1; ++i) {
        const QChar c = head.at(i);
        if (c == QLatin1Char('\\') && i + 1 < head.size() - 1) {
            const QChar next = head.at(++i);
            if (next != QLatin1Char('/')) {
                part += c;
            }
            part += next;
        } else if (c == QLatin1Char('/')) {
            parts << part;
            part.clear();
        } else {
            part += c;
        }
    }
    parts << part;
    if (parts.size() != 2 || parts.at(0).isEmpty()) {
        return false;
    }
    *pattern = QRegularExpression(parts.at(0));
    if ( ! pattern->isValid()) {
        return false;
    }
    return Config::Replacement::parse(
        parts.at(1),
        pattern->captureCount(),
        replacement
    );
}

/*!
    Matches @a value when it is a `regexp/<pattern>/<replacement>/` with a
    valid pattern and no fallback (Config::Parser matches a fallback on its
    own), and keeps it as it is.
*/
bool Config::RegExp::match(const QString & value, QString *matched) const
{
    QString head;
    QString fallback;
    bool hasFallback = false;
    QRegularExpression pattern;
    Config::Replacement replacement;
    if ( ! Config::RegExp::split(value, &head, &fallback, &hasFallback) || hasFallback
        || ! Config::RegExp::parse(value, &pattern, &replacement)
    ) {
        return false;
    }
    *matched = value;
    return true;
}

/*!
    Builds the description of a replacement by regular expression.
*/
QString Config::RegExp::describe() const
{
    return QString(QLL("\"regexp/<pattern>/<replacement>/\" (with \" | <value>\" for "
                       "where it matches nothing)"));
}
