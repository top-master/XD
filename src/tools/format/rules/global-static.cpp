// Copyright (C) 2015 The XD Company Ltd.
// SPDX-License-Identifier: Apache-2.0

#include "./global-static.h"

#include "../format-document.h"
#include "../parser/code-helpers.h"

#include <QtCore/QString>

/*!
    @class FormatGlobalStaticRule
    @internal

    Reports a static object of a Qt class (a cache, a registry, a table),
    which `Q_GLOBAL_STATIC` builds safely across threads, and tears down
    safely when the program exits. A static of any other type that a
    function declares, such as a table of `char` pairs, goes to file scope
    (an unnamed namespace): a reader then finds it beside the other
    statics, and no compiler has to guard its first use. A short list that
    only helps a loop repeat a task may stay. The rule cannot tell the two
    apart; hence in agent mode (`--ai`), it asks the agent to, with the
    `global-static.md` asset as the prompt.
*/

/*!
    Finds each `static` object whose type is a Qt class, in a function or at
    file or namespace scope (a class's static member is left out), and each
    other `static` variable that a function declares.
*/
void FormatGlobalStaticRule::run(FormatDocument *document) const
{
    for (int i = 0; i + 2 < document->symbols().size(); ++i) {
        if (document->token(i) != STATIC
            || document->scopeOf(i) == FormatDocument::ClassScope
            || document->isDirectiveLine(document->symbolLine(i))
        ) {
            continue;
        }
        const bool inFunction = document->scopeOf(i) == FormatDocument::FunctionScope
            || document->scopeOf(i) == FormatDocument::BlockScope;
        int k = i + 1;
        while (document->token(k) == CONST
            || document->token(k) == VOLATILE
        ) {
            ++k;
        }
        if (document->token(k) != IDENTIFIER
            || ! document->lexem(k).startsWith('Q')
            || document->lexem(k).size() < 2
            || ! QChar::isUpper(document->lexem(k).at(1))
        ) {
            if (inFunction && this->isVariable(document, i)) {
                const QString message = QString(QLL(
                    "move the static mapping out of the function, to file scope "
                    "(an unnamed namespace); a short list to loop task(s) over "
                    "each entry can stay"));
                const int offset = document->symbols().at(i).from;
                if (document->agentMode()) {
                    document->request(
                        offset,
                        "global-static",
                        message,
                        1,
                        0
                    );
                } else {
                    document->report(offset, "global-static", message);
                }
            }
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

/*!
    Tells whether the `static` at @a index of @a document declares a
    variable: its type is followed by a name and then by a `;`, `=`, `[`,
    `{` or `(` (which may start the variable's initializer).
*/
bool FormatGlobalStaticRule::isVariable(
        const FormatDocument *document,
        int index) const
{
    int depth = 0;
    for (int k = index + 1; k + 1 < document->symbols().size(); ++k) {
        const Token token = document->token(k);
        if (token == LANGLE) {
            ++depth;
        } else if (token == RANGLE || token == GTGT) {
            depth -= token == GTGT ? 2 : 1;
        } else if (depth <= 0 && token == IDENTIFIER) {
            const Token next = document->token(k + 1);
            if (k > index + 1
                && (next == SEMIC || next == EQ || next == LBRACK
                    || next == LBRACE || next == LPAREN)
            ) {
                return true;
            }
        } else if (depth <= 0 && token != SCOPE && token != STAR && token != AND
            && token != CONST && token != VOLATILE && ! isBuiltinType(token)
        ) {
            return false;
        }
    }
    return false;
}
