// Copyright (C) 2015 The XD Company Ltd.
// SPDX-License-Identifier: Apache-2.0

#include "./moc-run.h"

#include "../format-document.h"

/*!
    @class FormatMocRunRule
    @internal

    Spells moc's own macro `Q_MOC_RUN`, which moc predefines, where code
    names `QT_MOC_RUN`, which nothing defines.
*/

/*!
    Renames each `QT_MOC_RUN` token to `Q_MOC_RUN`.
*/
void FormatMocRunRule::run(FormatDocument *document) const
{
    for (int i = 0; i < document->symbols().size(); ++i) {
        if (document->token(i) == IDENTIFIER
            && document->lexem(i) == "QT_MOC_RUN"
        ) {
            document->replace(document->symbols().at(i).from, document->symbols().at(i).len,
                              "Q_MOC_RUN");
        }
    }
}
