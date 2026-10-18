// Copyright (C) 2015 The XD Company Ltd.
// SPDX-License-Identifier: Apache-2.0

#include "./code-backticks.h"

#include "../format-document.h"
#include "../lang/comment-line.h"
#include "../parser/comment-helpers.h"

#include <QtCore/QList>
#include <QtCore/QRegularExpression>
#include <QtCore/QSet>
#include <QtCore/QString>
#include <QtCore/QVector>

/*!
    Matches a word of a comment that may name code: an access chain (such as
    `watcher()->load()`), a qualified name, a call, or a plain name.
*/
Q_GLOBAL_STATIC_WITH_ARGS(QRegularExpression, codeNamePattern, (QString(QLL(
        "(?<![\\w`:./<>#@\\\\-])((?:[A-Za-z_]\\w*(?:\\(\\))?->)+[A-Za-z_]\\w*(?:\\(\\))?"
        "|(?:[A-Za-z_]\\w*::)+~?[A-Za-z_]\\w*(?:\\(\\))?"
        "|[A-Za-z_]\\w*\\(\\)|[A-Za-z_]\\w*)(?![\\w`:/(<-])"))))

/*!
    Matches a name spelled the way code is: with an underscore, or with an
    upper-case letter after a lower-case one, or a Qt class name.
*/
Q_GLOBAL_STATIC_WITH_ARGS(QRegularExpression, codeLikePattern, (QString(QLL("_|[a-z][A-Z]|^Q[A-Z][a-z]"))))

/*!
    Builds @a text with each bare name from the code wrapped in backticks:
    an access chain such as `watcher()->load()`, a call such as `run()`, a
    qualified name such as `Cache::Miss`, and a name of @a names (the names
    the document's code uses) that reads as code (it holds an underscore, or
    an upper-case letter after a lower-case one); with @a chainsOnly set,
    only the access chains.
*/
static QString backtickNames(const QString & text, const QSet<QString > *names,
                             bool chainsOnly)
{
    QString result;
    int from = 0;
    QRegularExpressionMatchIterator it = codeNamePattern()->globalMatch(text);
    while (it.hasNext()) {
        const QRegularExpressionMatch found = it.next();
        const QString name = found.captured(1);
        const bool isChain = name.contains(QLL("->"));
        const bool isCall = name.endsWith(QLL("()"))
            || name.contains(QLL("::"));
        const bool isKnown = names->contains(name)
            && codeLikePattern()->match(name).hasMatch();
        if ( ! isChain && (chainsOnly || ( ! isCall && ! isKnown))) {
            continue;
        }

        // Leaves a word of a path or an address alone.
        const int chunkStart = text.lastIndexOf(
            QLatin1Char(' '),
            found.capturedStart()
        ) + 1;
        int chunkEnd = text.indexOf(QLatin1Char(' '), found.capturedEnd());
        chunkEnd = chunkEnd < 0 ? text.size() : chunkEnd;
        const QString chunk = text.mid(chunkStart, chunkEnd - chunkStart);
        if (chunk.contains(QLL("://")) || chunk.contains(QLatin1Char('/'))
            || chunk.contains(QLatin1Char('\\'))
        ) {
            continue;
        }
        result += text.mid(from, found.capturedStart() - from);
        result += QLatin1Char('`') + name + QLatin1Char('`');
        from = found.capturedEnd();
    }
    result += text.mid(from);
    return result;
}

/*!
    Builds @a text with each bare code name wrapped in backticks (see
    backtickNames()), for a plain comment; @a context is the set of names
    that the code uses.
*/
static QString mapCodeNames(const QString & text, const void *context)
{
    return backtickNames(text, static_cast<const QSet<QString > * >(context), false);
}

/*!
    Builds @a text with each bare access chain wrapped in backticks, for a
    doc-comment, which leaves its other names bare; @a context is the set of
    names the code uses.
*/
static QString mapAccessChains(const QString & text, const void *context)
{
    return backtickNames(text, static_cast<const QSet<QString > * >(context), true);
}

/*!
    @class FormatCodeBackticksRule
    @internal

    Wraps each bare name from the code in backticks, in a plain comment. A
    doc-comment leaves its names bare, for the doc generator to link, except
    an access chain (such as `watcher()->load()`), which Doxygen cannot link
    and which then reads as code. In agent mode (`--ai`), the rule asks the
    agent to name the member such a chain reaches by its class instead (see
    FormatDocument::request()): a doc-comment names a member that way, unless
    the chain shows an expression as an example.
*/

/*!
    Wraps the code names of each line of @a group (only its access chains in
    a doc-comment), when it does not read as code put out of use.
*/
void FormatCodeBackticksRule::runGroup(
        FormatDocument *document,
        const QList<int > & group) const
{
    const bool isDoc = document->comments().at(group.first()).isDoc();
    const QList<FormatCommentLine > lines = groupLines(document, group);
    if (looksLikeCode(lines)) {
        return;
    }
    QSet<QString > names;
    for (int i = 0; i < document->symbols().size(); ++i) {
        if (document->token(i) == IDENTIFIER) {
            names.insert(QString::fromLatin1(document->lexem(i)));
        }
    }
    const QVector<bool > fenced = fencedLines(lines);
    for (int i = 0; i < lines.size(); ++i) {
        if (fenced.at(i) || lines.at(i).text.startsWith("MARK:")) {
            continue;
        }
        const QString text = QString::fromUtf8(lines.at(i).text);

        // Asks an agent to name the member a doc-comment's access chain
        // reaches, which only the meaning tells.
        if (isDoc && document->agentMode()) {
            const QString prose = proseOnly(text);
            QRegularExpressionMatchIterator chains = codeNamePattern()->globalMatch(prose);
            while (chains.hasNext()) {
                const QString chain = chains.next().captured(1);
                if (chain.contains(QLL("->"))) {
                    document->request(
                        lines.at(i).offset,
                        "code-backticks",
                        QString(QLL("name the member that `%1` reaches by its " "class (such as `Class::member()`), which " "Doxygen links; keep the chain, in " "backticks, only when it shows an " "expression as an example")).arg(chain)
                    );
                }
            }
            continue;
        }
        const QString mapped = mapProse(
            text,
            isDoc ? mapAccessChains : mapCodeNames,
            &names
        );
        if (mapped != text) {
            document->replace(lines.at(i).offset, lines.at(i).text.size(), mapped.toUtf8());
        }
    }
}
