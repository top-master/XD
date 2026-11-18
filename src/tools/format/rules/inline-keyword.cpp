// Copyright (C) 2015 The XD Company Ltd.
// SPDX-License-Identifier: Apache-2.0

#include "./inline-keyword.h"

#include "../format-document.h"
#include "../parser/class-helpers.h"
#include "../parser/code-helpers.h"

#include <QtCore/QByteArray>
#include <QtCore/QList>
#include <QtCore/QPair>

/*!
    @class FormatInlineKeywordRule
    @internal

    Spells out `inline` on each function a class body defines, when another
    function defined in that body already does; a class whose in-body
    functions all leave it out keeps them that way.

    A function marked `override` or `Q_DECL_OVERRIDE` follows `[Lint]
    inlineOverride`, which a condition may pick, such as
    `hasCompiler("win32-msvc2010") ? "Q_DECL_OVERRIDE_IL" : "override"`:

    * `override` (the default) or `Q_DECL_OVERRIDE`: the marker stays, and
      the function counts like any other;
    * another name, such as `Q_DECL_OVERRIDE_IL`, which XD gives MSVC 2010
      (it rejects `override` together with `inline`, error C2216): the name
      takes the place of the marker on an inline function, which counts like
      any other;
    * empty: an inline function loses its marker; another one keeps it.
*/

/*!
    Adds `inline` to each in-class definition that lacks it, in each class
    body of @a document that has an inline one, and gives an inline
    override the marker `[Lint] inlineOverride` names.
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
        const QString marker = document->configAt(
            document->symbols().at(brace).from
        ).inlineOverride;
        const bool keepsMarker = marker == QLL("override")
            || marker == QLL("Q_DECL_OVERRIDE");
        // Lists the overrides that lack `inline`, with their markers.
        QList<QPair<int, int > > lackingOverrides;
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
            int overrideToken = -1;
            const int start = this->declarationStart(
                document,
                body,
                &hasInline,
                &exempt,
                &overrideToken
            );

            // Gives an inline override the marker of `[Lint] inlineOverride`.
            if (overrideToken >= 0 && ! keepsMarker) {
                if (hasInline) {
                    this->respell(document, overrideToken, marker);
                    anyInline = true;
                } else if (marker.isEmpty() || exempt || start < 0) {
                    continue;
                } else {
                    lackingOverrides.append(qMakePair(start, overrideToken));
                }
                continue;
            }
            anyInline = anyInline || hasInline;
            if ( ! hasInline && ! exempt && start >= 0) {
                lacking.append(start);
            }
        }
        if ( ! anyInline) {
            continue;
        }
        for (int i = 0; i < lacking.size(); ++i) {
            this->addInline(document, lacking.at(i));
        }
        for (int i = 0; i < lackingOverrides.size(); ++i) {
            const QPair<int, int > item = lackingOverrides.at(i);
            this->addInline(document, item.first);
            this->respell(document, item.second, marker);
        }
    }
}

/*!
    Adds `inline` in front of the declaration that starts at the token
    @a start of @a document, and moves its head's later lines right as far,
    up to the line that closes its parameters; hence the parameters stay
    lined up under the `(`.
*/
void FormatInlineKeywordRule::addInline(
        FormatDocument *document,
        int start) const
{
    const QByteArray keyword("inline ");
    document->replace(document->symbols().at(start).from, 0, keyword);
    const int line = document->symbolLine(start);
    int open = start;
    while (open < document->symbols().size()
        && document->token(open) != LPAREN
    ) {
        ++open;
    }
    const int close = open < document->symbols().size()
        ? document->match(open) : -1;
    const int last = close >= 0 ? document->symbolLine(close) : line;
    const int indent = textWidth(document->indentOf(line));
    for (int l = line + 1; l <= last; ++l) {
        if (textWidth(document->indentOf(l)) > indent) {
            document->replace(document->lineStart(l), 0, QByteArray(
                keyword.size(), ' '
            ));
        }
    }
}

/*!
    Puts @a marker in place of the `override` or `Q_DECL_OVERRIDE` at
    @a overrideToken of @a document; an empty @a marker removes that marker
    together with the space before it.
*/
void FormatInlineKeywordRule::respell(
        FormatDocument *document,
        int overrideToken, const QString & marker) const
{
    const int end = document->symbolEnd(overrideToken);
    if (marker.isEmpty()) {
        const int from = document->symbolEnd(overrideToken - 1);
        document->replace(from, end - from, QByteArray());
        return;
    }
    const int from = document->symbols().at(overrideToken).from;
    document->replace(from, end - from, marker.toLatin1());
}

/*!
    Builds the index of the first token of the declaration whose body opens
    at @a body of @a document (after an access specifier and a template
    head, and the macros that lead it), and sets @a hasInline when it says
    `inline` or names a macro with `INLINE` in it (such as `Q_ALWAYS_INLINE`),
    @a exempt for a `friend` or `constexpr` one, which needs no keyword, and
    @a overrideToken to the index of an `override` or `Q_DECL_OVERRIDE`
    after its parameters.
*/
int FormatInlineKeywordRule::declarationStart(
        const FormatDocument *document, int body,
        bool *hasInline, bool *exempt, int *overrideToken) const
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
                if (close < 0
                    || close + 1 >= body
                    || document->symbolLine(close + 1)
                        == document->symbolLine(close)
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
    int parameters = -1;
    for (int i = start; i < body; ++i) {
        const Token token = document->token(i);
        if (token == LPAREN) {
            parameters = i;
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

    // Looks for `override` between the parameters and the body.
    const int close = parameters >= 0 ? document->match(parameters) : -1;
    for (int i = close + 1; close > 0 && i < body; ++i) {
        const QByteArray name = document->lexem(i);
        if (name == "override" || name == "Q_DECL_OVERRIDE") {
            *overrideToken = i;
        }
    }
    return start < body ? start : -1;
}
