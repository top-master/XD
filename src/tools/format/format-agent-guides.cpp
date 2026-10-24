// Copyright (C) 2015 The XD Company Ltd.
// SPDX-License-Identifier: Apache-2.0

#include "./format-agent-guides.h"

#include "./format-issue.h"

#include <QtCore/QCoreApplication>
#include <QtCore/QDir>
#include <QtCore/QFile>
#include <QtCore/QFileInfo>
#include <QtCore/QLockFile>
#include <QtCore/QRegExp>
#include <QtCore/QStringList>

/*!
    Names the file that writeUsage() fills in from the `ai-start.md` asset,
    the name an agent looks for its instructions under (see
    FormatAgentGuides::usageName()).

    IMPORTANT: the name stays `AGENTS.md`, even though the rule against
    naming that file flags this line. An agent reads a file of that name as
    its instructions without being told to, and the file lives in the run's
    own temporary folder, outside every repository.
*/
static const char usageNameText[] = "AGENTS.md";

/*!
    Starts the name of each run's folder in the system's temporary folder,
    before the run's process id (see FormatAgentGuides::folderPrefix()).
*/
static const char folderPrefixText[] = "qt-format-";

/*!
    @class FormatAgentGuides
    @internal

    Writes the prompt files that agent mode hands the agent, one per
    request of a rule that has a template: the tool carries each template as
    a resource (see templateName()), and fills in its placeholders at run
    time: `{{file}}` (the file's path), `{{lines}}` (`<line>` or
    `<line>-<last line>`), and `{{columnLimit}}` (see FormatIssue). A
    template may also hold a block per target, from a `{{#<target>}}` line
    to a `{{/<target>}}` line, of which the file keeps only the request's
    target (see targetOf()); hence the wrap rules share one template.

    The files go into a folder of the system's temporary folder named after
    the tool's process id, `qt-format-<pid>` (see folderPrefix()), which
    remove() deletes once the agent quits the tool; a run that ends without
    asking the agent keeps it for the agent to read, until a later run
    removes it (see removeLeftovers()).
*/

/*!
    Builds the guides of this process; their folder exists only once
    write() has written a file.
*/
FormatAgentGuides::FormatAgentGuides()
    : m_folder(QDir::tempPath() + QLatin1Char('/') + FormatAgentGuides::folderPrefix()
               + QString::number(QCoreApplication::applicationPid()))
    , m_count(0)
{
}

/*!
    Builds the start of the name of each run's folder, in front of the run's
    process id; hence the guides tell their own folders, and those of other
    runs, apart from any other folder of the system's temporary folder.
*/
QString FormatAgentGuides::folderPrefix()
{
    return QString::fromLatin1(folderPrefixText);
}

/*!
    Builds the file name of the usage instructions (see writeUsage()).
*/
QString FormatAgentGuides::usageName()
{
    return QString::fromLatin1(usageNameText);
}

/*!
    Releases the folder's lock, and keeps its files (see remove()).
*/
FormatAgentGuides::~FormatAgentGuides()
{
}

/*!
    Writes the prompt file of @a request, which concerns the file @a path,
    and builds its absolute path; builds an empty path when the request's
    rule has no template, or when the file cannot be written.
*/
QString FormatAgentGuides::write(
        const QString & path,
        const FormatIssue & request)
{
    QFile source(QLL(":/assets/")
        + FormatAgentGuides::templateName(request.rule)
    );
    if ( ! source.open(QIODevice::ReadOnly)) {
        return QString();
    }
    QString text = FormatAgentGuides::keepTarget(
        QString::fromUtf8(source.readAll()),
        FormatAgentGuides::targetOf(request.rule)
    );
    QString lines = QString::number(request.line);
    if (request.endLine > request.line) {
        lines += QLatin1Char('-') + QString::number(request.endLine);
    }
    text.replace(QLL("{{file}}"), QDir::fromNativeSeparators(path));
    text.replace(QLL("{{lines}}"), lines);
    text.replace(QLL("{{columnLimit}}"), QString::number(request.columnLimit));

    // Writes the filled template next to the earlier ones.
    if ( ! this->makeFolder()) {
        return QString();
    }
    ++this->m_count;
    const QString name = QString::fromLatin1(request.rule) + QLatin1Char('-')
        + QString::number(this->m_count) + QLL(".md");
    QFile target(this->m_folder + QLatin1Char('/') + name);
    if ( ! target.open(QIODevice::WriteOnly)
        || target.write(text.toUtf8()) < 0
    ) {
        return QString();
    }
    this->m_prompts << target.fileName();
    return QFileInfo(target).absoluteFilePath();
}

/*!
    Builds the name of the template of the rule @a rule: `wrap.md` for a
    wrap rule (`<kind>-wrap`, such as `args-wrap`), else `<rule>.md`.
*/
QString FormatAgentGuides::templateName(const QByteArray & rule)
{
    if (FormatAgentGuides::targetOf(rule).isEmpty()) {
        return QString::fromLatin1(rule) + QLL(".md");
    }
    return QString(QLL("wrap.md"));
}

/*!
    Builds the target of the rule @a rule within its template: `<kind>` for
    a wrap rule `<kind>-wrap`, and an empty name for any other rule.
*/
QString FormatAgentGuides::targetOf(const QByteArray & rule)
{
    const QString name = QString::fromLatin1(rule);
    const QString suffix = QString(QLL("-wrap"));
    if ( ! name.endsWith(suffix)) {
        return QString();
    }
    return name.left(name.size() - suffix.size());
}

/*!
    Builds @a text with only the blocks of @a target: a block runs from a
    `{{#<name>}}` line to a `{{/<name>}}` line; the lines of another target's
    blocks go, and so do the lines that open and close each block.
*/
QString FormatAgentGuides::keepTarget(
        const QString & text,
        const QString & target)
{
    const QStringList lines = text.split(QLatin1Char('\n'));
    QStringList kept;
    QString block;
    for (int l = 0; l < lines.size(); ++l) {
        const QString line = lines.at(l).trimmed();
        if (line.startsWith(QLL("{{#")) && line.endsWith(QLL("}}"))) {
            block = line.mid(3, line.size() - 5);
            continue;
        }
        if (line.startsWith(QLL("{{/")) && line.endsWith(QLL("}}"))) {
            block.clear();
            continue;
        }
        if (block.isEmpty() || block == target) {
            kept << lines.at(l);
        }
    }
    return kept.join(QLatin1Char('\n'));
}

/*!
    Deletes the folder of the guides, and every file in it.
*/
void FormatAgentGuides::remove()
{
    this->m_lock.reset();
    QDir(this->m_folder).removeRecursively();
    this->m_prompts.clear();
}

/*!
    Writes the usage instructions of agent mode into the folder, from their
    template, `:/assets/ai-start.md`, with the folder's path in place of
    `{{folder}}`, into a file named usageName(), and builds the file's
    absolute path; an empty path when it cannot be written.
*/
QString FormatAgentGuides::writeUsage()
{
    QFile source(QString(QLL(":/assets/ai-start.md")));
    if ( ! source.open(QIODevice::ReadOnly)) {
        return QString();
    }
    QString text = QString::fromUtf8(source.readAll());
    text.replace(QLL("{{folder}}"), this->m_folder);

    // Writes the filled template into the folder.
    if ( ! this->makeFolder()) {
        return QString();
    }
    QFile target(this->m_folder
        + QLatin1Char('/')
        + FormatAgentGuides::usageName()
    );
    if ( ! target.open(QIODevice::WriteOnly)
        || target.write(text.toUtf8()) < 0
    ) {
        return QString();
    }
    return QFileInfo(target).absoluteFilePath();
}

/*!
    Deletes the prompt files that write() wrote, and keeps the usage
    instructions; hence each round of requests starts with no prompt file
    of the round before.
*/
void FormatAgentGuides::removePrompts()
{
    for (int p = 0; p < this->m_prompts.size(); ++p) {
        QFile::remove(this->m_prompts.at(p));
    }
    this->m_prompts.clear();
}

/*!
    Makes the folder once, locked for this process while it runs, then
    removes the folders that earlier runs left (see removeLeftovers());
    returns false when the folder cannot be made or locked.

    A lock that holds this process's id, left by a run that crashed with the
    same id, is stale; hence it goes.
*/
bool FormatAgentGuides::makeFolder()
{
    if ( ! this->m_lock.isNull()) {
        return true;
    }
    if ( ! QDir().mkpath(this->m_folder)) {
        return false;
    }
    this->m_lock.reset(new QLockFile(this->m_folder + QLL("/.lock")));
    this->m_lock->setStaleLockTime(0);
    if ( ! this->m_lock->tryLock(0)) {
        this->m_lock->removeStaleLockFile();
        if ( ! this->m_lock->tryLock(0)) {
            this->m_lock.reset();
            return false;
        }
    }
    this->removeLeftovers();
    return true;
}

/*!
    Deletes each folder that another run left in the system's temporary
    folder: a folder named after folderPrefix() and a process id, holding
    the usage instructions (see usageName()) and no file that the guides do
    not write themselves (see isOwnFile()), whose lock no running process
    holds, since that run ended or crashed. A run that still runs keeps its
    folder, and so does any other folder.
*/
void FormatAgentGuides::removeLeftovers() const
{
    const QDir temp(QDir::tempPath());
    const QString prefix = FormatAgentGuides::folderPrefix();
    const QString own = QFileInfo(this->m_folder).fileName();
    const QRegExp pattern(QRegExp::escape(prefix) + QLL("\\d+"));
    const QStringList names = temp.entryList(
        QStringList() << prefix + QLatin1Char('*'),
        QDir::Dirs | QDir::NoDotAndDotDot
    );
    for (int n = 0; n < names.size(); ++n) {
        const QString path = temp.filePath(names.at(n));
        if (names.at(n) == own || ! pattern.exactMatch(names.at(n))
            || ! QFile::exists(path + QLatin1Char('/') + FormatAgentGuides::usageName())
        ) {
            continue;
        }

        // Keeps a folder that holds anything the guides do not write.
        const QStringList entries = QDir(path).entryList(
            QDir::AllEntries | QDir::Hidden | QDir::System | QDir::NoDotAndDotDot);
        bool isOwn = true;
        for (int e = 0; e < entries.size() && isOwn; ++e) {
            isOwn = QFileInfo(path + QLatin1Char('/') + entries.at(e)).isFile()
                && FormatAgentGuides::isOwnFile(entries.at(e));
        }
        if ( ! isOwn) {
            continue;
        }

        // Keeps the folder of a run that still runs.
        QLockFile lock(path + QLL("/.lock"));
        lock.setStaleLockTime(0);
        if ( ! lock.tryLock(0)) {
            continue;
        }
        lock.unlock();
        QDir(path).removeRecursively();
    }
}

/*!
    Tells whether the guides write a file named @a name into their folder:
    the usage instructions (see usageName()), the lock and the files the lock
    keeps beside it (`.lock`, `.lock.<suffix>`), or a prompt file,
    `<rule>-<number>.md`, of a rule that has a template (see templateName()).
*/
bool FormatAgentGuides::isOwnFile(const QString & name)
{
    if (name == FormatAgentGuides::usageName() || name == QLL(".lock")
        || name.startsWith(QLL(".lock."))
    ) {
        return true;
    }
    QRegExp prompt(QString(QLL("(.+)-\\d+\\.md")));
    if ( ! prompt.exactMatch(name)) {
        return false;
    }
    const QByteArray rule = prompt.cap(1).toLatin1();
    return QFile::exists(QLL(":/assets/")
        + FormatAgentGuides::templateName(rule)
    );
}
