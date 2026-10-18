// Copyright (C) 2015 The XD Company Ltd.
// SPDX-License-Identifier: Apache-2.0

#include "./not-space.h"

#include "../format-document.h"

#include <QtCore/QByteArray>

/*!
    \class FormatNotSpaceRule
    \internal

    Puts a space on each side of a logical not, so that the `!` stands out:
    `if (!ready)` becomes `if ( ! ready)`. A run of signs (`!!x`) keeps them
    together and spaces the run; a line's indent already counts as the space
    in front. The `!=` operator, `operator!`, and preprocessor lines are left
    as they are, and so is a side that has its space already.
*/

/*!
    Adds the missing spaces around each `!` of \a document.
*/
void FormatNotSpaceRule::run(FormatDocument *document) const
{
    const QByteArray & text = document->text();
    for (int i = 0; i < document->symbols().size(); ++i) {
        if (document->token(i) != NOT || document->isDirectiveLine(document->symbolLine(i))
            || (i > 0 && document->token(i - 1) == OPERATOR)
        ) {
            continue;
        }
        const int from = document->symbols().at(i).from;
        const int end = document->symbolEnd(i);

        // Adds the space in front of the first sign of a run.
        if (i == 0 || document->token(i - 1) != NOT) {
            const char before = from > 0 ? text.at(from - 1) : '\n';
            if (before != ' ' && before != '\t' && before != '\n') {
                document->replace(from, 0, " ");
            }
        }

        // Adds the space after the last sign of a run.
        if (i + 1 < document->symbols().size()
            && document->token(i + 1) != NOT
        ) {
            const char after = end < text.size() ? text.at(end) : '\n';
            if (after != ' '
                && after != '\t'
                && after != '\n'
                && after != '\r'
            ) {
                document->replace(end, 0, " ");
            }
        }
    }
}
