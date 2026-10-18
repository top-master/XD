// Copyright (C) 2015 The XD Company Ltd.
// SPDX-License-Identifier: Apache-2.0

#include "./comment-helpers.h"

#include "../lang/comment-line.h"

#include <QtCore/QByteArray>
#include <QtCore/QList>
#include <QtCore/QRegularExpression>
#include <QtCore/QPair>
#include <QtCore/QString>
#include <QtCore/QVector>

/*!
    Builds the content lines of the comment at \a index of \a document, at
    least one (an empty one for an empty comment).
*/
QList<FormatCommentLine > commentLines(const FormatDocument *document,
                                       int index)
{
    QList<FormatCommentLine > lines;
    const FormatComment & comment = document->comments().at(index);
    const QByteArray text = document->commentText(index);
    if (comment.isLine()) {
        int start = 2;
        while (start < text.size() && (text.at(start) == '/' || text.at(start) == '!'
            || text.at(start) == '<')
            && start < 4
        ) {
            ++start;
        }
        while (start < text.size()
            && (text.at(start) == ' '
                || text.at(start) == '\t')
        ) {
            ++start;
        }
        QByteArray body = text.mid(start);
        while (body.endsWith(' ')
            || body.endsWith('\t')
            || body.endsWith('\r')
        ) {
            body.chop(1);
        }
        lines << FormatCommentLine(comment.offset + start, body);
        return lines;
    }

    // Splits a block comment into its lines.
    int open = 2;
    while (open < text.size()
        && (text.at(open) == '*'
            || text.at(open) == '!')
        && open < 3
    ) {
        ++open;
    }
    const int close = text.endsWith("*/") ? text.size() - 2 : text.size();
    int from = open;
    while (from <= close) {
        int end = text.indexOf('\n', from);
        if (end < 0 || end > close) {
            end = close;
        }
        int start = from;
        while (start < end
            && (text.at(start) == ' '
                || text.at(start) == '\t')
        ) {
            ++start;
        }
        if (from != open && start < end && text.at(start) == '*'
            && (start + 1 >= end || text.at(start + 1) == ' ')
        ) {
            ++start;
            while (start < end && text.at(start) == ' ') {
                ++start;
            }
        }
        QByteArray body = text.mid(start, end - start);
        while (body.endsWith(' ')
            || body.endsWith('\t')
            || body.endsWith('\r')
        ) {
            body.chop(1);
        }
        lines << FormatCommentLine(comment.offset + start, body);
        from = end + 1;
    }

    // Gives a comment with no content its one empty line.
    if (lines.isEmpty()) {
        lines << FormatCommentLine(comment.offset + open, QByteArray());
    }
    return lines;
}

/*!
    Builds the content lines of the comment group \a group of \a document.
*/
QList<FormatCommentLine > groupLines(const FormatDocument *document,
                                     const QList<int > & group)
{
    QList<FormatCommentLine > lines;
    for (int i = 0; i < group.size(); ++i) {
        lines << commentLines(document, group.at(i));
    }
    return lines;
}

/*!
    Tells whether the comment group \a lines is one word, with no space in
    it (such as the text of a data row, `// ept-enco`): such a comment
    is a label, not a sentence.
*/
bool isSingleWord(const QList<FormatCommentLine > & lines)
{
    int words = 0;
    for (int i = 0; i < lines.size(); ++i) {
        const QByteArray & text = lines.at(i).text;
        if (text.isEmpty()) {
            continue;
        }
        if (text.contains(' ') || text.contains('\t')) {
            return false;
        }
        ++words;
    }
    return words == 1;
}

/*!
    Tells whether the comment group \a lines reads as code put out of use
    (a line ends like a statement or a block does). A line that ends with a
    `;` while the next line goes on with its sentence (it starts lower-case
    and does not end like code) is prose; hence where a wrap breaks a
    sentence does not decide it.
*/
bool looksLikeCode(const QList<FormatCommentLine > & lines)
{
    for (int i = 0; i < lines.size(); ++i) {
        const QByteArray & text = lines.at(i).text;
        if (text.startsWith('#') || text.endsWith('{') || text.endsWith('}')
            || text.endsWith("*/")
        ) {
            return true;
        }
        if ( ! text.endsWith(';')) {
            continue;
        }

        // Reads a `;` that the next line's sentence goes on from as prose.
        const QByteArray next = i + 1 < lines.size()
            ? lines.at(i + 1).text : QByteArray();
        const bool goesOn = ! next.isEmpty() && next.at(0) >= 'a' && next.at(0) <= 'z'
            && ! next.endsWith(';') && ! next.endsWith('{') && ! next.endsWith('}');
        if ( ! goesOn) {
            return true;
        }
    }
    return false;
}

/*!
    Tells, for each of \a lines, whether it sits in a fenced code block (or
    is one of the block's fences).
*/
QVector<bool > fencedLines(const QList<FormatCommentLine > & lines)
{
    QVector<bool > fenced(lines.size(), false);
    bool inFence = false;
    for (int i = 0; i < lines.size(); ++i) {
        const bool isFence = lines.at(i).text.startsWith("```");
        fenced[i] = inFence || isFence;
        if (isFence) {
            inFence = ! inFence;
        }
    }
    return fenced;
}

/*!
    Builds \a text with \a map applied to each part outside of backticks; the
    parts in backticks (inline code) stay as they are.
*/
QString mapProse(const QString & text, QString (*map)(const QString &, const void *),
                 const void *context)
{
    QString result;
    int from = 0;
    while (from <= text.size()) {
        int tick = text.indexOf(QLatin1Char('`'), from);
        if (tick < 0) {
            result += map(text.mid(from), context);
            break;
        }
        result += map(text.mid(from, tick - from), context);
        const int close = text.indexOf(QLatin1Char('`'), tick + 1);
        if (close < 0) {
            result += text.mid(tick);
            break;
        }
        result += text.mid(tick, close - tick + 1);
        from = close + 1;
    }
    return result;
}

/*!
    Groups the comments of \a document into whole comments: the `//` comments
    on lines of their own that follow each other at one column, with the same
    doc-comment marker, form one group, and every other
    comment forms a group of its own.
*/
QList<QList<int > > commentGroups(const FormatDocument *document)
{
    QList<QList<int > > groups;
    for (int i = 0; i < document->comments().size(); ++i) {
        const FormatComment & comment = document->comments().at(i);
        if ( ! groups.isEmpty()
            && comment.isLine()
            && ! comment.trailing
        ) {
            const int previous = groups.last().last();
            const FormatComment & before = document->comments().at(previous);
            const int line = document->commentLine(i);
            if (before.isLine() && ! before.trailing && before.kind == comment.kind
                && document->commentLine(previous) == line - 1
                && comment.offset - document->lineStart(line)
                == before.offset - document->lineStart(line - 1)
            ) {
                groups.last().append(i);
                continue;
            }
        }
        groups.append(QList<int >() << i);
    }
    return groups;
}

/*!
    Tells whether the comment at \a index of \a document labels a closing
    line, which keeps its trailing comment: a line whose code is only braces
    and semicolons, or an `#else` or `#endif` directive.
*/
bool isClosingLabel(const FormatDocument *document, int index)
{
    const FormatComment & comment = document->comments().at(index);
    if ( ! comment.trailing) {
        return false;
    }
    const int line = document->commentLine(index);
    const QByteArray code = document->text().mid(
        document->lineStart(line),
        comment.offset - document->lineStart(line)
    ).trimmed();
    if (code.startsWith('#')) {
        const QByteArray directive = code.mid(1).trimmed();
        return directive.startsWith("endif") || directive.startsWith("else");
    }
    for (int i = 0; i < code.size(); ++i) {
        const char c = code.at(i);
        if (c != '}' && c != ';' && c != ' ' && c != '\t') {
            return false;
        }
    }
    return ! code.isEmpty();
}

/*!
    Tells whether the comment at \a index of \a document labels a value
    inside a line of code (such as a block comment that names the parameter
    an argument passes): code stands on both sides of it.
*/
bool isInlineLabel(const FormatDocument *document, int index)
{
    const FormatComment & comment = document->comments().at(index);
    const int end = comment.offset + comment.length;
    return comment.trailing
        && ! document->isOnlyWhitespace(
            end,
            document->lineEnd(document->commentEndLine(index))
        );
}

/*!
    Builds the offset ranges of the data tables of \a document whose comments
    no rule touches: the braced initializer that follows a comment saying
    `format: table`, up to its closing brace.
*/
QList<QPair<int, int > > tableRanges(const FormatDocument *document)
{
    QList<QPair<int, int > > ranges;
    for (int i = 0; i < document->comments().size(); ++i) {
        if ( ! document->commentText(i).contains("format: table")) {
            continue;
        }
        const FormatComment & comment = document->comments().at(i);
        for (int k = document->symbolAfter(comment.offset + comment.length);
             k < document->symbols().size();
             ++k
        ) {
            const Token token = document->token(k);
            if (token == SEMIC) {
                break;
            }
            if (token == LBRACE && document->match(k) > k) {
                ranges << qMakePair(
                    document->symbols().at(k).from,
                    document->symbolEnd(document->match(k))
                );
                break;
            }
        }
    }
    return ranges;
}

/*!
    Builds \a text with each part outside of backticks cleared to spaces, so
    that a check reads only prose, at the same offsets.
*/
QString proseOnly(const QString & text)
{
    QString result = text;
    bool inCode = false;
    for (int i = 0; i < result.size(); ++i) {
        if (result.at(i) == QLatin1Char('`')) {
            inCode = ! inCode;
            result[i] = QLatin1Char(' ');
        } else if (inCode) {
            result[i] = QLatin1Char(' ');
        }
    }
    return result;
}

/*!
    Matches the marks of a code name in a word: `::`, `->`, `()`, an
    underscore, or a lower-case letter followed by an upper-case one.
*/
Q_GLOBAL_STATIC_WITH_ARGS(QRegularExpression, codeWordPattern,
                          (QString(QLL("::|->|\\(\\)|_|[a-z][A-Z]"))))

/*!
    Tells whether \a word of a comment (with any punctuation around it) is a
    code keyword rather than a plain word: a name in backticks, or one that
    carries the marks of code, such as `readAll()` or
    `QSslConfiguration::setSessionTicketKey`.
*/
bool isCodeWord(const QString & word)
{
    return word.contains(QLatin1Char('`'))
        || codeWordPattern()->match(word).hasMatch();
}

/*!
    Tells whether \a word (in any case) points to what follows it, as an
    article (`a`, `an`, `the`), a conjunction (`and`, `or`, `but`, `nor`),
    or a relative word (`where`, `which`, `that`, `when`, `while`, `whose`,
    `whom`) does; a wrapped line may end on such a word, since the reader
    reads on to the next line for it anyway.
*/
bool pointsAhead(const QString & word)
{
    static const char *const words[] = {
        "a", "an", "the", "and", "or", "but", "nor", "where", "which", "that", "when",
        "while", "whose", "whom"
    };
    const QString lower = word.toLower();
    for (size_t i = 0; i < sizeof(words) / sizeof(words[0]); ++i) {
        if (lower == QLatin1String(words[i])) {
            return true;
        }
    }
    return false;
}
