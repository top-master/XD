// Copyright (C) 2015 The XD Company Ltd.
// SPDX-License-Identifier: Apache-2.0

#include "./reference-space.h"

#include "../format-document.h"
#include "../parser/code-helpers.h"

#include <QtCore/QByteArray>
#include <QtCore/QString>

/*!
    @class FormatReferenceSpaceRule
    @internal

    Spaces a `&` that declares a named reference on both sides
    (`const QString & name`), and a `&&` of an rvalue reference the same way;
    a project's `.formatrc` may set the spacing after each (see
    FormatSettings). A `&` with no name after it (a return type, a cast) and
    an address or a bitwise and are left as they are.
*/

/*!
    Spaces each `&` and `&&` that declares a named reference.
*/
void FormatReferenceSpaceRule::run(FormatDocument *document) const
{
    for (int i = 1; i + 2 < document->symbols().size(); ++i) {
        const Token token = document->token(i);
        if ((token != AND && token != ANDAND) || document->token(i + 1) != IDENTIFIER
            || ! isTypeEnd(document->token(i - 1))
            || document->isDirectiveLine(document->symbolLine(i))
        ) {
            continue;
        }
        const Token after = document->token(i + 2);
        if (after != COMMA && after != RPAREN && after != EQ && after != SEMIC && after != LBRACK
            && after != COLON
        ) {
            continue;
        }
        if ( ! this->isDeclaration(document, i)
            || ! isSameLineGap(document, i - 1, i) || ! isSameLineGap(
                document,
                i,
                i + 1
            )
        ) {
            continue;
        }

        // Puts one space before the operator, and after it what the
        // project's setting asks for; a setting by regular expression whose
        // pattern matches the line replaces it instead.
        const FormatSpacing & setting = token == AND
            ? document->settings().refSuffix : document->settings().moveSuffix;
        const FormatSpacing spacing = document->onSetting(setting, i);
        if (spacing.isNull()) {
            continue;
        }
        const int from = document->symbolEnd(i - 1);
        const int to = document->symbols().at(i + 1).from;
        const int afterOperator = document->symbolEnd(i);
        const QByteArray suffix = spacing.keepAsIs
            ? document->text().mid(afterOperator, to - afterOperator) : spacing.text;
        const QByteArray wanted = " " + document->lexem(i) + suffix;
        if (document->text().mid(from, to - from) != wanted) {
            document->replace(from, to - from, wanted);
        }
    }
}

/*!
    Tells whether the `&` at @a ampersand of @a document declares a name: the
    type before it holds `const` or a template, or it starts a statement, or
    it is a parameter of a function's declaration or definition (a list whose
    name follows a type, or a constructor's own class name).
*/
bool FormatReferenceSpaceRule::isDeclaration(
        const FormatDocument *document,
        int ampersand) const
{
    // Walks back over the type's tokens.
    int start = ampersand - 1;
    bool hasQualifier = false;
    while (start >= 0) {
        const Token token = document->token(start);
        if (token == CONST || token == VOLATILE || token == RANGLE || token == GTGT
            || isBuiltinType(token)
        ) {
            hasQualifier = true;
        }
        if (token == RANGLE || token == GTGT) {
            // Steps back to the template's `<`.
            int depth = 0;
            for (; start >= 0; --start) {
                const Token inner = document->token(start);
                depth += inner == RANGLE
                    ? 1 : (inner == GTGT ? 2 : (inner == LANGLE ? -1 : 0));
                if (depth <= 0) {
                    break;
                }
            }
            --start;
            continue;
        }
        if (token == IDENTIFIER || token == SCOPE || token == CONST || token == VOLATILE
            || token == STAR || isBuiltinType(token)
        ) {
            --start;
            continue;
        }
        break;
    }
    if (hasQualifier) {
        return true;
    }
    const Token before = start >= 0 ? document->token(start) : SEMIC;
    if (before == SEMIC || before == LBRACE || before == RBRACE) {
        return document->token(ampersand + 2) == EQ;
    }
    if (before != LPAREN && before != COMMA) {
        return false;
    }

    // Finds the parameter list's `(`, then what it follows.
    int paren = start;
    while (paren >= 0 && document->token(paren) != LPAREN) {
        const Token token = document->token(paren);
        if (token == RPAREN || token == RBRACK) {
            paren = document->match(paren);
            if (paren < 0) {
                return false;
            }
        } else if (token == SEMIC || token == LBRACE || token == RBRACE) {
            return false;
        }
        --paren;
    }
    if (paren < 1 || document->match(paren) < 0) {
        return false;
    }
    if (document->token(paren - 1) == FOR) {
        return true;
    }
    if (document->token(paren - 1) != IDENTIFIER) {
        return false;
    }
    int name = paren - 1;
    while (name >= 2
        && document->token(name - 1) == SCOPE
        && document->token(name - 2) == IDENTIFIER
    ) {
        name -= 2;
    }
    const Token typeEnd = document->token(name - 1);
    if (isTypeEnd(typeEnd) || typeEnd == AND || typeEnd == TILDE) {
        return true;
    }

    // Takes a constructor: its name repeats its class, or it sits in the class.
    if (name < paren - 1) {
        return document->lexem(paren - 1) == document->lexem(paren - 3);
    }
    return (typeEnd == SEMIC || typeEnd == LBRACE || typeEnd == RBRACE || typeEnd == COLON
        || typeEnd == EXPLICIT || typeEnd == INLINE)
        && document->scopeOf(paren) == FormatDocument::ClassScope;
}
