// Copyright (C) 2015 The XD Company Ltd.
// SPDX-License-Identifier: Apache-2.0

#include "./scope-conditions.h"

#include "../format-document.h"
#include "./class-helpers.h"
#include "./code-helpers.h"
#include "./layout-helpers.h"

#include "config-condition.h"

#include <QtCore/QFile>
#include <QtCore/QFileInfo>
#include <QtCore/QList>

/*!
    @class FormatScopeConditions
    @internal

    Tells which names of a setting's condition (see Config::Condition) hold
    at a token of a document, for any rule that a setting steers: the names
    come from the scope that holds the token, which is the member function
    whose body holds it, else the class body that does, and from the token
    itself (see names()).

    The scope of each body, and the members of each class, are read once and
    cached; hence a rule asks for each token it formats.
*/

/*!
    Builds the conditions of @a document, which must stay alive as long as
    the conditions are in use.
*/
FormatScopeConditions::FormatScopeConditions(const FormatDocument *document)
    : m_document(document)
{
}

/*!
    Builds the names a condition takes, each one true at a token when:

    * `always`: always, and `never`: never;
    * `already-existing`: the token is a member name that already follows
      `this->`;
    * `is-base-member`: the token is a member name that a base class
      declares, and the scope's own class does not;
    * `is-template`: the scope's function, or its class, is a template;
    * `is-outline`: the scope's function is defined outside its class body;
    * `is-header`: the file is a header (see FormatDocument::isHeader()),
      and `is-non-header`: it is not.
*/
QStringList FormatScopeConditions::names()
{
    return QStringList() << QString(QLL("always")) << QString(QLL("never"))
        << QString(QLL("already-existing")) << QString(QLL("is-base-member"))
        << QString(QLL("is-template")) << QString(QLL("is-outline"))
        << QString(QLL("is-header")) << QString(QLL("is-non-header"))
        << QString(QLL("is-signature"));
}

namespace {
class ArgumentContext : public Config::Condition::Context
{
    typedef Config::Condition::Context super;
public:
    ArgumentContext(const FormatScopeConditions *scope, int token)
        : m_scope(scope), m_token(token) {}
    qreal argumentCount(int depth, int argument) const Q_DECL_OVERRIDE
    { return m_scope->argumentCount(m_token, depth, argument); }
private:
    const FormatScopeConditions *m_scope;
    int m_token;
};
}

/*!
    Tells whether the condition that Config::Condition::compile() made
    @a program of holds at the token @a token.
*/
bool FormatScopeConditions::holds(const QStringList & program, int token)
{
    if ( ! program.isEmpty() && program.first() == QLL("@expression")) {
        ArgumentContext context(this, token);
        QHash<QString, qreal > values;
        values.insert(QString(QLL("value")), this->argumentCount(token, 0));
        values.insert(
            QString(QLL("spaceremaining")),
            this->remainingSpace(token)
        );
        values.insert(
            QString(QLL("percent")),
            100 - this->remainingSpace(token)
        );
        return Config::Condition::evaluate(
            program,
            this->holding(token),
            values,
            &context
        );
    }
    return Config::Condition::evaluate(program, this->holding(token));
}

/*!
    Tells whether the condition @a program holds at the token @a token, with
    @a percent as the value of its percent terms (see
    Config::Condition::evaluate()), negative where the use site has none.
    When a term misses what it needs (Config::Condition logs that), the
    default condition @a fallback decides instead; the formatting goes on.
*/
bool FormatScopeConditions::holds(
        const QStringList & program, int token, qreal percent,
        const QStringList & fallback)
{
    const QSet<QString > names = this->holding(token);
    bool ok = true;
    bool result = false;
    if ( ! program.isEmpty() && program.first() == QLL("@expression")) {
        ArgumentContext context(this, token);
        QHash<QString, qreal > values;
        values.insert(QString(QLL("value")), this->argumentCount(token, 0));
        values.insert(QString(QLL("spaceremaining")), percent >= 0
            ? 100 - percent : this->remainingSpace(token));
        if (percent >= 0) {
            values.insert(QString(QLL("percent")), percent);
        }
        result = Config::Condition::evaluate(
            program,
            names,
            values,
            &context,
            &ok
        );
    } else if (percent < 0) {
        result = Config::Condition::evaluate(program, names, &ok);
    } else {
        result = Config::Condition::evaluate(program, names, percent, &ok);
    }
    if (ok) {
        return result;
    }
    if (percent < 0) {
        return Config::Condition::evaluate(fallback, names);
    }
    return Config::Condition::evaluate(fallback, names, percent);
}

/*!
    Builds the names that hold at the token @a token (see names()).
*/
QSet<QString > FormatScopeConditions::holding(int token)
{
    QSet<QString > names;
    names << QString(QLL("always"));
    const int target = this->argumentTarget(token);
    if (target >= 0 && isDeclarationParen(m_document, target)) {
        names << QString(QLL("is-signature"));
    }
    if (this->m_document->isHeader()) {
        names << QString(QLL("is-header"));
    } else {
        names << QString(QLL("is-non-header"));
    }
    const FormatMemberScope scope = this->scopeOf(token);
    if (scope.isOutline) {
        names << QString(QLL("is-outline"));
    }
    if (scope.isTemplate) {
        names << QString(QLL("is-template"));
    }
    if (this->m_document->token(token) != IDENTIFIER) {
        return names;
    }

    // Names what holds for a member name.
    const QByteArray name = this->m_document->lexem(token);
    if (scope.baseMembers.contains(name)) {
        names << QString(QLL("is-base-member"));
    }
    if ((scope.members.names.contains(name) || scope.baseMembers.contains(name))
        && token >= 2 && this->m_document->token(token - 1) == ARROW
        && this->m_document->token(token - 2) == THIS
    ) {
        names << QString(QLL("already-existing"));
    }
    return names;
}

/*!
    Builds the scope of the token @a token: the member function whose body
    holds it (see functionScope()), else the class body that holds it, else
    a scope without a class.
*/
FormatMemberScope FormatScopeConditions::scopeOf(int token)
{
    const int body = this->enclosingFunctionBody(token);
    if (body >= 0) {
        const FormatMemberScope scope = this->functionScope(body);
        if ( ! scope.className.isEmpty()) {
            return scope;
        }
    }
    FormatMemberScope scope;
    const int classBody = enclosingClassBody(this->m_document, token);
    if (classBody < 0) {
        return scope;
    }
    scope.body = classBody;
    scope.className = classNameOf(this->m_document, classBody);
    if (scope.className.isEmpty()) {
        return scope;
    }
    scope.members = this->membersOf(scope.className);
    scope.baseMembers = this->baseMembersOf(scope.members);
    scope.isTemplate = scope.members.isTemplate;
    return scope;
}

/*!
    Builds the scope of the member function whose body's `{` is at @a brace:
    its class from `<Class>::<member>(` in the head of an outline
    definition, else the class body it sits in; its parameters; and whether
    it is static, an outline, and a template. The scope has no class for a
    body that is no member function's.
*/
FormatMemberScope FormatScopeConditions::functionScope(int brace)
{
    if (this->m_scopes.contains(brace)) {
        return this->m_scopes.value(brace);
    }
    const FormatDocument *document = this->m_document;
    FormatMemberScope scope;
    scope.body = brace;
    const FormatDocument::Scope outer = document->scopeOf(brace);
    scope.isOutline = outer == FormatDocument::FileScope
        || outer == FormatDocument::NamespaceScope;
    if (scope.isOutline || outer == FormatDocument::ClassScope) {
        QByteArray className;
        for (int k = document->headStart(brace); k + 1 < brace; ++k) {
            const Token token = document->token(k);
            scope.isTemplate = scope.isTemplate || token == TEMPLATE;
            scope.isStatic = scope.isStatic || token == STATIC;
            if (token == LPAREN && document->match(k) > k) {
                k = document->match(k);
                continue;
            }
            if ( ! scope.isOutline) {
                if (scope.params < 0
                    && token == IDENTIFIER
                    && document->token(k + 1) == LPAREN
                    && ! isMacroName(document->lexem(k))
                ) {
                    scope.memberToken = k;
                    scope.params = k + 1;
                }
                continue;
            }

            // Steps over a class template's arguments: `W<T>::run(`.
            int scopeToken = k + 1;
            if (token == IDENTIFIER
                && document->token(scopeToken) == LANGLE
            ) {
                int depth = 0;
                for (; scopeToken < brace; ++scopeToken) {
                    const Token angle = document->token(scopeToken);
                    depth += angle == LANGLE ? 1 : 0;
                    depth -= angle == RANGLE ? 1 : (angle == GTGT ? 2 : 0);
                    if (depth <= 0) {
                        break;
                    }
                }
                ++scopeToken;
                scope.isTemplate = scope.isTemplate || depth == 0;
            }
            if (token != IDENTIFIER
                || document->token(scopeToken) != SCOPE
            ) {
                continue;
            }
            int member = scopeToken + 1;
            if (document->token(member) == TILDE) {
                ++member;
            }
            if (scope.params < 0 && document->token(member) == IDENTIFIER
                && document->token(member + 1) == LPAREN
                && document->token(member + 2) != STAR
            ) {
                className = document->lexem(k);
                scope.memberToken = member;
                scope.params = member + 1;
            }
        }
        if ( ! scope.isOutline) {
            className = classNameOf(
                document,
                enclosingClassBody(document, brace)
            );
        }
        if (scope.params >= 0 && document->match(scope.params) >= 0) {
            scope.className = className;
        }
    }
    if ( ! scope.className.isEmpty()) {
        scope.members = this->membersOf(scope.className);
        scope.baseMembers = this->baseMembersOf(scope.members);
        scope.isTemplate = scope.isTemplate || scope.members.isTemplate;
    }
    this->m_scopes.insert(brace, scope);
    return scope;
}

/*!
    Builds the index of the `{` of the outermost function body that holds
    the token @a token, or -1 when none does: a lambda's body, and a block or
    a braced initializer inside the body, count as part of its function's,
    while a class or a namespace body ends the search.
*/
int FormatScopeConditions::enclosingFunctionBody(int token) const
{
    const FormatDocument *document = this->m_document;
    int found = -1;
    int depth = 0;
    for (int k = token - 1; k >= 0; --k) {
        const Token kind = document->token(k);
        if (kind == RBRACE) {
            ++depth;
            continue;
        }
        if (kind != LBRACE) {
            continue;
        }
        if (depth > 0) {
            --depth;
            continue;
        }
        const FormatDocument::Scope scope = document->braceScope(k);
        if (scope == FormatDocument::ClassScope
            || scope == FormatDocument::NamespaceScope
        ) {
            break;
        }
        if (scope == FormatDocument::FunctionScope) {
            found = k;
        }
    }
    return found;
}

/*!
    Builds the members of the class named @a className: from its body in the
    document, else in a header named after the document's file (with a `.h`,
    `_p.h`, or `.hpp` suffix), else in one the file quotes in an `#include`;
    each header is looked for next to the file, then in the include folders
    (see FormatDocument::findInclude()).
*/
FormatClassMembers FormatScopeConditions::membersOf(
        const QByteArray & className)
{
    if (this->m_classes.contains(className)) {
        return this->m_classes.value(className);
    }
    const FormatDocument *document = this->m_document;
    FormatClassMembers members;
    const int brace = findClassBody(document, className);
    if (brace >= 0) {
        members = collectMembers(document, brace, className);
    } else if ( ! document->path().isEmpty()) {
        // Looks for the class in the headers named after the file, then in
        // the ones the file includes.
        const QFileInfo info(document->path());
        QStringList headers;
        const char *const suffixes[] = { ".h", "_p.h", ".hpp" };
        for (size_t s = 0; s < sizeof(suffixes) / sizeof(suffixes[0]); ++s) {
            const QString suffix = QString::fromLatin1(suffixes[s]);
            headers << info.completeBaseName() + suffix;
        }
        headers << document->quotedIncludes();
        for (int h = 0; h < headers.size(); ++h) {
            QFile header(document->findInclude(headers.at(h)));
            if (header.fileName().isEmpty()
                || ! header.open(QIODevice::ReadOnly)
            ) {
                continue;
            }
            const FormatDocument other(header.readAll(), header.fileName());
            const int found = findClassBody(&other, className);
            if (found >= 0) {
                members = collectMembers(&other, found, className);
                break;
            }
        }
    }
    this->m_classes.insert(className, members);
    return members;
}

/*!
    Builds the names of the members that the bases of the class whose
    members are @a members declare, and the bases of those in turn, up to 8
    levels deep; a base the conditions find no body of adds none, and the
    class's own members are left out.
*/
QSet<QByteArray > FormatScopeConditions::baseMembersOf(
        const FormatClassMembers & members)
{
    QSet<QByteArray > names;
    QSet<QByteArray > seen;
    QList<QByteArray > queue = members.bases;
    for (int depth = 0; depth < 8 && ! queue.isEmpty(); ++depth) {
        QList<QByteArray > next;
        for (int b = 0; b < queue.size(); ++b) {
            if (seen.contains(queue.at(b))) {
                continue;
            }
            seen.insert(queue.at(b));
            const FormatClassMembers base = this->membersOf(queue.at(b));
            names.unite(base.names);
            next << base.bases;
        }
        queue = next;
    }
    names.subtract(members.names);
    return names;
}

namespace {
static bool isArgumentTarget(const FormatDocument *document, int open)
{
    if (open < 1
        || document->token(open) != LPAREN
        || document->match(open) <= open
    ) {
        return false;
    }
    const Token before = document->token(open - 1);
    return before == IDENTIFIER || before == RPAREN || before == RBRACK
        || before == RANGLE || before == GTGT;
}

static int enclosingArgumentTarget(const FormatDocument *document, int token)
{
    for (int i = token - 1; i >= 1; --i) {
        if (isArgumentTarget(document, i) && document->match(i) >= token) {
            return i;
        }
    }
    return -1;
}

static void directCalls(const FormatDocument *document, int first, int last,
                        QList<int > *calls)
{
    for (int i = first; i <= last; ++i) {
        if (isArgumentTarget(document, i)) {
            calls->append(i);
            i = document->match(i);
        }
    }
}
}

/*!
    Counts arguments of calls reached from the target: zero selects the
    target, negative depths select parents, and positive depths select calls
    within its arguments. A positive argument selects that one-based item;
    zero selects all items. Returns -1 when no target exists at that depth.
*/
int FormatScopeConditions::argumentTarget(int token) const
{
    if (isArgumentTarget(m_document, token)) {
        return token;
    }
    if (isArgumentTarget(m_document, token + 1)) {
        return token + 1;
    }
    return enclosingArgumentTarget(m_document, token);
}

qreal FormatScopeConditions::remainingSpace(int token) const
{
    const int open = this->argumentTarget(token);
    if (open < 0) {
        return 0;
    }
    const int line = m_document->symbolLine(open);
    const QByteArray head = m_document->text().mid(
        m_document->lineStart(line),
        m_document->symbolEnd(open) - m_document->lineStart(line)
    );
    const int maximum = qMax(1, m_document->settingsAt(
        m_document->symbols().at(open).from
    ).wrapMax);
    return 100.0 * (maximum - textWidth(head)) / maximum;
}

int FormatScopeConditions::argumentCount(
        int token, int depth, int argument) const
{
    int target = this->argumentTarget(token);
    if (target < 0 || argument < 0) {
        return -1;
    }
    while (depth < 0) {
        target = enclosingArgumentTarget(m_document, target);
        if (target < 0) {
            return -1;
        }
        ++depth;
    }
    QList<int > targets;
    targets << target;
    for (int level = 0; level < depth; ++level) {
        QList<int > next;
        for (int k = 0; k < targets.size(); ++k) {
            const int open = targets.at(k);
            const QList<QPair<int, int > > items =
                splitArguments(m_document, open, m_document->match(open));
            for (int i = 0; i < items.size(); ++i) {
                if (level == 0 && argument > 0 && i + 1 != argument) {
                    continue;
                }
                directCalls(
                    m_document,
                    items.at(i).first,
                    items.at(i).second,
                    &next
                );
            }
        }
        if (next.isEmpty()) {
            return -1;
        }
        targets = next;
    }
    int count = 0;
    for (int i = 0; i < targets.size(); ++i) {
        const int open = targets.at(i);
        const int close = m_document->match(open);
        const QList<QPair<int, int > > items =
            splitArguments(m_document, open, close);
        if (items.size() == 1 && m_document->token(items.first().first) == VOID
            && items.first().first == items.first().second
            && isDeclarationParen(m_document, open)
        ) {
            continue;
        }
        count += items.size();
    }
    return count;
}

/*!
    Evaluates the argument-count setting at @a token. A supplied percentage
    overrides the remaining space calculated from the target's column.
*/
bool FormatScopeConditions::argsOver(int token, int count, qreal spaceRemaining)
{
    const FormatSettings &settings = m_document->settingsAt(
        m_document->symbols().at(token).from
    );
    if (settings.wrapArgsOverCondition.isEmpty()) {
        return count > settings.wrapArgsOver;
    }
    if (spaceRemaining < -1e99) {
        spaceRemaining = this->remainingSpace(token);
    }
    QHash<QString, qreal > values;
    values.insert(QString(QLL("value")), count);
    values.insert(QString(QLL("spaceremaining")), spaceRemaining);
    values.insert(QString(QLL("percent")), 100 - spaceRemaining);
    ArgumentContext context(this, token);
    bool ok = true;
    const bool result = Config::Condition::evaluate(
        settings.wrapArgsOverCondition,
        this->holding(token),
        values,
        &context,
        &ok
    );
    return ok ? result : count > settings.wrapArgsOver;
}
