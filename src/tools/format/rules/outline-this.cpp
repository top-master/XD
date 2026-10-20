// Copyright (C) 2015 The XD Company Ltd.
// SPDX-License-Identifier: Apache-2.0

#include "./outline-this.h"

#include "../format-document.h"
#include "../format-rules.h"
#include "../parser/class-helpers.h"
#include "../parser/code-helpers.h"
#include "../parser/scope-conditions.h"

#include <QtCore/QByteArray>
#include <QtCore/QList>
#include <QtCore/QSet>
#include <QtCore/QString>
#include <QtCore/QStringList>

/*!
    @class FormatOutlineThisRule
    @internal

    Reaches the members of a member function's own class, and of its base
    classes, through `this->` where `[Relations] thisMode` asks for it (see
    FormatSettings), and drops a `this->` where it does not: by default for a
    base class's member, for an outline member definition in a header (a body
    written outside the class body, such as `void Widget::run() { ... }`,
    where a bare name could read as a free function or a global), and
    wherever the code already says `this->`. The condition's names are those
    of FormatScopeConditions, checked at each member name.

    A member that a parameter or a local shadows is left as it is, and so is
    a static one, every member of a static function, every member in the body
    of a lambda that does not capture `this`, and a name the rule finds no
    class member for. A class's members come from its body in the file, else
    from a header named after the file, else from a header the file quotes in
    an `#include` (see FormatScopeConditions); a base class declared only in
    a header that the file includes with `<...>` adds no members.
*/

/*!
    Tells whether the lambda capture list that the `[` at \a bracket of
    \a document opens captures `this`: it names `this` or `*this`, or it
    starts with the default capture `=` or `&`.
*/
static bool capturesThis(const FormatDocument *document, int bracket)
{
    const int close = document->match(bracket);
    const Token first = document->token(bracket + 1);
    if (first == EQ
        || (first == AND
            && (bracket + 2 == close || document->token(bracket + 2) == COMMA))
    ) {
        return true;
    }
    for (int k = bracket + 1; k < close; ++k) {
        if (document->token(k) != THIS) {
            continue;
        }
        const int start = document->token(k - 1) == STAR ? k - 1 : k;
        const Token before = document->token(start - 1);
        if (before == LBRACK || before == COMMA) {
            return true;
        }
    }
    return false;
}

/*!
    Adds or drops `this->` in front of each member name in each member
    function body of \a document, as `[Relations] thisMode` asks.
*/
void FormatOutlineThisRule::run(FormatDocument *document) const
{
    FormatScopeConditions conditions(document);
    for (int brace = 0; brace < document->symbols().size(); ++brace) {
        if (document->token(brace) != LBRACE
            || document->braceScope(brace) != FormatDocument::FunctionScope
        ) {
            continue;
        }
        const FormatMemberScope scope = conditions.functionScope(brace);
        if (scope.className.isEmpty() || scope.isStatic
            || scope.members.statics.contains(document->lexem(scope.memberToken))
            || (scope.members.names.isEmpty() && scope.baseMembers.isEmpty())
        ) {
            continue;
        }

        // Leaves out the names that a parameter or a local declares.
        const int close = document->match(brace);
        QSet<QByteArray > shadowed = this->localNames(
            document,
            scope.params + 1,
            document->match(scope.params)
        );
        shadowed.unite(this->localNames(document, brace + 1, close));

        QList<FormatGrowth > growths;
        for (int i = brace + 1; i < close; ++i) {
            // Leaves the body of a lambda that does not capture `this` as it
            // is, since `this->` does not compile there.
            const int lambda = lambdaBody(document, i);
            if (lambda >= 0 && ! capturesThis(document, i)) {
                i = document->match(lambda);
                continue;
            }
            if (document->token(i) != IDENTIFIER
                || document->isDirectiveLine(document->symbolLine(i))
            ) {
                continue;
            }
            const QByteArray name = document->lexem(i);
            if (( ! scope.members.names.contains(name) && ! scope.baseMembers.contains(name))
                || scope.members.statics.contains(name) || shadowed.contains(name)
            ) {
                continue;
            }
            const Token before = document->token(i - 1);
            const Token after = document->token(i + 1);
            const bool hasThis = before == ARROW
                && document->token(i - 2) == THIS;
            if (( ! hasThis
                && (before == DOT || before == ARROW || before == SCOPE || before == TILDE
                    || before == GOTO))
                || after == SCOPE || (after == COLON && before == SEMIC)
                || isMacroArgument(document, i)
            ) {
                continue;
            }

            // Adds or drops the `this->` the condition asks for.
            const bool wanted = conditions.holds(
                document->settings().thisMode,
                i
            );
            const int from = document->symbols().at(hasThis ? i - 2 : i).from;
            const int lineStart = document->lineStart(document->symbolLine(i));
            const int column = from - lineStart;
            if (wanted
                && ! hasThis
                && document->replace(from, 0, "this->")
            ) {
                growths << FormatGrowth(i, column, 6);
            } else if ( ! wanted && hasThis) {
                const int length = document->symbols().at(i).from - from;
                if (document->replace(from, length, QByteArray())) {
                    growths << FormatGrowth(i, column, -length);
                }
            }
        }
        formatShiftContinuations(document, growths);
    }
}

/*!
    Builds the names that the tokens from \a from up to \a to of \a document
    declare: a name right after a type, followed by what may end a
    declaration (`;`, `=`, `,`, `)`, `(`, `[`, or a range-for's `:`), and
    the names that declaration lists after commas.
*/
QSet<QByteArray > FormatOutlineThisRule::localNames(
        const FormatDocument *document, int from,
        int to) const
{
    QSet<QByteArray > names;
    for (int i = from; i < to; ++i) {
        if (document->token(i) != IDENTIFIER
            || ! endsType(document->token(i - 1))
        ) {
            continue;
        }
        const Token before = document->token(i - 2);
        if (document->token(i - 1) == IDENTIFIER
            && (before == DOT
                || before == ARROW)
        ) {
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
            while (document->token(name) == STAR
                || document->token(name) == AND
            ) {
                ++name;
            }
            if (document->token(name) == IDENTIFIER) {
                names.insert(document->lexem(name));
            }
        }
    }
    return names;
}
