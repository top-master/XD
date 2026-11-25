// Copyright (C) 2015 The XD Company Ltd.
// SPDX-License-Identifier: Apache-2.0

#include "./reference-local.h"

#include "../format-document.h"
#include "../format-file-config.h"
#include "../parser/code-helpers.h"
#include "../parser/layout-helpers.h"

#include <QtCore/QByteArray>
#include <QtCore/QString>

/*!
    @class FormatReferenceLocalRule
    @internal

    Reports a local variable that a non-const reference names, such as
    `int & offset = list[i];`, and asks for a pointer in its place
    (`int *offset = &(list[i]);`, then `*offset`): some compilers mishandle a
    non-const reference local, and a debugger shows a pointer more plainly.
    A `const` reference, a parameter, and a range-based `for` stay as they
    are. In agent mode (`--ai`), the request's prompt comes from the
    `reference-local.md` asset.

    `[Lint] byRefVariable` says what it does: `warn` (the default) reports,
    `error` reports to the error output, and `ignore` reports nothing.
*/

/*!
    Reports each local of @a document whose declaration starts a statement
    in a function's body, and names a non-const reference that an `=`
    initializes.
*/
void FormatReferenceLocalRule::run(FormatDocument *document) const
{
    for (int i = 1; i + 2 < document->symbols().size(); ++i) {
        if (document->token(i) != AND
            || document->token(i + 1) != IDENTIFIER
            || document->token(i + 2) != EQ
            || document->isDirectiveLine(document->symbolLine(i))
        ) {
            continue;
        }
        const int offset = document->symbols().at(i).from;
        const FormatFileConfig &config = document->configAt(offset);
        const QString fallback = config.byRefVariable
            == FormatFileConfig::LintIgnore
            ? QString(QLL("ignore"))
            : config.byRefVariable == FormatFileConfig::LintError
                ? QString(QLL("error")) : QString(QLL("warn"));
        const QString value = settingValueAt(
            document,
            offset,
            QString(QLL("lint.byRefVariable")),
            fallback
        ).toString();
        const FormatFileConfig::LintLevel level =
            FormatFileConfig::lintLevel(value, config);
        if (level == FormatFileConfig::LintIgnore) {
            continue;
        }
        const FormatDocument::Scope scope = document->scopeOf(i);
        if (scope != FormatDocument::FunctionScope
            && scope != FormatDocument::BlockScope
        ) {
            continue;
        }

        // Walks back over the type, and its template arguments, to the
        // start of the statement.
        int start = i - 1;
        bool isConst = false;
        while (start >= 0) {
            const Token token = document->token(start);
            if (token == RANGLE || token == GTGT) {
                int depth = 0;
                for (; start >= 0; --start) {
                    const Token inner = document->token(start);
                    depth += inner == RANGLE
                        ? 1 : (inner == GTGT ? 2 : (inner == LANGLE ? -1 : 0));
                    if (depth <= 0) {
                        break;
                    }
                }
                --start;
                continue;
            }
            if (token == CONST) {
                isConst = true;
            } else if (token != IDENTIFIER
                && token != SCOPE
                && token != STAR
                && token != VOLATILE
                && token != AUTO
                && token != STATIC
                && ! isBuiltinType(token)
            ) {
                break;
            }
            --start;
        }
        const Token before = start >= 0 ? document->token(start) : SEMIC;
        if (isConst
            || start == i - 1
            || (before != SEMIC
                && before != LBRACE
                && before != RBRACE)
        ) {
            continue;
        }

        // Names the pointer that replaces the reference.
        const int typeFrom = document->symbols().at(start + 1).from;
        const QByteArray type = document
            ->text()
            .mid(typeFrom, document->symbolEnd(i - 1) - typeFrom)
            .simplified();
        const QString message = QString(QLL("name the local through a pointer, `%1 *%2 = &(...);`, then use "
            "`*%2`, in place of a non-const reference")).arg(
                QString::fromUtf8(type),
                QString::fromUtf8(document->lexem(i + 1))
            );
        if (document->agentMode()) {
            document->request(offset, "reference-local", message, 1, 0);
        } else {
            document->report(
                offset,
                "reference-local",
                message,
                level == FormatFileConfig::LintError
            );
        }
    }
}
