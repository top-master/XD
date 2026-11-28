// Copyright (C) 2015 The XD Company Ltd.
// SPDX-License-Identifier: Apache-2.0

#include "./ast-transforms.h"
#include "../format-document.h"
#include "../format-rules.h"
#include "../parser/class-helpers.h"
#include "../parser/class-relations.h"
#include "../parser/code-helpers.h"
#include "../parser/comment-helpers.h"
#include "../parser/comment-layout.h"
#include "../parser/condition-context.h"
#include "../parser/condition-layout.h"
#include "../parser/expression-layout.h"
#include "../parser/layout-helpers.h"
#include "../parser/scope-conditions.h"
#include "../parser/type-mapping.h"

#include "config-condition.h"

#include <QtCore/QDir>
#include <QtCore/QFileInfo>
#include <QtCore/QMap>
#include <QtCore/QRegularExpression>
#include <QtCore/QtAlgorithms>

namespace {

bool isInDisabledRange(const QVector<QPair<int, int > > & ranges, int offset)
{
    for (int i = 0; i < ranges.size(); ++i) {
        if (offset >= ranges.at(i).first && offset < ranges.at(i).second) {
            return true;
        }
    }
    return false;
}

int lineOfOffset(const QVector<int > & lineStarts, int offset)
{
    int low = 0;
    int high = lineStarts.size();
    while (low < high) {
        const int mid = low + (high - low) / 2;
        if (lineStarts.at(mid) <= offset) {
            low = mid + 1;
        } else {
            high = mid;
        }
    }
    return qMax(1, low);
}

bool isSpanSelected(
        const QVector<QPair<int, int > > & lineRanges,
        const QVector<int > & lineStarts,
        int startOffset,
        int endOffset)
{
    if (lineRanges.isEmpty()) {
        return true;
    }
    const int firstLine = lineOfOffset(lineStarts, startOffset);
    const int lastLine = lineOfOffset(
        lineStarts,
        qMax(startOffset, endOffset - 1)
    );
    for (int i = 0; i < lineRanges.size(); ++i) {
        if (lastLine >= lineRanges.at(i).first
            && firstLine <= lineRanges.at(i).second
        ) {
            return true;
        }
    }
    return false;
}

void transformNodeBasic(
        AstNode *node,
        QVector<AstComment * > *comments,
        const QByteArray & source,
        const Symbols & symbols,
        const QVector<QPair<int, int > > & disabledRanges,
        const QVector<QPair<int, int > > & lineRanges,
        const QVector<int > & lineStarts,
        const AstTransforms::Options & options)
{
    if ( ! node) {
        return;
    }

    if (isInDisabledRange(disabledRanges, node->startOffset)
        || ! isSpanSelected(
            lineRanges,
            lineStarts,
            node->startOffset,
            node->endOffset)
    ) {
        node->preserveVerbatim = true;
    }

    if ( ! node->preserveVerbatim
        && options.braceControls
        && (node->kind == AstNode::IfStmt
            || node->kind == AstNode::ForStmt
            || node->kind == AstNode::WhileStmt)
        && node->body
        && ! node->body->isBlock
    ) {
        node->needsBraces = true;
    }

    if ( ! node->preserveVerbatim
        && (options.formatNamespaces
            || options.labelNamespaces)
        && node->kind == AstNode::NamespaceDecl
        && ! node->name.isEmpty()
    ) {
        bool hasTrailingLabel = false;
        for (int i = 0; i < node->comments.size(); ++i) {
            AstComment *c = node->comments.at(i);
            if (c->placement == AstComment::Trailing
                && c->offset >= node->endOffset - 1
            ) {
                hasTrailingLabel = true;
                break;
            }
        }
        if ( ! hasTrailingLabel) {
            node->transformedText = "// Namespace `" + node->name + "`.";
        }
    }

    for (int i = 0; i < node->children.size(); ++i) {
        transformNodeBasic(
            node->children.at(i),
            comments,
            source,
            symbols,
            disabledRanges,
            lineRanges,
            lineStarts,
            options
        );
    }
}

bool spanLessThan(const AstTransformState::SpanRewrite & a,
                  const AstTransformState::SpanRewrite & b)
{
    if (a.from != b.from) {
        return a.from < b.from;
    }
    return a.to < b.to;
}

int declarationName(int paren, const FormatDocument *document)
{
    int name = paren - 1;
    bool isOperator = false;
    for (int k = paren - 1; k >= qMax(1, paren - 16); --k) {
        const Token token = document->token(k);
        if (token == OPERATOR) {
            name = k;
            isOperator = true;
            break;
        }
        if (token == SEMIC
            || token == LBRACE
            || token == RBRACE
            || token == COLON
            || ((token == LPAREN
                || token == RPAREN)
                && k < paren - 2
            )
        ) {
            break;
        }
    }
    if ( ! isOperator && document->token(name) != IDENTIFIER) {
        return -1;
    }
    if ( ! isOperator && document->token(name - 1) == TILDE) {
        return name - 1;
    }
    while (name >= 2
        && document->token(name - 1) == SCOPE
        && document->token(name - 2) == IDENTIFIER
    ) {
        name -= 2;
    }
    return name;
}

QList<QByteArray > commentedSignatureLines(
        const FormatDocument *document,
        int paren,
        int close,
        const QList<QByteArray > & lines,
        const QByteArray & prefix,
        const QByteArray & suffix,
        const QByteArray & indent,
        int limit)
{
    QList<QByteArray > result = lines;
    if (result.size() < 2) {
        return result;
    }

    const int name = declarationName(paren, document);
    if (name > 0
        && document->symbolLine(name) == document->symbolLine(paren)
    ) {
        const int column = document
            ->codeSlice(
                document->lineStart(document->symbolLine(paren)),
                document->symbols().at(name).from
            )
            .size();
        if (column > 0
            && column < result.first().size()
            && result.first().startsWith(prefix)
        ) {
            const QByteArray head = result.first();
            result[0] = head.left(column).trimmed();
            result.insert(1, indent + head.mid(column));
        }
    }

    if (suffix.contains('\n') || ! result.last().endsWith(suffix)) {
        return result;
    }
    QByteArray last = result.last();
    last.chop(suffix.size());
    QByteArray rest = suffix.mid(1);
    while (rest.startsWith(' ') || rest.startsWith('\t')) {
        rest.remove(0, 1);
    }
    while (rest.endsWith(' ') || rest.endsWith('\t')) {
        rest.chop(1);
    }
    QByteArray brace;
    if (rest.endsWith('{')) {
        brace = "{";
        rest.chop(1);
        while (rest.endsWith(' ') || rest.endsWith('\t')) {
            rest.chop(1);
        }
    }
    bool trailsLast = false;
    for (int index = 0; index < document->comments().size() && ! trailsLast; ++index) {
        const FormatComment & comment = document->comments().at(index);
        trailsLast = comment.isLine() && comment.offset >= document->symbolEnd(
            close - 1)
            && comment.offset < document->symbols().at(close).from
            && document->commentLine(index)
                == document->symbolEndLine(close - 1);
    }
    const QByteArray pad = continuationPad(indent.size());
    result.removeLast();
    QByteArray parameterLine = last;
    if ( ! trailsLast) {
        parameterLine += ')';
        const bool joins = rest.startsWith(';') || rest.startsWith(',');
        if ( ! rest.isEmpty()
            && brace.isEmpty()
            && textWidth(parameterLine) + (joins ? 0 : 1) + textWidth(rest)
                <= limit
        ) {
            parameterLine += joins ? rest : ' ' + rest;
            rest.clear();
        }
        result << parameterLine;
    } else {
        result << parameterLine;
        QByteArray closeLine = pad + ')';
        const bool joins = rest.startsWith(';') || rest.startsWith(',');
        if ( ! rest.isEmpty()
            && textWidth(closeLine) + (joins ? 0 : 1) + textWidth(rest) <= limit
        ) {
            closeLine += joins ? rest : ' ' + rest;
            rest.clear();
        }
        result << closeLine;
    }
    if ( ! rest.isEmpty()) {
        result << pad + rest;
    }
    if ( ! brace.isEmpty()) {
        result << indent + brace;
    }
    return result;
}

typedef QVector<AstTransformState::SpanRewrite > SpanList;

void addAtomicGroup(const FormatDocument *doc,
                    const SpanList & group,
                    SpanList *out)
{
    if (group.isEmpty()) {
        return;
    }
    bool anyDiff = false;
    const QByteArray & text = doc->text();
    for (int i = 0; i < group.size(); ++i) {
        const AstTransformState::SpanRewrite & s = group.at(i);
        if (s.from < 0 || s.to < s.from || s.to > text.size()) {
            return;
        }
        if ( ! doc->isSpanEditable(s.from, s.to - s.from)) {
            return;
        }
        if (text.mid(s.from, s.to - s.from) != s.bytes) {
            anyDiff = true;
        }
    }
    if (anyDiff) {
        *out += group;
    }
}

void addSingleSpan(
        const FormatDocument *doc,
        int from,
        int to,
        const QByteArray & bytes,
        SpanList *out,
        bool restoreComments = false)
{
    if (from < 0 || to < from || to > doc->text().size()) {
        return;
    }
    if ( ! doc->isSpanEditable(from, to - from)) {
        return;
    }
    const QByteArray finalBytes = restoreComments
        ? restoreCommentsInReplacement(doc, from, to - from, bytes) : bytes;
    if (doc->text().mid(from, to - from) != finalBytes) {
        out->append(AstTransformState::SpanRewrite(from, to, finalBytes));
    }
}

void shiftRangesAndIssues(
        const FormatDocument *doc,
        const SpanList & nonOverlapping,
        QVector<QPair<int, int > > *ranges,
        QList<FormatIssue > *issues,
        QList<FormatIssue > *requests)
{
    const QByteArray & text = doc->text();
    for (int i = nonOverlapping.size() - 1; i >= 0; --i) {
        const AstTransformState::SpanRewrite & span = nonOverlapping.at(i);
        const int line = doc->lineOf(span.from);
        const int oldLineStart = doc->lineStart(line);
        const int removed = text.mid(span.from, span.to - span.from)
            .count('\n');
        const int added = span.bytes.count('\n');
        const int deltaLines = added - removed;
        if (deltaLines != 0) {
            const int shiftLine = (span.from == oldLineStart) ? line - 1 : line;
            for (int r = 0; r < ranges->size(); ++r) {
                QPair<int, int > *range = &((*ranges)[r]);
                if (shiftLine < range->first) {
                    range->first += deltaLines;
                    range->second += deltaLines;
                } else if (shiftLine <= range->second) {
                    range->second += deltaLines;
                }
            }
            FormatDocument::shiftIssues(issues, shiftLine, deltaLines);
            FormatDocument::shiftIssues(requests, shiftLine, deltaLines);
        }
    }
}

SpanList filterNonOverlapping(const FormatDocument *doc, const SpanList & input)
{
    SpanList sorted = input;
    qSort(sorted.begin(), sorted.end(), spanLessThan);
    SpanList accepted;
    accepted.reserve(sorted.size());
    int lastEnd = 0;
    const QByteArray & text = doc->text();
    for (int i = 0; i < sorted.size(); ++i) {
        const AstTransformState::SpanRewrite & s = sorted.at(i);
        if (s.from < lastEnd || s.to < s.from || s.to > text.size()) {
            continue;
        }
        if (text.mid(s.from, s.to - s.from) == s.bytes) {
            continue;
        }
        accepted.append(s);
        lastEnd = s.to;
    }
    return accepted;
}

class TransformContext
{
public:

    explicit TransformContext(FormatDocument *initialDoc)
        : doc(initialDoc)
        , lineRanges(initialDoc->lineRanges())
    {
    }

    bool commitPhase(const SpanList & rawSpans, bool buildNextDoc = true)
    {
        if (rawSpans.isEmpty()) {
            return false;
        }
        const SpanList spans = filterNonOverlapping(doc, rawSpans);
        if (spans.isEmpty()) {
            return false;
        }

        QList<FormatIssue > issues = doc->issues();
        QList<FormatIssue > requests = doc->requests();
        shiftRangesAndIssues(doc, spans, &lineRanges, &issues, &requests);

        const QByteArray & oldText = doc->text();
        QByteArray nextText;
        nextText.reserve(oldText.size() + 64);
        int cursor = 0;
        for (int i = 0; i < spans.size(); ++i) {
            const AstTransformState::SpanRewrite & s = spans.at(i);
            nextText.append(oldText.constData() + cursor, s.from - cursor);
            nextText.append(s.bytes);
            cursor = s.to;
        }
        nextText.append(
            oldText.constData() + cursor,
            oldText.size() - cursor
        );

        if ( ! buildNextDoc) {
            doc->setIssues(issues);
            doc->setRequests(requests);
            doc->setLineRanges(lineRanges);
            return true;
        }

        QSharedPointer<FormatDocument > nextDoc(
                new FormatDocument(nextText, doc->path()));
        nextDoc->setFormatConfig(doc->formatConfig());
        nextDoc->setAgentMode(doc->agentMode());
        nextDoc->setIncludePaths(doc->includePaths());
        nextDoc->setConfig(doc->config());
        if (nextText.contains("Format rule") || nextText.contains('@')) {
            QString ignoredError;
            nextDoc->setInlineConfiguration(
                doc->formatConfig(),
                &ignoredError
            );
        }
        nextDoc->setLineRanges(lineRanges);
        nextDoc->setIssues(issues);
        nextDoc->setRequests(requests);

        ownedDoc = nextDoc;
        doc = ownedDoc.data();
        return true;
    }

    FormatDocument *doc;
    QSharedPointer<FormatDocument > ownedDoc;
    QVector<QPair<int, int > > lineRanges;
};

void collectLineJoin(const FormatDocument *source, SpanList *out)
{
    int joinedUntil = 0;
    for (int index = 0; index < source->symbols().size(); ++index) {
        if (index < joinedUntil) {
            continue;
        }
        QList<int > brackets;
        int questions = 0;
        int limit = index;
        for (; limit + 1 < source->symbols().size(); ++limit) {
            const Token token = source->token(limit);
            if (token == LPAREN
                || token == LBRACK
                || (token == LBRACE
                    && ( ! brackets.isEmpty()
                        || source->braceScope(limit)
                            == FormatDocument::InitScope
                    )
                )
            ) {
                brackets.append(limit);
            } else if ((token == RPAREN
                    || token == RBRACK
                    || token == RBRACE)
                && ! brackets.isEmpty()
            ) {
                brackets.removeLast();
            } else if (brackets.isEmpty()
                && (token == SEMIC
                    || token == LBRACE
                    || token == RBRACE)
            ) {
                break;
            }
        }
        joinedUntil = qMax(index + 1, limit + 1);
        brackets.clear();
        QList<bool > bracketUnjoined;
        FormatScopeConditions conditions(source);
        int templateEnd = -1;
        for (int cursor = index; cursor < limit && cursor + 1 < source->symbols().size(); ++cursor)
        {
            const Token token = source->token(cursor);
            if (token == LANGLE
                && templateEnd < cursor
                && ! brackets.isEmpty()
                && isDeclarationParen(source, brackets.last())
            ) {
                templateEnd = templateClose(source, cursor, limit + 1);
            }
            bool closedUnjoined = false;
            if (token == LPAREN || token == LBRACK || token == LBRACE) {
                brackets.append(cursor);
                bracketUnjoined.append(false);
            } else if ((token == RPAREN
                    || token == RBRACK
                    || token == RBRACE)
                && ! brackets.isEmpty()
            ) {
                brackets.removeLast();
                closedUnjoined = bracketUnjoined.takeLast();
                if (closedUnjoined && ! bracketUnjoined.isEmpty()) {
                    bracketUnjoined[bracketUnjoined.size() - 1] = true;
                }
            }
            if (token == SEMIC || token == LBRACE || token == RBRACE) {
                questions = 0;
            } else if (token == QUESTION) {
                ++questions;
            } else if (token == COLON && questions > 0) {
                --questions;
            }
            const int next = cursor + 1;
            if (source->symbolEndLine(cursor) >= source->symbolLine(next)) {
                continue;
            }
            bool joined = false;
            do {
                if ( ! codeTargetAnyAt(
                        source,
                        source->symbols().at(cursor).from)
                    || cursor < templateEnd
                    || source->isDirectiveLine(source->symbolEndLine(cursor))
                    || source->isDirectiveLine(source->symbolLine(next))
                    || source->hasCommentBetween(
                        source->symbolEnd(cursor),
                        source->symbols().at(next).from)
                    || (token == STRING_LITERAL
                        && source->token(next) == STRING_LITERAL)
                ) {
                    break;
                }
                const Token after = source->token(next);
                const int nextLine = source->symbolLine(next);
                const int lineEnd = source->lastSymbolOfLine(nextLine);
                if ((after == DOT
                        || after == ARROW)
                    && textWidth(source->lineText(source->symbolEndLine(cursor)))
                        > wrapMaxAt(
                            source,
                            source->symbols().at(cursor).from)
                ) {
                    break;
                }
                const bool singleLineOrphanClose = token == RPAREN
                    && cursor > 0
                    && source->match(cursor) >= 0
                    && isFunctionCallParen(source, source->match(cursor))
                    && source->symbolLine(source->match(cursor))
                        == source->symbolEndLine(cursor - 1)
                    && source->firstSymbolOfLine(source->symbolEndLine(cursor))
                        == cursor
                    && source->lastSymbolOfLine(source->symbolEndLine(cursor))
                        == cursor;
                const bool multilineParenClose = token == RPAREN
                    && closedUnjoined;
                if (((after == ANDAND
                            || after == OROR)
                        && (multilineParenClose
                            || ( ! singleLineOrphanClose
                                && isClosingLine(source->codeLineText(source->symbolEndLine(cursor))))
                        )
                    )
                    || (isClosingLine(source->codeLineText(nextLine))
                        && ! (after == RPAREN
                            && lineEnd == next
                            && source->match(next) >= 1
                            && isFunctionCallParen(
                                source,
                                source->match(next)))
                        && lineEnd >= next
                        && (source->token(lineEnd + 1) == ANDAND
                            || source->token(lineEnd + 1) == OROR)
                        && source->symbolLine(lineEnd + 1) > nextLine
                    )
                ) {
                    break;
                }
                bool insideExpression = false;
                if ( ! brackets.isEmpty()) {
                    const int open = brackets.last();
                    insideExpression = source->token(open) != LBRACE
                        || source->braceScope(open)
                            == FormatDocument::InitScope;
                }
                if ( ! insideExpression
                    && ! isLeadingOperator(after)
                    && after != DOT
                    && after != ARROW
                    && token != EQ
                    && ! (after == COLON
                        && questions > 0)
                ) {
                    break;
                }
                const FormatFileConfig & fileConfig = source->configAt(
                    source->symbols().at(cursor).from
                );
                if ( ! brackets.isEmpty()) {
                    const int open = brackets.last();
                    const int close = source->match(open);
                    const Token beforeOpen = source->token(open - 1);
                    if (source->token(open) == LPAREN && close > open) {
                        const int openFrom = source->symbols().at(open).from;
                        const int openLine = source->symbolLine(open);
                        const int openMax = wrapMaxAt(source, openFrom);
                        const QList<QPair<int, int > > items =
                            splitArguments(source, open, close);
                        const bool isCallLike = beforeOpen == IDENTIFIER
                            || beforeOpen == RPAREN
                            || beforeOpen == RANGLE
                            || beforeOpen == GTGT;
                        if ( ! fileConfig.wrapArgsOverCondition.isEmpty()
                            && isCallLike
                            && ! conditions.argsOver(open, items.size())
                        ) {
                            break;
                        }
                        if ( ! fileConfig.wrapConditionsOverCondition.isEmpty()
                            && (beforeOpen == IF
                                || beforeOpen == WHILE)
                            && ! conditions.conditionsOver(
                                open,
                                conditions.conditionArgumentCount(open, 0))
                        ) {
                            break;
                        }
                        if (beforeOpen != IF
                            && beforeOpen != WHILE
                            && ! isDeclarationParen(source, open)
                        ) {
                            const int prefixWidth = textWidth(source->codeSlice(
                                source->lineStart(openLine),
                                openFrom,
                                true
                            ));
                            FormatExpressionLayout exprLayout(
                                source, &conditions, 0
                            );
                            const int trailingSemi =
                                source->token(close + 1) == SEMIC
                                ? 1 : 0;
                            const bool exceedsMax = prefixWidth
                                + textWidth(
                                    exprLayout.oneLine(open, close))
                                + trailingSemi > openMax;
                            if (exceedsMax
                                && (beforeOpen == FOR
                                    || ( ! isCallLike
                                        && items.size() >= 2
                                        && ! exprLayout.hasChain(
                                            open + 1,
                                            close - 1))
                                )
                            ) {
                                break;
                            }
                        }
                    }
                }
                const bool tight = token == LPAREN
                    || token == LBRACK
                    || after == RPAREN
                    || after == RBRACK
                    || after == COMMA
                    || after == SEMIC
                    || token == DOT
                    || token == ARROW
                    || after == DOT
                    || after == ARROW;
                const int from = source->symbolEnd(cursor);
                addSingleSpan(
                    source,
                    from,
                    source->symbols().at(next).from,
                    tight ? QByteArray() : QByteArray(" "),
                    out
                );
                joined = true;
            } while (false);
            if ( ! joined && ! bracketUnjoined.isEmpty()) {
                bracketUnjoined[bracketUnjoined.size() - 1] = true;
            }
        }
    }
}

void collectRelativeIncludesAndMoc(
        const FormatDocument *source,
        bool adjustIncludes,
        bool fixMocMacro,
        SpanList *out)
{
    const QDir directory = source->path().isEmpty()
        ? QDir() : QFileInfo(source->path()).absoluteDir();
    const QRegularExpression include(
            QString(QLL("^\\s*#\\s*include\\s*\"([^\"]+)\"")));
    for (int index = 0; index < source->symbols().size(); ++index) {
        if (adjustIncludes && ! source->path().isEmpty()) {
            const int line = source->symbolLine(index);
            if (source->firstSymbolOfLine(line) == index
                && source->isDirectiveLine(line)
            ) {
                const QString text = QString::fromUtf8(source->lineText(line));
                const QRegularExpressionMatch found = include.match(text);
                if (found.hasMatch()) {
                    const QString name = found.captured(1);
                    if ( ! name.startsWith(QLL("./"))
                        && ! name.startsWith(QLL("../"))
                        && ! QDir::isAbsolutePath(name)
                        && QFileInfo(directory.filePath(name)).isFile()
                    ) {
                        const int offset = source->lineStart(line)
                            + text.left(found.capturedStart(1)).toUtf8().size();
                        addSingleSpan(source, offset, offset, "./", out);
                    }
                }
            }
        }
        if (fixMocMacro
            && source->token(index) == IDENTIFIER
            && source->lexem(index) == "QT_MOC_RUN"
        ) {
            addSingleSpan(
                source,
                source->symbols().at(index).from,
                source->symbolEnd(index),
                "Q_MOC_RUN",
                out
            );
        }
    }
}

void collectMoveOperators(
        const FormatDocument *source,
        bool moveOperators,
        bool joinMembers,
        SpanList *out)
{
    if ( ! moveOperators && ! joinMembers) {
        return;
    }
    FormatScopeConditions conditions(source);
    for (int index = 1; index + 1 < source->symbols().size(); ++index) {
        const Token kind = source->token(index);
        const bool member = kind == DOT
            || kind == ARROW
            || kind == DOTSTAR
            || kind == ARROW_STAR;
        if ( ! ((member && joinMembers) || ( ! member && moveOperators))) {
            continue;
        }
        SpanList group;
        const bool handled = layoutChainOperand(
            source,
            &conditions,
            index,
            &group)
            || joinsClosingLine(source, index, &group);
        if (handled) {
            addAtomicGroup(source, group, out);
            continue;
        }
        if (kind == ANDAND
            || kind == OROR
            || kind == LTLT
            || kind == PLUS
            || kind == MINUS
            || kind == OR
            || kind == HAT
            || kind == EQEQ
            || kind == NE
            || kind == QUESTION
        ) {
            const int line = source->symbolLine(index);
            const int from = source->symbolEnd(index - 1);
            const int next = source->symbols().at(index + 1).from;
            if (source->lastSymbolOfLine(line) == index
                && ! source->isDirectiveLine(line)
                && source->symbolEndLine(index - 1) == line
                && source->token(index - 1) != OPERATOR
                && ! source->hasProtectedCommentBetween(
                    source->symbolEnd(index),
                    next)
                && ! source->spansDirective(from, next)
            ) {
                group.append(
                    AstTransformState::SpanRewrite(
                        from,
                        source->symbolEnd(index),
                        QByteArray()
                    )
                );
                group.append(
                    AstTransformState::SpanRewrite(
                        next,
                        next,
                        source->lexem(index) + ' '
                    )
                );
                addAtomicGroup(source, group, out);
            }
        }
    }
}

void collectReplaceTypes(FormatDocument *source, SpanList *out)
{
    int mappingSkipUntil = 0;
    QHash<QString, QStringList > programs;
    for (int index = 0; index < source->symbols().size(); ++index) {
        if (index < mappingSkipUntil
            || source->token(index) != IDENTIFIER
            || source->token(index + 1) != LPAREN
        ) {
            continue;
        }
        const int close = source->match(index + 1);
        if (close <= index + 1
            || source->isDirectiveLine(source->symbolLine(index))
        ) {
            continue;
        }
        const FormatFileConfig & config = source->configAt(
            source->symbols().at(index).from
        );
        const QString last = QString::fromLatin1(source->lexem(index));
        bool mayMatch = false;
        for (int m = 0; m < config.replaceTypes.size() && ! mayMatch; ++m) {
            mayMatch = FormatTypeMapping::lastPartMatches(
                config.replaceTypes.at(m).first,
                last
            );
        }
        if ( ! mayMatch) {
            continue;
        }

        int nameStart = index;
        while (nameStart >= 2
            && source->token(nameStart - 1) == SCOPE
            && source->token(nameStart - 2) == IDENTIFIER
        ) {
            nameStart -= 2;
        }
        if (nameStart >= 1 && source->token(nameStart - 1) == SCOPE) {
            --nameStart;
        }
        if ( ! FormatTypeMapping::isCall(source, nameStart)) {
            continue;
        }
        int nameEnd = index;
        const QString written = FormatTypeMapping::readName(
            source,
            nameStart,
            &nameEnd
        );
        const QStringList scopes =
            FormatTypeMapping::visibleScopes(source, nameStart);
        QString capture;
        const QStringList arguments =
            FormatTypeMapping::callArguments(source, index + 1, close);
        QStringList bound;
        const int mapping = FormatTypeMapping::mappingOf(
            config.replaceTypes,
            FormatTypeMapping::candidates(written, scopes),
            arguments,
            &capture,
            &bound
        );
        if (mapping < 0) {
            continue;
        }

        const FormatReplaceTypesContext context(source, index, &config);
        QStringList *program = &(programs[config.replaceTypesTarget]);
        if (program->isEmpty()) {
            Config::Evaluator::MismatchScope scope(
                &context, Config::NameMismatchIgnore
            );
            *program = Config::Condition::compile(
                config.replaceTypesTarget,
                &context
            );
            scope.close();
            if ( ! context.lastCompiled()) {
                continue;
            }
        }
        const QVariant wanted = FormatFileConfig::resolve(
            QString(QLL("replace.types.target")),
            *program,
            &context
        );
        if ( ! context.lastResolvePassed() || ! wanted.toBool()) {
            continue;
        }

        const int from = source->symbols().at(nameStart).from;
        QString head;
        QString call;
        QString error;
        if ( ! FormatTypeMapping::replacement(
            config.replaceTypes,
            mapping,
            capture,
            arguments,
            bound,
            &context,
            &head,
            &call,
            &error
        )) {
            source->report(
                from,
                "replace-types",
                QString(QLL("%1: %2")).arg(written, error)
            );
            continue;
        }
        if (head.isEmpty()) {
            continue;
        }
        const QString shortest = FormatTypeMapping::shortestName(
            head,
            scopes
        );
        const int to = source->symbolEnd(close);
        const QString actionKey = QString(QLL("replace.types.action"));
        const QVariant action = config.hasExpression(actionKey)
            ? config.resolveSetting(actionKey, &context) : QVariant();
        const QString mode = action.isValid() ? action.toString()
                                              : config.replaceTypesAction;
        if (mode == QLL("replace")) {
            const SpanList before = *out;
            addSingleSpan(
                source,
                from,
                to,
                (shortest + call).toLatin1(),
                out,
                true
            );
            if (out->size() > before.size()) {
                mappingSkipUntil = close + 1;
                continue;
            }
        }
        source->report(
            from,
            "replace-types",
            QString(QLL("write %1%2")).arg(shortest, call)
        );
    }
}

void collectTemplateCloseSpace(const FormatDocument *source, SpanList *out)
{
    for (int index = 1; index < source->symbols().size(); ++index) {
        if (source->token(index) != LANGLE
            || source->isDirectiveLine(source->symbolLine(index))
        ) {
            continue;
        }
        const Token previous = source->token(index - 1);
        const bool isCast = previous == STATIC_CAST
            || previous == DYNAMIC_CAST
            || previous == REINTERPRET_CAST
            || previous == CONST_CAST;
        if ( ! isCast
            && ! (previous == IDENTIFIER
                && (index < 2
                    || source->token(index - 2) != OPERATOR)
            )
        ) {
            continue;
        }
        QList<int > closers;
        int depth = 0;
        int cursor = index;
        bool valid = true;
        for (; cursor < source->symbols().size(); ++cursor) {
            const Token kind = source->token(cursor);
            if (kind == LANGLE) {
                ++depth;
            } else if (kind == RANGLE || kind == GTGT) {
                depth -= kind == GTGT ? 2 : 1;
                if (depth < 0) {
                    valid = false;
                    break;
                }
                closers.append(cursor);
                if (depth == 0) {
                    break;
                }
            } else if (kind == LPAREN || kind == LBRACK) {
                const int close = source->match(cursor);
                if (close < 0) {
                    valid = false;
                    break;
                }
                cursor = close;
            } else if (kind != IDENTIFIER
                && kind != SCOPE
                && kind != STAR
                && kind != AND
                && kind != CONST
                && kind != VOLATILE
                && kind != COMMA
                && kind != INTEGER_LITERAL
                && ! isBuiltinType(kind)
            ) {
                valid = false;
                break;
            }
        }
        const Token after = source->token(cursor + 1);
        valid = valid
            && depth == 0
            && (after == IDENTIFIER
                || after == SCOPE
                || after == LPAREN
                || after == RPAREN
                || after == COMMA
                || after == SEMIC
                || after == AND
                || after == STAR
                || after == ANDAND
                || after == LBRACE
                || after == RANGLE
                || after == GTGT
                || after == COLON
                || after == EQ);
        if (valid && ! closers.isEmpty()) {
            SpanList group;
            for (int closer = 0; closer < closers.size(); ++closer) {
                const int token = closers.at(closer);
                const int offset = source->symbols().at(token).from;
                const bool spaced = offset > 0
                    && source->text().at(offset - 1) == ' ';
                if (source->token(token) == GTGT) {
                    group.append(
                        AstTransformState::SpanRewrite(
                            offset,
                            offset + 2,
                            spaced ? "> >" : " > >"
                        )
                    );
                } else if ( ! spaced) {
                    group.append(
                        AstTransformState::SpanRewrite(offset, offset, " ")
                    );
                }
            }
            addAtomicGroup(source, group, out);
        }
    }
}

void runEarlyDiagnostics(FormatDocument *source, const AstRuleOptions & options)
{
    QList<int > lambdaBodies;
    int declarationStart = 0;
    int declarationSkipUntil = 0;
    const bool inXd = options.checkLambdas && formatIsInXdTree(source->path());

    for (int index = 0; index < source->symbols().size(); ++index) {
        const int start = source->symbols().at(index).from;
        if (options.checkMemberInitializers
            && index >= declarationSkipUntil
            && source->scopeOf(index) == FormatDocument::ClassScope
        ) {
            const Token kind = source->token(index);
            const Token previous = index > 0 ? source->token(index - 1)
                                             : NOTOKEN;
            if (kind == SEMIC
                || kind == LBRACE
                || kind == RBRACE
                || (kind == COLON
                    && (previous == PUBLIC
                        || previous == PRIVATE
                        || previous == PROTECTED
                        || previous == SLOTS
                        || previous == SIGNALS)
                )
            ) {
                declarationStart = index + 1;
            } else if (kind == LPAREN) {
                declarationSkipUntil = qMax(index, source->match(index)) + 1;
            } else if (kind == EQ) {
                bool skip = previous == RPAREN
                    || previous == CONST
                    || previous == OPERATOR;
                for (int token = declarationStart; token < index && ! skip; ++token) {
                    const Token head = source->token(token);
                    skip = head == STATIC
                        || head == TYPEDEF
                        || head == USING
                        || head == ENUM
                        || head == FRIEND
                        || head == TEMPLATE
                        || (head == IDENTIFIER
                            && source->lexem(token) == "constexpr");
                }
                if ( ! skip) {
                    source->report(
                        start,
                        "member-init",
                        QString(QLL("initialize the member in the constructor's "
                            "initializer list instead"))
                    );
                }
                const int statement = statementEnd(source, index);
                if (statement >= 0) {
                    declarationSkipUntil = statement;
                }
            }
        }
        if (options.checkLambdas) {
            while ( ! lambdaBodies.isEmpty()
                && index > source->match(lambdaBodies.last())
            ) {
                lambdaBodies.removeLast();
            }
            const int body = lambdaBody(source, index);
            if (body >= 0) {
                if ( ! lambdaBodies.isEmpty()) {
                    source->report(
                        start,
                        "no-lambda",
                        QString(QLL("move the outer lambda into a named function: "
                            "a lambda never encloses another one"))
                    );
                } else if (inXd) {
                    source->report(
                        start,
                        "no-lambda",
                        QString(QLL("replace the lambda with a named member or "
                            "static function"))
                    );
                }
                lambdaBodies.append(body);
            }
        }
        if (options.checkConstOverloads
            && source->token(index) == LBRACE
            && source->braceScope(index) == FormatDocument::ClassScope
        ) {
            QHash<QByteArray, int > firstMutable;
            QSet<QByteArray > reported;
            const QList<int > tokens = classBodyTokens(source, index);
            for (int t = 0; t < tokens.size(); ++t) {
                const int i = tokens.at(t);
                if (source->token(i) != IDENTIFIER
                    || source->token(i + 1) != LPAREN
                    || ! endsType(source->token(i - 1))
                    || source->match(i + 1) < 0
                ) {
                    continue;
                }
                const int close = source->match(i + 1);
                int commas = 0;
                for (int k = i + 2; k < close; ++k) {
                    if (source->token(k) == LPAREN
                        || source->token(k) == LANGLE
                    ) {
                        k = qMax(k, source->match(k));
                    } else if (source->token(k) == COMMA) {
                        ++commas;
                    }
                }
                const QByteArray key = source->lexem(i)
                    + '/'
                    + QByteArray::number(commas);
                const bool isConst = source->token(close + 1) == CONST;
                if ( ! isConst && ! firstMutable.contains(key)) {
                    firstMutable.insert(key, i);
                } else if (isConst
                    && firstMutable.contains(key)
                    && ! reported.contains(key)
                ) {
                    reported.insert(key);
                    source->report(
                        source->symbols().at(firstMutable.value(key)).from,
                        "const-overload-order",
                        QString(QLL("declare the const overload of %1() first"))
                            .arg(QString::fromLatin1(source->lexem(i)))
                    );
                }
            }
        }
        if (options.checkGuardedGetters
            && source->token(index) == RETURN
            && index + 5 < source->symbols().size()
        ) {
            int value = index + 1;
            if (source->token(value) == THIS
                && source->token(value + 1) == ARROW
            ) {
                value += 2;
            }
            if (source->token(value) == IDENTIFIER
                && source->token(value + 1) == DOT
                && source->token(value + 2) == IDENTIFIER
                && source->lexem(value + 2) == "data"
                && source->token(value + 3) == LPAREN
                && source->token(value + 4) == RPAREN
            ) {
                const QByteArray name = source->lexem(value);
                bool guarded = false;
                for (int candidate = 0; candidate + 1 < source->symbols().size() && ! guarded; ++candidate)
                {
                    if (source->token(candidate) != IDENTIFIER
                        || source->lexem(candidate) != "QPointer"
                        || source->token(candidate + 1) != LANGLE
                    ) {
                        continue;
                    }
                    int closing = candidate + 1;
                    int depth = 0;
                    for (; closing < source->symbols().size(); ++closing) {
                        const Token token = source->token(closing);
                        depth += token == LANGLE
                            ? 1
                            : (token == RANGLE ? -1 : (token == GTGT ? -2 : 0));
                        if (depth <= 0) {
                            break;
                        }
                    }
                    guarded = source->token(closing + 1) == IDENTIFIER
                        && source->token(closing + 2) == SEMIC
                        && source->lexem(closing + 1) == name;
                }
                if (guarded) {
                    source->report(
                        start,
                        "qpointer-getter",
                        QString(QLL("return the QPointer itself, not its data()"))
                    );
                }
            }
        }
        if (options.checkCodeShapes
            && ! source->isDirectiveLine(source->symbolLine(index))
        ) {
            const Token previous = index > 0 ? source->token(index - 1)
                                             : NOTOKEN;
            if (index == 0
                || previous == SEMIC
                || previous == LBRACE
                || previous == RBRACE
            ) {
                reportStatementShapes(source, index);
            }
            if (source->token(index) == LBRACE
                && source->braceScope(index) == FormatDocument::FunctionScope
            ) {
                reportLongInlineBody(source, index);
            }
            const int line = source->symbolLine(index);
            if (source->firstSymbolOfLine(line) == index
                && QString::fromUtf8(source->lineText(line)).size() > 70
            ) {
                const int last = source->lastSymbolOfLine(line);
                for (int token = index; token < last; ++token) {
                    if (source->token(token) == RPAREN
                        && source->token(token + 1) == COLON
                        && source->match(token) >= index
                        && source->scopeOf(token)
                            != FormatDocument::FunctionScope
                        && source->scopeOf(token) != FormatDocument::BlockScope
                        && source->symbolLine(token + 1) == line
                        && token + 2 <= last
                    ) {
                        source->report(
                            source->lineStart(line),
                            "code-shape",
                            QString(
                                QLL("break the member-initializer list onto lines of its own")
                            )
                        );
                        break;
                    }
                }
            }
        }
    }
}

void collectSuperTypedefs(FormatDocument *source, SpanList *out)
{
    const FormatConfig & runConfig = source->formatConfig();
    const FormatFileConfig & fileConfig = source->config();
    const bool useDefaults = ! source->hasInlineConfiguration()
        && ! source->hasSelectedRanges()
        && runConfig.fileLineSpecs.isEmpty()
        && runConfig.lineSpecs.isEmpty()
        && fileConfig.typedefSuper == FormatFileConfig::defaultTypedefSuper()
        && fileConfig.replaceSuperSites
            == FormatFileConfig::defaultReplaceSuperSites();
    if (useDefaults) {
        return;
    }
    for (int index = 0; index < source->symbols().size(); ++index) {
        if (source->token(index) != LBRACE
            || source->braceScope(index) != FormatDocument::ClassScope
        ) {
            continue;
        }
        const int closing = source->match(index);
        if (closing <= index) {
            continue;
        }
        bool isStruct = false;
        const QByteArray base = FormatRelations::baseName(
            source,
            index,
            &isStruct
        );
        if ( ! base.isEmpty()
            && ! FormatRelations::hasSuper(source, index)
            && FormatRelations::wantsTypedef(source, index, base)
        ) {
            const int start = source->symbols().at(index).from;
            if (isStruct) {
                source->report(
                    start,
                    "super-typedef",
                    QString(QLL("give the struct a private typedef %1 super;"))
                        .arg(QString::fromUtf8(base))
                );
            } else {
                const int first = index + 1;
                const int firstLine = source->symbolLine(first);
                const QByteArray text = "typedef " + base + " super;";
                if (firstLine == source->symbolLine(index)) {
                    const int offset = source->symbolEnd(index);
                    const bool hasSpace = offset < source->text().size()
                        && (source->text().at(offset) == ' '
                            || source->text().at(offset) == '\t');
                    addSingleSpan(
                        source,
                        offset,
                        offset,
                        " " + text + (hasSpace ? "" : " "),
                        out
                    );
                } else if (source->token(first) == Q_OBJECT_TOKEN
                    || source->token(first) == Q_GADGET_TOKEN
                ) {
                    const int endOff = contentEnd(source, firstLine);
                    addSingleSpan(
                        source,
                        endOff,
                        endOff,
                        "\n" + source->indentOf(firstLine) + text,
                        out
                    );
                } else {
                    const Token token = source->token(first);
                    const bool access = token == PUBLIC
                        || token == PRIVATE
                        || token == PROTECTED
                        || token == RBRACE;
                    const QByteArray indent = access
                        ? source->indentOf(source->symbolLine(index)) + "    "
                        : source->indentOf(firstLine);
                    const int ls = source->lineStart(firstLine);
                    addSingleSpan(
                        source,
                        ls,
                        ls,
                        indent + text + "\n",
                        out
                    );
                }
            }
        }
    }
}

void collectSuperReferences(const FormatDocument *source, SpanList *out)
{
    const FormatConfig & runConfig = source->formatConfig();
    const FormatFileConfig & defaultConfig = source->config();
    const bool useDefaults = ! source->hasInlineConfiguration()
        && ! source->hasSelectedRanges()
        && runConfig.fileLineSpecs.isEmpty()
        && runConfig.lineSpecs.isEmpty()
        && defaultConfig.typedefSuper == FormatFileConfig::defaultTypedefSuper()
        && defaultConfig.replaceSuperSites
            == FormatFileConfig::defaultReplaceSuperSites();
    if (useDefaults) {
        return;
    }

    int superSkipUntil = 0;
    QHash<QString, QStringList > pathsByExtensions;
    QHash<QString, QSharedPointer<FormatDocument > > owners;
    QHash<QString, int > classBraces;
    for (int index = 0; index < source->symbols().size(); ++index) {
        if (index < superSkipUntil
            || (source->token(index) != IDENTIFIER
                && source->token(index) != SCOPE)
            || source->isDirectiveLine(source->symbolLine(index))
        ) {
            continue;
        }
        if ( ! ((source->token(index) == IDENTIFIER && (source->token(
                index + 1) == SCOPE || source->token(
                    index + 1) == LANGLE)) || (source->token(index) == SCOPE && source->token(
                        index + 1) == IDENTIFIER))
        ) {
            continue;
        }
        const int start = source->symbols().at(index).from;
        FormatScopeConditions scopes(source);
        const QByteArray className =
            FormatRelations::ownerOf(source, &scopes, index);
        if (className.isEmpty()) {
            continue;
        }
        const FormatFileConfig & fileConfig = source->configAt(start);
        QString extensions = fileConfig.extensions;
        const QString extensionsKey = QString(QLL("general.extensions"));
        if (fileConfig.hasExpression(extensionsKey)) {
            const QVariant resolved = scopes.settingAt(
                extensionsKey,
                index
            );
            if (resolved.isValid()) {
                extensions = resolved.toString();
            }
        }
        if ( ! pathsByExtensions.contains(extensions)) {
            FormatFileConfig effectiveConfig = fileConfig;
            effectiveConfig.extensions = extensions;
            pathsByExtensions.insert(
                extensions,
                FormatRelations::relatedFiles(source, effectiveConfig)
            );
        }
        const QString key = extensions
            + QLatin1Char('\n')
            + QString::fromUtf8(className);
        if ( ! classBraces.contains(key)) {
            int found = FormatRelations::findQualifiedClass(
                source,
                className
            );
            const QStringList paths = pathsByExtensions.value(extensions);
            for (int path = 0; found < 0 && path < paths.size(); ++path) {
                QSharedPointer<FormatDocument > related(
                        FormatRelations::readRelated(
                            paths.at(path),
                            runConfig
                        ));
                if (related) {
                    found = FormatRelations::findQualifiedClass(
                        related.data(),
                        className
                    );
                    if (found >= 0) {
                        owners.insert(key, related);
                    }
                }
            }
            classBraces.insert(key, found);
        }
        const int brace = classBraces.value(key);
        if (brace < 0) {
            continue;
        }
        const FormatDocument *owner = owners.contains(key)
            ? owners.value(key).data() : source;
        bool isStruct = false;
        const QByteArray base = FormatRelations::baseName(
            owner,
            brace,
            &isStruct
        );
        const int scopeToken = base.isEmpty()
            ? -1
            : FormatRelations::matchBase(
                source,
                index,
                base,
                ! className.contains("::")
            );
        if (scopeToken >= 0
            && ! FormatRelations::isShadowed(source, &scopes, index, base)
        ) {
            if (FormatRelations::permitsSuperReference(
                owner,
                brace,
                base,
                source,
                index
            )) {
                const bool constructor = index > 0
                    && source->token(index - 1) == USING
                    && scopeToken + 1 < source->symbols().size()
                    && source->lexem(scopeToken + 1)
                        == FormatRelations::baseIdentifier(base);
                const int to = constructor
                    ? source->symbolEnd(scopeToken + 1)
                    : source->symbols().at(scopeToken).from;
                const SpanList before = *out;
                addSingleSpan(
                    source,
                    start,
                    to,
                    constructor ? "super::super" : "super",
                    out,
                    true
                );
                if (out->size() > before.size()) {
                    superSkipUntil = scopeToken + 1;
                    continue;
                }
            }
            superSkipUntil = scopeToken + 1;
        }
    }
}

void collectNamespaceBraces(const FormatDocument *source, SpanList *out)
{
    for (int i = 0; i < source->symbols().size(); ++i) {
        if (source->token(i) != NAMESPACE
            || (i > 0
                && source->token(i - 1) == USING)
        ) {
            continue;
        }
        int open = i + 1;
        while (open < source->symbols().size()
            && source->token(open) != LBRACE
            && source->token(open) != EQ
            && source->token(open) != SEMIC
        ) {
            ++open;
        }
        if (open >= source->symbols().size()
            || source->token(open) != LBRACE
        ) {
            continue;
        }
        const int close = source->match(open);
        if (close <= open
            || source->spansDirective(
                source->symbols().at(open).from,
                source->symbolEnd(close))
        ) {
            continue;
        }
        const int openLine = source->symbolLine(open);
        const QByteArray indent = source->indentOf(openLine);
        const int closeLine = source->symbolLine(close);

        if (open + 1 <= close
            && source->symbolLine(open + 1) == source->symbolLine(open)
            && ! source->hasProtectedCommentBetween(
                source->symbolEnd(open),
                source->symbols().at(open + 1).from)
        ) {
            const int from = source->symbolEnd(open);
            addSingleSpan(
                source,
                from,
                source->symbols().at(open + 1).from,
                "\n" + indent,
                out,
                true
            );
        }
        if (source->firstSymbolOfLine(closeLine) != close
            && ! source->hasProtectedCommentBetween(
                source->symbolEnd(close - 1),
                source->symbols().at(close).from)
        ) {
            const int from = source->symbolEnd(close - 1);
            addSingleSpan(
                source,
                from,
                source->symbols().at(close).from,
                "\n" + indent,
                out,
                true
            );
        }
        if (close + 1 < source->symbols().size()
            && source->token(close + 1) != SEMIC
            && source->symbolLine(close + 1) == closeLine
        ) {
            const int from = source->symbolEnd(close);
            const int to = source->symbols().at(close + 1).from;
            if (source->isCodeWhitespace(from, to)) {
                addSingleSpan(source, from, to, "\n" + indent, out, true);
            }
        }
    }
}

void buildAddInlineKeywordSpans(
        const FormatDocument *document, int start, SpanList *group)
{
    const QByteArray keyword("inline ");
    const int startOffset = document->symbols().at(start).from;
    group->append(
        AstTransformState::SpanRewrite(startOffset, startOffset, keyword)
    );
    const int line = document->symbolLine(start);
    int open = start;
    while (open < document->symbols().size()
        && document->token(open) != LPAREN
    ) {
        ++open;
    }
    const int close = open < document->symbols().size() ? document->match(open)
                                                        : -1;
    const int last = close >= 0 ? document->symbolLine(close) : line;
    const int indent = textWidth(document->indentOf(line));
    for (int l = line + 1; l <= last; ++l) {
        if (textWidth(document->indentOf(l)) > indent) {
            const int ls = document->lineStart(l);
            group->append(
                AstTransformState::SpanRewrite(
                    ls,
                    ls,
                    QByteArray(keyword.size(), ' ')
                )
            );
        }
    }
}

void buildSetOverrideMarkerSpans(
        const FormatDocument *document,
        int overrideToken,
        const QString & marker,
        SpanList *group)
{
    const int end = document->symbolEnd(overrideToken);
    if (marker.isEmpty()) {
        const int from = document->symbolEnd(overrideToken - 1);
        group->append(
            AstTransformState::SpanRewrite(
                from,
                end,
                restoreCommentsInReplacement(
                    document,
                    from,
                    end - from,
                    QByteArray()
                )
            )
        );
        return;
    }
    const int from = document->symbols().at(overrideToken).from;
    group->append(AstTransformState::SpanRewrite(
        from,
        end,
        marker.toLatin1()
    ));
}

void collectInlineKeywords(const FormatDocument *source, SpanList *out)
{
    for (int brace = 0; brace < source->symbols().size(); ++brace) {
        if (source->token(brace) != LBRACE
            || source->braceScope(brace) != FormatDocument::ClassScope
        ) {
            continue;
        }
        const int closing = source->match(brace);
        if (closing <= brace) {
            continue;
        }
        QList<int > lacking;
        bool anyInline = false;
        QList<QPair<int, int > > lackingOverrides;
        const QList<int > tokens = classBodyTokens(source, brace);
        for (int t = 0; t < tokens.size(); ++t) {
            const int body = tokens.at(t);
            if (source->token(body) != LBRACE
                || source->braceScope(body) != FormatDocument::FunctionScope
            ) {
                continue;
            }
            bool hasInline = false;
            bool exempt = false;
            int overrideToken = -1;
            const int funcStart = functionDeclarationStart(
                source,
                body,
                &hasInline,
                &exempt,
                &overrideToken
            );
            if (overrideToken >= 0) {
                const int offset = source->symbols().at(overrideToken).from;
                const QString marker = settingValueAt(
                    source,
                    offset,
                    QString(QLL("lint.inlineOverride")),
                    source->configAt(offset).inlineOverride
                ).toString();
                const bool keepsMarker = marker == QLL("override")
                    || marker == QLL("Q_DECL_OVERRIDE");
                if ( ! keepsMarker) {
                    if (hasInline) {
                        SpanList group;
                        buildSetOverrideMarkerSpans(
                            source,
                            overrideToken,
                            marker,
                            &group
                        );
                        addAtomicGroup(source, group, out);
                        anyInline = true;
                    } else if ( ! marker.isEmpty()
                        && ! exempt
                        && funcStart >= 0
                    ) {
                        lackingOverrides.append(
                            qMakePair(funcStart, overrideToken)
                        );
                    }
                    continue;
                }
            }
            anyInline = anyInline || hasInline;
            if ( ! hasInline && ! exempt && funcStart >= 0) {
                lacking.append(funcStart);
            }
        }
        if ( ! anyInline) {
            continue;
        }
        for (int i = 0; i < lacking.size(); ++i) {
            SpanList group;
            buildAddInlineKeywordSpans(source, lacking.at(i), &group);
            addAtomicGroup(source, group, out);
        }
        for (int i = 0; i < lackingOverrides.size(); ++i) {
            const QPair<int, int > item = lackingOverrides.at(i);
            const int offset = source->symbols().at(item.second).from;
            const QString marker = settingValueAt(
                source,
                offset,
                QString(QLL("lint.inlineOverride")),
                source->configAt(offset).inlineOverride
            ).toString();
            SpanList group;
            buildAddInlineKeywordSpans(source, item.first, &group);
            buildSetOverrideMarkerSpans(
                source,
                item.second,
                marker,
                &group
            );
            addAtomicGroup(source, group, out);
        }
    }
}

void collectLeadInitializers(
        const FormatDocument *source, bool alignOnly, SpanList *out)
{
    int initializerSkipUntil = 0;
    for (int index = 1; index < source->symbols().size(); ++index) {
        if (index < initializerSkipUntil
            || source->token(index) != COLON
            || source->token(index - 1) != RPAREN
        ) {
            continue;
        }
        int endBrace = -1;
        for (int k = index + 1; k < source->symbols().size(); ++k) {
            const Token token = source->token(k);
            if (token == LPAREN
                || (token == LBRACE
                    && source->braceScope(k) == FormatDocument::InitScope)
            ) {
                k = qMax(k, source->match(k));
                continue;
            }
            if (token == LBRACE) {
                endBrace = k;
            }
            if (token == LBRACE || token == SEMIC || token == RBRACE) {
                break;
            }
        }
        if (endBrace < 0
            || source->braceScope(endBrace) != FormatDocument::FunctionScope
        ) {
            continue;
        }

        if ( ! alignOnly) {
            for (int k = index; k < endBrace; ++k) {
                const Token token = source->token(k);
                if (token == LPAREN) {
                    k = qMax(k, source->match(k));
                    continue;
                }
                const int line = source->symbolLine(k);
                if ((token != COLON
                        && token != COMMA)
                    || source->lastSymbolOfLine(line) != k
                    || source->symbolEndLine(k - 1) != line
                    || source->hasProtectedCommentBetween(
                        source->symbolEnd(k),
                        source->symbols().at(k + 1).from)
                ) {
                    continue;
                }
                const int from = source->symbolEnd(k - 1);
                if (source->spansDirective(
                    from,
                    source->symbols().at(k + 1).from
                )) {
                    continue;
                }
                SpanList pair;
                pair.append(
                    AstTransformState::SpanRewrite(
                        from,
                        source->symbolEnd(k),
                        QByteArray()
                    )
                );
                const int nextFrom = source->symbols().at(k + 1).from;
                pair.append(
                    AstTransformState::SpanRewrite(
                        nextFrom,
                        nextFrom,
                        source->lexem(k) + ' '
                    )
                );
                addAtomicGroup(source, pair, out);
            }
        } else {
            const int colonLine = source->symbolLine(index);
            if (source->firstSymbolOfLine(colonLine) == index) {
                const QByteArray indent = source->indentOf(colonLine);
                for (int k = index + 1; k < endBrace; ++k) {
                    const int line = source->symbolLine(k);
                    if (source->token(k) != COMMA
                        || source->firstSymbolOfLine(line) != k
                        || source->indentOf(line) == indent
                    ) {
                        continue;
                    }
                    const int ls = source->lineStart(line);
                    addSingleSpan(
                        source,
                        ls,
                        ls + source->indentOf(line).size(),
                        indent,
                        out
                    );
                }
            }
        }
        initializerSkipUntil = endBrace + 1;
    }
}

void collectControlBraces(
        FormatDocument *source,
        bool expandInlineOnly,
        bool joinElseOnly,
        SpanList *out)
{
    for (int i = 0; i < source->symbols().size(); ++i) {
        const Token tok = source->token(i);
        const bool isCtrl = tok == IF
            || tok == ELSE
            || tok == DO
            || tok == FOR
            || tok == WHILE
            || (tok == IDENTIFIER
                && (source->lexem(i) == "foreach"
                    || source->lexem(i) == "Q_FOREACH"));
        if ( ! isCtrl || source->isDirectiveLine(source->symbolLine(i))) {
            continue;
        }
        if (joinElseOnly) {
            if (tok == ELSE
                && i > 0
                && source->token(i - 1) == RBRACE
                && source->symbolEndLine(i - 1) < source->symbolLine(i)
                && ! source->hasProtectedCommentBetween(
                    source->symbolEnd(i - 1),
                    source->symbols().at(i).from)
                && ! source->spansDirective(
                    source->symbolEnd(i - 1),
                    source->symbols().at(i).from)
            ) {
                const int from = source->symbolEnd(i - 1);
                addSingleSpan(
                    source,
                    from,
                    source->symbols().at(i).from,
                    " ",
                    out,
                    true
                );
            }
            continue;
        }
        const int ctrlBody = controlBodyStart(source, i);
        if (ctrlBody >= 0
            && source->token(ctrlBody) == LBRACE
            && ! source->isHeader()
        ) {
            if (expandInlineOnly) {
                SpanList group;
                if (expandInlineBody(source, i, ctrlBody, &group)) {
                    addAtomicGroup(source, group, out);
                }
            }
            continue;
        }
        if (expandInlineOnly
            || ctrlBody < 0
            || source->token(ctrlBody) == LBRACE
            || source->token(ctrlBody) == SEMIC
        ) {
            continue;
        }
        const Token bodyTok = source->token(ctrlBody);
        if (bodyTok == IF
            || bodyTok == FOR
            || bodyTok == WHILE
            || bodyTok == DO
            || bodyTok == SWITCH
            || bodyTok == TRY
        ) {
            source->report(
                source->symbols().at(i).from,
                "control-braces",
                QString(QLL("wrap the nested control statement in braces"))
            );
            continue;
        }
        SpanList group;
        braceStatementBody(source, i, ctrlBody - 1, ctrlBody, &group);
        addAtomicGroup(source, group, out);
    }
}

void collectWrapOperators(FormatDocument *source, SpanList *out)
{
    FormatScopeConditions conditions(source);
    for (int index = 0; index < source->symbols().size(); ++index) {
        const int line = source->symbolLine(index);
        if (source->firstSymbolOfLine(line) != index
            || source->isDirectiveLine(line)
        ) {
            continue;
        }
        const int limit = wrapMaxAt(source, source->lineStart(line));
        int first = -1;
        int last = -1;
        if ( ! findWholeStatement(source, line, &first, &last)) {
            continue;
        }
        if ( ! codeTargetAnyAt(source, source->lineStart(line))) {
            const int endLine = source->symbolEndLine(last);
            int current = line;
            while (current <= endLine
                && textWidth(source->codeLineText(current))
                    <= wrapColumn(source, source->lineStart(current))
            ) {
                ++current;
            }
            if (current > endLine) {
                continue;
            }
        }
        const QByteArray indent = source->indentOf(line);
        FormatExpressionLayout layout(
            source,
            &conditions,
            textWidth(indent)
        );
        if (textWidth(indent + layout.oneLine(first, last)) <= limit
            || ! layout.hasOuterChain(first, last - 1)
        ) {
            continue;
        }
        const QByteArray laidOut = layout.layout(
            first,
            last - 1,
            textWidth(indent),
            1
        );
        const int from = source->symbols().at(first).from;
        const int length = source->symbolEnd(last - 1) - from;
        if (laidOut == source->codeSlice(from, from + length)) {
            continue;
        }
        const SpanList before = *out;
        addSingleSpan(source, from, from + length, laidOut, out, true);
        if (out->size() > before.size() && layout.brokeUngrouped()) {
            source->request(
                from,
                "operator-wrap",
                QString(QLL("the tool broke this expression at its operators "
                    "by width alone; regroup its lines so that the "
                    "operands that belong together (a key and its "
                    "value, a format and its arguments) share one, "
                    "each line within %1 columns and led by its "
                    "operator")).arg(limit),
                1,
                limit
            );
        }
    }
}

void collectWrapTernaries(const FormatDocument *source, SpanList *out)
{
    FormatScopeConditions conditions(source);
    const FormatConditionLayout layout(source, &conditions);
    for (int index = 0; index < source->symbols().size(); ++index) {
        const int line = source->symbolLine(index);
        if (source->firstSymbolOfLine(line) != index
            || source->isDirectiveLine(line)
        ) {
            continue;
        }
        const int first = index;
        const int last = source->lastSymbolOfLine(line);
        if (first >= 0
            && (first == 0
                || source->token(first - 1) == SEMIC
                || source->token(first - 1) == LBRACE
                || source->token(first - 1) == RBRACE
                || source->token(first - 1) == COLON)
            && source->token(first) != IF
            && source->token(first) != WHILE
            && source->token(first) != FOR
            && source->token(first) != SWITCH
            && source->token(first) != DO
            && source->token(first) != ELSE
            && source->token(first) != CASE
        ) {
            const int end = statementEnd(source, first);
            int question = -1;
            int colon = -1;
            if (end > last
                && findTernary(source, first, end - 1, &question, &colon)
                && source->symbolLine(question) > line
                && source->symbolLine(question) < source->symbolLine(colon)
                && ! source->hasProtectedCommentBetween(
                    source->symbols().at(first).from,
                    source->symbolEnd(end))
                && ! source->spansDirective(
                    source->symbols().at(first).from,
                    source->symbolEnd(end))
            ) {
                const QByteArray pad = source->indentOf(line) + "    ";
                const int endOfLine = source->lineEnd(line);
                const QByteArray newline =
                    source->text().mid(endOfLine - 1, 1) == "\r"
                    ? "\r\n" : "\n";
                const QByteArray gap = newline + pad;
                const int beforeQuestion = source->symbolEnd(question - 1);
                const int questionFrom = source->symbols().at(question).from;
                const int beforeColon = source->symbolEnd(colon - 1);
                const int colonFrom = source->symbols().at(colon).from;
                if (source->codeSlice(beforeQuestion, questionFrom) != gap) {
                    addSingleSpan(
                        source,
                        beforeQuestion,
                        questionFrom,
                        gap,
                        out,
                        true
                    );
                }
                if (source->codeSlice(beforeColon, colonFrom) != gap) {
                    addSingleSpan(
                        source,
                        beforeColon,
                        colonFrom,
                        gap,
                        out,
                        true
                    );
                }
                continue;
            }
        }
        if (last <= first
            || source->token(last) != SEMIC
            || textWidth(source->codeLineText(line))
                <= wrapMaxAt(source, source->lineStart(line))
        ) {
            continue;
        }
        const Token before = first > 0 ? source->token(first - 1) : SEMIC;
        if (before != SEMIC
            && before != LBRACE
            && before != RBRACE
            && before != COLON
        ) {
            continue;
        }
        const Token startToken = source->token(first);
        if (startToken == IF
            || startToken == WHILE
            || startToken == FOR
            || startToken == SWITCH
            || startToken == DO
            || startToken == ELSE
            || startToken == CASE
            || startToken == RBRACE
        ) {
            continue;
        }
        bool isPlain = ! source->hasProtectedCommentBetween(
            source->symbols().at(first).from,
            source->symbolEnd(last)
        );
        for (int k = first; k < last && isPlain; ++k) {
            const Token token = source->token(k);
            if ((token == LPAREN
                    || token == LBRACK
                    || token == LBRACE)
                && source->match(k) > k
            ) {
                k = source->match(k);
                continue;
            }
            isPlain = token != SEMIC && token != RBRACE;
        }
        int question = -1;
        int colon = -1;
        if ( ! isPlain
            || ! findTernary(source, first, last - 1, &question, &colon)
        ) {
            continue;
        }
        const int indent = source->indentOf(line).size();
        const QList<QByteArray > lines = ternaryLines(
            sourceText(source, first, question - 1),
            sourceText(source, question, colon - 1),
            sourceText(source, colon, last - 1),
            indent,
            indent,
            1,
            wrapMaxAt(source, source->lineStart(line))
        );
        if (lines.size() < 2) {
            continue;
        }
        const QByteArray originalHead = sourceText(
            source,
            first,
            question - 1
        );
        const QList<QByteArray > head = lines.first() == originalHead
            ? layout.layoutHead(first, question - 1, indent)
            : QList<QByteArray >() << lines.first();
        QByteArray text;
        for (int l = 0; l < head.size(); ++l) {
            text += (l > 0 ? "\n" : "") + head.at(l);
        }
        for (int l = 1; l < lines.size(); ++l) {
            int nestedQuestion = -1;
            int nestedColon = -1;
            if (l == lines.size() - 1
                && findTernary(
                    source,
                    colon + 1,
                    last - 1,
                    &nestedQuestion,
                    &nestedColon)
            ) {
                const QByteArray pad = continuationPad(indent);
                const bool sharesBranch = lines.at(l).trimmed().startsWith('?');
                const int nestedIndent = indent
                    + (sharesBranch ? continuationIndent(0) : 0);
                const QList<QByteArray > nestedHead = layout.layoutHead(
                    colon + 1,
                    nestedQuestion - 1,
                    nestedIndent
                );
                QByteArray prefix = pad;
                if (sharesBranch) {
                    prefix += sourceText(source, question, colon - 1) + " ";
                }
                prefix += ": ";
                text += "\n" + prefix + nestedHead.first();
                for (int part = 1; part < nestedHead.size(); ++part) {
                    text += "\n" + nestedHead.at(part);
                }
                const QByteArray middle =
                    sourceText(source, nestedQuestion, nestedColon - 1);
                const QByteArray tail =
                    sourceText(source, nestedColon, last - 1);
                const int limit = wrapMaxAt(
                    source,
                    source->lineStart(line)
                );
                const QByteArray nestedPad = continuationPad(indent, 2);
                if (nestedHead.size() > 1) {
                    text += "\n" + nestedPad + middle + "\n" + nestedPad + tail;
                } else if (textWidth(
                        prefix + nestedHead.first() + " " + middle + " " + tail
                    ) + 1
                        > limit
                ) {
                    const QByteArray oneBranch = nestedPad
                        + middle
                        + " "
                        + tail;
                    if (textWidth(oneBranch) + 1 <= limit) {
                        text += "\n" + oneBranch;
                    } else {
                        text += "\n"
                            + nestedPad
                            + middle
                            + "\n"
                            + nestedPad
                            + tail;
                    }
                } else {
                    text += " " + middle + " " + tail;
                }
            } else {
                text += "\n" + lines.at(l);
            }
        }
        const int from = source->symbols().at(first).from;
        addSingleSpan(
            source,
            from,
            source->symbols().at(last).from,
            text,
            out,
            true
        );
    }
}

void collectConnectLayout(const FormatDocument *source, SpanList *out)
{
    int connectSkipUntil = 0;
    for (int i = 0; i < source->symbols().size(); ++i) {
        if (i < connectSkipUntil
            || source->token(i) != IDENTIFIER
            || source->token(i + 1) != LPAREN
            || source->isDirectiveLine(source->symbolLine(i))
        ) {
            continue;
        }
        const QByteArray name = source->lexem(i);
        if (name != "connect" && name != "disconnect") {
            continue;
        }
        const Token before = i > 0 ? source->token(i - 1) : NOTOKEN;
        if ((before == SCOPE
                && i >= 3
                && endsType(source->token(i - 3)))
            || (before != SCOPE
                && before != DOT
                && before != ARROW
                && endsType(before))
        ) {
            continue;
        }
        const int open = i + 1;
        const int close = source->match(open);
        if (close <= open
            || source->hasProtectedCommentBetween(
                source->symbols().at(open).from,
                source->symbols().at(close).from)
            || source->spansDirective(
                source->symbols().at(open).from,
                source->symbols().at(close).from)
        ) {
            continue;
        }
        QList<QByteArray > arguments;
        int from = open + 1;
        bool singleLines = true;
        for (int k = open + 1; k <= close; ++k) {
            const Token token = source->token(k);
            if ((token == LPAREN
                    || token == LBRACK
                    || token == LBRACE)
                && k != close
            ) {
                k = qMax(k, source->match(k));
                continue;
            }
            if (token != COMMA && k != close) {
                continue;
            }
            if (k == from) {
                singleLines = false;
                break;
            }
            const int argStart = source->symbols().at(from).from;
            const int argEnd = source->symbolEnd(k - 1);
            const QByteArray argument = source->codeSlice(argStart, argEnd);
            singleLines = singleLines && ! argument.contains('\n');
            arguments.append(argument);
            from = k + 1;
        }
        const bool hasType = arguments.size() == 5
            && arguments.last().startsWith("Qt::");
        if ( ! singleLines || (arguments.size() != 4 && ! hasType)) {
            continue;
        }
        const int line = source->symbolLine(open);
        const int column = QString::fromUtf8(
            source->codeSlice(source->lineStart(line), source->symbolEnd(open))
        ).size();
        const QByteArray pad(column, ' ');
        const QByteArray rest = source->codeSlice(
            source->symbolEnd(close),
            contentEndOf(source, close)
        );
        QByteArray layout = arguments.at(0)
            + ", "
            + arguments.at(1)
            + ", "
            + arguments.at(2)
            + ", "
            + arguments.at(3);
        const int limit = wrapMaxAt(
            source,
            source->symbols().at(open).from
        );
        if (hasType || column + layout.size() + 1 + rest.size() > limit) {
            const QByteArray pairEnd =
                source->text().mid(contentEndOf(source, open), 1) == "\r"
                ? QByteArray(",\r\n") : QByteArray(",\n");
            layout = arguments.at(0)
                + ", "
                + arguments.at(1)
                + pairEnd
                + pad
                + arguments.at(2)
                + ", "
                + arguments.at(3);
            if (hasType) {
                layout += pairEnd + pad + arguments.at(4);
            }
        }
        const int repStart = source->symbolEnd(open);
        const int repEnd = source->symbols().at(close).from;
        if (source->codeSlice(repStart, repEnd) != layout) {
            addSingleSpan(source, repStart, repEnd, layout, out, true);
        }
        connectSkipUntil = close + 1;
    }
}

void collectWrapParameters(const FormatDocument *source, SpanList *out)
{
    int parameterSkipUntil = 0;
    FormatScopeConditions conditions(source);
    for (int index = 1; index < source->symbols().size(); ++index) {
        if (index < parameterSkipUntil || source->token(index) != LPAREN) {
            continue;
        }
        const int closing = source->match(index);
        if (closing <= index + 1 || ! isDeclarationParen(source, index)) {
            continue;
        }
        const int start = source->symbols().at(index).from;
        const int line = source->symbolLine(index);
        const int closeLine = source->symbolLine(closing);
        const FormatFileConfig & fileConfig = source->configAt(start);
        const int limit = wrapMaxAt(source, start);
        const QList<QPair<int, int > > items =
            splitArguments(source, index, closing);
        const bool commented = source->hasCommentBetween(
            source->symbolEnd(index),
            source->symbols().at(closing).from
        );
        bool eligible = ! items.isEmpty()
            && ! source->isDirectiveLine(line)
            && ! source->hasProtectedCommentBetween(
                source->symbolEnd(index),
                source->symbols().at(closing).from
            );
        const bool alreadyBroken = closeLine != line;
        const bool sameLineColon = source->token(closing + 1) == COLON
            && source->symbolLine(closing + 1) == closeLine;
        if (sameLineColon
            && (alreadyBroken
                || textWidth(source->codeSlice(
                    source->lineStart(line),
                    source->symbolEnd(closing),
                    true
                )) <= limit
            )
        ) {
            continue;
        }
        if (alreadyBroken) {
            eligible = eligible && items.size() >= 2;
            for (int item = 0; item < items.size() && eligible && ! commented; ++item) {
                eligible = source->symbolLine(items.at(item).first)
                    == source->symbolEndLine(items.at(item).second);
            }
        } else {
            eligible = eligible
                && textWidth(source->codeLineText(line)) > limit;
            const int last = source->lastSymbolOfLine(line);
            for (int token = closing + 1; token <= last && eligible; ++token) {
                if (source->token(token) == LBRACE) {
                    eligible = textWidth(source->codeSlice(
                        source->lineStart(line),
                        source->symbols().at(token).from,
                        true
                    )) > limit;
                }
            }
        }
        if ( ! eligible) {
            continue;
        }
        const int from = source->lineStart(line);
        const int to = contentEnd(source, closeLine);
        const QByteArray prefix = source->codeSlice(
            from,
            source->symbolEnd(index)
        );
        const QByteArray suffix = source->codeSlice(
            source->symbols().at(closing).from,
            to
        );
        const bool breaksAfterOpen = conditions.holds(
            QString(QLL("structure.wrapSignatureAt")),
            fileConfig.wrapSignatureAt,
            index,
            FormatFileConfig::schema()
                ->programFor(QString(QLL("structure.wrapSignatureAt")))
        );
        const QByteArray pad = source->indentOf(line) + "        ";
        FormatExpressionLayout layout(source, &conditions, 0);
        QList<QByteArray > lines;
        bool fits = false;
        if (alreadyBroken) {
            if (line < source->symbolLine(items.first().first)) {
                lines = layout.layoutDeclaration(
                    index,
                    prefix,
                    suffix,
                    true,
                    commented,
                    pad,
                    limit,
                    &fits
                );
            } else {
                bool fitsUnder = false;
                bool fitsBroken = false;
                const QList<QByteArray > under = layout.layoutDeclaration(
                    index,
                    prefix,
                    suffix,
                    false,
                    false,
                    QByteArray(textWidth(prefix), ' '),
                    limit,
                    &fitsUnder
                );
                const QList<QByteArray > broken = layout.layoutDeclaration(
                    index,
                    prefix,
                    suffix,
                    true,
                    commented,
                    pad,
                    limit,
                    &fitsBroken
                );
                if ( ! commented
                    && ! breaksAfterOpen
                    && fitsUnder
                    && under.join("\n") == source->codeSlice(from, to)
                ) {
                    lines = under;
                } else {
                    lines = fitsBroken || breaksAfterOpen || commented ? broken
                                                                       : under;
                }
            }
        } else {
            const QByteArray head = prefix
                + sourceText(
                    source,
                    items.first().first,
                    items.first().second)
                + (items.size() == 1 ? suffix : ",");
            if (conditions.argsOver(index, items.size())) {
                lines = layout.layoutDeclaration(
                    index,
                    prefix,
                    suffix,
                    true,
                    true,
                    pad,
                    limit,
                    &fits
                );
            } else if (textWidth(head) <= limit
                && ! breaksAfterOpen
                && ! commented
            ) {
                lines = layout.layoutDeclaration(
                    index,
                    prefix,
                    suffix,
                    false,
                    false,
                    QByteArray(textWidth(prefix), ' '),
                    limit,
                    &fits
                );
            } else {
                lines = layout.layoutDeclaration(
                    index,
                    prefix,
                    suffix,
                    true,
                    commented,
                    pad,
                    limit,
                    &fits
                );
            }
        }
        if (commented) {
            lines = commentedSignatureLines(
                source,
                index,
                closing,
                lines,
                prefix,
                suffix,
                source->indentOf(line),
                limit
            );
        }
        const QByteArray newline = source->text().mid(to, 1) == "\r" ? "\r\n"
                                                                     : "\n";
        const QByteArray wrapped = lines.join(newline);
        if (wrapped != source->codeSlice(from, to)) {
            addSingleSpan(source, from, to, wrapped, out, true);
        }
        parameterSkipUntil = closing + 1;
    }
}

bool collectWrapArgumentsPass(
        FormatDocument *source,
        bool allowPreBreak,
        SpanList *out,
        bool *hadPreBreak)
{
    if (hadPreBreak) {
        *hadPreBreak = false;
    }
    FormatScopeConditions conditions(source);
    bool any = false;
    for (int i = 1; i < source->symbols().size(); ++i) {
        if (source->token(i) != LPAREN && source->token(i) != LBRACE) {
            continue;
        }
        const int close = source->match(i);
        const bool isCall = source->token(i) == LPAREN
            && isFunctionCallParen(source, i);
        const bool isBrace = source->token(i) == LBRACE
            && source->braceScope(i) == FormatDocument::InitScope;
        if (close <= i + 1 || ( ! isCall && ! isBrace)) {
            continue;
        }
        const bool commentedList = source->hasCommentBetween(
            source->symbols().at(i).from + 1,
            source->symbols().at(close).from
        );
        const int line = source->symbolLine(i);
        const int lineStart = source->lineStart(line);
        const int from = source->symbols().at(i).from;
        const FormatFileConfig & fileConfig = source->configAt(from);
        const int closeFrom = source->symbols().at(close).from;
        const int closeEnd = source->symbolEnd(close);

        bool isPlain = ! source->hasProtectedCommentBetween(
            from + 1,
            closeFrom)
            && ! source->spansDirective(from, closeEnd);
        for (int k = i + 1; k < close && isPlain; ++k) {
            const Token token = source->token(k);
            if ((token == LPAREN
                    || token == LBRACK
                    || token == LBRACE)
                && source->match(k) > k
            ) {
                k = source->match(k);
                continue;
            }
            isPlain = token != SEMIC;
        }
        const QList<QPair<int, int > > arguments =
            splitArguments(source, i, close);
        const bool isSplit = source->symbolLine(i) != source->symbolLine(close);
        if ( ! isPlain || arguments.isEmpty()) {
            continue;
        }
        int indent = textWidth(source->indentOf(line));
        const int headKeyword = controlKeywordFor(source, i);
        const bool isHeadLine = headKeyword >= 0
            && source->symbolLine(headKeyword) == line;
        const int headClose = headKeyword >= 0
            ? source->match(headKeyword + 1)
            : -1;
        const bool hugsHead = compactAlwaysAt(source, from)
            && isCall
            && isHeadLine
            && (source->token(headKeyword) == IF
                || source->token(headKeyword) == WHILE)
            && close + 1 == headClose
            && ! source->hasProtectedCommentBetween(
                closeEnd,
                source->symbols().at(headClose).from
            );
        if (isHeadLine && ! hugsHead) {
            indent += 4;
        }
        const int previousOperator =
            precedingChainOperator(source, close + 1, false);
        const bool inlineOperator = previousOperator >= 0
            && source->token(previousOperator) != QUESTION
            && source->firstSymbolOfLine(source->symbolLine(previousOperator))
                != previousOperator;
        const int extraPad = inlineOperator
            ? conditions.operatorPadding(previousOperator) : 0;
        FormatExpressionLayout layout(source, &conditions, indent);
        layout.setKeepsCommentedLines(commentedList);
        const int column = textWidth(source->codeSlice(
            lineStart,
            from,
            true
        ));
        const QByteArray oneLine = layout.oneLine(i, close);
        const bool endsInChain = layout.hasChain(
            arguments.last().first,
            arguments.last().second
        );
        const int closeLine = source->symbolLine(close);
        const int lastArgEnd = source->symbolEnd(arguments.last().second);
        const int lastArgLine = source->symbolEndLine(arguments.last().second);
        const Token afterClose = source->token(close + 1);
        const bool keepInlineCloser = isCall
            && afterClose != DOT
            && afterClose != ARROW
            && ! source->hasCommentBetween(lastArgEnd, closeFrom);
        const bool orphanedClose = keepInlineCloser
            && closeLine > lastArgLine
            && (lastArgLine == line || isCallInConditionChain(source, i));
        const bool hasMultipleOrChain = arguments.size() >= 2 || endsInChain;
        const bool argumentsBelow =
            source->symbolLine(arguments.first().first) > line;
        const bool over = hasMultipleOrChain
            && conditions.argsOver(i, arguments.size());
        const bool mayBreak = hasMultipleOrChain
            && (fileConfig.wrapArgsOverCondition.isEmpty()
                ? isBrace
                    || (endsInChain
                        && ( ! isSplit || argumentsBelow))
                    || over
                    || reachesWrapArgsAt(source, &conditions, i)
                : over);
        const bool needsBreak = mayBreak
            && column + textWidth(oneLine) > wrapColumn(source, from);
        const bool isMisplaced = mayBreak
            && hasMisplacedLines(source, i, close, indent, extraPad);
        if ( ! needsBreak && ! isMisplaced) {
            if (orphanedClose) {
                const SpanList before = *out;
                addSingleSpan(
                    source,
                    lastArgEnd,
                    closeFrom,
                    QByteArray(),
                    out,
                    true
                );
                if (out->size() > before.size()) {
                    any = true;
                }
            }
            continue;
        }
        int joinCount = 0;
        const bool keepsLines = keepsUserLines(
            source,
            i,
            close,
            arguments,
            &conditions,
            fileConfig,
            &joinCount
        );
        if (keepsLines
            && headKeyword < 0
            && ! reachesWrapArgsAt(source, &conditions, i)
            && isAlignedUnderFirst(source, i, close, arguments)
        ) {
            continue;
        }

        if (allowPreBreak) {
            const int body = needsBreak ? inlineFunctionBody(source, i) : -1;
            if (body >= 0) {
                const int gapStart = source->symbolEnd(body - 1);
                const QByteArray padIndent = source->indentOf(line);
                const SpanList before = *out;
                addSingleSpan(
                    source,
                    gapStart,
                    source->symbols().at(body).from,
                    "\n" + padIndent,
                    out,
                    true
                );
                if (out->size() > before.size()) {
                    any = true;
                    if (hadPreBreak) {
                        *hadPreBreak = true;
                    }
                    continue;
                }
            }
            int logicalOperator = -1;
            QByteArray logicalIndent;
            if (isCall
                && needsBreak
                && logicalBreakBeforeCall(
                    source,
                    i,
                    &logicalOperator,
                    &logicalIndent)
            ) {
                const int gapStart = source->symbolEnd(logicalOperator - 1);
                const SpanList before = *out;
                addSingleSpan(
                    source,
                    gapStart,
                    source->symbols().at(logicalOperator).from,
                    "\n" + logicalIndent,
                    out,
                    true
                );
                if (out->size() > before.size()) {
                    any = true;
                    if (hadPreBreak) {
                        *hadPreBreak = true;
                    }
                    continue;
                }
            }
        }

        const QByteArray text = layout.layoutList(
            i,
            i,
            close,
            column,
            keepsLines,
            joinCount,
            extraPad
        );
        const int length = (hugsHead ? source->symbols().at(headClose).from
                                     : closeEnd)
            - from;
        if (source->codeSlice(from, from + length) != text) {
            const SpanList before = *out;
            addSingleSpan(source, from, from + length, text, out, true);
            if (out->size() > before.size()) {
                any = true;
                if (layout.brokeUngrouped()) {
                    const int wrapMax = wrapMaxAt(source, from);
                    source->request(
                        from,
                        "args-wrap",
                        QString(QLL("the tool put each argument on a line of its own; "
                            "put the arguments that belong together on one "
                            "line, each line within %1 columns")).arg(wrapMax),
                        closeLine - line + 1,
                        wrapMax
                    );
                }
            }
        }
    }
    return any;
}

void collectWrapConditions(FormatDocument *source, SpanList *out)
{
    int conditionSkipUntil = 0;
    FormatScopeConditions conditions(source);
    const FormatConditionLayout layout(source, &conditions);
    for (int i = 0; i < source->symbols().size(); ++i) {
        if (i < conditionSkipUntil) {
            continue;
        }
        const Token token = source->token(i);
        if ((token != IF
                && token != WHILE)
            || source->token(i + 1) != LPAREN
        ) {
            continue;
        }
        const int open = i + 1;
        const int close = source->match(open);
        const int line = source->symbolLine(i);
        const FormatFileConfig & fileConfig = source->configAt(
            source->symbols().at(i).from
        );
        if (close <= open
            || source->isDirectiveLine(line)
            || source->hasProtectedCommentBetween(
                source->symbols().at(open).from,
                source->symbolEnd(close))
        ) {
            continue;
        }
        const QList<int > operators = layout.operators(open, close);
        if (operators.isEmpty()) {
            const bool isLong = source->symbolEndLine(close) == line
                && textWidth(source->codeLineText(line)) > wrapMaxAt(
                    source,
                    source->lineStart(line)
                );
            const QByteArray text = isLong
                ? layout.layoutPlain(open, close)
                : QByteArray();
            if ( ! text.isEmpty()) {
                const int from = source->symbols().at(open).from;
                int to = source->symbolEnd(close);
                QByteArray replaced = text;
                if (source->token(close + 1) == LBRACE
                    && source->symbolLine(close + 1)
                        == source->symbolEndLine(close)
                ) {
                    to = source->symbolEnd(close + 1);
                    replaced += " {";
                }
                addSingleSpan(source, from, to, replaced, out, true);
            }
            conditionSkipUntil = close + 1;
            continue;
        }
        if ( ! fileConfig.wrapConditionsOverCondition.isEmpty()
            && ! conditions.conditionsOver(open, operators.size() + 1)
        ) {
            conditionSkipUntil = close + 1;
            continue;
        }
        const int endLine = source->symbolEndLine(close);
        if (endLine == line) {
            if (textWidth(source->codeLineText(line))
                    <= wrapColumn(source, source->symbols().at(i).from)
            ) {
                continue;
            }
        } else if ( ! isConditionSplitTooLong(source, layout, open, close)
            && ! hasMisplacedLines(
                source,
                open,
                close,
                source->indentOf(line).size())
            && ! (codeTargetAnyAt(source, source->symbols().at(i).from)
                && ! layout.forcedOperators(open, close).isEmpty())
        ) {
            continue;
        } else if (source->agentMode()) {
            const int limit = wrapMaxAt(source, source->lineStart(line));
            source->request(
                source->symbols().at(i).from,
                "condition-wrap",
                QString(QLL("this condition has a line past %1 columns; "
                    "regroup its lines so that the operands that "
                    "belong together share one, each line within %1 "
                    "columns and led by its && or ||")).arg(limit),
                endLine - line + 1,
                limit
            );
            conditionSkipUntil = close + 1;
            continue;
        }
        bool hasBrace = false;
        for (int k = open + 1; k < close && ! hasBrace; ++k) {
            hasBrace = source->token(k) == LBRACE;
        }
        if (hasBrace) {
            continue;
        }
        const int indent = source->indentOf(line).size();
        const int column = source->symbols().at(open).from
            - source->lineStart(line);
        const int from = source->symbols().at(open).from;
        int to = source->symbolEnd(close);
        QByteArray replaced = layout.layoutCondition(
            open,
            close,
            indent,
            column
        );
        if (source->token(close + 1) == LBRACE
            && source->symbolLine(close + 1) == source->symbolEndLine(close)
        ) {
            to = source->symbolEnd(close + 1);
            replaced += " {";
        }
        addSingleSpan(source, from, to, replaced, out, true);
        conditionSkipUntil = close + 1;
    }
}

void collectCloseConditions(const FormatDocument *source, SpanList *out)
{
    for (int index = 0; index < source->symbols().size(); ++index) {
        const Token tok = source->token(index);
        if ((tok != IF
                && tok != WHILE
                && tok != FOR
                && tok != SWITCH)
            || source->token(index + 1) != LPAREN
            || source->isDirectiveLine(source->symbolLine(index))
        ) {
            continue;
        }
        const int close = source->match(index + 1);
        if (close <= index
            || source->token(close + 1) != LBRACE
            || source->symbolLine(close) == source->symbolLine(index)
            || source->spansDirective(
                source->symbols().at(index).from,
                source->symbolEnd(close + 1))
        ) {
            continue;
        }
        const int start = source->symbols().at(index).from;
        const QByteArray indent = source->indentOf(source->symbolLine(index));
        const int last = close - 1;
        const int closeLine = source->symbolLine(close);
        const Token lastToken = source->token(last);
        const bool hugsGroup = compactAlwaysAt(source, start)
            && (lastToken == RPAREN || lastToken == RBRACK)
            && source->match(last) >= 0
            && source->symbolLine(source->match(last))
                == source->symbolLine(index)
            && source->firstSymbolOfLine(closeLine) == last
            && source->indentOf(closeLine) == indent;
        bool eligible = true;
        if (hugsGroup || source->symbolEndLine(last) == closeLine) {
            eligible = isSameLineGap(source, last, close);
            if (eligible) {
                const int from = source->symbolEnd(last);
                const int to = source->symbols().at(close).from;
                const QByteArray gap = hugsGroup ? QByteArray() : "\n" + indent;
                if (source->codeSlice(from, to) != gap) {
                    addSingleSpan(source, from, to, gap, out, true);
                }
            }
        } else if (source->indentOf(closeLine) != indent) {
            const int from = source->lineStart(closeLine);
            addSingleSpan(
                source,
                from,
                from + source->indentOf(closeLine).size(),
                indent,
                out
            );
        }
        if (eligible) {
            const int from = source->symbolEnd(close);
            const int to = source->symbols().at(close + 1).from;
            if (source->codeSlice(from, to) != " "
                && ! source->hasProtectedCommentBetween(from, to)
                && ! source->spansDirective(from, to)
            ) {
                addSingleSpan(source, from, to, " ", out, true);
            }
        }
    }
}

void collectSpaceReferences(
        FormatDocument *source,
        bool regexpOnly,
        QSet<int > *regexpLinesDone,
        SpanList *out)
{
    source->setPass(0);
    for (int index = 1; index + 2 < source->symbols().size(); ++index) {
        const Token kind = source->token(index);
        const Token after = source->token(index + 2);
        if ((kind != AND
                && kind != ANDAND)
            || source->token(index + 1) != IDENTIFIER
            || ! isTypeEnd(source->token(index - 1))
            || source->isDirectiveLine(source->symbolLine(index))
            || (after != COMMA
                && after != RPAREN
                && after != EQ
                && after != SEMIC
                && after != LBRACK
                && after != COLON)
            || ! isReferenceDeclaration(source, index)
            || ! isSameLineGap(source, index - 1, index)
            || ! isSameLineGap(source, index, index + 1)
        ) {
            continue;
        }
        const int start = source->symbols().at(index).from;
        const int end = source->symbolEnd(index);
        const int line = source->symbolLine(index);
        const FormatFileConfig & fileConfig = source->configAt(start);
        FormatSpacing setting = kind == AND ? fileConfig.refSuffix
                                            : fileConfig.moveSuffix;
        const QVariant resolved = settingValueAt(
            source,
            start,
            QString(kind == AND ? QLL("types.refSuffix")
                                : QLL("types.moveSuffix")),
            QVariant()
        );
        if (resolved.isValid()) {
            setting = FormatFileConfig::toSpacing(
                resolved.toString(),
                QString(kind == AND ? QLL(" ") : QLL("keep-as-is"))
            );
        }
        if (regexpOnly) {
            if ( ! setting.isRegExp()) {
                continue;
            }
            QByteArray replacedLine;
            const FormatSpacing res = source->onSetting(
                setting,
                index,
                &replacedLine
            );
            if (res.isNull()) {
                regexpLinesDone->insert(line);
                if ( ! replacedLine.isEmpty()) {
                    addSingleSpan(
                        source,
                        source->lineStart(line),
                        source->lineEnd(line),
                        replacedLine,
                        out
                    );
                }
            }
            continue;
        }
        if (setting.isRegExp()) {
            if (regexpLinesDone->contains(line)) {
                continue;
            }
        }
        const FormatSpacing spacing = setting.isRegExp()
            ? source->onSetting(setting, index) : setting;
        if ( ! spacing.isNull()) {
            const int from = source->symbolEnd(index - 1);
            const int to = source->symbols().at(index + 1).from;
            const QByteArray suffix = spacing.keepAsIs
                ? source->codeSlice(end, to) : spacing.text;
            const QByteArray wanted = " " + source->lexem(index) + suffix;
            if (source->codeSlice(from, to) != wanted) {
                addSingleSpan(source, from, to, wanted, out, true);
            }
        }
    }
}

void runLateDiagnostics(FormatDocument *source, const AstRuleOptions & options)
{
    for (int i = 0; i < source->symbols().size(); ++i) {
        const int start = source->symbols().at(i).from;
        if (options.checkStaticVariables
            && i + 2 < source->symbols().size()
        ) {
            do {
                if (source->token(i) != STATIC
                    || source->scopeOf(i) == FormatDocument::ClassScope
                    || source->isDirectiveLine(source->symbolLine(i))
                ) {
                    break;
                }
                const bool inFunction =
                    source->scopeOf(i) == FormatDocument::FunctionScope
                    || source->scopeOf(i) == FormatDocument::BlockScope;
                int k = i + 1;
                while (source->token(k) == CONST
                    || source->token(k) == VOLATILE
                ) {
                    ++k;
                }
                if (source->token(k) != IDENTIFIER
                    || ! source->lexem(k).startsWith('Q')
                    || source->lexem(k).size() < 2
                    || ! QChar::isUpper(source->lexem(k).at(1))
                ) {
                    if (inFunction && isStaticVariable(source, i)) {
                        const QString message = QString(QLL("move the static mapping out of the function, to file scope "
                            "(an unnamed namespace); a short list to loop task(s) over "
                            "each entry can stay"));
                        if (source->agentMode()) {
                            source->request(
                                start,
                                "global-static",
                                message,
                                1,
                                0
                            );
                        } else {
                            source->report(start, "global-static", message);
                        }
                    }
                    break;
                }
                ++k;
                if (source->token(k) == LANGLE) {
                    int depth = 0;
                    for (; k < source->symbols().size(); ++k) {
                        const Token token = source->token(k);
                        depth += token == LANGLE
                            ? 1
                            : (token == RANGLE ? -1 : (token == GTGT ? -2 : 0));
                        if (depth <= 0 || token == SEMIC) {
                            break;
                        }
                    }
                    ++k;
                }
                if (source->token(k) != IDENTIFIER) {
                    break;
                }
                const Token after = source->token(k + 1);
                if (after == SEMIC
                    || after == EQ
                    || (inFunction
                        && after == LPAREN)
                ) {
                    source->report(
                        start,
                        "global-static",
                        QString(QLL("hold the static object in a Q_GLOBAL_STATIC"))
                    );
                }
            } while (false);
        }
        if (options.checkReferenceLocal
            && i > 0
            && i + 2 < source->symbols().size()
        ) {
            do {
                if (source->token(i) != AND
                    || source->token(i + 1) != IDENTIFIER
                    || source->token(i + 2) != EQ
                    || source->isDirectiveLine(source->symbolLine(i))
                ) {
                    break;
                }
                const FormatFileConfig & config = source->configAt(start);
                const QString fallback =
                    config.byRefVariable == FormatFileConfig::LintIgnore
                    ? QString(QLL("ignore"))
                    : config.byRefVariable == FormatFileConfig::LintError
                        ? QString(QLL("error")) : QString(QLL("warn"));
                const QString value = settingValueAt(
                    source,
                    start,
                    QString(QLL("lint.byRefVariable")),
                    fallback
                ).toString();
                const FormatFileConfig::LintLevel level =
                    FormatFileConfig::lintLevel(value, config);
                if (level == FormatFileConfig::LintIgnore) {
                    break;
                }
                const FormatDocument::Scope scope = source->scopeOf(i);
                if (scope != FormatDocument::FunctionScope
                    && scope != FormatDocument::BlockScope
                ) {
                    break;
                }
                int varStart = i - 1;
                bool isConst = false;
                while (varStart >= 0) {
                    const Token token = source->token(varStart);
                    if (token == RANGLE || token == GTGT) {
                        int depth = 0;
                        for (; varStart >= 0; --varStart) {
                            const Token inner = source->token(varStart);
                            depth += inner == RANGLE
                                ? 1
                                : (inner == GTGT ? 2 : (inner == LANGLE ? -1 : 0));
                            if (depth <= 0) {
                                break;
                            }
                        }
                        --varStart;
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
                    --varStart;
                }
                const Token before = varStart >= 0 ? source->token(varStart)
                                                   : SEMIC;
                if (isConst
                    || varStart == i - 1
                    || (before != SEMIC
                        && before != LBRACE
                        && before != RBRACE)
                ) {
                    break;
                }
                const int typeFrom = source->symbols().at(varStart + 1).from;
                const QByteArray type =
                    source->text()
                    .mid(typeFrom, source->symbolEnd(i - 1) - typeFrom)
                    .simplified();
                const QString message = QString(QLL("name the local through a pointer, `%1 *%2 = &(...);`, then use "
                    "`*%2`, in place of a non-const reference")).arg(
                        QString::fromUtf8(type),
                        QString::fromUtf8(source->lexem(i + 1))
                    );
                if (source->agentMode()) {
                    source->request(
                        start,
                        "reference-local",
                        message,
                        1,
                        0
                    );
                } else {
                    source->report(
                        start,
                        "reference-local",
                        message,
                        level == FormatFileConfig::LintError
                    );
                }
            } while (false);
        }
        if (options.checkPrivateAccess
            && source->token(i) == IDENTIFIER
            && source->token(i + 1) == LPAREN
            && ! source->isDirectiveLine(source->symbolLine(i))
        ) {
            const QByteArray name = source->lexem(i);
            const Token previous = i > 0 ? source->token(i - 1) : NOTOKEN;
            if ((name == "d_func"
                    || name == "q_func")
                && ! isTypeEnd(previous)
                && previous != AND
            ) {
                source->report(
                    start,
                    "q-d-macro",
                    QString(QLL("write Q_%1(<Class>) instead of %2()"))
                        .arg(QLatin1Char(name.at(0) == 'd' ? 'D' : 'Q'))
                        .arg(QString::fromLatin1(name))
                );
            }
        }
        if (options.checkExcludedFlags && source->token(i) == NOT) {
            int member = i + 1;
            while (source->token(member) == IDENTIFIER
                && (source->token(member + 1) == DOT
                    || source->token(member + 1) == ARROW)
            ) {
                member += 2;
            }
            if (member > i + 1
                && source->token(member) == IDENTIFIER
                && source->token(member + 1) == LPAREN
            ) {
                const QByteArray name = source->lexem(member);
                if (name == "includes" || name == "includesLax") {
                    source->report(
                        start,
                        "excludes",
                        QString(
                            QLL("check a cleared flag with %1(), not !%2()")
                        )
                        .arg(name == "includes" ? QLL("excludes") : QLL("excludesLax"))
                        .arg(QString::fromLatin1(name))
                    );
                }
            }
        }
    }
}

static int enclosingClassBrace(const FormatDocument *source, int token)
{
    int depth = 0;
    for (int k = token - 1; k >= 0; --k) {
        const Token tok = source->token(k);
        if (tok == RBRACE) {
            ++depth;
        } else if (tok == LBRACE) {
            if (depth > 0) {
                --depth;
            } else if (source->braceScope(k) == FormatDocument::ClassScope) {
                return k;
            } else {
                break;
            }
        }
    }
    return -1;
}

static QByteArray classMemberIndent(const FormatDocument *source, int token)
{
    const int brace = enclosingClassBrace(source, token);
    const int line = source->symbolLine(brace >= 0 ? brace : token);
    return source->indentOf(line) + "    ";
}

static QByteArray enclosingClassName(const FormatDocument *source, int token)
{
    const int brace = enclosingClassBrace(source, token);
    if (brace < 1) {
        return QByteArray();
    }
    for (int n = brace - 1; n >= 0; --n) {
        const Token t = source->token(n);
        if (t == CLASS || t == STRUCT) {
            QByteArray name;
            for (int m = n + 1; m < brace; ++m) {
                const Token mt = source->token(m);
                if (mt == LANGLE) {
                    const int end = templateClose(source, m, brace);
                    if (end > m) {
                        m = end;
                        continue;
                    }
                }
                if (mt == COLON
                    || (mt == IDENTIFIER
                        && (source->lexem(m) == "final"
                            || source->lexem(m) == "Q_DECL_FINAL")
                    )
                ) {
                    break;
                }
                if (mt == IDENTIFIER) {
                    name = source->lexem(m);
                }
            }
            return name;
        }
        if (t == SEMIC || t == RBRACE || t == LBRACE) {
            break;
        }
    }
    return QByteArray();
}

static bool isClassMacroMember(const FormatDocument *source, int first)
{
    if (first < 0 || first >= source->symbols().size()) {
        return false;
    }
    const Token lead = source->token(first);
    if (lead == Q_OBJECT_TOKEN || lead == Q_GADGET_TOKEN) {
        return true;
    }
    if (lead != IDENTIFIER || ! isMacroName(source->lexem(first))) {
        return false;
    }
    const int next = first + 1;
    if (next >= source->symbols().size()) {
        return true;
    }
    const Token nextTok = source->token(next);
    if (nextTok == SEMIC
        || nextTok == RBRACE
        || source->symbolLine(next) > source->symbolLine(first)
    ) {
        return true;
    }
    if (nextTok != LPAREN) {
        return false;
    }
    const int close = source->match(next);
    if (close <= next
        || source->lexem(first) == enclosingClassName(source, first)
    ) {
        return false;
    }
    const int after = close + 1;
    return after >= source->symbols().size()
        || source->token(after) == SEMIC
        || source->token(after) == RBRACE
        || source->symbolLine(after) > source->symbolEndLine(close);
}

static bool isClassVariableMember(const FormatDocument *source, int first)
{
    if (first < 0 || first >= source->symbols().size()) {
        return false;
    }
    const Token lead = source->token(first);
    if (lead == CLASS
        || lead == STRUCT
        || lead == UNION
        || lead == ENUM
        || lead == TYPEDEF
        || lead == USING
        || lead == FRIEND
        || lead == TEMPLATE
        || lead == OPERATOR
        || lead == TILDE
        || lead == VIRTUAL
        || lead == EXPLICIT
        || lead == INLINE
        || lead == Q_OBJECT_TOKEN
        || lead == Q_GADGET_TOKEN
        || lead == RBRACE
    ) {
        return false;
    }
    for (int k = first; k < source->symbols().size(); ++k) {
        const Token tok = source->token(k);
        if (tok == LANGLE) {
            const int end = templateClose(
                source,
                k,
                source->symbols().size()
            );
            if (end > k) {
                k = end;
                continue;
            }
        }
        if (tok == LPAREN) {
            if (k + 1 < source->symbols().size()
                && source->token(k + 1) == STAR
            ) {
                const int match = source->match(k);
                if (match > k) {
                    k = match;
                    continue;
                }
            }
            return false;
        }
        if (tok == SEMIC
            || tok == EQ
            || tok == LBRACK
            || tok == COMMA
            || tok == COLON
        ) {
            return true;
        }
        if (tok == LBRACE) {
            return source->braceScope(k) == FormatDocument::InitScope;
        }
        if (tok == RBRACE) {
            return false;
        }
    }
    return false;
}

void collectSplitStatements(const FormatDocument *source, SpanList *out)
{
    for (int index = 0; index + 1 < source->symbols().size(); ++index) {
        if (source->isAccessLabel(index)) {
            const int line = source->symbolLine(index);
            const int next = index + 1;
            const Token nextTok = source->token(next);
            const bool nextIsAccess = next + 1 < source->symbols().size()
                && source->isAccessLabel(next + 1);
            if (source->isDirectiveLine(line)
                || nextTok == RBRACE
                || nextIsAccess
            ) {
                continue;
            }
            const bool isCompactMember = isClassVariableMember(
                source,
                next)
                || isClassMacroMember(source, next);
            const int end = source->symbolEnd(index);
            const int nextFrom = source->symbols().at(next).from;
            const int lineEndOff = source->lineEnd(line);
            const QByteArray eol =
                source->text().mid(lineEndOff - 1, 1) == "\r"
                ? QByteArray("\r\n") : QByteArray("\n");
            const int nextLine = source->symbolLine(next);
            if (nextLine == line) {
                if (isSameLineGap(source, index, next)) {
                    const QByteArray gap = (isCompactMember ? eol : eol + eol)
                        + classMemberIndent(source, index);
                    addSingleSpan(source, end, nextFrom, gap, out, true);
                }
            } else if ( ! source->hasCommentBetween(end, nextFrom)) {
                bool hasDirective = false;
                for (int l = line + 1; l < nextLine && ! hasDirective; ++l) {
                    hasDirective = source->isDirectiveLine(l);
                }
                if ( ! hasDirective) {
                    const int emptyLines = nextLine - line - 1;
                    int wantedEmpty = emptyLines;
                    if ( ! isCompactMember) {
                        wantedEmpty = 1;
                    } else if (codeTargetAnyAt(source, end)) {
                        wantedEmpty = 0;
                    } else if (emptyLines > 1) {
                        wantedEmpty = 1;
                    }
                    if (wantedEmpty != emptyLines) {
                        const int fromOff = contentEnd(source, line);
                        const int toOff = source->lineStart(nextLine);
                        const QByteArray gap = wantedEmpty == 1 ? eol + eol
                                                                : eol;
                        addSingleSpan(
                            source,
                            fromOff,
                            toOff,
                            gap,
                            out,
                            true
                        );
                    }
                }
            }
            continue;
        }
        if (source->token(index) != SEMIC) {
            continue;
        }
        const int line = source->symbolLine(index);
        const int first = source->firstSymbolOfLine(line);
        const int last = source->lastSymbolOfLine(line);
        bool eligible = first >= 0
            && last > index
            && ! source->isDirectiveLine(line)
            && source->token(first) != CASE
            && source->token(first) != DEFAULT;
        if (eligible) {
            const FormatDocument::Scope scope = source->scopeOf(first);
            eligible = scope != FormatDocument::EnumScope
                && scope != FormatDocument::InitScope;
        }
        for (int token = first; token <= last && eligible; ++token) {
            const Token kind = source->token(token);
            if (kind == LBRACE || kind == RBRACE) {
                eligible = false;
            }
        }
        for (int token = first; token < index && eligible; ++token) {
            if (source->token(token) == LPAREN) {
                const int close = source->match(token);
                if (close >= index) {
                    eligible = false;
                } else if (close > token) {
                    token = close;
                }
            }
        }
        if (eligible) {
            const int end = source->symbolEnd(index);
            const int next = source->symbols().at(index + 1).from;
            if (isSameLineGap(source, index, index + 1)) {
                addSingleSpan(
                    source,
                    end,
                    next,
                    "\n" + source->indentOf(line),
                    out,
                    true
                );
            }
        }
    }
}

struct LineShiftRecord
{
    int index;
    int column;
    int delta;
};

void recordShiftContinuations(
        const FormatDocument *document,
        const QList<LineShiftRecord > & growths,
        QHash<int, int > *pendingLineShift,
        const QHash<int, QByteArray > *indentOverrides = Q_NULLPTR)
{
    QMap<int, QList<LineShiftRecord > > byLine;
    for (int i = 0; i < growths.size(); ++i) {
        byLine[document->symbolLine(growths.at(i).index)].append(growths.at(i));
    }
    QMap<int, QList<LineShiftRecord > >::const_iterator it =
        byLine.constBegin();
    for (; it != byLine.constEnd(); ++it) {
        const int line = it.key();
        const QList<LineShiftRecord > & onLine = it.value();
        int endLine = line;
        int depth = 0;
        for (int k = onLine.first().index; k < document->symbols().size(); ++k) {
            const Token token = document->token(k);
            depth += token == LPAREN || token == LBRACK ? 1 : 0;
            depth -= token == RPAREN || token == RBRACK ? 1 : 0;
            const bool ends = token == SEMIC
                || token == LBRACE
                || token == RBRACE;
            if (depth < 0 || (ends && depth == 0)) {
                endLine = document->symbolLine(k);
                break;
            }
        }
        for (int other = line + 1; other <= endLine; ++other) {
            if (document->firstSymbolOfLine(other) < 0) {
                continue;
            }
            const QByteArray indent =
                (indentOverrides && indentOverrides->contains(other))
                ? indentOverrides->value(other) : document->indentOf(other);
            int shift = 0;
            for (int g = 0; g < onLine.size(); ++g) {
                if (onLine.at(g).column < indent.size()) {
                    shift += onLine.at(g).delta;
                }
            }
            if (shift != 0) {
                (*pendingLineShift)[other] += shift;
            }
        }
    }
}

void collectQualifyMembers(const FormatDocument *source,
                           SpanList *out,
                           QHash<int, int > *pendingLineShift)
{
    FormatScopeConditions conditions(source);
    for (int index = 0; index < source->symbols().size(); ++index) {
        if (source->token(index) != IDENTIFIER
            || source->isDirectiveLine(source->symbolLine(index))
        ) {
            continue;
        }
        const FormatMemberScope scope = conditions.scopeOf(index);
        if (scope.body < 0
            || scope.params < 0
            || scope.memberToken < 0
            || scope.className.isEmpty()
            || scope.isStatic
            || scope.members.statics.contains(source->lexem(scope.memberToken))
            || ( ! scope.members.names.contains(source->lexem(index))
                && ! scope.baseMembers.contains(source->lexem(index)))
            || scope.members.statics.contains(source->lexem(index))
            || insideUncapturedLambda(source, scope.body, index)
        ) {
            continue;
        }
        const int closing = source->match(scope.body);
        QSet<QByteArray > shadowed = declaredLocalNames(
            source,
            scope.params + 1,
            source->match(scope.params)
        );
        shadowed.unite(declaredLocalNames(source, scope.body + 1, closing));
        const Token before = index > 0 ? source->token(index - 1) : NOTOKEN;
        const Token after = source->token(index + 1);
        const bool hasThis = index >= 2
            && before == ARROW
            && source->token(index - 2) == THIS;
        const bool accessible = ! shadowed.contains(source->lexem(index))
            && (hasThis
                || (before != DOT
                    && before != ARROW
                    && before != SCOPE
                    && before != TILDE
                    && before != GOTO
                ))
            && after != SCOPE
            && ! (after == COLON && before == SEMIC)
            && ! isMacroArgument(source, index);
        if (closing <= index || ! accessible) {
            continue;
        }
        const int start = source->symbols().at(index).from;
        const bool wanted = conditions.holds(
            QString(QLL("relations.thisMode")),
            source->configAt(start).thisMode,
            index
        );
        if (wanted != hasThis) {
            const int from = source
                ->symbols()
                .at(hasThis ? index - 2 : index)
                .from;
            const int line = source->symbolLine(index);
            const int column = from - source->lineStart(line);
            const int length = hasThis ? start - from : 0;
            const QByteArray inserted = wanted ? QByteArray("this->")
                                               : QByteArray();
            if (length > 0
                && source->hasProtectedCommentBetween(from, from + length)
            ) {
                continue;
            }
            const SpanList beforeSize = *out;
            addSingleSpan(
                source,
                from,
                from + length,
                inserted,
                out,
                length > 0
            );
            if (out->size() > beforeSize.size()) {
                QList<LineShiftRecord > growths;
                LineShiftRecord rec;
                rec.index = index;
                rec.column = column;
                rec.delta = inserted.size() - length;
                growths << rec;
                recordShiftContinuations(source, growths, pendingLineShift);
            }
        }
    }
}

void collectContinuationIndentAndExpandBodies(
        const FormatDocument *source,
        bool indentContinuations,
        bool expandBodies,
        const QHash<int, int > & initialLineShifts,
        SpanList *out)
{
    QHash<int, QByteArray > indentOverrides;
    QHash<int, int >::const_iterator it = initialLineShifts.constBegin();
    for (; it != initialLineShifts.constEnd(); ++it) {
        const int line = it.key();
        const int shift = it.value();
        const QByteArray cur = source->indentOf(line);
        if (shift > 0) {
            indentOverrides.insert(line, cur + QByteArray(shift, ' '));
        } else if (shift < 0 && cur.size() + shift > 0) {
            indentOverrides.insert(line, cur.mid(-shift));
        }
    }

    for (int index = 0; index < source->symbols().size(); ++index) {
        if (indentContinuations
            && source->symbolLine(index) > 1
            && source->firstSymbolOfLine(source->symbolLine(index)) == index
            && isLeadingOperator(source->token(index))
        ) {
            const int line = source->symbolLine(index);
            do {
                const int first = index;
                if (first < 1
                    || ! isLeadingOperator(source->token(first))
                    || source->isDirectiveLine(line)
                    || source->symbolEndLine(first - 1) >= line
                ) {
                    break;
                }
                const Token before = source->token(first - 1);
                if (before == SEMIC
                    || before == LBRACE
                    || before == RBRACE
                    || before == COLON
                    || before == COMMA
                    || before == EQ
                    || before == LPAREN
                    || before == RETURN
                    || isLeadingOperator(before)
                ) {
                    break;
                }
                int base = -1;
                bool keepsIndent = false;
                const int group =
                    continuationGroup(source, first, &base, &keepsIndent);
                if (keepsIndent || base < 1 || base >= line) {
                    break;
                }
                const Token opener = group >= 1 ? source->token(group - 1)
                                                : NOTOKEN;
                const Token leading = source->token(first);
                const bool isCondition = opener == IF
                    || opener == WHILE
                    || (opener == FOR
                        && isForCondition(source, group, first));
                const bool isOperand = isCondition
                    && leading != ANDAND
                    && leading != OROR;
                const QByteArray baseIndent =
                    indentOverrides.contains(base)
                    ? indentOverrides.value(base) : source->indentOf(base);
                QByteArray indent = baseIndent
                    + continuationPad(0, isOperand ? 2 : 1);

                const int close = group >= 0 ? source->match(group) : -1;
                const Token afterGroup = close >= 0
                    ? source->token(close + 1)
                    : NOTOKEN;
                bool nestedTernaryGroup = false;
                if (group >= 0
                    && source->token(group) == LPAREN
                    && ! isCondition
                    && source->symbolLine(group) == base
                    && (afterGroup == ANDAND
                        || afterGroup == OROR)
                ) {
                    const int head = source->headStart(close + 1);
                    const int end = statementEnd(source, close + 1);
                    int outerQuestion = -1;
                    int outerColon = -1;
                    nestedTernaryGroup = end > close + 1 && findTernary(
                            source,
                            head,
                            end - 1,
                            &outerQuestion,
                            &outerColon)
                        && close < outerQuestion;
                }
                if (group >= 0
                    && source->token(group) == LPAREN
                    && ! isCondition
                    && source->symbolLine(group) == base
                    && (afterGroup == DOT
                        || afterGroup == ARROW
                        || nestedTernaryGroup)
                ) {
                    indent = baseIndent + continuationPad(0, 2);
                }
                const int steps = conditionSteps(
                    source,
                    group,
                    base,
                    first
                );
                if (steps > 0) {
                    indent = baseIndent + continuationPad(0, steps);
                }
                const int rank = leadRank(source->token(first));
                for (int earlier = line - 1; earlier > base; --earlier) {
                    const int lead = source->firstSymbolOfLine(earlier);
                    int earlierBase = -1;
                    bool earlierKeeps = false;
                    if (lead < 0
                        || ! isLeadingOperator(source->token(lead))
                        || continuationGroup(
                            source,
                            lead,
                            &earlierBase,
                            &earlierKeeps)
                            != group
                        || earlierBase != base
                    ) {
                        continue;
                    }
                    const int earlierRank = leadRank(source->token(lead));
                    const QByteArray earlierIndent =
                        indentOverrides.contains(earlier)
                        ? indentOverrides.value(earlier)
                        : source->indentOf(earlier);
                    if (earlierRank < rank) {
                        indent = earlierIndent + continuationPad(0);
                        break;
                    }
                    if (earlierRank == rank) {
                        indent = earlierIndent;
                        break;
                    }
                }
                if (leading == QUESTION) {
                    const int head = source->headStart(first);
                    const int end = statementEnd(source, first);
                    int outerQuestion = -1;
                    int outerColon = -1;
                    if (end > first
                        && findTernary(
                            source,
                            head,
                            end - 1,
                            &outerQuestion,
                            &outerColon)
                        && outerQuestion < first
                    ) {
                        const int headLine = source->symbolLine(head);
                        const QByteArray hIndent =
                            indentOverrides.contains(headLine)
                            ? indentOverrides.value(headLine)
                            : source->indentOf(headLine);
                        indent = hIndent + continuationPad(0, 2);
                    }
                }
                const QByteArray current = indentOverrides.contains(line)
                    ? indentOverrides.value(line) : source->indentOf(line);
                if (current != indent) {
                    indentOverrides.insert(line, indent);
                    QHash<int, int > subShift;
                    QList<LineShiftRecord > growths;
                    LineShiftRecord rec;
                    rec.index = first;
                    rec.column = current.size() - 1;
                    rec.delta = indent.size() - current.size();
                    growths << rec;
                    recordShiftContinuations(
                        source,
                        growths,
                        &subShift,
                        &indentOverrides
                    );
                    QHash<int, int >::const_iterator sit =
                        subShift.constBegin();
                    for (; sit != subShift.constEnd(); ++sit) {
                        const int other = sit.key();
                        const int delta = sit.value();
                        const QByteArray oc =
                            indentOverrides.contains(other)
                            ? indentOverrides.value(other)
                            : source->indentOf(other);
                        if (delta > 0) {
                            indentOverrides.insert(
                                other,
                                oc + QByteArray(delta, ' ')
                            );
                        } else if (delta < 0 && oc.size() + delta > 0) {
                            indentOverrides.insert(other, oc.mid(-delta));
                        }
                    }
                }
            } while (false);
        }

        if (expandBodies && index > 0 && source->token(index) == RBRACE) {
            const int open = source->match(index);
            if (open < 0 || ! closesBracketChain(source, index)) {
                continue;
            }
            FormatScopeConditions conditions(source);
            if ( ! conditions.holds(
                QString(QLL("structure.bodyBraces")),
                source->configAt(source->symbols().at(open).from).bodyBraces,
                open
            )) {
                continue;
            }
            const FormatDocument::Scope scope = source->braceScope(open);
            if (scope != FormatDocument::FunctionScope
                && scope != FormatDocument::ClassScope
                && scope != FormatDocument::EnumScope
                && scope != FormatDocument::NamespaceScope
            ) {
                continue;
            }
            const int from = source->symbols().at(open).from;
            const int to = source->symbolEnd(index);
            bool isPlain = ! source->hasProtectedCommentBetween(from, to)
                && ! source->spansDirective(from, to);
            for (int k = open; k <= index && isPlain; ++k) {
                isPlain = source->symbolLine(k) == source->symbolEndLine(k);
            }
            if ( ! isPlain) {
                continue;
            }
            const int openLine = source->symbolLine(open);
            const QByteArray indent = source->indentOf(openLine);
            const QByteArray bodyIndent =
                scope == FormatDocument::NamespaceScope
                ? indent : indent + continuationPad(0);
            QList<QByteArray > lines = source
                ->codeSlice(
                    source->symbolEnd(open),
                    source->symbols().at(index).from
                )
                .split('\n');
            const QByteArray first = lines.takeFirst().trimmed();
            if ( ! lines.isEmpty()) {
                lines.last() = source->indentOf(source->symbolLine(index))
                    + lines.last().trimmed();
                if (lines.last().trimmed().isEmpty()) {
                    lines.removeLast();
                }
            }
            int shallowest = -1;
            for (int l = 0; l < lines.size(); ++l) {
                const QByteArray & lineText = lines.at(l);
                int width = 0;
                while (width < lineText.size()
                    && (lineText.at(width) == ' '
                        || lineText.at(width) == '\t')
                ) {
                    ++width;
                }
                if (width < lineText.size()) {
                    shallowest = shallowest < 0 ? width
                                                : qMin(shallowest, width);
                }
            }
            const int shift = shallowest < 0
                ? 0 : qMax(0, bodyIndent.size() - shallowest);
            QByteArray text = "{";
            if ( ! first.isEmpty()) {
                text += "\n" + bodyIndent + first;
            }
            for (int l = 0; l < lines.size(); ++l) {
                const QByteArray & lineText = lines.at(l);
                const QByteArray pad = lineText.trimmed().isEmpty()
                    ? QByteArray() : QByteArray(shift, ' ');
                text += "\n" + pad + lineText;
            }
            text += "\n" + indent + "}";
            addSingleSpan(source, from, to, text, out, true);
        }
    }

    QHash<int, QByteArray >::const_iterator iit = indentOverrides.constBegin();
    for (; iit != indentOverrides.constEnd(); ++iit) {
        const int line = iit.key();
        const QByteArray oldIndent = source->indentOf(line);
        if (iit.value() != oldIndent) {
            const int ls = source->lineStart(line);
            addSingleSpan(
                source,
                ls,
                ls + oldIndent.size(),
                iit.value(),
                out
            );
        }
    }
}

void collectNotSpace(const FormatDocument *source, SpanList *out)
{
    for (int index = 0; index < source->symbols().size(); ++index) {
        if (source->token(index) != NOT
            || source->isDirectiveLine(source->symbolLine(index))
            || (index > 0
                && source->token(index - 1) == OPERATOR)
        ) {
            continue;
        }
        const int notStart = source->symbols().at(index).from;
        const int notEnd = source->symbolEnd(index);
        const int before = index > 0 ? source->symbolEnd(index - 1) : 0;
        const int after = index + 1 < source->symbols().size()
            ? source->symbols().at(index + 1).from : notEnd;
        if (index == 0 || source->token(index - 1) != NOT) {
            if (notStart > 0
                && source->codeSlice(before, notStart).isEmpty()
            ) {
                addSingleSpan(source, before, notStart, " ", out, true);
            }
        }
        if (index + 1 < source->symbols().size()
            && source->token(index + 1) != NOT
        ) {
            if (source->codeSlice(notEnd, after).isEmpty()) {
                addSingleSpan(source, notEnd, after, " ", out, true);
            }
        }
    }
}

void collectNamespaceLabels(const FormatDocument *source,
                            const AstNode *root,
                            AstTransformState *state)
{
    if ( ! root) {
        return;
    }
    for (int i = 0; i < root->children.size(); ++i) {
        const AstNode *child = root->children.at(i);
        if (child->kind == AstNode::NamespaceDecl) {
            const int open = child->openToken;
            const int close = child->closeToken;
            if (open >= 0
                && close > open
                && ! source->spansDirective(
                    source->symbols().at(open).from,
                    source->symbolEnd(close))
            ) {
                const int closeLine = source->symbolLine(close);
                const bool hasNext = close + 1 < source->symbols().size()
                    && source->symbolLine(close + 1) == closeLine;
                if ( ! hasNext
                    && source->isSpanEditable(source->symbolEnd(close), 0)
                ) {
                    const int from = source->symbolEnd(close);
                    const int to = source->lineEnd(closeLine);
                    const QByteArray rest = source->text().mid(
                        from,
                        to - from
                    );
                    const QByteArray comment = rest.trimmed();
                    if (comment.isEmpty() || comment.startsWith("//")) {
                        const QByteArray name = child->name;
                        const int scope = name.lastIndexOf("::");
                        const QByteArray lastName = scope < 0
                            ? name : name.mid(scope + 2);
                        const QByteArray above = closeLine > 1
                            ? source->lineText(closeLine - 1).trimmed()
                            : QByteArray();
                        const bool namesAfter = ! comment.isEmpty()
                            && ! isNamespaceLabel(comment)
                            && commentNamesNamespace(comment, lastName);
                        const bool namesAbove = comment.isEmpty()
                            && source->firstSymbolOfLine(closeLine) == close
                            && above.startsWith("//")
                            && commentNamesNamespace(above, lastName);
                        if (name.isEmpty()
                            || ( ! namesAfter
                                && ! namesAbove)
                        ) {
                            QByteArray wanted;
                            if ( ! name.isEmpty()) {
                                wanted = " // Namespace `" + name + "`.";
                            } else if ( ! comment.isEmpty()) {
                                wanted = " " + comment;
                            }
                            if (rest != wanted) {
                                state->namespaceLabels.insert(
                                    child,
                                    wanted
                                );
                                state->namespaceLabelReplaceEnd.insert(
                                    child,
                                    to
                                );
                            }
                        }
                    }
                }
            }
            if (child->body) {
                collectNamespaceLabels(source, child->body, state);
            }
        }
    }
}

} // namespace

/*!
    @class AstTransforms
    @internal

    Applies structural AST transforms (`needsBraces`, verbatim selection flags,
    and namespace labels) and collects multi-rule diagnostics and formatting
    overrides before lowering into the output layout IR.
*/
void AstTransforms::apply(
        AstNode *root,
        QVector<AstComment * > *comments,
        const QByteArray & source,
        const Symbols & symbols,
        const QVector<QPair<int, int > > & disabledRanges,
        const QVector<QPair<int, int > > & lineRanges,
        const QVector<int > & lineStarts,
        const Options & options)
{
    transformNodeBasic(
        root,
        comments,
        source,
        symbols,
        disabledRanges,
        lineRanges,
        lineStarts,
        options
    );
}

/*!
    Runs all enabled code diagnostics and formatting transforms over
    @a document and its syntax tree @a root according to @a options, returning
    the resulting transform state for `AstPrinter::buildDoc`.
*/
AstTransformState AstTransforms::runDiagnosticsAndTransforms(
        FormatDocument *document, AstNode *root, const Options & options)
{
    AstTransformState state;
    if ( ! document) {
        return state;
    }

    TransformContext ctx(document);

    // Phase 1a: expression line joining, relative includes,
    // operator-first, moc macro.
    if (options.joinExpressions
        || options.adjustIncludes
        || options.moveOperators
        || options.joinMembers
        || options.fixMocMacro
    ) {
        if (options.joinExpressions) {
            SpanList joinSpans;
            collectLineJoin(ctx.doc, &joinSpans);
            ctx.commitPhase(joinSpans, true);
        }
        SpanList phase1aSpans;
        if (options.adjustIncludes || options.fixMocMacro) {
            collectRelativeIncludesAndMoc(
                ctx.doc,
                options.adjustIncludes,
                options.fixMocMacro,
                &phase1aSpans
            );
        }
        if (options.moveOperators || options.joinMembers) {
            collectMoveOperators(
                ctx.doc,
                options.moveOperators,
                options.joinMembers,
                &phase1aSpans
            );
        }
        ctx.commitPhase(phase1aSpans, true);
    }

    // Phase 1b: configured type/macro call replacements (`replace-types`).
    if (options.replaceTypes) {
        SpanList typeSpans;
        collectReplaceTypes(ctx.doc, &typeSpans);
        ctx.commitPhase(typeSpans, true);
    }

    // Phase 1c: template closers, early diagnostics, super typedefs,
    // namespaces, inline keywords, initializer-list leads, and
    // control-flow braces.
    if (options.spaceTemplateClosers) {
        SpanList tplSpans;
        collectTemplateCloseSpace(ctx.doc, &tplSpans);
        ctx.commitPhase(tplSpans, true);
    }

    if (options.checkMemberInitializers
        || options.checkLambdas
        || options.checkConstOverloads
        || options.checkGuardedGetters
        || options.checkCodeShapes
    ) {
        runEarlyDiagnostics(ctx.doc, options);
    }

    if (options.manageSuper) {
        SpanList typedefSpans;
        collectSuperTypedefs(ctx.doc, &typedefSpans);
        ctx.commitPhase(typedefSpans, true);

        SpanList refSpans;
        collectSuperReferences(ctx.doc, &refSpans);
        ctx.commitPhase(refSpans, true);
    }

    if (options.formatNamespaces || options.inlineMethods) {
        SpanList structSpans;
        if (options.formatNamespaces) {
            collectNamespaceBraces(ctx.doc, &structSpans);
        }
        if (options.inlineMethods) {
            collectInlineKeywords(ctx.doc, &structSpans);
        }
        ctx.commitPhase(structSpans, true);
    }

    if (options.leadInitializers) {
        SpanList initMoveSpans;
        collectLeadInitializers(ctx.doc, false, &initMoveSpans);
        ctx.commitPhase(initMoveSpans, true);

        SpanList initAlignSpans;
        collectLeadInitializers(ctx.doc, true, &initAlignSpans);
        ctx.commitPhase(initAlignSpans, true);
    }

    if (options.braceControls) {
        SpanList inlineExpandSpans;
        collectControlBraces(ctx.doc, true, false, &inlineExpandSpans);
        ctx.commitPhase(inlineExpandSpans, true);

        SpanList controlBraceSpans;
        collectControlBraces(ctx.doc, false, false, &controlBraceSpans);
        ctx.commitPhase(controlBraceSpans, true);

        SpanList elseJoinSpans;
        collectControlBraces(ctx.doc, false, true, &elseJoinSpans);
        ctx.commitPhase(elseJoinSpans, true);
    }

    // Phase 2: wrapping layouts.
    if (options.wrapOperators) {
        SpanList opWrapSpans;
        collectWrapOperators(ctx.doc, &opWrapSpans);
        ctx.commitPhase(opWrapSpans, true);
    }
    if (options.wrapTernaries) {
        SpanList ternSpans;
        collectWrapTernaries(ctx.doc, &ternSpans);
        ctx.commitPhase(ternSpans, true);
    }
    if (options.layoutConnections) {
        SpanList connSpans;
        collectConnectLayout(ctx.doc, &connSpans);
        ctx.commitPhase(connSpans, true);
    }
    if (options.wrapParameters) {
        SpanList paramSpans;
        collectWrapParameters(ctx.doc, &paramSpans);
        ctx.commitPhase(paramSpans, true);
    }
    if (options.wrapConditions) {
        SpanList condSpans;
        collectWrapConditions(ctx.doc, &condSpans);
        ctx.commitPhase(condSpans, true);
    }
    if (options.wrapArguments) {
        for (int argPass = 0; argPass < 4; ++argPass) {
            SpanList argSpans;
            bool hadPreBreak = false;
            collectWrapArgumentsPass(
                ctx.doc,
                argPass == 0,
                &argSpans,
                &hadPreBreak
            );
            if ( ! ctx.commitPhase(argSpans, true)) {
                break;
            }
            Q_UNUSED(hadPreBreak);
        }
    }
    if (options.moveOperators || options.joinMembers) {
        SpanList postWrapMoveSpans;
        collectMoveOperators(
            ctx.doc,
            options.moveOperators,
            options.joinMembers,
            &postWrapMoveSpans
        );
        ctx.commitPhase(postWrapMoveSpans, true);
    }

    // Phase 3: fine spacing, late diagnostics, one-statement, outline-this,
    // continuation indent, body braces, not-space, and namespace labels.
    if (options.closeConditions) {
        SpanList closeSpans;
        collectCloseConditions(ctx.doc, &closeSpans);
        ctx.commitPhase(closeSpans, true);
    }
    if (options.spaceReferences) {
        QSet<int > regexpLinesDone;
        SpanList regexpSpans;
        collectSpaceReferences(
            ctx.doc,
            true,
            &regexpLinesDone,
            &regexpSpans
        );
        ctx.commitPhase(regexpSpans, true);

        SpanList refSpans;
        collectSpaceReferences(ctx.doc, false, &regexpLinesDone, &refSpans);
        ctx.commitPhase(refSpans, true);
    }

    if (options.checkStaticVariables
        || options.checkReferenceLocal
        || options.checkPrivateAccess
        || options.checkExcludedFlags
    ) {
        runLateDiagnostics(ctx.doc, options);
    }

    if (options.splitStatements) {
        SpanList splitSpans;
        collectSplitStatements(ctx.doc, &splitSpans);
        ctx.commitPhase(splitSpans, true);
    }

    // Builds the final AST once if an earlier phase materialized a new buffer.
    if (ctx.ownedDoc) {
        state.ownedDocument = ctx.ownedDoc;
        state.finalDocument = ctx.ownedDoc.data();
        state.ownedArena.reset(new AstArena);
        const AstParseResult rebuilt = state.finalDocument->buildAst(
            state.ownedArena.data()
        );
        state.finalRoot = rebuilt.root;
    } else {
        state.finalDocument = document;
        state.finalRoot = root;
    }

    QHash<int, int > pendingLineShift;
    SpanList finalSpans;
    if (options.qualifyMembers) {
        collectQualifyMembers(state.finalDocument, &finalSpans, &pendingLineShift);
    }
    if (options.indentContinuations
        || options.expandBodies
        || ! pendingLineShift.isEmpty()
    ) {
        collectContinuationIndentAndExpandBodies(
            state.finalDocument,
            options.indentContinuations,
            options.expandBodies,
            pendingLineShift,
            &finalSpans
        );
    }
    if (options.spaceLogicalNot) {
        collectNotSpace(state.finalDocument, &finalSpans);
    }
    state.spanRewrites = filterNonOverlapping(
        state.finalDocument,
        finalSpans
    );

    if (options.formatNamespaces || options.labelNamespaces) {
        collectNamespaceLabels(state.finalDocument, state.finalRoot, &state);
    }

    // Computes final line ranges, issues, and requests back onto `document`.
    state.finalLineRanges = ctx.lineRanges;
    QList<FormatIssue > finalIssues = state.finalDocument->issues();
    QList<FormatIssue > finalRequests = state.finalDocument->requests();
    if ( ! state.spanRewrites.isEmpty()) {
        shiftRangesAndIssues(
            state.finalDocument,
            state.spanRewrites,
            &state.finalLineRanges,
            &finalIssues,
            &finalRequests
        );
    }
    document->setLineRanges(state.finalLineRanges);
    document->setIssues(finalIssues);
    document->setRequests(finalRequests);
    return state;
}
