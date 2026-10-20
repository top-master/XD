// Copyright (C) 2015 The XD Company Ltd.
// SPDX-License-Identifier: Apache-2.0

#include "./config-parser.h"

#include "./config-escape.h"
#include "./config-regexp.h"
#include "./config-scanner.h"

#include <QtCore/QDir>
#include <QtCore/QFile>
#include <QtCore/QFileInfo>
#include <QtCore/QScopedPointer>
#include <QtCore/QSettings>
#include <QtCore/QTemporaryFile>

/*!
    \namespace Config
    \internal

    Reads a tool's settings files: Scanner finds them from a project's root
    down, and Parser reads them, checking each value against the Schema the
    tool gives, made of Section, Key, and ValueMatcher
    (such as Word or AnySpace).
*/

/*!
    \class Config::Parser
    \internal

    Reads a tool's settings: `.ini` files (sections and keys in any case) and
    `<section>.<key>=<value>` assignments, each overriding what came before,
    checked against the tool's Config::Schema. readLevels() reads the files
    that Config::Scanner finds level by level.
*/

/*!
    Builds the parser of the settings \a schema lists, each at its default,
    and each key that extends another one following that key.
*/
Config::Parser::Parser(const Config::Schema & schema)
    : m_schema(schema)
{
    for (int s = 0; s < schema.sections().size(); ++s) {
        const QList<Config::Key > & keys = schema.sections().at(s).keys();
        for (int k = 0; k < keys.size(); ++k) {
            if (keys.at(k).extends.isEmpty()) {
                this->m_values.insert(
                    keys.at(k).name,
                    keys.at(k).defaultValue
                );
            }
        }
    }
}

/*!
    Numbers each temporary copy that Config::Parser::readFile() reads, so that
    no copy takes the path of an earlier one.
*/
Q_GLOBAL_STATIC(QAtomicInt, copiesMade)

/*!
    Tells whether QSettings reads any value of \a settings as a list, which
    it does for a value with a comma outside double quotes.
*/
static bool hasListValue(QSettings & settings)
{
    const QStringList keys = settings.allKeys();
    for (int i = 0; i < keys.size(); ++i) {
        const QVariant::Type type = settings.value(keys.at(i)).type();
        if (type == QVariant::StringList || type == QVariant::List) {
            return true;
        }
    }
    return false;
}

/*!
    Tells whether \a c is a hexadecimal digit.
*/
static bool isHexDigit(char c)
{
    return (c >= '0' && c <= '9') || (c >= 'a' && c <= 'f')
        || (c >= 'A' && c <= 'F');
}

/*!
    Builds a copy of the `.ini` text \a data in which each comma of a value
    that stands outside double quotes is the escape `\x2c`; hence QSettings
    reads such a value as one string, with its commas and the spaces after
    them, where it would split the value into a list at each comma and drop
    the spaces that follow.

    The copy keeps every other byte, and finds lines, comments, sections,
    quotes, and escapes the way QSettings does. A hexadecimal digit right
    after a comma would extend the `\x2c` escape; hence `\z` follows it there,
    an escape QSettings reads as nothing.
*/
static QByteArray iniWithPlainCommas(const QByteArray & data)
{
    QByteArray copy;
    copy.reserve(data.size() + 16);
    bool atLineStart = true;
    bool inComment = false;
    bool inKeyLine = false;
    bool inValue = false;
    bool inQuotes = false;
    for (int i = 0; i < data.size(); ++i) {
        const char c = data.at(i);
        const bool isNewLine = c == '\n' || c == '\r';

        // Copies a comment up to the end of its line.
        if (inComment) {
            copy += c;
            if (isNewLine) {
                inComment = false;
                atLineStart = true;
            }
            continue;
        }

        // Copies the spaces that start a line, and tells the line's kind by
        // the first character after them.
        if (atLineStart) {
            if (c == ' ' || c == '\t' || isNewLine) {
                copy += c;
                continue;
            }
            atLineStart = false;
            inKeyLine = c != '[';
            inValue = false;
            inQuotes = false;
            if (c == ';') {
                inComment = true;
                copy += c;
                continue;
            }
        }

        // Copies an escape whole, with the line break it may escape.
        if (c == '\\') {
            copy += c;
            if (i + 1 < data.size()) {
                const char escaped = data.at(++i);
                copy += escaped;
                if (i + 1 < data.size()
                    && ((escaped == '\n' && data.at(i + 1) == '\r')
                        || (escaped == '\r' && data.at(i + 1) == '\n'))
                ) {
                    copy += data.at(++i);
                }
            }
            continue;
        }

        // Ends the line at a line break outside double quotes, as a quoted
        // part of a value goes on past a line break.
        if (isNewLine && ! inQuotes) {
            copy += c;
            atLineStart = true;
            continue;
        }
        if (c == '"') {
            inQuotes = ! inQuotes;
        } else if (c == ';' && ! inQuotes) {
            inComment = true;
        } else if (c == '=' && inKeyLine && ! inQuotes) {
            inValue = true;
        } else if (c == ',' && inValue && ! inQuotes) {
            copy += "\\x2c";
            if (i + 1 < data.size() && isHexDigit(data.at(i + 1))) {
                copy += "\\z";
            }
            continue;
        }
        copy += c;
    }
    return copy;
}

/*!
    Reads the `.ini` file \a file over the settings so far; returns false,
    with \a error naming the file, for a key or a value that the
    schema does not take.

    A value keeps its commas as plain characters, with the spaces after
    them, whether or not it stands in double quotes.
*/
bool Config::Parser::readFile(const QString & file, QString *error)
{
    const QString origin = QDir::toNativeSeparators(file);
    if (!QFileInfo(file).isFile()) {
        *error = QString(QLL("%1: no such settings file")).arg(origin);
        return false;
    }
    // Names the copy apart from every earlier one: `QSettings` reuses its
    // parse of a file whose path, size, and time stamp (in whole seconds)
    // match, and `QTemporaryFile` takes the name of a removed file again.
    const QString copyName = QDir::tempPath()
        + QString(QLL("/config-parser-%1-XXXXXX.ini")).arg(copiesMade()->fetchAndAddRelaxed(1));
    QTemporaryFile copy(copyName);
    QScopedPointer<QSettings > settings(new QSettings(
        file,
        QSettings::IniFormat
    ));

    // Reads a copy of the file whose commas `QSettings` takes as plain
    // characters, once the file has a value it would split into a list.
    if (hasListValue(*settings)) {
        QFile input(file);
        if ( ! input.open(QIODevice::ReadOnly) || ! copy.open()
            || copy.write(iniWithPlainCommas(input.readAll())) < 0
        ) {
            *error = QString(QLL("%1: cannot read the settings file")).arg(origin);
            return false;
        }
        copy.close();
        settings.reset(new QSettings(copy.fileName(), QSettings::IniFormat));
    }

    const QStringList keys = settings->allKeys();
    for (int i = 0; i < keys.size(); ++i) {
        const int slash = keys.at(i).lastIndexOf(QLatin1Char('/'));
        const QString section = slash < 0 ? QString(QLL("General")) : keys.at(i).left(slash);
        const QString key = keys.at(i).mid(slash + 1);
        const QString shown = QString(QLL("[%1] %2")).arg(section, key);
        QString reason;
        if (!this->set(section + QLatin1Char('.') + key, shown,
                       settings->value(keys.at(i)).toString(), &reason)
        ) {
            *error = origin + QLL(": ") + reason;
            return false;
        }
    }
    return true;
}

/*!
    Reads the files named \a fileName that apply to \a path (see
    Config::Scanner::levelFiles()) over the settings so far, from the root's
    down.
*/
bool Config::Parser::readLevels(const QString & path, const QString & fileName, QString *error)
{
    const QStringList files = Config::Scanner::levelFiles(path, fileName);
    for (int i = 0; i < files.size(); ++i) {
        if (!this->readFile(files.at(i), error)) {
            return false;
        }
    }
    return true;
}

/*!
    Reads the assignment \a assignment (`<section>.<key>=<value>`, the name
    in any case) over the settings so far; returns false, with \a error, for
    a name or a value the schema does not take.

    A value in double quotes takes the escapes of a C++ string literal (see
    Config::Escape); a value without quotes stays as it is, hence a regular
    expression's backslashes need no doubling there.
*/
bool Config::Parser::assign(const QString & assignment, QString *error)
{
    const int equals = assignment.indexOf(QLatin1Char('='));
    if (equals <= 0) {
        *error = QString(QLL("%1: not a <section>.<key>=<value> setting")).arg(assignment);
        return false;
    }
    const QString name = assignment.left(equals).trimmed();
    QString value = assignment.mid(equals + 1);

    // Reads a value in double quotes as a C++ string literal, and leaves any
    // other value as it is.
    if (value.size() >= 2 && value.startsWith(QLatin1Char('"'))
        && value.endsWith(QLatin1Char('"'))
    ) {
        QString reason;
        if (!Config::Escape::decode(value.mid(1, value.size() - 2), &value, &reason)) {
            *error = name + QLL(": ") + reason;
            return false;
        }
    }
    return this->set(name, name, value, error);
}

/*!
    Builds the value of the setting \a name (in any case), after every file
    and assignment read so far; a key that extends another one and that
    nothing overrode yet takes that key's value (see Config::Key::extends).
*/
QString Config::Parser::value(const QString & name) const
{
    const QString lower = name.toLower();
    QHash<QString, QString >::const_iterator found = this->m_values.constFind(lower);
    if (found != this->m_values.constEnd()) {
        return found.value();
    }
    const Config::Key key = this->m_schema.key(lower);
    if ( ! key.extends.isEmpty()) {
        return this->value(key.extends);
    }
    return QString();
}

/*!
    Sets the setting \a name (`<section>.<key>`, in any case) to \a value;
    returns false, with \a error naming the setting as \a shown and listing
    the values it takes, for a name or a value the schema does not take.
*/
bool Config::Parser::set(const QString & name, const QString & shown, const QString & value,
                       QString *error)
{
    const Config::Key key = this->m_schema.key(name);
    if (key.isNull()) {
        const int dot = name.indexOf(QLatin1Char('.'));
        if (dot < 0 || this->m_schema.section(name.left(dot)).isNull()) {
            *error = QString(QLL("unknown section of setting %1")).arg(shown);
        } else {
            *error = QString(QLL("unknown setting %1")).arg(shown);
        }
        return false;
    }
    QString matched;
    if (Config::Parser::matchValue(key, value, &matched)) {
        this->m_values.insert(key.name, matched);
        return true;
    }
    QStringList choices;
    for (int i = 0; i < key.matchers.size(); ++i) {
        choices << key.matchers.at(i)->describe();
    }
    *error = QString(QLL("%1 = %2: use %3")).arg(shown, value, choices.join(QString(QLL(" or "))));
    return false;
}

/*!
    Matches \a value (in any of its alias spellings) against the matchers of
    \a key, and gives \a matched the value to keep; a
    `regexp/<pattern>/<replacement>/ | <fallback>` matches when its head
    matches and so does its fallback (which then keeps its matched spelling,
    and may be no `regexp/` itself).
*/
bool Config::Parser::matchValue(const Config::Key & key, const QString & value,
                                QString *matched)
{
    const QString resolved = key.aliases.value(value.toLower(), value);
    QString head;
    QString fallback;
    bool hasFallback = false;
    if (Config::RegExp::split(resolved, &head, &fallback, &hasFallback) && hasFallback) {
        QString nestedHead;
        QString nestedFallback;
        bool nested = false;
        QString matchedHead;
        QString matchedFallback;
        if (Config::RegExp::split(fallback, &nestedHead, &nestedFallback, &nested)
            || !Config::Parser::matchValue(key, head, &matchedHead)
            || !Config::Parser::matchValue(key, fallback, &matchedFallback)
        ) {
            return false;
        }
        *matched = matchedHead + QLL(" |");
        if (!matchedFallback.isEmpty()) {
            *matched += QLatin1Char(' ') + matchedFallback;
        }
        return true;
    }
    for (int i = 0; i < key.matchers.size(); ++i) {
        if (key.matchers.at(i)->match(resolved, matched)) {
            return true;
        }
    }
    return false;
}
