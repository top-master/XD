// Copyright (C) 2015 The XD Company Ltd.
// SPDX-License-Identifier: Apache-2.0

#include "./member-init.h"

#include "../format-document.h"
#include "../parser/code-helpers.h"

#include <QtCore/QString>

/*!
    \class FormatMemberInitRule
    \internal

    Reports a data member that a class initializes where it declares it,
    which some supported compilers reject; the constructor initializes it.
*/

/*!
    Finds each declaration in a class body that assigns a value, other than
    a static member, a pure or defaulted function, and a type alias.
*/
void FormatMemberInitRule::run(FormatDocument *document) const
{
    int start = 0;
    for (int i = 0; i < document->symbols().size(); ++i) {
        const Token token = document->token(i);
        if (document->scopeOf(i) != FormatDocument::ClassScope) {
            continue;
        }
        if (token == SEMIC || token == LBRACE || token == RBRACE
            || (token == COLON && (document->token(i - 1) == PUBLIC
                || document->token(i - 1) == PRIVATE
                || document->token(i - 1) == PROTECTED
                || document->token(i - 1) == SLOTS
                || document->token(i - 1) == SIGNALS))
        ) {
            start = i + 1;
            continue;
        }
        if (token == LPAREN) {
            i = qMax(i, document->match(i));
            continue;
        }
        if (token != EQ) {
            continue;
        }
        bool skip = document->token(i - 1) == RPAREN || document->token(i - 1) == CONST
            || document->token(i - 1) == OPERATOR;
        for (int k = start; k < i && ! skip; ++k) {
            const Token head = document->token(k);
            skip = head == STATIC || head == TYPEDEF || head == USING || head == ENUM
                || head == FRIEND || head == TEMPLATE
                || (head == IDENTIFIER && document->lexem(k) == "constexpr");
        }
        if ( ! skip) {
            document->report(document->symbols().at(i).from, "member-init",
                             QString(QLL("initialize the member in the constructor's "
                                         "initializer list instead")));
        }
        const int end = statementEnd(document, i);
        i = end < 0 ? i : end - 1;
    }
}
