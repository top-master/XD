// Copyright (C) 2015 The XD Company Ltd.
// SPDX-License-Identifier: Apache-2.0

#include "./outline-this.h"

#include "../format-document.h"
#include "../format-rules.h"
#include "../parser/class-helpers.h"

#include <QtCore/QByteArray>
#include <QtCore/QDir>
#include <QtCore/QFile>
#include <QtCore/QFileInfo>
#include <QtCore/QHash>
#include <QtCore/QList>
#include <QtCore/QSet>
#include <QtCore/QString>

/*!
    \class FormatOutlineThisRule
    \internal

    Reaches a class's own members through `this->` in its outline member
    definitions (a body written outside the class body, such as
    `void Widget::run() { ... }`), where a bare name could read as a free
    function or a global; a member that a parameter or a local shadows is left
    as it is, and so is a static one, and every member of a static function.
*/

/*!
    Adds `this->` in front of each bare member name in each outline member
    definition of \a document.
*/
void FormatOutlineThisRule::run(FormatDocument *document) const
{
    QHash<QByteArray, FormatClassMembers > cache;
    for (int brace = 0; brace < document->symbols().size(); ++brace) {
        if (document->token(brace) != LBRACE
            || document->braceScope(brace) != FormatDocument::FunctionScope
        ) {
            continue;
        }
        const FormatDocument::Scope scope = document->scopeOf(brace);
        if (scope != FormatDocument::FileScope && scope != FormatDocument::NamespaceScope) {
            continue;
        }

        // Finds `<Class>::<member>(` in the head.
        int params = -1;
        int classToken = -1;
        int memberToken = -1;
        for (int k = document->headStart(brace); k + 2 < brace; ++k) {
            if (document->token(k) != IDENTIFIER || document->token(k + 1) != SCOPE) {
                continue;
            }
            int member = k + 2;
            if (document->token(member) == TILDE) {
                ++member;
            }
            if (document->token(member) == IDENTIFIER && document->token(member + 1) == LPAREN
                && document->token(member + 2) != STAR
            ) {
                classToken = k;
                memberToken = member;
                params = member + 1;
            }
        }
        if (params < 0 || document->match(params) < 0) {
            continue;
        }
        const QByteArray className = document->lexem(classToken);
        const FormatClassMembers members = this->membersOf(document, className, &cache);
        if (members.names.isEmpty() || members.statics.contains(document->lexem(memberToken))) {
            continue;
        }

        // Leaves out the names that a parameter or a local declares.
        const int close = document->match(brace);
        QSet<QByteArray > shadowed = this->localNames(document, params + 1, document->match(params));
        shadowed.unite(this->localNames(document, brace + 1, close));

        QList<FormatGrowth > growths;
        for (int i = brace + 1; i < close; ++i) {
            if (document->token(i) != IDENTIFIER || document->isDirectiveLine(document->symbolLine(i))) {
                continue;
            }
            const QByteArray name = document->lexem(i);
            if (!members.names.contains(name) || shadowed.contains(name)) {
                continue;
            }
            const Token before = document->token(i - 1);
            const Token after = document->token(i + 1);
            if (before == DOT || before == ARROW || before == SCOPE || before == TILDE
                || before == GOTO || after == SCOPE || (after == COLON && before == SEMIC)
                || isMacroArgument(document, i)
            ) {
                continue;
            }
            const int from = document->symbols().at(i).from;
            if (document->replace(from, 0, "this->")) {
                const int column = from - document->lineStart(document->symbolLine(i));
                growths << FormatGrowth(i, column, 6);
            }
        }
        formatShiftContinuations(document, growths);
    }
}

/*!
    Builds the members of the class named \a className: from its body in
    \a document, else in the header next to the document's file (its name
    with a `.h`, `_p.h`, or `.hpp` suffix); \a cache keeps each class's
    result for the run.
*/
FormatClassMembers FormatOutlineThisRule::membersOf(const FormatDocument *document,
                                                    const QByteArray & className,
                                                    QHash<QByteArray, FormatClassMembers > *cache) const
{
    if (cache->contains(className)) {
        return cache->value(className);
    }
    FormatClassMembers members;
    const int brace = findClassBody(document, className);
    if (brace >= 0) {
        members = collectMembers(document, brace, className);
    } else if (!document->path().isEmpty()) {
        const QFileInfo info(document->path());
        const char *const suffixes[] = { ".h", "_p.h", ".hpp" };
        for (size_t s = 0; s < sizeof(suffixes) / sizeof(suffixes[0]); ++s) {
            QFile header(info.dir().filePath(info.completeBaseName()
                                             + QString::fromLatin1(suffixes[s])));
            if (!header.open(QIODevice::ReadOnly)) {
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
    cache->insert(className, members);
    return members;
}

/*!
    Builds the names that the tokens from \a from up to \a to of \a document
    declare: a name right after a type, followed by what may end a
    declaration (`;`, `=`, `,`, `)`, `(`, `[`, or a range-for's `:`), and
    the names that declaration lists after commas.
*/
QSet<QByteArray > FormatOutlineThisRule::localNames(const FormatDocument *document, int from,
                                                    int to) const
{
    QSet<QByteArray > names;
    for (int i = from; i < to; ++i) {
        if (document->token(i) != IDENTIFIER || !endsType(document->token(i - 1))) {
            continue;
        }
        const Token before = document->token(i - 2);
        if (document->token(i - 1) == IDENTIFIER && (before == DOT || before == ARROW)) {
            continue;
        }
        const Token after = document->token(i + 1);
        if (after != SEMIC && after != EQ && after != COMMA && after != RPAREN && after != LPAREN
            && after != LBRACK && after != COLON
        ) {
            continue;
        }
        names.insert(document->lexem(i));

        // Takes the names the same declaration lists after commas, such as
        // `m` and `d` of `int y, m, d;`.
        for (int k = i + 1; k < to; ++k) {
            const Token token = document->token(k);
            if (token == LPAREN || token == LBRACK || token == LBRACE) {
                k = qMax(k, document->match(k));
                continue;
            }
            if (token == SEMIC || token == RPAREN || token == RBRACE) {
                break;
            }
            if (token != COMMA) {
                continue;
            }
            int name = k + 1;
            while (document->token(name) == STAR || document->token(name) == AND) {
                ++name;
            }
            if (document->token(name) == IDENTIFIER) {
                names.insert(document->lexem(name));
            }
        }
    }
    return names;
}
