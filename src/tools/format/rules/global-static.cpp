// Copyright (C) 2015 The XD Company Ltd.
// SPDX-License-Identifier: Apache-2.0

#include "./global-static.h"

#include "../format-document.h"

#include <QtCore/QString>

/*!
    \class FormatGlobalStaticRule
    \internal

    Reports a static object of a Qt class (a cache, a registry, a table),
    which `Q_GLOBAL_STATIC` builds safely across threads, and tears down
    safely when the program exits.
*/

/*!
    Finds each `static` object whose type is a Qt class, in a function or at
    file or namespace scope (a class's static member is left out).
*/
void FormatGlobalStaticRule::run(FormatDocument *document) const
{
    for (int i = 0; i + 2 < document->symbols().size(); ++i) {
        if (document->token(i) != STATIC || document->scopeOf(i) == FormatDocument::ClassScope
            || document->isDirectiveLine(document->symbolLine(i))
        ) {
            continue;
        }
        int k = i + 1;
        while (document->token(k) == CONST
            || document->token(k) == VOLATILE
        ) {
            ++k;
        }
        if (document->token(k) != IDENTIFIER || ! document->lexem(k).startsWith('Q')
            || document->lexem(k).size() < 2 || ! QChar::isUpper(document->lexem(k).at(1))
        ) {
            continue;
        }
        ++k;
        if (document->token(k) == LANGLE) {
            int depth = 0;
            for (; k < document->symbols().size(); ++k) {
                const Token token = document->token(k);
                depth += token == LANGLE
                    ? 1
                    : (token == RANGLE ? -1 : (token == GTGT ? -2 : 0));
                if (depth <= 0 || token == SEMIC) {
                    break;
                }
            }
            ++k;
        }
        if (document->token(k) != IDENTIFIER) {
            continue;
        }
        const Token after = document->token(k + 1);
        const bool inFunction = document->scopeOf(i) == FormatDocument::FunctionScope
            || document->scopeOf(i) == FormatDocument::BlockScope;
        if (after == SEMIC
            || after == EQ
            || (inFunction
                && after == LPAREN)
        ) {
            document->report(document->symbols().at(i).from, "global-static",
                             QString(QLL("hold the static object in a Q_GLOBAL_STATIC")));
        }
    }
}
