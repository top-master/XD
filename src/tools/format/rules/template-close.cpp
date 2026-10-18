// Copyright (C) 2015 The XD Company Ltd.
// SPDX-License-Identifier: Apache-2.0

#include "./template-close.h"

#include "../format-document.h"
#include "../parser/code-helpers.h"

#include <QtCore/QList>
#include <QtCore/QPair>

/*!
    @class FormatTemplateCloseRule
    @internal

    Puts a space before each `>` that closes a template's argument list or a
    cast's type, nested ones included (`QList<QPair<int, int > >`).
*/

/*!
    Finds each template argument list and cast type, and spaces its closing
    brackets; a `<` whose tokens up to the closing `>` cannot all name types
    is a comparison, and is left as it is.
*/
void FormatTemplateCloseRule::run(FormatDocument *document) const
{
    for (int i = 1; i < document->symbols().size(); ++i) {
        if (document->token(i) != LANGLE
            || document->isDirectiveLine(document->symbolLine(i))
        ) {
            continue;
        }
        const Token before = document->token(i - 1);
        const bool isCast = before == STATIC_CAST || before == DYNAMIC_CAST
            || before == REINTERPRET_CAST || before == CONST_CAST;
        if ( ! isCast
            && (before != IDENTIFIER
                || document->token(i - 2) == OPERATOR)
        ) {
            continue;
        }
        int end = i;
        if (this->spaceArguments(document, i, &end)) {
            i = end;
        }
    }
}

/*!
    Spaces the closing brackets of the argument list that the `<` at @a open
    of @a document starts, and sets @a end to the index of its last token;
    returns false, with nothing queued, when the list holds a
    token that no type name has.
*/
bool FormatTemplateCloseRule::spaceArguments(
        FormatDocument *document, int open,
        int *end) const
{
    QList<int > closers;
    int depth = 0;
    int i = open;
    for (; i < document->symbols().size(); ++i) {
        const Token token = document->token(i);
        if (token == LANGLE) {
            ++depth;
            continue;
        }
        if (token == RANGLE || token == GTGT) {
            depth -= token == GTGT ? 2 : 1;
            if (depth < 0) {
                return false;
            }
            closers.append(i);
            if (depth == 0) {
                break;
            }
            continue;
        }
        if (token == LPAREN || token == LBRACK) {
            if (document->match(i) < 0) {
                return false;
            }
            i = document->match(i);
            continue;
        }
        if (token != IDENTIFIER && token != SCOPE && token != STAR && token != AND
            && token != CONST && token != VOLATILE && token != COMMA && token != INTEGER_LITERAL
            && ! isBuiltinType(token)
        ) {
            return false;
        }
    }
    if (depth != 0) {
        return false;
    }

    // Leaves a comparison such as `a < b > c` alone.
    const Token after = document->token(i + 1);
    if (after != IDENTIFIER && after != SCOPE && after != LPAREN && after != RPAREN
        && after != COMMA && after != SEMIC && after != AND && after != STAR && after != ANDAND
        && after != LBRACE && after != RANGLE && after != GTGT && after != COLON && after != EQ
    ) {
        return false;
    }

    // Spaces each closer that lacks it; a `>>` closes two lists.
    for (int k = 0; k < closers.size(); ++k) {
        const int index = closers.at(k);
        const int from = document->symbols().at(index).from;
        const bool spaced = from > 0 && document->text().at(from - 1) == ' ';
        if (document->token(index) == GTGT) {
            document->replace(from, 2, spaced ? "> >" : " > >");
        } else if ( ! spaced) {
            document->replace(from, 0, " ");
        }
    }
    *end = i;
    return true;
}
