// Copyright (C) 2015 The XD Company Ltd.
// SPDX-License-Identifier: Apache-2.0

#include "./config-parser.h"

#include "./config-escape.h"
#include "./config-regexp.h"
#include "./config-scanner.h"

#include <QtCore/QBuffer>
#include <QtCore/QDir>
#include <QtCore/QFile>
#include <QtCore/QFileInfo>

/*!
    @namespace Config
    @internal

    Reads a tool's settings files: Scanner finds them from a project's root
    down, and Parser reads them, checking each value against the Schema the
    tool gives, made of Section, Key, and ValueMatcher
    (such as Word or AnySpace).
*/

/*!
    @class Config::Parser
    @internal

    Reads a tool's settings: `.ini` files (sections and keys in any case) and
    `<section>.<key>=<value>` assignments, each overriding what came before,
    checked against the tool's Config::Schema. readLevels() reads the files
    that Config::Scanner finds level by level.
*/

/*!
    Builds the parser of the settings @a schema lists, each at its default,
    and each key that extends another one following that key.
*/
Config::Parser::Parser(const Config::Schema & schema)
    : m_schema(schema)
{
    for (int s = 0; s < schema.sections().size(); ++s) {
        const QList<Config::Key > & keys = schema.sections().at(s).keys();
        for (int k = 0; k < keys.size(); ++k) {
            if (keys.at(k).extends.isEmpty()) {
                this->m_settings.values.insert(
                    keys.at(k).name,
                    keys.at(k).defaultValue
                );
            }
        }
    }
}

/*!
    Destroys the parser.
*/
Config::Parser::~Parser()
{
}

namespace {

/*!
    Tells whether the `.ini` line @a line goes on in the next one, as `.ini`
    files take it: a value in double quotes goes on past a line break, and
    so does a line that ends in a `\` (an escaped line break). A `;` outside
    double quotes starts a comment, which ends the line.
*/
static bool continuesOnNextLine(const QByteArray & line)
{
    bool inQuotes = false;
    for (int i = 0; i < line.size(); ++i) {
        const char c = line.at(i);
        if (c == '\\') {
            if (i + 1 >= line.size()) {
                return true;
            }
            const char escaped = line.at(++i);
            if ((escaped == '\n' || escaped == '\r') && i + 1 >= line.size()) {
                return true;
            }
            if (escaped == '\r' && line.at(i + 1) == '\n' && i + 2 >= line.size()) {
                return true;
            }
            continue;
        }
        if (c == '"') {
            inQuotes = ! inQuotes;
        } else if (c == ';' && ! inQuotes) {
            return false;
        }
    }
    return inQuotes;
}

/*!
    Builds the key or section name @a text of an `.ini` line, as `.ini`
    files spell one: a `\` stands for a `/`, and `%<hex><hex>` or
    `%U<hex><hex><hex><hex>` for the character of that code.
*/
static QString iniKey(const QByteArray & text)
{
    QString key;
    for (int i = 0; i < text.size(); ++i) {
        const char c = text.at(i);
        if (c == '\\') {
            key += QLatin1Char('/');
            continue;
        }
        if (c != '%' || i + 1 >= text.size()) {
            key += QLatin1Char(c);
            continue;
        }
        const bool wide = text.at(i + 1) == 'U';
        const int digits = wide ? 4 : 2;
        const int first = i + (wide ? 2 : 1);
        bool ok = false;
        const int code = first + digits <= text.size()
            ? text.mid(first, digits).toInt(&ok, 16) : 0;
        if ( ! ok) {
            key += QLatin1Char('%');
            continue;
        }
        key += QChar(code);
        i = first + digits - 1;
    }
    return key;
}

/*!
    Pairs each character that may follow a `\` in an `.ini` value with the
    character that escape stands for (see iniValue()).
*/
static const char iniEscapes[][2] = {
    { 'a', '\a' }, { 'b', '\b' }, { 'f', '\f' }, { 'n', '\n' },
    { 'r', '\r' }, { 't', '\t' }, { 'v', '\v' }, { '"', '"' },
    { '?', '?' }, { '\'', '\'' }, { '\\', '\\' }
};

/*!
    Builds the value @a text of an `.ini` line (the bytes after its `=`),
    as `.ini` files spell one: the spaces around it drop out, a part in
    double quotes keeps its spaces and loses its quotes, and a `\` starts an
    escape: `\a`, `\b`, `\f`, `\n`, `\r`, `\t`, `\v`, `\"`, `\?`, `\'`,
    `\\`, `\x<hex>...`, `\<octal>...`, an escaped line break (which drops
    out), or any other character (which drops out with it). A comma stays a
    plain character, and a value that starts with `@@` loses one `@`.
*/
static QString iniValue(const QByteArray & text)
{
    QString value;
    QByteArray run;
    bool quoted = false;
    bool inQuotes = false;
    int chopLimit = 0;
    int i = 0;
    while (i < text.size() && (text.at(i) == ' ' || text.at(i) == '\t')) {
        ++i;
    }
    while (i < text.size()) {
        const char c = text.at(i++);
        if (c != '\\' && c != '"') {
            run += c;
            continue;
        }
        value += QString::fromUtf8(run);
        run.clear();
        if (c == '"') {
            quoted = true;
            inQuotes = ! inQuotes;

            // Skips the spaces after a closing quote, as `.ini` files do.
            if ( ! inQuotes) {
                while (i < text.size() && (text.at(i) == ' ' || text.at(i) == '\t')) {
                    ++i;
                }
                chopLimit = value.size();
            }
            continue;
        }
        if (i >= text.size()) {
            break;
        }
        const char escaped = text.at(i++);
        int code = -1;
        for (int k = 0; k < int(sizeof(iniEscapes) / sizeof(iniEscapes[0])); ++k) {
            if (iniEscapes[k][0] == escaped) {
                code = k;
                break;
            }
        }
        if (code >= 0) {
            value += QLatin1Char(iniEscapes[code][1]);
            chopLimit = value.size();
            continue;
        }
        if (escaped == 'x' || (escaped >= '0' && escaped <= '7')) {
            const int base = escaped == 'x' ? 16 : 8;
            uint codePoint = escaped == 'x' ? 0 : uint(escaped - '0');
            bool digits = escaped != 'x';
            while (i < text.size()) {
                bool ok = false;
                const uint digit = QByteArray(1, text.at(i)).toUInt(&ok, base);
                if ( ! ok) {
                    break;
                }
                codePoint = codePoint * base + digit;
                digits = true;
                ++i;
            }
            if (digits) {
                value += QChar(codePoint);
            }
        } else if ((escaped == '\r' || escaped == '\n') && i < text.size()
            && (text.at(i) == '\n' || text.at(i) == '\r') && text.at(i) != escaped
        ) {
            ++i;
        }
        chopLimit = value.size();
    }
    value += QString::fromUtf8(run);

    // Drops the spaces at the end of a value without quotes.
    if ( ! quoted) {
        int end = value.size();
        while (end > chopLimit
            && (value.at(end - 1) == QLatin1Char(' ') || value.at(end - 1) == QLatin1Char('\t'))
        ) {
            --end;
        }
        value.truncate(end);
    }
    if (value.startsWith(QLL("@@"))) {
        value.remove(0, 1);
    }
    return value;
}

} // namespace

/*!
    Reads the `.ini` file @a file over the settings so far; returns false,
    with @a error naming the file, for a key or a value that the
    schema does not take (see readData()).
*/
bool Config::Parser::readFile(const QString & file, QString *error)
{
    const QString origin = QDir::fromNativeSeparators(file);
    QFile input(file);
    if ( ! QFileInfo(file).isFile()) {
        *error = QString(QLL("%1: no such settings file")).arg(origin);
        return false;
    }
    if ( ! input.open(QIODevice::ReadOnly)) {
        *error = QString(QLL("%1: cannot read the settings file")).arg(origin);
        return false;
    }
    return this->readData(&input, origin, error);
}

/*!
    Reads the `.ini` text @a data over the settings so far, naming @a origin
    in an error (see the readData() that reads a device).
*/
bool Config::Parser::readData(const QByteArray &data, const QString &origin,
                              QString *error)
{
    QByteArray text = data;
    QBuffer buffer(&text);
    buffer.open(QIODevice::ReadOnly);
    return this->readData(&buffer, origin, error);
}

/*!
    Reads the `.ini` text of @a device over the settings so far, line by
    line, naming @a origin in an error; returns false for a key or a value
    that the schema does not take.

    A line goes on in the next one only where `.ini` files take it so (see
    continuesOnNextLine()). A value keeps its commas as plain characters,
    with the spaces after them, whether or not it stands in double quotes.
*/
bool Config::Parser::readData(QIODevice *device, const QString &origin,
                              QString *error)
{
    QString section;
    while ( ! device->atEnd()) {
        QByteArray line = device->readLine();
        while (continuesOnNextLine(line) && ! device->atEnd()) {
            line += device->readLine();
        }
        if ( ! this->readLine(line, &section, origin, error)) {
            return false;
        }
    }
    return true;
}

/*!
    Reads the `.ini` line @a line, under the section @a section (empty for
    the `[General]` one, which a section line in @a line changes), naming
    @a origin in an error.

    A key of an open section (see Config::Section::setOpen()) keeps its own
    spelling and each line it is given on, in order; its value in double
    quotes takes the escapes of a C++ string literal, as an assignment's
    does. Any other key takes its value the way `.ini` files spell one (see
    iniValue()), and the latest line of a key wins. A line that starts with
    `;` or `#` is a comment, and a `;` outside double quotes starts one.
*/
bool Config::Parser::readLine(const QByteArray & line, QString *section,
                              const QString & origin, QString *error)
{
    // Finds the line's `=` and its comment, outside double quotes.
    const QByteArray text = line.trimmed();
    if (text.isEmpty() || text.startsWith(';') || text.startsWith('#')) {
        return true;
    }
    int equals = -1;
    int end = text.size();
    bool inQuotes = false;
    for (int i = 0; i < text.size(); ++i) {
        const char c = text.at(i);
        if (c == '\\') {
            ++i;
        } else if (c == '"') {
            inQuotes = ! inQuotes;
        } else if (c == '=' && ! inQuotes && equals < 0) {
            equals = i;
        } else if (c == ';' && ! inQuotes) {
            end = i;
            break;
        }
    }

    // Starts a section.
    if (text.startsWith('[')) {
        const int close = text.indexOf(']');
        *section = iniKey(text.mid(1, (close < 0 ? end : close) - 1).trimmed());
        if (section->compare(QLL("General"), Qt::CaseInsensitive) == 0) {
            section->clear();
        }
        return true;
    }
    if (equals <= 0 || equals > end) {
        return true;
    }

    // Splits the key from its section, as a `/` in the key names a deeper one.
    const QString path = (section->isEmpty() ? QString() : *section + QLatin1Char('/'))
        + iniKey(text.left(equals).trimmed());
    const int slash = path.lastIndexOf(QLatin1Char('/'));
    const QString sectionName = slash < 0 ? QString(QLL("General")) : path.left(slash);
    const QString key = path.mid(slash + 1);
    const QString name = sectionName + QLatin1Char('.') + key;
    const QString shown = QString(QLL("[%1] %2")).arg(sectionName, key);
    QString value;
    if (this->m_schema.key(name).open) {
        value = QString::fromUtf8(text.mid(equals + 1)).trimmed();
        if (value.size() >= 2 && value.startsWith(QLatin1Char('"'))
            && value.endsWith(QLatin1Char('"'))
        ) {
            QString reason;
            if ( ! Config::Escape::decode(
                    value.mid(1, value.size() - 2),
                    &value,
                    &reason
                )
            ) {
                *error = origin + QLL(": ") + shown + QLL(": ") + reason;
                return false;
            }
        }
    } else {
        value = iniValue(text.mid(equals + 1, end - equals - 1));
    }
    QString reason;
    if ( ! this->set(name, shown, value, &reason)) {
        *error = origin + QLL(": ") + reason;
        return false;
    }
    return true;
}

/*!
    @class Config::LayeredParser
    @internal

    Applies permanent and scoped groups of setting lines over a base parser.
    Scoped groups override permanent groups and are removed in reverse order.
*/

/*!
    @class Config::Lines
    @internal

    Stores the lines of one configuration layer.
*/

/*! Joins the setting lines with newlines for the `.ini` parser. */
QByteArray Config::Lines::text() const
{
    QByteArray result;
    for (int i = 0; i < m_lines.size(); ++i) {
        if (i > 0) {
            result += '\n';
        }
        result += m_lines.at(i);
    }
    return result;
}

/*!
    Adds @a line without whitespace outside quoted values.
*/
void Config::Lines::appendCompact(const QByteArray &line)
{
    const QString input = QString::fromUtf8(line);
    QString compact;
    bool quoted = false;
    bool escaped = false;
    for (int i = 0; i < input.size(); ++i) {
        const QChar value = input.at(i);
        if (quoted) {
            compact += value;
            if (escaped) {
                escaped = false;
            } else if (value == QLatin1Char('\\')) {
                escaped = true;
            } else if (value == QLatin1Char('"')) {
                quoted = false;
            }
        } else if (value == QLatin1Char('"')) {
            quoted = true;
            compact += value;
        } else if ( ! value.isSpace()) {
            compact += value;
        }
    }
    this->append(compact.toUtf8());
}

/*! Builds the layers over the settings in @a base. */
Config::LayeredParser::LayeredParser(const Parser &base)
    : Parser(base), m_layerBegin(0), m_layerEnd(1)
{
}

/*!
    Stores @a value in each layer from m_layerBegin up to m_layerEnd (see
    Parser::store()); the index past m_layers stands for the innermost
    layer, Parser's own settings.
*/
void Config::LayeredParser::store(const QString & name, const QString & section,
                                  const QString & entry, const QString & value)
{
    int layer = this->m_layerBegin;
    do {
        Settings *target = layer < this->m_layers.size()
            ? &this->m_layers[layer] : &this->m_settings;
        Config::Parser::storeInto(target, name, section, entry, value);
    } while (++layer < this->m_layerEnd);
}

/*!
    Reads @a lines from @a origin into the layers that m_layerBegin and
    m_layerEnd name, reporting errors in @a error: an `.ini` text when its
    first line starts a section, else one `<section>.<key>=<value>`
    assignment per line.
*/
bool Config::LayeredParser::apply(const Lines &lines, const QString &origin,
                                  QString *error)
{
    for (int i = 0; i < lines.m_lines.size(); ++i) {
        const QByteArray line = lines.m_lines.at(i).trimmed();
        if (line.isEmpty()) {
            continue;
        }
        if (line.startsWith('[')) {
            return this->readData(lines.text(), origin, error);
        }
        break;
    }
    for (int i = 0; i < lines.m_lines.size(); ++i) {
        const QByteArray line = lines.m_lines.at(i).trimmed();
        if ( ! line.isEmpty()
            && ! this->assign(QString::fromUtf8(line), error)
        ) {
            *error = origin + QLL(": ") + *error;
            return false;
        }
    }
    return true;
}

/*!
    Adds permanent @a lines from @a origin, reporting errors in @a error.

    They come after every scope open so far; hence they change the
    permanent layer and each scope's alike, read once, and a scope's value
    of a key they set gives way to theirs. The layers stay as they were on
    an error.
*/
bool Config::LayeredParser::addLines(
        const Lines &lines,
        const QString &origin, QString *error)
{
    const QList<Settings > layers = this->m_layers;
    const Settings settings = this->m_settings;
    this->m_layerBegin = 0;
    const bool applied = this->apply(lines, origin, error);
    this->m_layerBegin = this->m_layers.size();
    if ( ! applied) {
        this->m_layers = layers;
        this->m_settings = settings;
    }
    return applied;
}

/*!
    Adds scoped @a lines from @a origin, reporting errors in @a error: the
    settings so far go below as a layer of their own, and only the scope's
    layer reads the lines; the layers stay as they were on an error.
*/
bool Config::LayeredParser::pushLines(
        const Lines &lines,
        const QString &origin, QString *error)
{
    this->m_layers.append(this->m_settings);
    this->m_layerBegin = this->m_layers.size();
    this->m_layerEnd = this->m_layerBegin + 1;
    if ( ! this->apply(lines, origin, error)) {
        this->m_settings = this->m_layers.takeLast();
        this->m_layerBegin = this->m_layers.size();
        this->m_layerEnd = this->m_layerBegin + 1;
        return false;
    }
    return true;
}

/*!
    Removes the innermost scope, reporting errors in @a error: its layer
    goes, and the settings of the layer below it count again.
*/
bool Config::LayeredParser::popLines(QString *error)
{
    if (this->m_layers.isEmpty()) {
        *error = QString(QLL("no scoped settings layer"));
        return false;
    }
    this->m_settings = this->m_layers.takeLast();
    this->m_layerBegin = this->m_layers.size();
    this->m_layerEnd = this->m_layerBegin + 1;
    return true;
}

/*!
    Reads the files named @a fileName that apply to @a path (see
    Config::Scanner::levelFiles()) over the settings so far, from the
    root's level down.
*/
bool Config::Parser::readLevels(const QString & path, const QString & fileName,
                                QString *error)
{
    const QStringList files = Config::Scanner::levelFiles(path, fileName);
    for (int i = 0; i < files.size(); ++i) {
        if ( ! this->readFile(files.at(i), error)) {
            return false;
        }
    }
    return true;
}

/*!
    Reads the assignment @a assignment (`<section>.<key>=<value>`, the name
    in any case) over the settings so far; returns false, with @a error, for
    a name or a value the schema does not take.

    A value in double quotes takes the escapes of a C++ string literal (see
    Config::Escape); a value without quotes stays as it is, hence a regular
    expression's backslashes need no doubling there.
*/
bool Config::Parser::assign(const QString & assignment, QString *error)
{
    const int equals = assignment.indexOf(QLatin1Char('='));
    if (equals <= 0) {
        *error = QString(QLL("%1: not a <section>.<key>=<value> setting"))
            .arg(assignment);
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
        if ( ! Config::Escape::decode(
                value.mid(1, value.size() - 2),
                &value,
                &reason
            )
        ) {
            *error = name + QLL(": ") + reason;
            return false;
        }
    }
    return this->set(name, name, value, error);
}

/*!
    Builds the value of the setting @a name (in any case), after every file
    and assignment read so far; a key that extends another one and that
    nothing overrode yet takes that key's value (see Config::Key::extends).
*/
QString Config::Parser::value(const QString & name) const
{
    const QString lower = name.toLower();
    QHash<QString, QString >::const_iterator found =
        this->m_settings.values.constFind(lower);
    if (found != this->m_settings.values.constEnd()) {
        return found.value();
    }
    const Config::Key key = this->m_schema.key(lower);
    if ( ! key.extends.isEmpty()) {
        return this->value(key.extends);
    }
    return QString();
}

/*!
    Builds the keys, with their values, that files and assignments gave the
    open @a section (in any case, see Config::Section::setOpen()): each in
    its own spelling, in the order they came, a key given twice included.
    Hence a tool can read a line in the light of the ones before it.
*/
QList<QPair<QString, QString > > Config::Parser::openEntries(
        const QString & section) const
{
    return this->m_settings.openEntries.value(section.toLower());
}

/*!
    Stores @a value as the setting @a name, a key the schema checked, and,
    for a key of the open @a section (else empty), as its @a entry there
    (see openEntries()); a Config::LayeredParser stores it in each layer a
    read aims at.
*/
void Config::Parser::store(const QString & name, const QString & section,
                           const QString & entry, const QString & value)
{
    Config::Parser::storeInto(&this->m_settings, name, section, entry, value);
}

/*!
    Stores @a value into @a settings, the way store() does.
*/
void Config::Parser::storeInto(Settings *settings, const QString & name,
                               const QString & section, const QString & entry,
                               const QString & value)
{
    settings->values.insert(name, value);
    if ( ! section.isEmpty()) {
        settings->openEntries[section].append(qMakePair(entry, value));
    }
}

/*!
    Sets the setting @a name (`<section>.<key>`, in any case) to @a value;
    returns false, with @a error naming the setting as @a shown and listing
    the values it takes, for a name or a value the schema does not take.
*/
bool Config::Parser::set(
        const QString & name, const QString & shown, const QString & value,
        QString *error)
{
    const Config::Key key = this->m_schema.key(name);
    if (key.isNull()) {
        if (this->m_schema.sectionOf(name).isNull()) {
            *error = QString(QLL("unknown section of setting %1")).arg(shown);
        } else {
            *error = QString(QLL("unknown setting %1")).arg(shown);
        }
        return false;
    }
    QString matched;
    if (Config::Parser::matchValue(key, value, &matched)) {
        QString section;
        if (key.open) {
            section = this->m_schema.sectionOf(name).name().toLower();
        }
        this->store(key.name, section, name.mid(section.size() + 1), matched);
        return true;
    }
    QStringList choices;
    for (int i = 0; i < key.matchers.size(); ++i) {
        choices << key.matchers.at(i)->describe();
    }
    *error = QString(QLL("%1 = %2: use %3")).arg(
        shown,
        value,
        choices.join(QString(QLL(" or ")))
    );
    return false;
}

/*!
    Matches @a value (in any of its alias spellings) against the matchers of
    @a key, and gives @a matched the value to keep; a
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
    if (Config::RegExp::split(resolved, &head, &fallback, &hasFallback)
        && hasFallback
    ) {
        QString nestedHead;
        QString nestedFallback;
        bool nested = false;
        QString matchedHead;
        QString matchedFallback;
        if (Config::RegExp::split(
                fallback,
                &nestedHead,
                &nestedFallback,
                &nested
            )
            || ! Config::Parser::matchValue(key, head, &matchedHead)
            || ! Config::Parser::matchValue(
                key,
                fallback,
                &matchedFallback
            )
        ) {
            return false;
        }
        *matched = matchedHead + QLL(" |");
        if ( ! matchedFallback.isEmpty()) {
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
