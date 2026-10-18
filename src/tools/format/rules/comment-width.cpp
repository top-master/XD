// Copyright (C) 2015 The XD Company Ltd.
// SPDX-License-Identifier: Apache-2.0

#include "./comment-width.h"

#include "../format-document.h"
#include "../parser/comment-helpers.h"

#include <QtCore/QRegularExpression>
#include <QtCore/QVector>

/*!
    Matches the marker that opens a list item of a comment (`*`, `-`, or a
    number with a period), with the spaces after it.
*/
Q_GLOBAL_STATIC_WITH_ARGS(QRegularExpression, itemPattern, (QString(QLL("^(?:[*-]|\\d+\\.)\\s+"))))

/*!
    Matches a separator line of a comment: a rule of dashes, equal signs,
    stars, or the like, with no word in it.
*/
Q_GLOBAL_STATIC_WITH_ARGS(QRegularExpression, separatorPattern, (QString(QLL("^[-=*_~#/+]+$"))))

/*!
    Tells whether the comment line \a text ends a paragraph and is never
    wrapped into one: a blank line, a command line (`\class`, `@file`), or
    a separator line (a rule of dashes).
*/
static bool isParagraphBreak(const QByteArray & text)
{
    return text.isEmpty() || text.startsWith('\\') || text.startsWith('@')
        || separatorPattern()->match(QString::fromUtf8(text)).hasMatch();
}

/*!
    \class FormatCommentWidthRule
    \internal

    Wraps each paragraph of a comment that has a line past the column limit
    (`[Comments] wrapMax`, 80 by default), from its first line on.
    Each line breaks at the space nearest the wrap column (`[Comments] wrapBegin`,
    75 by default) that follows a `,`, `;`, `)` or `.`, or comes before a `(`,
    since a line reads best when it ends there. Such a space counts only
    within 25 columns before the wrap column and not past the limit
    (with `[Comments] wrapComma = soft`, the default, within `[Comments] wrapEmptyLimit`
    columns for a `,` after a word, which a sentence holds many of;
    a `),` counts as the `)`).

    Without such a space, a word that points to what follows (see pointsAhead(),
    such as `the` or `where`) ends the line, when it ends within `wrapEmptyLimit`
    columns before the wrap column; else the last plain space up to the
    wrap column breaks the line, or, when a word reaches past it, the
    first one within the limit. No break falls inside a backtick code span,
    a word too long for any line gets a line of its own, and a last line of
    17 characters or fewer takes words from the line before it.

    A code keyword (see isCodeWord()) that the break would push to the
    next line, while the break leaves at least `[Comments] wrapEmptyLimit`
    columns free before the wrap column (8 by default),
    stays on the line instead: the line may then pass the limit by up to
    `[Comments] wrapExtendsForKeywords` columns (20 by default), and fits()
    counts it as fitting.

    `wrapBegin` may list several columns (`75 | 51`): when the last line of
    one run fills less than half of its width, the next column runs, and the
    first run whose last line fills at least half wins (else the one whose
    last line fills the most).

    With `[Comments] target = any` (or `--any-comment`), a paragraph also
    wraps again when one of its lines ends `wrapEmptyLimit` columns or more
    short of the wrap column while the first word of the next line would
    still fit on it (a code keyword within the extension), since such a break
    wastes the room. The default, `target = long`, leaves such a paragraph
    as it is; hence a diff touches only the comments that are too long.

    A paragraph is a run of the comment's lines with one prefix (the code and
    markers before the text), up to a blank line, a fenced block, a command
    line (`\class`, `@file`), a separator line (a rule of dashes), or a list
    item, whose own lines go on under its text; a line that ends with a
    hyphen after a letter joins the next one without a space. A comment at
    the end of a line of code is not wrapped, and a line that no break brings
    within the limit is reported.
*/

/*!
    Finds the first space of \a text after \a from that is outside backtick
    code, or the end of \a text.
*/
static int nextSpace(const QString & text, int from)
{
    bool inCode = false;
    for (int i = 0; i <= from && i < text.size(); ++i) {
        if (text.at(i) == QLatin1Char('`')) {
            inCode = ! inCode;
        }
    }
    for (int i = from + 1; i < text.size(); ++i) {
        if (text.at(i) == QLatin1Char('`')) {
            inCode = ! inCode;
        } else if (text.at(i) == QLatin1Char(' ') && ! inCode) {
            return i;
        }
    }
    return text.size();
}

/*!
    Builds the lines of \a text wrapped at \a column, the first line after a
    prefix of \a firstWidth columns and every other line after one of
    \a nextWidth columns, within the limits of \a settings (see
    FormatCommentWidthRule).
*/
QStringList FormatCommentWidthRule::wrap(const QString & text, int firstWidth, int nextWidth,
                                         int column, const FormatSettings & settings)
{
    const int limit = settings.commentWrapMax;
    QStringList lines;
    QString rest = text;
    int width = firstWidth;
    while (width + rest.size() > column) {
        // Finds each space a line may break at, outside of backtick code.
        int punctuation = -1;
        int pointing = -1;
        int plainBefore = -1;
        int plainAfter = -1;
        int wordStart = 0;
        bool inCode = false;
        for (int i = 0; i < rest.size(); ++i) {
            const QChar c = rest.at(i);
            if (c == QLatin1Char('`')) {
                inCode = ! inCode;
                continue;
            }
            if (c != QLatin1Char(' ') || inCode || i == 0) {
                continue;
            }
            const int end = width + i;
            if (end > limit) {
                break;
            }
            const QChar before = rest.at(i - 1);
            const bool afterPunctuation = before == QLatin1Char(',') || before == QLatin1Char(';')
                || before == QLatin1Char(')') || before == QLatin1Char('.');
            const bool beforeParenthesis = i + 1 < rest.size()
                && rest.at(i + 1) == QLatin1Char('(');
            const bool wordComma = before == QLatin1Char(',')
                && (i < 2 || rest.at(i - 2) != QLatin1Char(')'))
                && settings.wrapComma == FormatSettings::WrapCommaSoft;
            const int window = wordComma ? settings.wrapEmptyLimit : 25;
            if ((afterPunctuation || beforeParenthesis) && end >= column - window
                && (punctuation < 0 || qAbs(end - column) < qAbs(width + punctuation - column))
            ) {
                punctuation = i;
            }
            if (end <= column) {
                plainBefore = i;
            } else if (plainAfter < 0) {
                plainAfter = i;
            }
            if (end <= column && end >= column - settings.wrapEmptyLimit
                && pointsAhead(rest.mid(wordStart, i - wordStart))
            ) {
                pointing = i;
            }
            wordStart = i + 1;
        }
        int at = punctuation >= 0 ? punctuation
            : pointing >= 0 ? pointing : plainBefore >= 0 ? plainBefore : plainAfter;

        // Gives a word too long for any line (such as a long code keyword)
        // a line of its own.
        if (at < 0) {
            at = nextSpace(rest, 0);
        }

        // Keeps a code keyword that the break would push to the next line,
        // when the break leaves enough room unused.
        const int after = nextSpace(rest, at);
        if (column - (width + at) >= settings.wrapEmptyLimit && width + after > column
            && width + after <= limit + settings.wrapExtendsForKeywords
            && isCodeWord(rest.mid(at + 1, after - at - 1))
        ) {
            at = after;
        }

        // Keeps a line of 17 characters or fewer from standing alone, when
        // the next word fits on it.
        if (at < rest.size() && at <= 17) {
            const int next = nextSpace(rest, at);
            const QString word = rest.mid(at + 1, next - at - 1);
            const int room = limit
                + (isCodeWord(word) ? settings.wrapExtendsForKeywords : 0);
            if (width + next <= room) {
                at = next;
            }
        }

        // Keeps the rest on this line, up to the limit, rather than leave a
        // last line of 17 characters or fewer.
        if (at < rest.size()
            && rest.size() - at - 1 <= 17
            && width + rest.size() <= limit
        ) {
            break;
        }
        if (at >= rest.size()) {
            break;
        }
        lines << rest.left(at);
        rest = rest.mid(at + 1);
        width = nextWidth;
    }
    lines << rest;

    // Moves words down from the line before a last line of 17 characters
    // or fewer, while that line keeps more than 17 and the last one fits.
    while (lines.size() > 1 && lines.last().size() <= 17) {
        const QString & before = lines.at(lines.size() - 2);
        int space = -1;
        bool inCode = false;
        for (int i = 0; i < before.size(); ++i) {
            if (before.at(i) == QLatin1Char('`')) {
                inCode = ! inCode;
            } else if (before.at(i) == QLatin1Char(' ') && ! inCode) {
                space = i;
            }
        }
        const QString moved = before.mid(space + 1) + QLatin1Char(' ')
            + lines.last();
        if (space <= 17 || nextWidth + moved.size() > limit) {
            break;
        }
        lines[lines.size() - 2] = before.left(space);
        lines.last() = moved;
    }
    return lines;
}

/*!
    Tells whether \a line, a whole line of the document, fits the limits of
    \a settings: within `wrapMax` columns, or, when it ends on a code keyword
    (see isCodeWord()), within `wrapMax` and `wrapExtendsForKeywords` columns.
*/
bool FormatCommentWidthRule::fits(const QString & line,
                                  const FormatSettings & settings)
{
    if (line.size() <= settings.commentWrapMax) {
        return true;
    }
    if (line.size() > settings.commentWrapMax + settings.wrapExtendsForKeywords) {
        return false;
    }
    const QString trimmed = line.trimmed();
    return isCodeWord(trimmed.mid(trimmed.lastIndexOf(QLatin1Char(' ')) + 1));
}

/*!
    Wraps the paragraphs of \a group that have a line past the column limit,
    and reports each such line that no wrap fixes.
*/
void FormatCommentWidthRule::runGroup(FormatDocument *document,
                                      const QList<int > & group) const
{
    bool trailing = false;
    for (int g = 0; g < group.size(); ++g) {
        trailing = trailing || document->comments().at(group.at(g)).trailing;
    }
    const QList<FormatCommentLine > lines = groupLines(document, group);
    const QVector<bool > fenced = fencedLines(lines);

    // Splits the lines into paragraphs, and wraps each one that runs long.
    bool wrapped = false;
    int i = trailing ? lines.size() : 0;
    while (i < lines.size()) {
        const QByteArray & text = lines.at(i).text;
        const int start = document->lineStart(document->lineOf(lines.at(i).offset));
        const QByteArray prefix = document->text().mid(
            start,
            lines.at(i).offset - start
        );
        if (fenced.at(i) || isParagraphBreak(text)) {
            ++i;
            continue;
        }
        const QRegularExpressionMatch item = itemPattern()->match(QString::fromUtf8(text));
        const QByteArray nextPrefix = item.hasMatch()
            ? prefix + QByteArray(item.capturedLength(), ' ') : prefix;
        int count = 1;
        while (i + count < lines.size()) {
            const FormatCommentLine & next = lines.at(i + count);
            const int nextStart = document->lineStart(document->lineOf(next.offset));
            if (fenced.at(i + count) || isParagraphBreak(next.text)
                || itemPattern()->match(QString::fromUtf8(next.text)).hasMatch()
                || document->text().mid(nextStart, next.offset - nextStart) != nextPrefix
            ) {
                break;
            }
            ++count;
        }
        wrapped = this->rewrap(document, lines, i, count, nextPrefix)
            || wrapped;
        i += count;
    }
    if (wrapped) {
        return;
    }

    // Reports each line that stays past the column limit.
    for (int g = 0; g < group.size(); ++g) {
        const int first = document->commentLine(group.at(g));
        const int last = document->commentEndLine(group.at(g));
        for (int line = first; line <= last; ++line) {
            const QString text = QString::fromUtf8(document->lineText(line));
            if ( ! FormatCommentWidthRule::fits(text, document->settings())) {
                document->report(
                    document->lineStart(line),
                    "comment-width",
                    QString(QLL("wrap the comment at %1 columns (it has %2)"))
                        .arg(document->settings().commentWrapMax).arg(text.size())
                );
            }
        }
    }
}

/*!
    Wraps the \a count lines of \a lines from \a first on, a paragraph whose
    lines after the first start with \a nextPrefix, when one of them runs
    past the column limit, or, with `[Comments] target = any`, when one ends
    `wrapEmptyLimit` columns or more short of the wrap column while the first
    word of the next line would fit on it; returns whether it queued a
    change.
*/
bool FormatCommentWidthRule::rewrap(FormatDocument *document,
                                    const QList<FormatCommentLine > & lines,
                                    int first, int count, const QByteArray & nextPrefix) const
{
    // Checks for a line past the column limit.
    const FormatSettings & settings = document->settings();
    const int firstLine = document->lineOf(lines.at(first).offset);
    bool tooLong = false;
    for (int k = 0; k < count && ! tooLong; ++k) {
        const int line = document->lineOf(lines.at(first + k).offset);
        tooLong = ! FormatCommentWidthRule::fits(
            QString::fromUtf8(document->lineText(line)),
            settings
        );
    }

    // Checks, for `[Comments] target = any`, for a line that ends well short
    // of the wrap column while the first word of the next line would still
    // fit on it, where the wrap would place it: up to the wrap column, or
    // past the limit as far as a code keyword may go.
    const int column = qMin(settings.commentWrapBegin.first(), settings.commentWrapMax);
    const int lineStart = document->lineStart(firstLine);
    const int firstPrefix = QString::fromUtf8(document->text().mid(
        lineStart,
        lines.at(first).offset - lineStart
    )).size();
    const bool anyTarget = settings.commentTarget == FormatSettings::CommentTargetAny;
    bool underfilled = false;
    for (int k = 0; anyTarget && k + 1 < count && ! tooLong && ! underfilled; ++k) {
        const QString text = QString::fromUtf8(lines.at(first + k).text);
        const QString next = QString::fromUtf8(lines.at(first + k + 1).text);
        const int width = (k == 0 ? firstPrefix : QString::fromUtf8(nextPrefix).size())
            + text.size();
        const QString word = next.left(nextSpace(next, -1));
        const int room = isCodeWord(word)
            ? settings.commentWrapMax + settings.wrapExtendsForKeywords : column;
        underfilled = column - width >= settings.wrapEmptyLimit
            && width + 1 + word.size() <= room;
    }
    if ( ! tooLong && ! underfilled) {
        return false;
    }

    // Joins the paragraph's text into one line; a word that a hyphen split
    // over two lines joins whole.
    QString text;
    for (int k = 0; k < count; ++k) {
        const QString part = QString::fromUtf8(lines.at(first + k).text);
        const bool hyphenated = text.size() >= 2 && text.endsWith(QLatin1Char('-'))
            && text.at(text.size() - 2).isLetter() && ! part.isEmpty() && part.at(0).isLetter();
        if ( ! text.isEmpty() && ! hyphenated) {
            text += QLatin1Char(' ');
        }
        text += part;
    }
    const int start = document->lineStart(firstLine);
    const int firstWidth = QString::fromUtf8(document->text().mid(
        start,
        lines.at(first).offset - start
    ))
        .size();
    const int nextWidth = QString::fromUtf8(nextPrefix).size();

    // Wraps it at each column in turn, until the last line of a run fills
    // half of its width.
    const QList<int > & columns = settings.commentWrapBegin;
    QStringList best;
    double bestFill = -1;
    for (int c = 0; c < columns.size(); ++c) {
        const int column = qMin(columns.at(c), settings.commentWrapMax);
        const QStringList wrapped = FormatCommentWidthRule::wrap(
            text,
            firstWidth,
            nextWidth,
            column,
            settings
        );
        const int room = column
            - (wrapped.size() == 1 ? firstWidth : nextWidth);
        const double fill = room > 0
            ? double(wrapped.last().size()) / room : 1;
        if (fill > bestFill) {
            best = wrapped;
            bestFill = fill;
        }
        if (fill >= 0.5) {
            break;
        }
    }

    // Writes the lines, unless they are the ones there already.
    const QByteArray newline = document->text().mid(
        document->lineEnd(firstLine) - 1,
        2
    )
        .startsWith('\r') ? QByteArray("\r\n") : QByteArray("\n");
    QByteArray replacement;
    for (int k = 0; k < best.size(); ++k) {
        if (k > 0) {
            replacement += newline + nextPrefix;
        }
        replacement += best.at(k).toUtf8();
    }
    const int from = lines.at(first).offset;
    const int to = lines.at(first + count - 1).offset
        + lines.at(first + count - 1).text.size();
    if (document->text().mid(from, to - from) == replacement) {
        return false;
    }
    return document->replace(from, to - from, replacement);
}
