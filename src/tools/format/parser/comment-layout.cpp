// Copyright (C) 2015 The XD Company Ltd.
// SPDX-License-Identifier: Apache-2.0

#include "./comment-layout.h"

#include "../format-document.h"
#include "../lexer/format-lexer.h"

#include <QtCore/QMap>
#include <QtCore/QVector>

namespace {

QList<FormatComment > unprotectedCommentsInRange(
        const FormatDocument *document, int offset, int length)
{
    QList<FormatComment > comments;
    const int end = offset + length;
    for (int i = document->commentAfter(offset); i < document->comments().size() && document->comments().at(i).offset < end; ++i) {
        const FormatComment & comment = document->comments().at(i);
        if (comment.offset + comment.length <= end
            && ! document->isProtectedComment(i)
        ) {
            comments.append(comment);
        }
    }
    return comments;
}

bool isHorizontalGap(const QByteArray & text, int from, int to)
{
    for (int i = from; i < to; ++i) {
        const char c = text.at(i);
        if (c != ' ' && c != '\t') {
            return false;
        }
    }
    return true;
}

bool isTieBreakerGap(const QByteArray & text, int from, int to)
{
    for (int i = from; i < to; ++i) {
        const char c = text.at(i);
        if (c != ' '
            && c != '\t'
            && c != '\r'
            && c != '\n'
            && c != '('
            && c != '['
            && c != '{'
        ) {
            return false;
        }
    }
    return true;
}

int lineIndentLength(const QByteArray & text, int offset)
{
    int start = text.lastIndexOf('\n', offset > 0 ? offset - 1 : 0);
    start = start < 0 ? 0 : start + 1;
    int width = 0;
    while (start + width < text.size()
        && (text.at(start + width) == ' '
            || text.at(start + width) == '\t')
    ) {
        ++width;
    }
    return width;
}

QByteArray indentAtOffset(const QByteArray & text, int offset)
{
    int start = text.lastIndexOf('\n', offset > 0 ? offset - 1 : 0);
    start = start < 0 ? 0 : start + 1;
    return text.mid(start, lineIndentLength(text, offset));
}

int lineNumberAtOffset(const QByteArray & text, int offset)
{
    int line = 1;
    const int limit = qMin(offset, text.size());
    for (int i = 0; i < limit; ++i) {
        if (text.at(i) == '\n') {
            ++line;
        }
    }
    return line;
}

Symbols tokenizeCode(const QByteArray & text,
                     QList<FormatComment > *comments = Q_NULLPTR)
{
    const Symbols all = qMove(FormatLexer::tokenize(
        text,
        FormatLexer::Options(true)
    ));
    Symbols code;
    code.reserve(all.size());
    for (int i = 0; i < all.size(); ++i) {
        const Symbol & symbol = all.at(i);
        if (symbol.len <= 0
            || symbol.token == WHITESPACE
            || symbol.token == PP_WHITESPACE
            || symbol.token == NOTOKEN
            || symbol.token == PP_NEWLINE
            || symbol.token == NEWLINE
        ) {
            continue;
        }
        if (symbol.token == CPP_COMMENT || symbol.token == C_COMMENT) {
            if (comments) {
                FormatComment entry;
                entry.offset = symbol.from;
                entry.length = symbol.len;
                comments->append(entry);
            }
            continue;
        }
        code.append(symbol);
    }
    return code;
}

} // namespace

/*!
    Attaches each unprotected comment in [@a offset, @a offset + @a length)
    to its anchor token among the @a tokenCount tokens starting at
    @a firstToken of @a document as leading or trailing, matching line and
    inline tie-break placement.
*/
QList<AttachedComment > attachComments(
        const FormatDocument *document,
        int offset,
        int length,
        int firstToken,
        int tokenCount)
{
    const QList<FormatComment > comments =
        unprotectedCommentsInRange(document, offset, length);
    QList<AttachedComment > attached;
    if (comments.isEmpty()) {
        return attached;
    }

    const QByteArray & source = document->text();
    const int end = offset + length;
    const int symbolCount = document->symbols().size();
    QVector<int > precedingTokens(comments.size(), -1);
    QVector<int > followingTokens(comments.size(), symbolCount);
    QVector<AttachedComment::Placement > placements(
        comments.size(), AttachedComment::Inline
    );

    for (int i = 0; i < comments.size(); ++i) {
        const FormatComment & entry = comments.at(i);
        precedingTokens[i] = document->symbolAfter(entry.offset) - 1;
        followingTokens[i] = document->symbolAfter(
            entry.offset + entry.length
        );
    }

    for (int i = 0; i < comments.size(); ++i) {
        const int preceding = precedingTokens.at(i);
        const int following = followingTokens.at(i);

        // Groups earlier comments on the same line when checking
        // own-line placement.
        int start = comments.at(i).offset;
        for (int k = i - 1; k >= 0; --k) {
            if (precedingTokens.at(k) != preceding
                || ! isHorizontalGap(
                    source,
                    comments.at(k).offset + comments.at(k).length,
                    start)
            ) {
                break;
            }
            start = comments.at(k).offset;
        }
        // Groups later comments on the same line when checking
        // end-of-line placement.
        int tail = comments.at(i).offset + comments.at(i).length;
        for (int k = i + 1; k < comments.size(); ++k) {
            if (followingTokens.at(k) != following
                || ! isHorizontalGap(source, tail, comments.at(k).offset)
            ) {
                break;
            }
            tail = comments.at(k).offset + comments.at(k).length;
        }
        const bool hasLeadingBreak = preceding < 0
            || document->symbolEndLine(preceding) < document->lineOf(start);
        const bool endOfLine = following >= symbolCount
            || document->symbolLine(following) > document->lineOf(tail - 1);
        if (hasLeadingBreak && endOfLine) {
            placements[i] = AttachedComment::OwnLine;
        } else if (endOfLine) {
            placements[i] = AttachedComment::EndOfLine;
        } else {
            placements[i] = AttachedComment::Inline;
        }
    }

    for (int i = 0; i < comments.size(); ++i) {
        AttachedComment entry;
        entry.comment = comments.at(i);
        entry.placement = placements.at(i);
        const int precedingGlobal = precedingTokens.at(i);
        const int followingGlobal = followingTokens.at(i);
        const int precedingInSpan = precedingGlobal >= firstToken
            ? precedingGlobal - firstToken : -1;
        const int followingInSpan =
            followingGlobal < firstToken + tokenCount
            ? followingGlobal - firstToken : tokenCount;

        if (entry.placement == AttachedComment::OwnLine) {
            if (followingInSpan < tokenCount) {
                entry.role = AttachedComment::Leading;
                entry.anchorToken = followingInSpan;
            } else if (precedingInSpan >= 0) {
                entry.role = AttachedComment::Trailing;
                entry.anchorToken = precedingInSpan;
            } else {
                entry.role = AttachedComment::Leading;
                entry.anchorToken = 0;
            }
            attached.append(entry);
            continue;
        }
        if (entry.placement == AttachedComment::EndOfLine) {
            if (precedingInSpan >= 0) {
                entry.role = AttachedComment::Trailing;
                entry.anchorToken = precedingInSpan;
            } else if (followingInSpan < tokenCount) {
                entry.role = AttachedComment::Leading;
                entry.anchorToken = followingInSpan;
            } else {
                entry.role = AttachedComment::Trailing;
                entry.anchorToken = 0;
            }
            attached.append(entry);
            continue;
        }

        // Resolves inline comments between tokens on one line in
        // contiguous groups.
        int groupEnd = i + 1;
        while (groupEnd < comments.size()
            && placements.at(groupEnd) == AttachedComment::Inline
            && precedingTokens.at(groupEnd) == precedingGlobal
            && followingTokens.at(groupEnd) == followingGlobal
        ) {
            ++groupEnd;
        }
        int firstLeading = groupEnd;
        int gapEnd = followingGlobal < symbolCount
            ? document->symbols().at(followingGlobal).from : end;
        const bool afterStatement =
            precedingGlobal >= 0 && (document->token(precedingGlobal) == SEMIC
                || document->token(precedingGlobal) == RBRACE);
        const bool beforeBrace = followingGlobal < symbolCount
            && document->token(followingGlobal) == LBRACE;
        const bool lineHead = precedingGlobal < 0
            || document->symbolEndLine(precedingGlobal) < document->lineOf(
                comments.at(i).offset
            );
        if (lineHead) {
            firstLeading = i;
        } else if ( ! afterStatement && ! beforeBrace) {
            for (int k = groupEnd; k > i; --k) {
                const FormatComment & item = comments.at(k - 1);
                if (isTieBreakerGap(
                    source,
                    item.offset + item.length,
                    gapEnd
                )) {
                    gapEnd = item.offset;
                    firstLeading = k - 1;
                } else {
                    break;
                }
            }
        }
        for (int k = i; k < groupEnd; ++k) {
            AttachedComment item;
            item.comment = comments.at(k);
            item.placement = AttachedComment::Inline;
            if (k < firstLeading) {
                item.role = AttachedComment::Trailing;
                item.anchorToken = precedingInSpan >= 0 ? precedingInSpan : 0;
            } else {
                item.role = AttachedComment::Leading;
                item.anchorToken = followingInSpan < tokenCount
                    ? followingInSpan : tokenCount;
            }
            attached.append(item);
        }
        i = groupEnd - 1;
    }
    return attached;
}

/*!
    Re-emits the unprotected comments inside [@a offset, @a offset + @a length)
    of @a document at their matching token boundaries inside @a requested,
    without copying or re-tokenizing the document.
*/
QByteArray restoreCommentsInReplacement(
        const FormatDocument *document,
        int offset,
        int length,
        const QByteArray & requested)
{
    const QList<FormatComment > comments =
        unprotectedCommentsInRange(document, offset, length);
    if (comments.isEmpty()) {
        return requested;
    }

    const QByteArray & original = document->text();
    QByteArray text = requested;
    QList<FormatComment > embeddedComments;
    Symbols after = tokenizeCode(text, &embeddedComments);
    if ( ! embeddedComments.isEmpty()) {
        QList<FormatComment > unmatched = comments;
        bool stripped = false;
        for (int i = embeddedComments.size() - 1; i >= 0; --i) {
            const FormatComment & candidate = embeddedComments.at(i);
            for (int orig = unmatched.size() - 1; orig >= 0; --orig) {
                const FormatComment & comment = unmatched.at(orig);
                if (requested.mid(candidate.offset, candidate.length)
                        == original.mid(comment.offset, comment.length)
                ) {
                    text.remove(candidate.offset, candidate.length);
                    unmatched.removeAt(orig);
                    stripped = true;
                    break;
                }
            }
        }
        if (stripped) {
            after = tokenizeCode(text);
        }
    }

    const int rangeEnd = offset + length;
    const int firstBefore = document->symbolAfter(offset);
    int lastBefore = document->symbolAfter(rangeEnd);
    while (lastBefore > firstBefore
        && document->symbolEnd(lastBefore - 1) > rangeEnd
    ) {
        --lastBefore;
    }
    const int beforeCount = lastBefore - firstBefore;
    const int afterCount = after.size();

    const QList<AttachedComment > attached =
        attachComments(document, offset, length, firstBefore, beforeCount);

    QVector<int > destinations(beforeCount, -1);
    if (beforeCount > 0 && afterCount > 0) {
        QVector<QByteArray > beforeTokens;
        QVector<QByteArray > afterTokens;
        beforeTokens.reserve(beforeCount);
        afterTokens.reserve(afterCount);
        for (int i = 0; i < beforeCount; ++i) {
            beforeTokens.append(document->lexem(firstBefore + i));
        }
        for (int i = 0; i < afterCount; ++i) {
            const Symbol & symbol = after.at(i);
            afterTokens.append(text.mid(symbol.from, symbol.len));
        }

        int prefix = 0;
        while (prefix < beforeCount
            && prefix < afterCount
            && beforeTokens.at(prefix) == afterTokens.at(prefix)
        ) {
            destinations[prefix] = prefix;
            ++prefix;
        }
        int beforeEnd = beforeCount;
        int afterEnd = afterCount;
        while (beforeEnd > prefix
            && afterEnd > prefix
            && beforeTokens.at(beforeEnd - 1) == afterTokens.at(
                afterEnd - 1)
        ) {
            destinations[--beforeEnd] = --afterEnd;
        }
        const int midBefore = beforeEnd - prefix;
        const int midAfter = afterEnd - prefix;
        if (midBefore > 0 && midAfter > 0) {
            QVector<QVector<int > > shared(
                midBefore + 1, QVector<int >(midAfter + 1, 0)
            );
            for (int b = midBefore - 1; b >= 0; --b) {
                for (int a = midAfter - 1; a >= 0; --a) {
                    shared[b][a] =
                        beforeTokens.at(prefix + b)
                        == afterTokens.at(prefix + a)
                        ? shared.at(b + 1).at(a + 1) + 1
                        : qMax(
                        shared.at(b + 1).at(a),
                        shared.at(b).at(a + 1)
                        );
                }
            }
            int b = 0;
            int a = 0;
            while (b < midBefore && a < midAfter) {
                if (beforeTokens.at(prefix + b) == afterTokens.at(
                    prefix + a
                )) {
                    destinations[prefix + b++] = prefix + a++;
                } else if (shared.at(b + 1).at(a) >= shared.at(b).at(a + 1)) {
                    ++b;
                } else {
                    ++a;
                }
            }
        }
    }

    QMap<int, QList<AttachedComment > > gaps;
    for (int i = 0; i < attached.size(); ++i) {
        const AttachedComment & item = attached.at(i);
        int gap = afterCount;
        if (beforeCount == 0) {
            gap = (item.role == AttachedComment::Trailing) ? 0 : afterCount;
        } else if (item.role == AttachedComment::Trailing
            && item.anchorToken >= 0
            && destinations.at(item.anchorToken) >= 0
        ) {
            gap = destinations.at(item.anchorToken) + 1;
        } else {
            const int startToken = qBound(0, item.anchorToken, beforeCount);
            for (int token = startToken; token < beforeCount; ++token) {
                if (destinations.at(token) >= 0) {
                    gap = destinations.at(token);
                    break;
                }
            }
        }
        gaps[gap].append(item);
    }

    QByteArray restored = text;
    QMap<int, QList<AttachedComment > >::const_iterator gap = gaps.constEnd();
    while (gap != gaps.constBegin()) {
        --gap;
        const int beforeEnd = gap.key() > 0
            ? after.at(gap.key() - 1).from + after.at(gap.key() - 1).len : 0;
        const int afterStart = gap.key() < afterCount ? after.at(gap.key()).from
                                                      : text.size();
        const QByteArray whitespace = text.mid(
            beforeEnd,
            afterStart - beforeEnd
        );
        const int newline = whitespace.lastIndexOf('\n');
        QByteArray padding = newline >= 0 ? whitespace.mid(newline + 1)
                                          : indentAtOffset(
                                              text,
                                              afterStart
                                          );
        if (lineNumberAtOffset(text, afterStart) == 1 && newline < 0) {
            padding = document->indentOf(document->lineOf(offset));
        }
        const int outside = document->symbolAfter(rangeEnd);
        const bool tokenAtEnd = gap.key() == afterCount
            && outside < document->symbols().size()
            && document->symbols().at(outside).from == rangeEnd;
        if (tokenAtEnd && newline < 0) {
            padding = document->indentOf(document->symbolLine(outside));
        }

        QByteArray replacement;
        const QList<AttachedComment > & entries = gap.value();
        for (int index = 0; index < entries.size(); ++index) {
            const AttachedComment & attachedEntry = entries.at(index);
            const FormatComment & comment = attachedEntry.comment;
            QByteArray body = original.mid(comment.offset, comment.length);
            const int commentEnd = comment.offset + comment.length;
            const bool followingContent = gap.key() < afterCount
                || index + 1 < entries.size();
            const QByteArray trailing = original.mid(
                commentEnd,
                rangeEnd - commentEnd
            );
            const bool atRangeEnd = ! followingContent
                && ! trailing.contains('\n')
                && trailing.trimmed().isEmpty();
            const bool newlineOutside = atRangeEnd
                && rangeEnd < original.size()
                && (original.at(rangeEnd) == '\n'
                    || (rangeEnd + 1 < original.size()
                        && original.at(rangeEnd) == '\r'
                        && original.at(rangeEnd + 1) == '\n'));
            const bool eofOutside = atRangeEnd && rangeEnd == original.size();
            if (comment.isLine()
                && body.endsWith('\r')
                && ! newlineOutside
                && commentEnd < original.size()
                && original.at(commentEnd) == '\n'
            ) {
                body.chop(1);
            }
            const int lineEnd = document->lineEnd(
                document->lineOf(comment.offset + comment.length - 1)
            );
            const int newlineOffset = lineEnd < original.size()
                ? lineEnd : original.lastIndexOf('\n', comment.offset - 1);
            const bool crlf = newlineOffset > 0
                && original.at(newlineOffset - 1) == '\r';
            const QByteArray lineBreak = crlf
                && ! (comment.isLine()
                    && body.endsWith('\r'))
                ? "\r\n" : "\n";

            const bool ownLine = attachedEntry.placement
                == AttachedComment::OwnLine;
            const bool leadsInline =
                attachedEntry.placement == AttachedComment::Inline
                && attachedEntry.role == AttachedComment::Leading;
            if (leadsInline && replacement.isEmpty()) {
                const bool nextToOpen = comment.offset > 0
                    && original.at(comment.offset - 1) == '('
                    && gap.key() > 0
                    && after.at(gap.key() - 1).token == LPAREN;
                replacement = newline >= 0
                    ? whitespace : nextToOpen ? QByteArray() : QByteArray(" ");
            }
            if (ownLine) {
                if (replacement.isEmpty()) {
                    if (newline >= 0) {
                        replacement = whitespace.left(newline + 1) + padding;
                    } else if (beforeEnd == 0
                        && gap.key() == 0
                        && document->lineOf(offset)
                            == document->lineOf(comment.offset)
                    ) {
                        const int existing = offset
                            - document->lineStart(document->lineOf(offset));
                        replacement = padding.mid(
                            qMin(existing, padding.size())
                        );
                    } else {
                        replacement = lineBreak + padding;
                    }
                } else if ( ! replacement.endsWith(padding)
                    || ! replacement.contains('\n')
                ) {
                    replacement += lineBreak + padding;
                }
            } else if ( ! leadsInline
                && (replacement.isEmpty()
                    || ! replacement.endsWith(' '))
            ) {
                replacement += ' ';
            }
            replacement += body;
            if (newlineOutside
                && trailing.endsWith('\r')
                && ! body.endsWith('\r')
            ) {
                replacement += '\r';
            }
            const bool needsNewline = followingContent
                || newline >= 0
                || (lineEnd < original.size() && original.at(lineEnd) == '\n');
            if ((comment.isLine()
                    || ownLine)
                && needsNewline
                && ! newlineOutside
            ) {
                replacement += lineBreak;
                if (followingContent || tokenAtEnd) {
                    replacement += padding;
                }
            } else if ( ! comment.isLine()
                && ! ownLine
                && ! newlineOutside
                && ! eofOutside
            ) {
                if (attachedEntry.role == AttachedComment::Trailing
                    && newline >= 0
                    && followingContent
                ) {
                    replacement += whitespace;
                } else {
                    replacement += ' ';
                }
            }
        }
        if (newline >= 0 && ! replacement.contains('\n')) {
            replacement = replacement.trimmed();
            replacement.prepend(' ');
            replacement += whitespace;
        } else if (newline < 0
            && gap.key() < afterCount
            && ! replacement.contains('\n')
            && (after.at(gap.key()).token == COMMA
                || after.at(gap.key()).token == RPAREN
                || after.at(gap.key()).token == SEMIC)
        ) {
            while (replacement.endsWith(' ')) {
                replacement.chop(1);
            }
        }
        restored.replace(beforeEnd, afterStart - beforeEnd, replacement);
    }
    return restored;
}
