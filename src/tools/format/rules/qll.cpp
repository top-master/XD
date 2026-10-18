// Copyright (C) 2015 The XD Company Ltd.
// SPDX-License-Identifier: Apache-2.0

#include "./qll.h"

#include "../format-document.h"

#include <QtCore/QByteArray>
#include <QtCore/QString>

/*!
    \class FormatQllRule
    \internal

    Reports a Latin-1 view of a string literal spelled `QLatin1String(...)`
    or `QLatin1Literal(...)`, which new logic writes as XD's shorter
    `QLL(...)`. It reports instead of renaming: `QLL` names `QLatin1Literal`,
    a subclass of `QLatin1String`, hence the switch changes the type, and
    only a build shows that the code still compiles.
*/

/*!
    Reports each `QLatin1String` or `QLatin1Literal` whose one argument is a
    string literal, outside of preprocessor directives.
*/
void FormatQllRule::run(FormatDocument *document) const
{
    for (int i = 0; i + 3 < document->symbols().size(); ++i) {
        if (document->token(i) != IDENTIFIER || document->token(i + 1) != LPAREN
            || document->token(i + 2) != STRING_LITERAL || document->token(i + 3) != RPAREN
            || document->isDirectiveLine(document->symbolLine(i))
        ) {
            continue;
        }
        const QByteArray name = document->lexem(i);
        if (name == "QLatin1String" || name == "QLatin1Literal") {
            document->report(document->symbols().at(i).from, "qll",
                             QString(QLL("in new logic, write QLL(...) and check that it "
                                         "compiles")));
        }
    }
}
