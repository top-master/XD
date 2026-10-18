// Copyright (C) 2015 The XD Company Ltd.
// SPDX-License-Identifier: Apache-2.0

#include "./const-overload.h"

#include "../format-document.h"
#include "../parser/class-helpers.h"

#include <QtCore/QByteArray>
#include <QtCore/QHash>
#include <QtCore/QList>
#include <QtCore/QSet>
#include <QtCore/QString>

/*!
    @class FormatConstOverloadRule
    @internal

    Reports a const and non-const overload pair whose non-const member is
    declared first; some supported compilers then bind a call from a const
    context to the non-const one.
*/

/*!
    Finds, in each class body of @a document, the member functions of one
    name and parameter count that differ by `const`, and reports the pair
    whose non-const member comes first.
*/
void FormatConstOverloadRule::run(FormatDocument *document) const
{
    for (int brace = 0; brace < document->symbols().size(); ++brace) {
        if (document->token(brace) != LBRACE
            || document->braceScope(brace) != FormatDocument::ClassScope
        ) {
            continue;
        }
        QHash<QByteArray, int > firstMutable;
        QSet<QByteArray > reported;
        const QList<int > tokens = classBodyTokens(document, brace);
        for (int t = 0; t < tokens.size(); ++t) {
            const int i = tokens.at(t);
            if (document->token(i) != IDENTIFIER || document->token(i + 1) != LPAREN
                || ! endsType(document->token(i - 1)) || document->match(i + 1) < 0
            ) {
                continue;
            }
            const int close = document->match(i + 1);
            int commas = 0;
            for (int k = i + 2; k < close; ++k) {
                if (document->token(k) == LPAREN
                    || document->token(k) == LANGLE
                ) {
                    k = qMax(k, document->match(k));
                } else if (document->token(k) == COMMA) {
                    ++commas;
                }
            }
            const QByteArray key = document->lexem(i) + '/'
                + QByteArray::number(commas);
            const bool isConst = document->token(close + 1) == CONST;
            if ( ! isConst && ! firstMutable.contains(key)) {
                firstMutable.insert(key, i);
            } else if (isConst
                && firstMutable.contains(key)
                && ! reported.contains(key)
            ) {
                reported.insert(key);
                document->report(
                    document->symbols().at(firstMutable.value(key)).from,
                    "const-overload-order",
                    QString(QLL("declare the const overload of %1() first")) .arg(QString::fromLatin1(document->lexem(i)))
                );
            }
        }
    }
}
