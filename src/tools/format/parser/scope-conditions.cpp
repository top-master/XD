// Copyright (C) 2015 The XD Company Ltd.
// SPDX-License-Identifier: Apache-2.0

#include "./scope-conditions.h"

#include "../format-document.h"
#include "./class-helpers.h"

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
        << QString(QLL("is-header")) << QString(QLL("is-non-header"));
}

/*!
    Tells whether the condition that Config::Condition::compile() made
    @a program of holds at the token @a token.
*/
bool FormatScopeConditions::holds(const QStringList & program, int token)
{
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
    if (percent < 0) {
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
            if ( ! scope.isOutline) {
                if (scope.params < 0 && token == IDENTIFIER && document->token(k + 1) == LPAREN
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
            if (document->token(member) == IDENTIFIER && document->token(member + 1) == LPAREN
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
