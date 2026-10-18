// Copyright (C) 2015 The XD Company Ltd.
// SPDX-License-Identifier: Apache-2.0

#include "./qpointer-getter.h"

#include "../format-document.h"

#include <QtCore/QByteArray>
#include <QtCore/QSet>
#include <QtCore/QString>

/*!
    \class FormatQPointerGetterRule
    \internal

    Reports a getter that hands out the raw pointer of a `QPointer` member
    (`return m_widget.data();`), where returning the `QPointer` itself keeps
    the caller's copy null once the object is deleted.
*/

/*!
    Lists the `QPointer` fields of \a document, then reports each `return`
    that hands out the `data()` of one of them.
*/
void FormatQPointerGetterRule::run(FormatDocument *document) const
{
    QSet<QByteArray > guarded;
    for (int i = 0; i + 1 < document->symbols().size(); ++i) {
        if (document->token(i) != IDENTIFIER || document->lexem(i) != "QPointer"
            || document->token(i + 1) != LANGLE
        ) {
            continue;
        }
        int k = i + 1;
        int depth = 0;
        for (; k < document->symbols().size(); ++k) {
            const Token token = document->token(k);
            depth += token == LANGLE
                ? 1 : (token == RANGLE ? -1 : (token == GTGT ? -2 : 0));
            if (depth <= 0) {
                break;
            }
        }
        if (document->token(k + 1) == IDENTIFIER
            && document->token(k + 2) == SEMIC
        ) {
            guarded.insert(document->lexem(k + 1));
        }
    }
    for (int i = 0; i + 5 < document->symbols().size(); ++i) {
        if (document->token(i) != RETURN) {
            continue;
        }
        int k = i + 1;
        if (document->token(k) == THIS
            && document->token(k + 1) == ARROW
        ) {
            k += 2;
        }
        if (document->token(k) == IDENTIFIER && guarded.contains(document->lexem(k))
            && document->token(k + 1) == DOT && document->token(k + 2) == IDENTIFIER
            && document->lexem(k + 2) == "data" && document->token(k + 3) == LPAREN
            && document->token(k + 4) == RPAREN
        ) {
            document->report(document->symbols().at(i).from, "qpointer-getter",
                             QString(QLL("return the QPointer itself, not its data()")));
        }
    }
}
