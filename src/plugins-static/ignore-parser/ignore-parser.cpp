// Copyright (C) 2015 The XD Company Ltd.
// SPDX-License-Identifier: Apache-2.0

#include "./ignore-parser.h"

#include "config-scanner.h"

#include <QtCore/QDir>
#include <QtCore/QFile>
#include <QtCore/QFileInfo>

/*!
    @class IgnoreParser
    @internal

    Tells which files and folders the ignore files of a project leave out,
    the way git reads its `.gitignore` files.

    The project's root is the folder that holds the closest `.git` above a path
    (see Config::Scanner::findRoot()); the ignore files are read from that
    root down to the path's own folder, in that order, and the last rule
    that matches decides. Hence a folder's ignore file overrides the
    ones of the folders above it. In each folder, the ignore files are read
    in the order of ignoreFileNames(): `.gitignore` first, then each name a
    tool appends (such as `.formatignore`), whose rules then override
    the earlier files'.

    The `.git` folder is always left out. A path added with
    addExplicitPath() works as a `!/<path>` rule after all the others: it is
    never left out, and nor is any folder on the way to it while that path
    (or the content of a named folder) is checked; the other content of such
    a folder still follows the rules.
*/

/*!
    Builds the parser that reads `.gitignore` files only.
*/
IgnoreParser::IgnoreParser()
{
    this->m_fileNames.append(QString(QLL(".gitignore")));
}

/*!
    Appends @a name to the ignore files that each folder may hold, after the
    ones before it; hence its rules override theirs.
*/
void IgnoreParser::addIgnoreFileName(const QString & name)
{
    this->m_fileNames.append(name);
}

/*!
    Keeps @a path from being left out, and every folder on the way to it, as
    a `!/<path>` rule after all the rules of the ignore files would.
*/
void IgnoreParser::addExplicitPath(const QString & path)
{
    this->m_explicitPaths.append(QDir::cleanPath(QFileInfo(path).absoluteFilePath()));
}

/*!
    Builds the regular expression of the glob @a glob, the way git reads a
    pattern: `*` and `?` stop at a `/`, `**` crosses folders, `[...]` is a
    character class, and a backslash takes the next character as is.
*/
static QString globToRegex(const QString & glob)
{
    QString regex;
    for (int i = 0; i < glob.size(); ++i) {
        const QChar c = glob.at(i);
        if (c == QLatin1Char('\\') && i + 1 < glob.size()) {
            regex += QRegularExpression::escape(QString(glob.at(++i)));
        } else if (c == QLatin1Char('*')
            && i + 1 < glob.size()
            && glob.at(i + 1) == QLatin1Char('*')
        ) {
            const bool atStart = i == 0 || glob.at(i - 1) == QLatin1Char('/');
            const bool atEnd = i + 2 >= glob.size();
            const bool beforeSlash = ! atEnd
                && glob.at(i + 2) == QLatin1Char('/');
            if (atStart && beforeSlash) {
                // Takes `**/` as zero or more folders.
                regex += QLL("(?:.*/)?");
                i += 2;
            } else {
                regex += QLL(".*");
                ++i;
            }
        } else if (c == QLatin1Char('*')) {
            regex += QLL("[^/]*");
        } else if (c == QLatin1Char('?')) {
            regex += QLL("[^/]");
        } else if (c == QLatin1Char('[')) {
            const int close = glob.indexOf(QLatin1Char(']'), i + 2);
            if (close < 0) {
                regex += QLL("\\[");
                continue;
            }
            QString range = glob.mid(i + 1, close - i - 1);
            if (range.startsWith(QLatin1Char('!'))) {
                range[0] = QLatin1Char('^');
            }
            regex += QLatin1Char('[') + range.replace(
                QLL("\\"),
                QLL("\\\\")
            )
                + QLatin1Char(']');
            i = close;
        } else {
            regex += QRegularExpression::escape(QString(c));
        }
    }
    return regex;
}

/*!
    Reads @a line of an ignore file found in the folder @a base into @a rule,
    and returns false for a line that holds no rule (a blank line, or a
    comment that starts with `#`).
*/
bool IgnoreParser::parseRule(const QString & line, const QString & base,
                             IgnoreRule *rule)
{
    QString text = line;
    if (text.endsWith(QLatin1Char('\r'))) {
        text.chop(1);
    }

    // Drops the trailing spaces that no backslash keeps.
    while (text.endsWith(QLatin1Char(' '))
        && ! text.endsWith(QLL("\\ "))
    ) {
        text.chop(1);
    }
    if (text.isEmpty() || text.startsWith(QLatin1Char('#'))) {
        return false;
    }
    rule->base = base;
    rule->text = text;
    rule->negated = text.startsWith(QLatin1Char('!'));
    if (rule->negated) {
        text.remove(0, 1);
    } else if (text.startsWith(QLL("\\!"))
        || text.startsWith(QLL("\\#"))
    ) {
        text.remove(0, 1);
    }
    rule->directoryOnly = text.endsWith(QLatin1Char('/'));
    if (rule->directoryOnly) {
        text.chop(1);
    }
    if (text.isEmpty()) {
        return false;
    }

    // Anchors a pattern that holds a `/` to its folder; any other pattern
    // matches a name at any depth below it.
    const bool anchored = text.contains(QLatin1Char('/'));
    if (text.startsWith(QLatin1Char('/'))) {
        text.remove(0, 1);
    }
    const QString body = globToRegex(text);
    rule->pattern.setPattern(anchored ? QLL("^") + body + QLatin1Char('$')
                                      : QLL("(?:^|/)") + body + QLatin1Char('$'));
    return true;
}

/*!
    Tells whether the ignore files leave out @a path, a folder when
    @a isDirectory is set: either the path itself, or a folder on the way to
    it from its project's root.
*/
bool IgnoreParser::isIgnored(const QString & path, bool isDirectory)
{
    const QString absolute = QDir::cleanPath(QFileInfo(path).absoluteFilePath());
    if (this->isOnExplicitPath(absolute)) {
        return false;
    }
    const QString root = this->rootOf(isDirectory ? absolute : QFileInfo(absolute).absolutePath());
    if (absolute == root
        || ! absolute.startsWith(root + QLatin1Char('/'))
    ) {
        return false;
    }

    // Checks the folders on the way down first; the content of a folder
    // that is left out is left out too. A folder on the way to a named path
    // stays open only for that path and for the content of a named folder,
    // never for its other content.
    const bool insideNamed = this->isInsideExplicitPath(absolute);
    const QStringList folders = Config::Scanner::levelFolders(
        root,
        absolute
    );
    for (int i = 1; i < folders.size(); ++i) {
        const QString & current = folders.at(i);
        if (insideNamed && this->isOnExplicitPath(current)) {
            continue;
        }
        if (this->matchesLast(root, current, true)) {
            return true;
        }
    }
    return this->matchesLast(root, absolute, isDirectory);
}

/*!
    Builds the rule (as `<ignore file>:<line>: <rule>`) that leaves out
    @a path, a folder when @a isDirectory is set, as the ignore files alone
    decide it (the paths added with addExplicitPath() do not count): the
    rule that leaves out a folder on the way to it, or else the path's own;
    an empty string when the ignore files keep it.
*/
QString IgnoreParser::excludingRule(const QString & path, bool isDirectory)
{
    const QString absolute = QDir::cleanPath(QFileInfo(path).absoluteFilePath());
    const QString root = this->rootOf(isDirectory ? absolute : QFileInfo(absolute).absolutePath());
    if (absolute == root
        || ! absolute.startsWith(root + QLatin1Char('/'))
    ) {
        return QString();
    }
    QStringList paths = Config::Scanner::levelFolders(root, absolute);
    paths.removeFirst();
    paths << absolute;
    for (int i = 0; i < paths.size(); ++i) {
        const QString & current = paths.at(i);
        const bool last = i + 1 == paths.size();
        const IgnoreRule *decider = Q_NULLPTR;
        if ( ! this->matchesLast(
                root,
                current,
                last ? isDirectory : true,
                &decider
            )
        ) {
            continue;
        }
        if (decider == Q_NULLPTR) {
            return QDir::toNativeSeparators(current) + QLL(": the .git folder");
        }
        return QString(QLL("%1:%2: %3")).arg(QDir::toNativeSeparators(decider->file))
            .arg(decider->line).arg(decider->text);
    }
    return QString();
}

/*!
    Builds the project root of @a folder, found once and kept.
*/
QString IgnoreParser::rootOf(const QString & folder)
{
    QString root = this->m_roots.value(folder);
    if (root.isEmpty()) {
        root = Config::Scanner::findRoot(folder);
        this->m_roots.insert(folder, root);
    }
    return root;
}

/*!
    Tells whether the last rule that matches @a path (a folder when
    @a isDirectory is set) leaves it out, reading the ignore files from
    @a root down to the path's folder; @a decider, when given, gets that
    rule (or null when only the `.git` folder rule left it out).
*/
bool IgnoreParser::matchesLast(
        const QString & root, const QString & path, bool isDirectory,
        const IgnoreRule **decider)
{
    // Leaves out `.git` before any rule, as git does.
    const QString name = path.mid(path.lastIndexOf(QLatin1Char('/')) + 1);
    bool ignored = name == QLL(".git");

    const QStringList folders = Config::Scanner::levelFolders(root, path);
    for (int i = 0; i < folders.size(); ++i) {
        const QString & folder = folders.at(i);
        for (int f = 0; f < this->m_fileNames.size(); ++f) {
            const QList<IgnoreRule > & rules = this->rulesOf(folder + QLatin1Char('/')
                                                            + this->m_fileNames.at(f));
            const QString relative = path.mid(folder.size() + 1);
            for (int r = 0; r < rules.size(); ++r) {
                const IgnoreRule & rule = rules.at(r);
                if (( ! rule.directoryOnly || isDirectory)
                    && rule.pattern.match(relative).hasMatch()
                ) {
                    ignored = ! rule.negated;
                    if (decider != Q_NULLPTR) {
                        *decider = &rule;
                    }
                }
            }
        }
    }
    return ignored;
}

/*!
    Builds the rules of the ignore file at @a file, read once and kept for
    the parser's life; a missing file has none.
*/
const QList<IgnoreRule > &IgnoreParser::rulesOf(const QString & file)
{
    QHash<QString, QList<IgnoreRule > >::const_iterator found = this->m_rules.constFind(file);
    if (found != this->m_rules.constEnd()) {
        return found.value();
    }
    QList<IgnoreRule > rules;
    QFile input(file);
    if (input.open(QIODevice::ReadOnly)) {
        const QString base = QFileInfo(file).absolutePath();
        const QStringList lines = QString::fromUtf8(input.readAll()).split(QLatin1Char('\n'));
        for (int i = 0; i < lines.size(); ++i) {
            IgnoreRule rule;
            if (IgnoreParser::parseRule(lines.at(i), base, &rule)) {
                rule.file = file;
                rule.line = i + 1;
                rules.append(rule);
            }
        }
    }
    return this->m_rules.insert(file, rules).value();
}

/*!
    Tells whether @a path is a path added with addExplicitPath(), or a folder
    on the way to one.
*/
bool IgnoreParser::isOnExplicitPath(const QString & path) const
{
    for (int i = 0; i < this->m_explicitPaths.size(); ++i) {
        const QString & explicitPath = this->m_explicitPaths.at(i);
        if (explicitPath == path
            || explicitPath.startsWith(path + QLatin1Char('/'))
        ) {
            return true;
        }
    }
    return false;
}

/*!
    Tells whether @a path lies inside a folder added with addExplicitPath().
*/
bool IgnoreParser::isInsideExplicitPath(const QString & path) const
{
    for (int i = 0; i < this->m_explicitPaths.size(); ++i) {
        if (path.startsWith(this->m_explicitPaths.at(i) + QLatin1Char('/'))) {
            return true;
        }
    }
    return false;
}

/*!
    Lists, sorted, every file under @a directory whose name matches one of
    @a nameFilters (wildcards, such as `*.cpp`), leaving out what the ignore
    files leave out; a folder that is left out is not entered.
*/
QStringList IgnoreParser::findFiles(
        const QString & directory, const QStringList & nameFilters)
{
    QStringList files;
    QSet<QString > visited;
    this->collect(
        QDir::cleanPath(QFileInfo(directory).absoluteFilePath()),
        nameFilters,
        &visited,
        &files
    );
    files.sort();
    return files;
}

/*!
    Appends to @a files the matching files under @a directory, entering each
    folder the ignore files keep; @a visited holds the real paths of the
    folders entered, so that a symbolic link back up is entered once.
*/
void IgnoreParser::collect(
        const QString & directory, const QStringList & nameFilters,
        QSet<QString > *visited, QStringList *files)
{
    const QString real = QFileInfo(directory).canonicalFilePath();
    if (real.isEmpty() || visited->contains(real)) {
        return;
    }
    visited->insert(real);
    const QDir dir(directory);
    const QFileInfoList entries = dir.entryInfoList(
        QDir::AllEntries | QDir::Hidden
            | QDir::System | QDir::NoDotAndDotDot,
        QDir::Name
    );
    for (int i = 0; i < entries.size(); ++i) {
        const QFileInfo & entry = entries.at(i);
        const QString path = QDir::cleanPath(entry.absoluteFilePath());
        if (entry.isDir()) {
            if ( ! this->isIgnored(path, true)) {
                this->collect(path, nameFilters, visited, files);
            }
            continue;
        }
        if (QDir::match(nameFilters, entry.fileName())
            && ! this->isIgnored(path, false)
        ) {
            files->append(path);
        }
    }
}
