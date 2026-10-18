// Copyright (C) 2015 The XD Company Ltd.
// SPDX-License-Identifier: Apache-2.0

#include "./excludes.h"

#include "../format-document.h"

#include <QtCore/QByteArray>
#include <QtCore/QString>

/*!
    \class FormatExcludesRule
    \internal

    Reports a negated `includes()` check of a flag set, which the
    `excludes()` check states directly.
*/

/*!
    Finds each `!` whose operand is a call of `includes()` or
    `includesLax()` through a member chain.
*/
void FormatExcludesRule::run(FormatDocument *document) const
{
    for (int i = 0; i + 1 < document->symbols().size(); ++i) {
        if (document->token(i) != NOT) {
            continue;
        }
        int k = i + 1;
        while (document->token(k) == IDENTIFIER
            && (document->token(k + 1) == DOT || document->token(k + 1) == ARROW)
        ) {
            k += 2;
        }
        if (document->token(k) != IDENTIFIER
            || document->token(k + 1) != LPAREN
            || k == i + 1
        ) {
            continue;
        }
        const QByteArray name = document->lexem(k);
        if (name == "includes" || name == "includesLax") {
            document->report(document->symbols().at(i).from, "excludes",
                             QString(QLL("check a cleared flag with %1(), not !%2()"))
                             .arg(name == "includes" ? QLL("excludes") : QLL("excludesLax"))
                             .arg(QString::fromLatin1(name)));
        }
    }
}
