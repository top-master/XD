// Copyright (C) 2015 The XD Company Ltd.
// SPDX-License-Identifier: Apache-2.0

#include "./super-typedef.h"

#include "../format-document.h"
#include "../format-line-spec.h"
#include "../parser/class-helpers.h"
#include "../parser/code-helpers.h"
#include "../parser/scope-conditions.h"

#include "config-choice.h"
#include "config-condition.h"

#include <QtCore/QByteArray>
#include <QtCore/QDir>
#include <QtCore/QFile>
#include <QtCore/QFileInfo>
#include <QtCore/QScopedPointer>
#include <QtCore/QSharedPointer>
#include <QtCore/QString>

class FormatSuperContext : public Config::Condition::Context
{
public:
    FormatSuperContext(const FormatSuperTypedefRule *rule, const FormatDocument *document,
                       int brace, const QByteArray &base,
                       const FormatConfig *runConfig = Q_NULLPTR)
        : m_rule(rule), m_document(document), m_brace(brace), m_base(base)
        , m_runConfig(runConfig) {}

    qreal argumentCount(int, int) const Q_DECL_OVERRIDE { return 0; }
    bool predicate(const QString &name, bool *value) const Q_DECL_OVERRIDE
    {
        if (name == QLL("isnewclass")) {
            *value = m_rule->isNewClass(m_document, m_brace);
        } else if (name == QLL("isneeded")) {
            *value = m_rule->isNeeded(m_document, m_brace, m_base);
        } else if (name == QLL("hassupertypedef")) {
            *value = m_rule->superIsBase(m_document, m_brace, m_base);
            if ( ! *value && m_runConfig) {
                *value = m_rule->willHaveSuper(m_document, m_brace, m_base, *m_runConfig);
            }
        } else {
            return false;
        }
        return true;
    }

private:
    const FormatSuperTypedefRule *m_rule;
    const FormatDocument *m_document;
    int m_brace;
    QByteArray m_base;
    const FormatConfig *m_runConfig;
};

static QString cleanPath(const QString &path)
{
    return QDir::cleanPath(QFileInfo(path).absoluteFilePath());
}

static QStringList relatedFiles(const FormatDocument *document, const FormatSettings &settings)
{
    if (document->path().isEmpty()) {
        return QStringList();
    }
    const QFileInfo current(document->path());
    QString stem = current.completeBaseName();
    if (stem.endsWith(QLL("_p"))) {
        stem.chop(2);
    }
    const QFileInfoList entries = current.dir().entryInfoList(QDir::Files, QDir::Name);
    QStringList paths;
    for (int i = 0; i < entries.size(); ++i) {
        const QFileInfo entry = entries.at(i);
        if ((entry.completeBaseName() == stem || entry.completeBaseName() == stem + QLL("_p"))
            && settings.acceptsFile(entry.fileName())
            && cleanPath(entry.filePath()) != cleanPath(document->path())
        ) {
            paths << entry.absoluteFilePath();
        }
    }
    const QStringList includes = document->quotedIncludes();
    for (int i = 0; i < includes.size(); ++i) {
        const QString path = document->findInclude(includes.at(i));
        if ( ! path.isEmpty() && settings.acceptsFile(path)
            && ! paths.contains(path) && cleanPath(path) != cleanPath(document->path())
        ) {
            paths << path;
        }
    }
    return paths;
}

static FormatDocument *readRelated(const QString &path, const FormatConfig &config,
                                   bool withSelection = true)
{
    QFile file(config.snapshots.value(cleanPath(path), path));
    if ( ! file.open(QIODevice::ReadOnly)) {
        return Q_NULLPTR;
    }
    const QByteArray text = file.readAll();
    QScopedPointer<FormatDocument > document(new FormatDocument(text, path));
    document->setFormatConfig(config);
    document->setIncludePaths(config.includePaths);
    if ( ! withSelection) {
        return document.take();
    }
    FormatSettings settings;
    QString error;
    if ( ! FormatSettings::load(path, config, &settings, &error)) {
        return Q_NULLPTR;
    }
    document->setSettings(settings);
    if ((text.contains("Format rule") || text.contains('@'))
        && ! document->setInlineConfiguration(config, &error)
    ) {
        return Q_NULLPTR;
    }
    const QString key = cleanPath(path);
    bool scheduled = false;
    for (int i = 0; i < config.scheduledFiles.size(); ++i) {
        if (cleanPath(config.scheduledFiles.at(i)) == key) {
            scheduled = true;
            break;
        }
    }
    const QStringList specs = config.fileLineSpecs.value(
        key, scheduled ? config.lineSpecs : QStringList()
    );
    if ( ! specs.isEmpty()) {
        QList<QByteArray > lines = text.split('\n');
        if (text.endsWith('\n')) {
            lines.removeLast();
        }
        QVector<QPair<int, int > > ranges;
        if ( ! FormatLineSpec::resolveAll(specs, lines, &ranges, &error)) {
            ranges.clear();
            ranges.append(qMakePair(0, 0));
        }
        document->setLineRanges(ranges);
    }
    return document.take();
}

static int matchBase(const FormatDocument *document, int first, const QByteArray &base,
                     bool allowGlobal)
{
    if ((document->token(first) != IDENTIFIER && document->token(first) != SCOPE)
        || (document->token(first) == IDENTIFIER && first > 0
            && document->token(first - 1) == SCOPE)
        || (document->token(first) == SCOPE
            && (document->token(first - 1) == IDENTIFIER
                || document->token(first - 1) == RANGLE
                || document->token(first - 1) == GTGT))
    ) {
        return -1;
    }
    const FormatDocument baseDocument(base);
    if (baseDocument.symbols().isEmpty()) {
        return -1;
    }
    QList<int > starts;
    starts << 0;
    int depth = 0;
    for (int i = 0; i < baseDocument.symbols().size(); ++i) {
        const Token token = baseDocument.token(i);
        if (token == LANGLE) {
            ++depth;
        } else if (token == RANGLE) {
            --depth;
        } else if (token == GTGT) {
            depth -= 2;
        } else if (token == SCOPE && depth == 0) {
            starts << i + 1;
        }
    }
    for (int candidate = 0; candidate < starts.size(); ++candidate) {
        int last = first;
        if (document->token(first) == SCOPE
            && baseDocument.token(starts.at(candidate)) != SCOPE
        ) {
            if (starts.at(candidate) != 0 || ! allowGlobal) {
                continue;
            }
            ++last;
        }
        bool matches = true;
        for (int i = starts.at(candidate); i < baseDocument.symbols().size(); ++i, ++last) {
            if (last >= document->symbols().size()
                || document->lexem(last) != baseDocument.lexem(i)
            ) {
                matches = false;
                break;
            }
        }
        if (matches && document->token(last) == SCOPE) {
            return last;
        }
    }
    return -1;
}

static QByteArray baseIdentifier(const QByteArray &base)
{
    const FormatDocument document(base);
    QByteArray name;
    int depth = 0;
    for (int i = 0; i < document.symbols().size(); ++i) {
        const Token token = document.token(i);
        if (token == LANGLE) {
            ++depth;
        } else if (token == RANGLE) {
            --depth;
        } else if (token == GTGT) {
            depth -= 2;
        } else if (depth == 0 && token == IDENTIFIER) {
            name = document.lexem(i);
        }
    }
    return name;
}

/*!
    @class FormatSuperTypedefRule
    @internal

    Adds a private `typedef <Base> super;` according to the class's settings,
    after an opening `Q_OBJECT` or `Q_GADGET` when present. Replaces a base
    qualifier when the selected settings and the matching typedef allow it.
    A `struct`, whose members start public, gets a report instead.
*/

/*!
    Checks each class body and base-qualified use in @a document.
*/
void FormatSuperTypedefRule::run(FormatDocument *document) const
{
    const FormatConfig &config = document->formatConfig();
    const FormatSettings &settings = document->settings();
    if ( ! document->hasInlineConfiguration() && ! document->hasSelectedRanges()
        && config.fileLineSpecs.isEmpty() && config.lineSpecs.isEmpty()
        && settings.typedefSuper == QLL("isNewClass() && isNeeded() ? always : never")
        && settings.replaceSuperSites == QLL("isNewClass() && hasSuperTypedef()")
    ) {
        return;
    }
    for (int i = 0; i < document->symbols().size(); ++i) {
        if (document->token(i) != LBRACE
            || document->braceScope(i) != FormatDocument::ClassScope
        ) {
            continue;
        }
        const int close = document->match(i);
        bool isStruct = false;
        const QByteArray base = this->baseName(document, i, &isStruct);
        if (close <= i
            || base.isEmpty()
            || this->hasSuper(document, i)
            || ! this->wantsTypedef(document, i, base)
        ) {
            continue;
        }
        if (isStruct) {
            document->report(document->symbols().at(i).from, "super-typedef",
                             QString(QLL("give the struct a private typedef %1 super;"))
                             .arg(QString::fromUtf8(base)));
            continue;
        }

        // Places the typedef after an opening `Q_OBJECT`, else first.
        const int first = i + 1;
        const int firstLine = document->symbolLine(first);
        const QByteArray line = "typedef " + base + " super;";
        if (firstLine == document->symbolLine(i)) {
            const int offset = document->symbolEnd(i);
            const bool hasSpace = offset < document->text().size()
                && (document->text().at(offset) == ' ' || document->text().at(offset) == '\t');
            document->replace(offset, 0, " " + line + (hasSpace ? "" : " "));
            continue;
        }
        const Token token = document->token(first);
        if (token == Q_OBJECT_TOKEN || token == Q_GADGET_TOKEN) {
            document->replace(
                contentEnd(document, firstLine),
                0,
                "\n"
                + document->indentOf(firstLine)
                + line
            );
            continue;
        }
        const bool isAccess = token == PUBLIC || token == PRIVATE
            || token == PROTECTED || token == RBRACE;
        const QByteArray indent = isAccess
            ? document->indentOf(document->symbolLine(i)) + "    "
                : document->indentOf(firstLine);
        document->replace(
            document->lineStart(firstLine),
            0,
            indent
                + line
                + "\n"
        );
    }
    this->replaceSites(document);
}

/*!
    Builds the text of the first base class of the class whose body opens at
    @a brace of @a document, or an empty one when it has no base;
    @a isStruct tells whether it is a `struct`.
*/
QByteArray FormatSuperTypedefRule::baseName(
        const FormatDocument *document, int brace,
        bool *isStruct) const
{
    // Finds the class key, then the `:` of the base list after it.
    int key = -1;
    int templateDepth = 0;
    for (int i = document->headStart(brace); i < brace && key < 0; ++i) {
        const Token token = document->token(i);
        if (token == LANGLE) {
            ++templateDepth;
        } else if (token == RANGLE) {
            --templateDepth;
        } else if (token == GTGT) {
            templateDepth -= 2;
        } else if (templateDepth == 0 && (token == CLASS || token == STRUCT)) {
            key = i;
            *isStruct = token == STRUCT;
        }
    }
    int colon = -1;
    int depth = 0;
    for (int i = key + 1; key >= 0 && i < brace; ++i) {
        const Token token = document->token(i);
        depth += token == LANGLE
            ? 1 : (token == RANGLE ? -1 : (token == GTGT ? -2 : 0));
        if (token == COLON && depth == 0) {
            colon = i;
            break;
        }
    }
    if (colon < 0) {
        return QByteArray();
    }
    int first = colon + 1;
    while (document->token(first) == PUBLIC
        || document->token(first) == PRIVATE
        || document->token(first) == PROTECTED
        || document->token(first) == VIRTUAL
    ) {
        ++first;
    }
    int last = first;
    depth = 0;
    for (int i = first; i < brace; ++i) {
        const Token token = document->token(i);
        depth += token == LANGLE
            ? 1 : (token == RANGLE ? -1 : (token == GTGT ? -2 : 0));
        if (token == COMMA && depth == 0) {
            break;
        }
        last = i;
    }
    if (first >= brace) {
        return QByteArray();
    }
    QByteArray result = document->lexem(first);
    for (int i = first + 1; i <= last; ++i) {
        const int previousEnd = document->symbolEnd(i - 1);
        const int currentStart = document->symbols().at(i).from;
        const QByteArray gap = document->text().mid(previousEnd, currentStart - previousEnd);
        result += gap.contains('\n') || gap.contains('\r')
            || document->hasCommentBetween(previousEnd, currentStart) ? QByteArray(" ") : gap;
        result += document->lexem(i);
    }
    return result;
}

/*!
    Tells whether the class body that opens at @a brace of @a document
    already declares `super` (with `typedef` or `using`), leaving out the
    bodies nested in it.
*/
bool FormatSuperTypedefRule::hasSuper(
        const FormatDocument *document,
        int brace) const
{
    const int close = document->match(brace);
    for (int i = brace + 1; i < close; ++i) {
        if (document->token(i) == LBRACE && document->match(i) > i) {
            i = document->match(i);
            continue;
        }
        if (document->token(i) == IDENTIFIER && document->lexem(i) == "super"
            && (document->token(i + 1) == SEMIC || document->token(i + 1) == EQ)
        ) {
            return true;
        }
    }
    return false;
}

static bool sameType(const FormatDocument *document, int first, int last,
                     const QByteArray &base)
{
    const FormatDocument expected(base);
    if (last - first != expected.symbols().size()) {
        return false;
    }
    for (int i = 0; i < expected.symbols().size(); ++i) {
        if (document->lexem(first + i) != expected.lexem(i)) {
            return false;
        }
    }
    return true;
}

bool FormatSuperTypedefRule::superIsBase(const FormatDocument *document, int brace,
                                        const QByteArray &base) const
{
    const int close = document->match(brace);
    for (int i = brace + 1; i < close; ++i) {
        if (document->token(i) == LBRACE && document->match(i) > i) {
            i = document->match(i);
            continue;
        }
        if (document->token(i) == TYPEDEF) {
            for (int end = i + 1; end < close && document->token(end) != SEMIC; ++end) {
                if (document->token(end) == IDENTIFIER && document->lexem(end) == "super"
                    && document->token(end + 1) == SEMIC
                ) {
                    return sameType(document, i + 1, end, base);
                }
            }
        } else if (document->token(i) == USING && document->lexem(i + 1) == "super"
                   && document->token(i + 2) == EQ) {
            int end = i + 3;
            while (end < close && document->token(end) != SEMIC) {
                ++end;
            }
            if (end < close) {
                return sameType(document, i + 3, end, base);
            }
        }
    }
    return false;
}

bool FormatSuperTypedefRule::isNewClass(const FormatDocument *document, int brace) const
{
    if ( ! document->hasSelectedRanges()) {
        return false;
    }
    const int start = document->headStart(brace);
    const int close = document->match(brace);
    if (close < brace) {
        return false;
    }
    for (int line = document->symbolLine(start); line <= document->symbolEndLine(close); ++line) {
        if ( ! document->isLineSelected(line)) {
            return false;
        }
    }
    return true;
}

static QList<QByteArray > enclosingNames(const FormatDocument *document, int token,
                                         bool includeClasses)
{
    QList<QByteArray > names;
    for (int i = 0; i < token; ++i) {
        if (document->token(i) != LBRACE || document->match(i) < token) {
            continue;
        }
        const FormatDocument::Scope scope = document->braceScope(i);
        if (scope == FormatDocument::ClassScope && includeClasses) {
            const QByteArray name = classNameOf(document, i);
            if ( ! name.isEmpty()) {
                names << name;
            }
        } else if (scope == FormatDocument::NamespaceScope) {
            for (int head = document->headStart(i); head < i; ++head) {
                if (document->token(head) != NAMESPACE) {
                    continue;
                }
                bool named = false;
                for (++head; head < i; ++head) {
                    if (document->token(head) == IDENTIFIER) {
                        names << document->lexem(head);
                        named = true;
                    }
                }
                if ( ! named) {
                    names << QByteArray("@anonymous");
                }
                break;
            }
        }
    }
    return names;
}

static QByteArray joinNames(const QList<QByteArray > &names)
{
    QByteArray result;
    for (int i = 0; i < names.size(); ++i) {
        if ( ! result.isEmpty()) {
            result += "::";
        }
        result += names.at(i);
    }
    return result;
}

static QByteArray qualifiedClass(const FormatDocument *document, int brace)
{
    QList<QByteArray > names = enclosingNames(document, brace, true);
    const QByteArray name = classNameOf(document, brace);
    if (name.isEmpty()) {
        return QByteArray();
    }
    names << name;
    return joinNames(names);
}

static int findQualifiedClass(const FormatDocument *document, const QByteArray &name)
{
    for (int i = 0; i < document->symbols().size(); ++i) {
        if (document->token(i) == LBRACE
            && document->braceScope(i) == FormatDocument::ClassScope
            && qualifiedClass(document, i) == name
        ) {
            return i;
        }
    }
    return -1;
}

static QByteArray qualifiedMemberOwner(const FormatDocument *document, int member,
                                       int position)
{
    int cursor = member - 1;
    if (document->token(cursor) == TILDE) {
        --cursor;
    }
    if (document->token(cursor) != SCOPE) {
        return QByteArray();
    }
    --cursor;
    QList<QByteArray > names;
    bool global = false;
    for (;;) {
        if (document->token(cursor) == RANGLE || document->token(cursor) == GTGT) {
            int depth = document->token(cursor) == GTGT ? 2 : 1;
            while (--cursor >= 0 && depth > 0) {
                const Token kind = document->token(cursor);
                depth += kind == RANGLE ? 1 : (kind == GTGT ? 2 : (kind == LANGLE ? -1 : 0));
            }
            --cursor;
        }
        if (document->token(cursor) != IDENTIFIER) {
            break;
        }
        names.prepend(document->lexem(cursor));
        if (document->token(cursor - 1) != SCOPE) {
            break;
        }
        cursor -= 2;
        if (document->token(cursor) != IDENTIFIER
            && document->token(cursor) != RANGLE && document->token(cursor) != GTGT
        ) {
            global = true;
            break;
        }
    }
    if (names.isEmpty()) {
        return QByteArray();
    }
    if ( ! global) {
        const QList<QByteArray > prefix = enclosingNames(document, position, false);
        for (int i = prefix.size() - 1; i >= 0; --i) {
            names.prepend(prefix.at(i));
        }
    }
    return joinNames(names);
}

static QByteArray ownerOf(const FormatDocument *document, FormatScopeConditions *scopes,
                         int token)
{
    const int classBody = enclosingClassBody(document, token);
    if (classBody >= 0) {
        return qualifiedClass(document, classBody);
    }
    const FormatMemberScope scope = scopes->scopeOf(token);
    if (scope.memberToken >= 0) {
        return qualifiedMemberOwner(document, scope.memberToken, scope.body);
    }
    int headerStart = document->headStart(token);
    for (int next = token + 1; next < document->symbols().size(); ++next) {
        if (document->token(next) == SEMIC) {
            break;
        }
        if (document->token(next) == RBRACE) {
            const int open = document->match(next);
            if (open >= 0 && open < token && document->braceScope(open) == FormatDocument::InitScope) {
                headerStart = qMin(headerStart, document->headStart(open));
                continue;
            }
            break;
        }
        if (document->token(next) == LBRACE) {
            if (document->braceScope(next) == FormatDocument::InitScope
                && document->match(next) > next
            ) {
                next = document->match(next);
                continue;
            }
            if (document->headStart(next) <= token) {
                const FormatMemberScope function = scopes->functionScope(next);
                if (function.memberToken >= 0 && token > function.memberToken) {
                    return qualifiedMemberOwner(document, function.memberToken, next);
                }
            }
            for (int head = headerStart; head < token; ++head) {
                if (document->token(head) == IDENTIFIER
                    && document->token(head + 1) == SCOPE
                    && document->token(head + 2) == IDENTIFIER
                    && document->token(head + 3) == LPAREN
                ) {
                    return qualifiedMemberOwner(document, head + 2, next);
                }
            }
            break;
        }
    }
    const int start = document->headStart(token);
    bool assignment = false;
    int member = -1;
    for (int i = start; i < token; ++i) {
        if (document->token(i) == EQ) {
            assignment = true;
        } else if ( ! assignment && document->token(i) == IDENTIFIER
                   && document->token(i + 1) == SCOPE
                   && document->token(i + 2) == IDENTIFIER) {
            member = i + 2;
        }
    }
    return assignment && member >= 0
        ? qualifiedMemberOwner(document, member, token) : QByteArray();
}

static bool isShadowed(const FormatDocument *document, FormatScopeConditions *scopes,
                       int token, const QByteArray &base)
{
    const QByteArray name = baseIdentifier(base);
    if (name.isEmpty() || document->lexem(token) != name) {
        return false;
    }
    const FormatMemberScope scope = scopes->scopeOf(token);
    if (scope.body >= 0 && document->braceScope(scope.body) == FormatDocument::FunctionScope) {
        const int head = document->headStart(scope.body);
        int templateDepth = 0;
        for (int i = head; i < scope.body; ++i) {
            const Token kind = document->token(i);
            if (kind == TEMPLATE && document->token(i + 1) == LANGLE) {
                ++i;
                templateDepth = 1;
            } else if (templateDepth > 0 && kind == LANGLE) {
                ++templateDepth;
            } else if (templateDepth > 0 && kind == RANGLE) {
                --templateDepth;
            } else if (templateDepth > 0 && kind == GTGT) {
                templateDepth -= 2;
            } else if (templateDepth > 0 && kind == IDENTIFIER
                       && document->lexem(i) == name
                       && (document->token(i - 1) == CLASS
                           || document->lexem(i - 1) == "typename")) {
                return true;
            }
        }
    }
    const int start = scope.body >= 0 ? scope.body + 1 : document->headStart(token);
    for (int i = start; i < token; ++i) {
        if (document->token(i) != IDENTIFIER || document->lexem(i) != name) {
            continue;
        }
        bool declaration = (document->token(i - 1) == USING
                            && document->token(i + 1) == EQ)
            || document->token(i - 1) == CLASS || document->token(i - 1) == STRUCT;
        if (document->token(i + 1) == SEMIC) {
            for (int before = i - 1; before >= start; --before) {
                const Token kind = document->token(before);
                if (kind == SEMIC || kind == LBRACE || kind == RBRACE) {
                    break;
                }
                declaration = declaration || kind == TYPEDEF;
            }
        }
        if ( ! declaration) {
            continue;
        }
        int depth = 0;
        int enclosing = -1;
        for (int before = i - 1; before >= start; --before) {
            const Token kind = document->token(before);
            if (kind == RBRACE) {
                ++depth;
            } else if (kind == LBRACE) {
                if (depth == 0) {
                    enclosing = before;
                    break;
                }
                --depth;
            }
        }
        if (enclosing < 0 || document->match(enclosing) >= token) {
            return true;
        }
    }
    return false;
}

static bool usesBase(const FormatDocument *document, const QByteArray &className,
                     const QByteArray &base)
{
    FormatScopeConditions scopes(document);
    for (int i = 0; i < document->symbols().size(); ++i) {
        if ((document->token(i) != IDENTIFIER && document->token(i) != SCOPE)
            || (document->token(i) == IDENTIFIER
                && document->token(i + 1) != SCOPE && document->token(i + 1) != LANGLE)
            || (document->token(i) == SCOPE && document->token(i + 1) != IDENTIFIER)
            || document->isDirectiveLine(document->symbolLine(i))
        ) {
            continue;
        }
        if ((document->lexem(i) != "super" || document->token(i + 1) != SCOPE)
            && matchBase(document, i, base, ! className.contains("::")) < 0
        ) {
            continue;
        }
        if (ownerOf(document, &scopes, i) == className
            && ! isShadowed(document, &scopes, i, base)
        ) {
            return true;
        }
    }
    return false;
}

bool FormatSuperTypedefRule::isNeeded(const FormatDocument *document, int brace,
                                      const QByteArray &base) const
{
    const QByteArray className = qualifiedClass(document, brace);
    if (className.isEmpty()) {
        return false;
    }
    if (usesBase(document, className, base)) {
        return true;
    }
    const FormatSettings &settings = document->settingsAt(document->symbols().at(brace).from);
    const QStringList paths = relatedFiles(document, settings);
    for (int i = 0; i < paths.size(); ++i) {
        QScopedPointer<FormatDocument > related(readRelated(paths.at(i), document->formatConfig(), false));
        if (related && usesBase(related.data(), className, base)) {
            return true;
        }
    }
    return false;
}

bool FormatSuperTypedefRule::wantsTypedef(const FormatDocument *document, int brace,
                                          const QByteArray &base) const
{
    const int offset = document->symbols().at(brace).from;
    const FormatSettings &settings = document->settingsAt(offset);
    const FormatSuperContext context(this, document, brace, base);
    QString choice;
    QString error;
    if ( ! Config::Choice::resolve(settings.typedefSuper,
                                   FormatSettings::superConditionNames(),
                                   FormatSettings::superChoices(), &context,
                                   &choice, &error)) {
        return false;
    }
    return choice == QLL("always")
        || (choice == QLL("if-needed") && this->isNeeded(document, brace, base));
}

bool FormatSuperTypedefRule::willHaveSuper(const FormatDocument *document, int brace,
                                          const QByteArray &base,
                                          const FormatConfig &config) const
{
    if (this->hasSuper(document, brace) || document->path().isEmpty()) {
        return false;
    }
    bool scheduled = false;
    const QString path = cleanPath(document->path());
    for (int i = 0; i < config.scheduledFiles.size(); ++i) {
        if (cleanPath(config.scheduledFiles.at(i)) == path) {
            scheduled = true;
            break;
        }
    }
    bool isStruct = false;
    if ( ! scheduled || this->baseName(document, brace, &isStruct).isEmpty() || isStruct) {
        return false;
    }
    const int first = brace + 1;
    if (first > document->match(brace)) {
        return false;
    }
    const int firstLine = document->symbolLine(first);
    int offset = document->lineStart(firstLine);
    if (firstLine == document->symbolLine(brace)) {
        offset = document->symbolEnd(brace);
    } else if (document->token(first) == Q_OBJECT_TOKEN
               || document->token(first) == Q_GADGET_TOKEN) {
        offset = contentEnd(document, firstLine);
    }
    if ( ! document->canInsert(offset)) {
        return false;
    }
    return this->wantsTypedef(document, brace, base);
}

void FormatSuperTypedefRule::replaceSites(FormatDocument *document) const
{
    FormatScopeConditions scopes(document);
    QHash<QString, QStringList > pathsByExtensions;
    QHash<QString, QSharedPointer<FormatDocument > > owners;
    QHash<QString, int > classBraces;
    QHash<QString, bool > decisions;
    for (int i = 0; i + 1 < document->symbols().size(); ++i) {
        if ((document->token(i) != IDENTIFIER && document->token(i) != SCOPE)
            || (document->token(i) == IDENTIFIER
                && document->token(i + 1) != SCOPE && document->token(i + 1) != LANGLE)
            || (document->token(i) == SCOPE && document->token(i + 1) != IDENTIFIER)
            || document->isDirectiveLine(document->symbolLine(i))
        ) {
            continue;
        }
        const QByteArray className = ownerOf(document, &scopes, i);
        if (className.isEmpty()) {
            continue;
        }
        const FormatSettings &settings = document->settingsAt(document->symbols().at(i).from);
        if ( ! pathsByExtensions.contains(settings.extensions)) {
            pathsByExtensions.insert(settings.extensions, relatedFiles(document, settings));
        }
        const QString key = settings.extensions + QLatin1Char('\n') + QString::fromUtf8(className);
        if ( ! classBraces.contains(key)) {
            int found = findQualifiedClass(document, className);
            const QStringList paths = pathsByExtensions.value(settings.extensions);
            for (int p = 0; found < 0 && p < paths.size(); ++p) {
                QSharedPointer<FormatDocument > related(readRelated(paths.at(p), document->formatConfig()));
                if (related) {
                    found = findQualifiedClass(related.data(), className);
                    if (found >= 0) {
                        owners.insert(key, related);
                    }
                }
            }
            classBraces.insert(key, found);
        }
        const int brace = classBraces.value(key);
        if (brace < 0) {
            continue;
        }
        const FormatDocument *owner = owners.contains(key) ? owners.value(key).data() : document;
        bool isStruct = false;
        const QByteArray base = this->baseName(owner, brace, &isStruct);
        const int scopeToken = base.isEmpty() ? -1
            : matchBase(document, i, base, ! className.contains("::"));
        if (scopeToken < 0 || isShadowed(document, &scopes, i, base)) {
            continue;
        }
        const QString decisionKey = key + QLatin1Char('\n') + settings.replaceSuperSites;
        if ( ! decisions.contains(decisionKey)) {
            QStringList program;
            QString error;
            bool ok = false;
            bool replace = false;
            bool available = false;
            const FormatSuperContext context(this, owner, brace, base, &document->formatConfig());
            if (Config::Condition::compile(settings.replaceSuperSites,
                                           FormatSettings::superConditionNames(),
                                           &program, &error)) {
                replace = Config::Condition::evaluate(program, QSet<QString >(),
                    QHash<QString, qreal >(), &context, &ok);
            }
            decisions.insert(decisionKey, ok && replace
                && context.predicate(QString(QLL("hassupertypedef")), &available)
                && available);
        }
        if (decisions.value(decisionKey)) {
            const int from = document->symbols().at(i).from;
            if (i > 0 && document->token(i - 1) == USING
                && scopeToken + 1 < document->symbols().size()
                && document->lexem(scopeToken + 1) == baseIdentifier(base)
            ) {
                const int to = document->symbolEnd(scopeToken + 1);
                document->replace(from, to - from, "super::super");
            } else {
                const int to = document->symbols().at(scopeToken).from;
                document->replace(from, to - from, "super");
            }
        }
        i = scopeToken;
    }
}
