// Copyright (C) 2015 The XD Company Ltd.
// SPDX-License-Identifier: Apache-2.0

#include "./no-lambda.h"

#include "../format-document.h"
#include "../format-rules.h"
#include "../parser/code-helpers.h"

#include <QtCore/QList>
#include <QtCore/QString>

/*!
    \class FormatNoLambdaRule
    \internal

    Reports each lambda in XD's own source tree, which builds on compilers
    that reject a lambda inside another one, and each lambda nested in
    another lambda anywhere else.
*/

/*!
    Finds each lambda (a `[...]` capture list, where an expression starts,
    followed by its parameters or body) and reports it.
*/
void FormatNoLambdaRule::run(FormatDocument *document) const
{
    const bool inXd = formatIsInXdTree(document->path());
    QList<int > bodies;
    for (int i = 0; i < document->symbols().size(); ++i) {
        while ( ! bodies.isEmpty() && i > document->match(bodies.last())) {
            bodies.removeLast();
        }
        const int body = lambdaBody(document, i);
        if (body < 0) {
            continue;
        }
        if ( ! bodies.isEmpty()) {
            document->report(document->symbols().at(i).from, "no-lambda",
                             QString(QLL("move the outer lambda into a named function: "
                                         "a lambda never encloses another one")));
        } else if (inXd) {
            document->report(document->symbols().at(i).from, "no-lambda",
                             QString(QLL("replace the lambda with a named member or "
                                         "static function")));
        }
        bodies.append(body);
    }
}
