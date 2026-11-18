// Copyright (C) 2015 The XD Company Ltd.
// SPDX-License-Identifier: Apache-2.0

#include "./config-parser.h"

#include "./config-escape.h"
#include "./config-regexp.h"
#include "./config-scanner.h"

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
    Tells whether the trimmed line @a line holds nothing to read: it is empty,
    or a comment, which starts with `;` or `#`.
*/
static bool isBlankOrComment(const QByteArray & line)
{
    return line.isEmpty() || line.startsWith(';') || line.startsWith('#');
}

/*!
    Describes what one scan of an `.ini` line finds outside of double quotes,
    where a `\` takes the character after it along (see scanLine()).
*/
struct LineScan
{
    /// Places the line's first `=`, or -1 for a line without one.
    int equals;
    /// Places the `;` that starts the line's comment, else the line's size.
    int comment;
    /// Tells that a value in double quotes is still open at the line's end.
    bool inQuotes;
    /// Tells that the line ends in a `\`, alone or before its line feed.
    bool escapedEnd;
};

/*!
    Scans the `.ini` line @a line for its `=`, its comment, an open value in
    double quotes, and an escaped line break; hence each reader of a line
    takes them the same way.
*/
static LineScan scanLine(const QByteArray & line)
{
    LineScan scan = { -1, int(line.size()), false, false };
    for (int i = 0; i < line.size(); ++i) {
        const char c = line.at(i);
        if (c == '\\') {
            if (i + 1 >= line.size()
                || (line.at(i + 1) == '\n' && i + 2 >= line.size())
            ) {
                scan.escapedEnd = true;
                break;
            }
            ++i;
        } else if (c == '"') {
            scan.inQuotes = ! scan.inQuotes;
        } else if (c == '=' && ! scan.inQuotes && scan.equals < 0) {
            scan.equals = i;
        } else if (c == ';' && ! scan.inQuotes) {
            scan.comment = i;
            break;
        }
    }
    return scan;
}

/*!
    Tells whether the `.ini` line @a line goes on in the next one, as `.ini`
    files take it: a value in double quotes goes on past a line break, and
    so does a line that ends in a `\` (an escaped line break). A `;` outside
    double quotes starts a comment, which ends the line, and a line that
    starts with `;` or `#` is a comment as a whole, which never goes on.
*/
static bool continuesOnNextLine(const QByteArray & line)
{
    if (isBlankOrComment(line.trimmed())) {
        return false;
    }
    const LineScan scan = scanLine(line);
    return scan.escapedEnd
        || (scan.comment == line.size() && scan.inQuotes);
}

/*!
    Builds the key or section name @a text of an `.ini` line, as `.ini`
    files spell one, in UTF-8: a `\` stands for a `/`, and `%<hex><hex>` or
    `%U<hex><hex><hex><hex>` for the character of that code.
*/
static QString iniKey(const QByteArray & text)
{
    QString key;
    QByteArray run;
    for (int i = 0; i < text.size(); ++i) {
        const char c = text.at(i);
        if (c == '\\') {
            run += '/';
            continue;
        }
        if (c != '%' || i + 1 >= text.size()) {
            run += c;
            continue;
        }

        // Takes a `%` code only when each of its characters is a hexadecimal
        // digit, with no sign, prefix, or space; else the `%` stays as it is.
        const bool wide = text.at(i + 1) == 'U';
        const int digits = wide ? 4 : 2;
        const int first = i + (wide ? 2 : 1);
        bool ok = first + digits <= text.size();
        for (int d = first; ok && d < first + digits; ++d) {
            ok = Config::Escape::digitOf(QLatin1Char(text.at(d)), 16) >= 0;
        }
        if ( ! ok) {
            run += '%';
            continue;
        }
        key += QString::fromUtf8(run);
        run.clear();
        key += QChar(text.mid(first, digits).toInt(Q_NULLPTR, 16));
        i = first + digits - 1;
    }
    return key + QString::fromUtf8(run);
}

/*!
    Builds the value @a text of an `.ini` line (the bytes after its `=`),
    as `.ini` files spell one: the spaces around it drop out, a part in
    double quotes keeps its quotes and its text as written (see readLine(),
    which decodes a value all in quotes), and elsewhere a `\` starts an
    escape: `\a`, `\b`, `\f`, `\n`, `\r`, `\t`, `\v`, `\"`, `\?`, `\'`,
    `\\`, `\x<hex>...`, `\<octal>...`, an escaped line break (which drops
    out), or any other character (which drops out with it). A comma stays a
    plain character, and a value that starts with `@@` loses one `@`.
*/
static QString iniValue(const QByteArray & text)
{
    QString value;
    QByteArray run;
    bool inQuotes = false;
    int chopLimit = 0;
    int i = 0;
    while (i < text.size() && (text.at(i) == ' ' || text.at(i) == '\t')) {
        ++i;
    }
    while (i < text.size()) {
        const char c = text.at(i++);

        // Keeps a quoted part as written, quotes and escapes included.
        if (inQuotes || c == '"') {
            run += c;
            if (c == '"') {
                inQuotes = ! inQuotes;
            } else if (c == '\\' && i < text.size()) {
                run += text.at(i++);
            }
            continue;
        }
        if (c != '\\') {
            run += c;
            continue;
        }
        value += QString::fromUtf8(run);
        run.clear();
        if (i >= text.size()) {
            break;
        }
        const char escaped = text.at(i++);

        // Drops the whole character that a `\` stands before, all of its
        // UTF-8 bytes.
        if (uchar(escaped) >= 0x80) {
            while (i < text.size() && (uchar(text.at(i)) & 0xC0) == 0x80) {
                ++i;
            }
            chopLimit = value.size();
            continue;
        }
        char simple = 0;
        if (Config::Escape::simple(escaped, &simple)) {
            value += QLatin1Char(simple);
            chopLimit = value.size();
            continue;
        }
        const bool isHex = escaped == 'x';
        if (isHex
            || Config::Escape::digitOf(QLatin1Char(escaped), 8) >= 0
        ) {
            // Reads every digit, an octal escape's first one included. A value
            // past U+10FFFF, which names no character, becomes U+FFFD.
            const int from = isHex ? i : i - 1;
            uint codePoint = 0;
            int read = 0;
            if (Config::Escape::readDigits(
                QString::fromLatin1(text),
                from,
                isHex ? 16 : 8,
                0,
                &codePoint,
                &read
            )) {
                Config::Escape::appendCodePoint(
                    &value,
                    Config::Escape::isCodePoint(codePoint) ? codePoint : 0xFFFD
                );
            }
            i = from + read;
        }
        chopLimit = value.size();
    }
    value += QString::fromUtf8(run);

    // Drops the spaces at the end of the value.
    int end = value.size();
    while (end > chopLimit
        && (value.at(end - 1) == QLatin1Char(' ') || value.at(end - 1) == QLatin1Char('\t'))
    ) {
        --end;
    }
    value.truncate(end);
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
    Reads the `.ini` text @a data over the settings so far, line by line,
    naming @a origin in an error; returns false for a key or a value that
    the schema does not take.

    A line ends in a line feed, a carriage return and a line feed, or a lone
    carriage return alike. A line goes on in the next one only where `.ini`
    files take it so (see continuesOnNextLine()). A value keeps its commas
    as plain characters, with the spaces after them, whether or not it
    stands in double quotes.
*/
bool Config::Parser::readData(const QByteArray &data, const QString &origin,
                              QString *error)
{
    // Leaves out the byte order mark that some editors write first.
    QByteArray text = data.startsWith("\xEF\xBB\xBF") ? data.mid(3) : data;
    text.replace("\r\n", "\n");
    text.replace('\r', '\n');
    QString section;
    QByteArray line;
    int from = 0;
    while (from < text.size()) {
        // Takes the next line onto the ones the line before goes on in.
        int next = text.indexOf('\n', from);
        next = next < 0 ? text.size() : next + 1;
        line += text.mid(from, next - from);
        from = next;
        if (continuesOnNextLine(line) && from < text.size()) {
            continue;
        }
        if ( ! this->readLine(line, &section, origin, error)) {
            return false;
        }
        line.clear();
    }
    return true;
}

/*!
    Reads the `.ini` text of @a device over the settings so far, as the
    readData() that reads a text does.
*/
bool Config::Parser::readData(QIODevice *device, const QString &origin,
                              QString *error)
{
    return this->readData(device->readAll(), origin, error);
}

/*!
    Reads the `.ini` line @a line, under the section @a section (empty for
    the `[General]` one, which a section line in @a line changes), naming
    @a origin in an error.

    A key of an open section (see Config::Section::setOpen()) keeps its own
    spelling and each line it is given on, in order; any other key takes
    its value the way `.ini` files spell one (see iniValue()), and the
    latest line of a key wins. A value all in double quotes takes the
    escapes of a C++ string literal, as an assignment's does.

    A line that starts with `;` or `#` is a comment, and a `;` outside
    double quotes starts one, for every key. Any other line that is no
    section line and has no `<key> = <value>` is an error.
*/
bool Config::Parser::readLine(const QByteArray & line, QString *section,
                              const QString & origin, QString *error)
{
    // Finds the line's `=` and its comment, outside double quotes.
    const QByteArray text = line.trimmed();
    if (isBlankOrComment(text)) {
        return true;
    }
    const LineScan scan = scanLine(text);
    const int equals = scan.equals;
    const int end = scan.comment;

    // Rejects a line that is neither a whole section line nor a
    // `<key> = <value>` one.
    const bool isSection = text.startsWith('[');
    const int close = isSection ? text.indexOf(']') : -1;
    const bool isBad = isSection
        ? close < 0
            || close > end
            || ! text.mid(close + 1, end - close - 1).trimmed().isEmpty()
        : equals <= 0;
    if (isBad) {
        *error = QString(QLL("%1: not a %2 line: ")).arg(
            origin,
            isSection ? QString(QLL("[<section>]"))
                      : QString(QLL("<key> = <value>"))
        ) + QString::fromUtf8(text.left(end).trimmed());
        return false;
    }

    // Starts a section.
    if (isSection) {
        *section = iniKey(text.mid(1, close - 1).trimmed());
        if (section->compare(QLL("General"), Qt::CaseInsensitive) == 0) {
            section->clear();
        }
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
    const QByteArray valueText = text.mid(equals + 1, end - equals - 1);
    QString value = this->m_schema.key(name).open
        ? QString::fromUtf8(valueText).trimmed()
        : iniValue(valueText);

    // Decodes a value all in double quotes.
    QString reason;
    if ( ! Config::Escape::unquote(value, &value, &reason)) {
        *error = origin + QLL(": ") + shown + QLL(": ") + reason;
        return false;
    }
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
    first line that is no comment starts a section, else one
    `<section>.<key>=<value>` assignment per line, past its comments.
*/
bool Config::LayeredParser::apply(const Lines &lines, const QString &origin,
                                  QString *error)
{
    for (int i = 0; i < lines.m_lines.size(); ++i) {
        const QByteArray line = lines.m_lines.at(i).trimmed();
        if (isBlankOrComment(line)) {
            continue;
        }
        if (line.startsWith('[')) {
            return this->readData(lines.text(), origin, error);
        }
        break;
    }
    for (int i = 0; i < lines.m_lines.size(); ++i) {
        // Cuts the line at its comment, as `.ini` mode does.
        QByteArray line = lines.m_lines.at(i).trimmed();
        line = line.left(scanLine(line).comment).trimmed();
        if ( ! isBlankOrComment(line)
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
    QString reason;
    if ( ! Config::Escape::unquote(value, &value, &reason)) {
        *error = name + QLL(": ") + reason;
        return false;
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
    QString reason;
    for (int i = 0; i < key.matchers.size(); ++i) {
        choices << key.matchers.at(i)->describe();
        if (reason.isEmpty()) {
            reason = key.matchers.at(i)->explain(value);
        }
    }
    *error = QString(QLL("%1 = %2: use %3%4")).arg(
        shown,
        value,
        choices.join(QString(QLL(" or "))),
        reason.isEmpty() ? QString() : QString(QLL("; ")) + reason
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
