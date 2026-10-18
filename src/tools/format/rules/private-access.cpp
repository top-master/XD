// Copyright (C) 2015 The XD Company Ltd.
// SPDX-License-Identifier: Apache-2.0

#include "./private-access.h"

#include "../format-document.h"
#include "../parser/code-helpers.h"

#include <QtCore/QByteArray>
#include <QtCore/QString>

/*!
    @class FormatPrivateAccessRule
    @internal

    Reports a bare `d_func()` or `q_func()` call, which `Q_D(Class)` and
    `Q_Q(Class)` spell out.
*/

/*!
    Finds each `d_func()` and `q_func()` call outside of a directive.
*/
void FormatPrivateAccessRule::run(FormatDocument *document) const
{
    for (int i = 0; i + 1 < document->symbols().size(); ++i) {
        if (document->token(i) != IDENTIFIER || document->token(i + 1) != LPAREN
            || document->isDirectiveLine(document->symbolLine(i))
        ) {
            continue;
        }
        const QByteArray name = document->lexem(i);
        if (name != "d_func" && name != "q_func") {
            continue;
        }
        const Token before = document->token(i - 1);
        if (isTypeEnd(before) || before == AND) {
            continue;
        }
        document->report(
            document->symbols().at(i).from,
            "q-d-macro",
            QString(QLL("write Q_%1(<Class>) instead of %2()")) .arg(QLatin1Char(name.at(0) == 'd' ? 'D' : 'Q')) .arg(QString::fromLatin1(name))
        );
    }
}
