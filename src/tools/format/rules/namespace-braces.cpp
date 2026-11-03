// Copyright (C) 2015 The XD Company Ltd.
// SPDX-License-Identifier: Apache-2.0

#include "./namespace-braces.h"

#include "../format-document.h"
#include "../parser/layout-helpers.h"

#include <QtCore/QByteArray>
#include <QtCore/QGlobalStatic>
#include <QtCore/QRegularExpression>

Q_GLOBAL_STATIC_WITH_ARGS(QRegularExpression, knownLabelPattern, (
    QString(QLL("^//\\s*(end\\s+(of\\s+)?)?namespace(\\s+[`'\"]?[\\w:]*[`'\"]?)?\\s*\\.?$")),
    QRegularExpression::CaseInsensitiveOption
))

/*!
    @class FormatNamespaceBracesRule
    @internal

    Gives a namespace's braces lines of their own: the line of
    `namespace xyz {` ends at its `{`, the text after it moving to the next
    line at the same indent (a namespace does not indent its body), and the
    namespace's `}` stands on a line of its own, at the indent of the line
    of the `{`. A named namespace's `}` is followed by the comment
    ` // Namespace `xyz`.`, which takes the place of a label of a known form
    after it (such as ` // namespace xyz`, see isKnownLabel()) and of any
    comment that does not name the namespace; an anonymous namespace takes
    none. A comment of its own that names the namespace (the last part of
    its name), after the `}` or on the line above it, stays as it is.

    A `using namespace` directive, a namespace alias, and a namespace with a
    directive between its braces stay as they are.
*/

/*!
    Tells whether @a comment is a label of a known form for the `}` of a
    namespace: `// namespace`, `// end namespace xyz`, or
    `// end of namespace xyz`, in any case, with the name in quotes or none,
    and with or without a period.
*/
static bool isKnownLabel(const QByteArray & comment)
{
    return knownLabelPattern()->match(QString::fromUtf8(comment)).hasMatch();
}

/*!
    Tells whether @a comment names @a name as a whole word, as a comment
    that someone wrote for the `}` of that namespace does.
*/
static bool namesNamespace(const QByteArray & comment, const QByteArray & name)
{
    const QString escaped = QRegularExpression::escape(QString::fromUtf8(name));
    const QRegularExpression word(QLL("\\b") + escaped + QLL("\\b"));
    return word.match(QString::fromUtf8(comment)).hasMatch();
}

/*!
    Lays out the braces of each namespace of @a document.
*/
void FormatNamespaceBracesRule::run(FormatDocument *document) const
{
    for (int i = 0; i + 1 < document->symbols().size(); ++i) {
        if (document->token(i) != NAMESPACE
            || (i > 0 && document->token(i - 1) == USING)
        ) {
            continue;
        }

        // Finds the `{`, after the name; a `=` or a `;` first makes an alias.
        int open = i + 1;
        while (open < document->symbols().size()
            && document->token(open) != LBRACE
            && document->token(open) != EQ
            && document->token(open) != SEMIC
        ) {
            ++open;
        }
        if (open >= document->symbols().size()
            || document->token(open) != LBRACE
        ) {
            continue;
        }
        const int close = document->match(open);
        if (close < 0 || document->spansDirective(
                document->symbols().at(open).from,
                document->symbolEnd(close)
            )
        ) {
            continue;
        }
        const QByteArray name = open > i + 1
            ? sourceText(document, i + 1, open - 1).simplified() : QByteArray();
        const int openLine = document->symbolLine(open);
        const QByteArray indent = document->indentOf(openLine);
        const int closeLine = document->symbolLine(close);

        // Ends the opening line at the `{`.
        if (open + 1 <= close && document->symbolLine(open + 1) == document->symbolLine(open)
            && ! document->hasCommentBetween(
                document->symbolEnd(open),
                document->symbols().at(open + 1).from
            )
        ) {
            const int from = document->symbolEnd(open);
            document->replace(
                from,
                document->symbols().at(open + 1).from - from,
                "\n" + indent
            );
        }

        // Puts the `}` on a line of its own.
        if (document->firstSymbolOfLine(closeLine) != close
            && ! document->hasCommentBetween(
                document->symbolEnd(close - 1),
                document->symbols().at(close).from
            )
        ) {
            const int from = document->symbolEnd(close - 1);
            document->replace(
                from,
                document->symbols().at(close).from - from,
                "\n" + indent
            );
        }

        // Labels the `}` of a named namespace, and moves a token after it to
        // the next line; an anonymous namespace keeps the comment it has.
        const bool hasNext = close + 1 < document->symbols().size()
            && document->symbolLine(close + 1) == closeLine;
        if (hasNext && document->token(close + 1) == SEMIC) {
            continue;
        }
        const int from = document->symbolEnd(close);
        const int to = hasNext
            ? document->symbols().at(close + 1).from : document->lineEnd(closeLine);
        const QByteArray rest = document->text().mid(from, to - from);
        const QByteArray comment = rest.trimmed();
        if ( ! comment.isEmpty() && ! comment.startsWith("//")) {
            continue;
        }

        // Keeps a comment of its own that already names the namespace, after
        // the `}` or on the line above it.
        const int scope = name.lastIndexOf("::");
        const QByteArray lastName = scope < 0 ? name : name.mid(scope + 2);
        const QByteArray above = closeLine > 1
            ? document->lineText(closeLine - 1).trimmed() : QByteArray();
        const bool namesAfter = ! comment.isEmpty() && ! isKnownLabel(comment)
            && namesNamespace(comment, lastName);
        const bool namesAbove = comment.isEmpty()
            && document->firstSymbolOfLine(closeLine) == close
            && above.startsWith("//") && namesNamespace(above, lastName);
        if ( ! name.isEmpty() && (namesAfter || namesAbove)) {
            continue;
        }
        QByteArray wanted;
        if ( ! name.isEmpty()) {
            wanted = " // Namespace `" + name + "`.";
        } else if ( ! comment.isEmpty()) {
            wanted = " " + comment;
        }
        if (hasNext) {
            wanted += "\n" + indent;
        }
        if (rest != wanted) {
            document->replace(from, to - from, wanted);
        }
    }
}
