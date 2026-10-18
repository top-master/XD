// Copyright (C) 2015 The XD Company Ltd.
// SPDX-License-Identifier: Apache-2.0

#include "./doc-placement.h"

#include "../format-document.h"
#include "../lang/comment-line.h"
#include "../parser/comment-helpers.h"

#include <QtCore/QByteArray>
#include <QtCore/QDir>
#include <QtCore/QFileInfo>
#include <QtCore/QList>
#include <QtCore/QRegularExpression>
#include <QtCore/QString>

/*!
    Matches a QDoc or Doxygen topic command, a command
    that names its symbol itself.
*/
Q_GLOBAL_STATIC_WITH_ARGS(QRegularExpression, docTopicPattern, (QString(QLL(
        "[\\\\@](class|fn|enum|typedef|variable|macro|namespace|page|module|headerfile"
        "|property|file|struct|union|def)\\b"))))

/*!
    Matches a field doc-comment that opens by saying
    that the field stores a value.
*/
Q_GLOBAL_STATIC_WITH_ARGS(QRegularExpression, boxOpenerPattern, (QString(QLL("^(Stores|Holds|Keeps|Contains)\\b"))))

/*!
    Matches a doc-comment that opens with a vague `Returns` or `Gets`.
*/
Q_GLOBAL_STATIC_WITH_ARGS(QRegularExpression, vagueOpenerPattern, (QString(QLL("^(Returns|Gets)\\b"))))

/*!
    \class FormatDocPlacementRule
    \internal

    Reports the comments a symbol's documentation should not be: a plain
    `//` block on a declaration outside of a function body (a doc-comment
    documents a symbol), a comment on a header's declaration whose source
    file holds the definition, a doc-comment with a blank line or another
    doc-comment before its symbol, and a field doc-comment that only says
    that the field stores a value.
*/

/*!
    Builds the path of the source file that matches the header at \a path
    (its name without a `_p`, with a C or C++ source suffix), or an empty
    string when \a path is no header or no such file exists.
*/
QString FormatDocPlacementRule::headerSource(const QString & path) const
{
    const QFileInfo info(path);
    const QString suffix = info.suffix();
    if (suffix != QLL("h")
        && suffix != QLL("hh")
        && suffix != QLL("hpp")
        && suffix != QLL("hxx")
    ) {
        return QString();
    }
    QString stem = info.completeBaseName();
    if (stem.endsWith(QLL("_p"))) {
        stem.chop(2);
    }
    const char *const sources[] = { "cpp", "cc", "cxx", "c", "mm" };
    for (size_t i = 0; i < sizeof(sources) / sizeof(sources[0]); ++i) {
        const QString candidate = info.dir().filePath(stem + QLatin1Char('.')
                                                      + QString::fromLatin1(sources[i]));
        if (QFileInfo(candidate).isFile()) {
            return candidate;
        }
    }
    return QString();
}

/*!
    Checks where \a group sits, and reports a misplaced documentation.
*/
void FormatDocPlacementRule::runGroup(FormatDocument *document,
                                      const QList<int > & group) const
{
    const FormatComment & first = document->comments().at(group.first());
    const int lastIndex = group.last();
    const FormatComment & last = document->comments().at(lastIndex);
    if (first.trailing) {
        return;
    }
    const QList<FormatCommentLine > lines = groupLines(document, group);
    const QByteArray head = lines.isEmpty()
        ? QByteArray() : lines.first().text;
    if (head.startsWith("MARK:") || looksLikeCode(lines)) {
        return;
    }

    // Finds the code after the group, and the scope it sits in.
    const int end = last.offset + last.length;
    const int next = document->symbolAfter(end);
    if (next >= document->symbols().size()) {
        return;
    }
    const int nextLine = document->symbolLine(next);
    const FormatDocument::Scope scope = document->scopeOf(next);
    const bool atDeclarationScope = scope == FormatDocument::FileScope
        || scope == FormatDocument::NamespaceScope || scope == FormatDocument::ClassScope;
    const Token token = document->token(next);
    const bool isMember = token != PUBLIC && token != PRIVATE && token != PROTECTED
        && token != SIGNALS && token != SLOTS && token != RBRACE
        && ! document->isDirectiveLine(nextLine);
    const bool attached = nextLine == document->commentEndLine(lastIndex) + 1;

    // Tells a declaration (it ends with `;`) from a definition (with a body).
    bool isDeclaration = false;
    bool isField = false;
    for (int i = next; i < document->symbols().size(); ++i) {
        const Token inner = document->token(i);
        if (inner == LPAREN) {
            i = qMax(i, document->match(i));
            continue;
        }
        if (inner == SEMIC || inner == LBRACE) {
            isDeclaration = inner == SEMIC;
            break;
        }
    }
    for (int i = next; i < document->symbols().size(); ++i) {
        const Token inner = document->token(i);
        if (inner == LPAREN || inner == LBRACE) {
            break;
        }
        if (inner == SEMIC || inner == EQ || inner == LBRACK) {
            isField = true;
            break;
        }
    }

    // Reports a comment on a header declaration that its source defines.
    if (atDeclarationScope && isMember && attached && isDeclaration && ! isField
        && token != CLASS && token != STRUCT && token != TYPEDEF && token != USING
        && token != FRIEND && token != ENUM
    ) {
        const QString source = this->headerSource(document->path());
        if ( ! source.isEmpty()) {
            document->report(first.offset, "doc-placement",
                             QString(QLL("move the documentation to the definition in %1"))
                             .arg(QFileInfo(source).fileName()));
            return;
        }
    }

    // Reports a plain comment block on a declaration.
    if ( ! first.isDoc()
        && first.isLine()
        && atDeclarationScope
        && isMember
        && attached
    ) {
        document->report(first.offset, "doc-placement",
                         QString(QLL("document the symbol with a /*! or /// doc-comment, "
                                     "not a // block")));
        return;
    }
    if ( ! first.isDoc()) {
        return;
    }

    // Reports an orphan doc-comment; a QDoc topic names its symbol itself.
    QByteArray all;
    for (int i = 0; i < lines.size(); ++i) {
        all += lines.at(i).text + '\n';
    }
    if (docTopicPattern()->match(QString::fromUtf8(all)).hasMatch()) {
        return;
    }
    const int nextComment = lastIndex + 1;
    if (nextComment < document->comments().size()
        && document->comments().at(nextComment).isDoc()
        && document->isOnlyWhitespace(end, document->comments().at(nextComment).offset)
        && document->comments().at(nextComment).offset < document->symbols().at(next).from
    ) {
        document->report(first.offset, "doc-placement",
                         QString(QLL("drop or move the doc-comment: another one follows it "
                                     "with no code in between")));
        return;
    }
    if (atDeclarationScope && isMember && ! attached
        && document->isOnlyWhitespace(end, document->symbols().at(next).from)
    ) {
        document->report(first.offset, "doc-placement",
                         QString(QLL("keep the doc-comment right above its symbol, "
                                     "with no blank line")));
        return;
    }

    // Reports a field's doc-comment that describes a box of memory.
    const QString opener = QString::fromUtf8(head);
    if (isField
        && atDeclarationScope
        && boxOpenerPattern()->match(opener).hasMatch()
    ) {
        document->report(first.offset, "doc-placement",
                         QString(QLL("say what the value is good for, not that it stores "
                                     "something")));
    } else if (vagueOpenerPattern()->match(opener).hasMatch()) {
        document->report(first.offset, "doc-placement",
                         QString(QLL("open with the exact action, such as Builds, not "
                                     "Returns or Gets")));
    }
}
