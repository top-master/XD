// Copyright (C) 2015 The XD Company Ltd.
// SPDX-License-Identifier: Apache-2.0

#include "./inline-keyword.h"

#include "../format-document.h"
#include "../parser/class-helpers.h"

#include <QtCore/QByteArray>
#include <QtCore/QList>

/*!
    @class FormatInlineKeywordRule
    @internal

    Spells out `inline` on each function a class body defines, when another
    function defined in that body already does; a class whose in-body
    functions all leave it out keeps them that way.
*/

/*!
    Adds `inline` to each in-class definition that lacks it, in each class
    body of @a document that has an inline one.
*/
void FormatInlineKeywordRule::run(FormatDocument *document) const
{
    for (int brace = 0; brace < document->symbols().size(); ++brace) {
        if (document->token(brace) != LBRACE
            || document->braceScope(brace) != FormatDocument::ClassScope
        ) {
            continue;
        }
        QList<int > lacking;
        bool anyInline = false;
        const QList<int > tokens = classBodyTokens(document, brace);
        for (int t = 0; t < tokens.size(); ++t) {
            const int body = tokens.at(t);
            if (document->token(body) != LBRACE
                || document->braceScope(body) != FormatDocument::FunctionScope
            ) {
                continue;
            }
            bool hasInline = false;
            bool exempt = false;
            const int start = this->declarationStart(
                document,
                body,
                &hasInline,
                &exempt
            );
            anyInline = anyInline || hasInline;
            if ( ! hasInline && ! exempt && start >= 0) {
                lacking.append(start);
            }
        }
        if ( ! anyInline) {
            continue;
        }
        for (int i = 0; i < lacking.size(); ++i) {
            document->replace(document->symbols().at(lacking.at(i)).from, 0, "inline ");
        }
    }
}

/*!
    Builds the index of the first token of the declaration whose body opens
    at @a body of @a document (after an access specifier and a template
    head, and the macros that lead it), and sets @a hasInline when it says
    `inline` or names a macro with `INLINE` in it (such as `Q_ALWAYS_INLINE`),
    and @a exempt for a `friend` or `constexpr` one, which needs no keyword.
*/
int FormatInlineKeywordRule::declarationStart(
        const FormatDocument *document, int body,
        bool *hasInline, bool *exempt) const
{
    int start = document->headStart(body);

    // Leaves a head that a directive cuts short: its start is not known.
    if (start > 0
        && document->isDirectiveLine(document->symbolLine(start - 1))
    ) {
        return -1;
    }
    // Steps over what leads the declaration, in any order, until nothing
    // more does: access specifiers, template heads, and macros.
    for (;;) {
        const int before = start;
        for (;;) {
            const Token token = document->token(start);
            const bool isAccess = token == PUBLIC || token == PRIVATE
                || token == PROTECTED;
            int label = start + (isAccess ? 1 : 0);
            const Token kind = document->token(label);
            if (kind == SLOTS
                || kind == SIGNALS
                || kind == Q_SLOTS_TOKEN
                || kind == Q_SIGNALS_TOKEN
            ) {
                ++label;
            }
            if (label > start && document->token(label) == COLON) {
                start = label + 1;
            } else if (token == TEMPLATE
                && document->token(start + 1) == LANGLE
            ) {
                int depth = 0;
                for (++start; start < body; ++start) {
                    const Token inner = document->token(start);
                    depth += inner == LANGLE
                        ? 1
                        : (inner == RANGLE ? -1 : (inner == GTGT ? -2 : 0));
                    if (depth <= 0) {
                        break;
                    }
                }
                ++start;
            } else {
                break;
            }
        }
        // Steps over the macros that lead the declaration (moc gives some of
        // them tokens of their own): one that expands to a template head, and a
        // statement-like macro call that ends its line (such as
        // `Q_DECLARE_FLAGS(...)`, which takes no `;`).
        while (start < body && isMacroName(document->lexem(start))
            && document->token(start + 1) != SCOPE
        ) {
            const QByteArray name = document->lexem(start);
            if (document->token(start + 1) == LPAREN) {
                const int close = document->match(start + 1);
                if (close < 0 || close + 1 >= body
                    || document->symbolLine(close + 1) == document->symbolLine(close)
                ) {
                    break;
                }
                start = close + 1;
                continue;
            }
            if (name.contains("INLINE")) {
                *hasInline = true;
            }
            ++start;
        }
        if (start == before) {
            break;
        }
    }
    for (int i = start; i < body; ++i) {
        const Token token = document->token(i);
        if (token == LPAREN) {
            break;
        }
        const QByteArray name = token == IDENTIFIER
            ? document->lexem(i) : QByteArray();
        if (token == INLINE || name.contains("INLINE")) {
            *hasInline = true;
        }
        if (token == FRIEND || name == "constexpr" || name == "Q_DECL_CONSTEXPR"
            || name == "Q_DECL_RELAXED_CONSTEXPR"
        ) {
            *exempt = true;
        }
    }
    return start < body ? start : -1;
}
