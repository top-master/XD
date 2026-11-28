// Copyright (C) 2015 The XD Company Ltd.
// SPDX-License-Identifier: Apache-2.0

#include "./ast-printer.h"
#include "../format-document.h"

#include <QtCore/QtAlgorithms>

namespace {

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

bool hasBlankLineBetween(const QByteArray & source,
                         int fromOffset,
                         int toOffset)
{
    int newlines = 0;
    const int safeFrom = qMax(0, fromOffset);
    const int safeTo = qMin(toOffset, source.size());
    for (int i = safeFrom; i < safeTo; ++i) {
        if (source.at(i) == '\n') {
            ++newlines;
            if (newlines >= 2) {
                return true;
            }
        }
    }
    return false;
}

class AstDocBuilder
{
public:

    AstDocBuilder(
            const QByteArray & source,
            const Symbols & symbols,
            const QVector<int > & lineStarts,
            const FormatFileConfig & config,
            DocArena *arena)
        : m_source(source)
        , m_symbols(symbols)
        , m_lineStarts(lineStarts)
        , m_config(config)
        , m_arena(arena)
    {
    }

    const DocNode *build(const AstNode *node)
    {
        if ( ! node) {
            return m_arena->text(QByteArray());
        }
        const DocNode *core = this->buildCore(node);
        return this->wrapComments(node, core);
    }

private:

    const DocNode *printCommentNode(const AstComment *comment)
    {
        comment->printed = true;
        const QByteArray & bytes = comment->text;
        if (bytes.contains('\n')) {
            return m_arena->verbatim(bytes);
        }
        return m_arena->text(bytes);
    }

    const DocNode *wrapComments(const AstNode *node, const DocNode *core)
    {
        if (node->comments.isEmpty()) {
            return core;
        }

        QVector<const DocNode * > leading;
        QVector<const DocNode * > trailing;
        const AstComment *prevLeading = Q_NULLPTR;

        for (int i = 0; i < node->comments.size(); ++i) {
            const AstComment *c = node->comments.at(i);
            if (c->printed) {
                continue;
            }
            if (c->placement == AstComment::Leading) {
                if (prevLeading
                    && hasBlankLineBetween(
                        m_source,
                        prevLeading->endOffset(),
                        c->offset)
                ) {
                    leading.append(m_arena->hardline());
                }
                leading.append(this->printCommentNode(c));
                leading.append(m_arena->hardline());
                prevLeading = c;
            } else if (c->placement == AstComment::Trailing) {
                if (c->trailingHint) {
                    trailing.append(
                        m_arena->lineSuffix(
                            m_arena->concat(
                                m_arena->text(" "),
                                this->printCommentNode(c)
                            )
                        )
                    );
                } else {
                    trailing.append(
                        m_arena->concat(
                            m_arena->text(" "),
                            this->printCommentNode(c)
                        )
                    );
                }
            }
        }

        QVector<const DocNode * > parts;
        parts.reserve(leading.size() + 1 + trailing.size());
        parts += leading;
        parts.append(core);
        parts += trailing;
        return m_arena->concat(parts);
    }

    const DocNode *printDanglingComments(const AstNode *container)
    {
        QVector<const DocNode * > parts;
        const AstComment *prev = Q_NULLPTR;
        for (int i = 0; i < container->comments.size(); ++i) {
            const AstComment *c = container->comments.at(i);
            if (c->placement != AstComment::Dangling || c->printed) {
                continue;
            }
            if (prev
                && hasBlankLineBetween(
                    m_source,
                    prev->endOffset(),
                    c->offset)
            ) {
                parts.append(m_arena->hardline());
            }
            parts.append(this->printCommentNode(c));
            parts.append(m_arena->hardline());
            prev = c;
        }
        return m_arena->concat(parts);
    }

    const DocNode *buildSequence(
            const QVector<AstNode * > & nodes, const AstNode *container)
    {
        if (nodes.isEmpty()) {
            return this->printDanglingComments(container);
        }
        QVector<const DocNode * > parts;
        parts.reserve(nodes.size() * 2);
        const AstNode *prev = Q_NULLPTR;
        for (int i = 0; i < nodes.size(); ++i) {
            const AstNode *child = nodes.at(i);
            if (i > 0) {
                parts.append(m_arena->hardline());
                if (prev
                    && hasBlankLineBetween(
                        m_source,
                        prev->endOffset,
                        child->startOffset)
                ) {
                    parts.append(m_arena->hardline());
                }
            }
            parts.append(this->build(child));
            prev = child;
        }
        return m_arena->concat(parts);
    }

    QByteArray joinedTokenSlice(int firstTok, int lastTok) const
    {
        if (firstTok < 0
            || lastTok < firstTok
            || lastTok >= m_symbols.size()
        ) {
            return QByteArray();
        }
        const int start = m_symbols.at(firstTok).from;
        const int end = m_symbols.at(lastTok).from + m_symbols.at(lastTok).len;
        return m_source.mid(start, end - start);
    }

    const DocNode *buildCore(const AstNode *node)
    {
        if (node->preserveVerbatim
            && node->kind != AstNode::TranslationUnit
        ) {
            return m_arena->verbatim(
                m_source.mid(
                    node->startOffset,
                    node->endOffset - node->startOffset
                )
            );
        }

        switch (node->kind) {
        case AstNode::TranslationUnit: {
            const DocNode *seq = this->buildSequence(node->children, node);
            return m_arena->concat(seq, m_arena->hardline());
        }
        case AstNode::CompoundStmt: {
            const DocNode *inner = node->children.isEmpty()
                ? this->printDanglingComments(node)
                : this->buildSequence(node->children, node);
            if (node->children.isEmpty() && node->comments.isEmpty()) {
                return m_arena->text("{}");
            }
            return m_arena->concat(
                m_arena->text("{"),
                m_arena->indent(m_arena->concat(m_arena->hardline(), inner)),
                m_arena->hardline(),
                m_arena->text("}")
            );
        }
        case AstNode::NamespaceDecl: {
            const QByteArray header = node->name.isEmpty()
                ? QByteArray("namespace")
                : QByteArray("namespace ") + node->name;
            const DocNode *body = node->body
                ? this->buildSequence(node->body->children, node->body)
                : this->printDanglingComments(node);
            QVector<const DocNode * > parts;
            parts.append(m_arena->text(header));
            parts.append(m_arena->hardline());
            parts.append(m_arena->text("{"));
            if (body && body != m_arena->text(QByteArray())) {
                parts.append(m_arena->hardline());
                parts.append(body);
            }
            parts.append(m_arena->hardline());
            parts.append(m_arena->text("}"));
            if ( ! node->transformedText.isEmpty()) {
                parts.append(
                    m_arena->lineSuffix(
                        m_arena->text(" " + node->transformedText)
                    )
                );
            }
            return m_arena->concat(parts);
        }
        case AstNode::ClassDecl: {
            if (node->isForward || ! node->body) {
                return m_arena->verbatim(
                    m_source.mid(
                        node->startOffset,
                        node->endOffset - node->startOffset
                    )
                );
            }
            const QByteArray header = m_source
                .mid(
                    node->startOffset,
                    m_symbols.at(node->openToken).from - node->startOffset
                )
                .trimmed();
            const DocNode *inner = node->body->children.isEmpty()
                ? this->printDanglingComments(node->body)
                : this->buildSequence(node->body->children, node->body);
            return m_arena->concat(
                m_arena->text(header),
                m_arena->hardline(),
                m_arena->text("{"),
                m_arena->indent(m_arena->concat(m_arena->hardline(), inner)),
                m_arena->hardline(),
                m_arena->text("};")
            );
        }
        case AstNode::FunctionDecl: {
            if ( ! node->body) {
                const QByteArray raw = m_source
                    .mid(
                        node->startOffset,
                        node->endOffset - node->startOffset
                    )
                    .trimmed();
                return m_arena->text(raw);
            }
            const int bodyFrom = m_symbols.at(node->body->firstToken).from;
            const QByteArray header =
                m_source.mid(
                    node->startOffset,
                    bodyFrom - node->startOffset
                )
                .trimmed();
            return m_arena->concat(
                m_arena->text(header),
                m_arena->hardline(),
                this->build(node->body)
            );
        }
        case AstNode::TemplateDecl: {
            if ( ! node->inner) {
                return m_arena->verbatim(
                    m_source.mid(
                        node->startOffset,
                        node->endOffset - node->startOffset
                    )
                );
            }
            const int innerFrom = node->inner->startOffset;
            const QByteArray prefix =
                m_source.mid(
                    node->startOffset,
                    innerFrom - node->startOffset
                )
                .trimmed();
            return m_arena->concat(
                m_arena->text(prefix),
                m_arena->hardline(),
                this->build(node->inner)
            );
        }
        case AstNode::LinkageSpec: {
            if ( ! node->body) {
                return m_arena->verbatim(
                    m_source.mid(
                        node->startOffset,
                        node->endOffset - node->startOffset
                    )
                );
            }
            return m_arena->concat(
                m_arena->text("extern " + node->name + " "),
                this->build(node->body)
            );
        }
        case AstNode::IfStmt: {
            const DocNode *condDoc = node->cond ? this->build(node->cond)
                                                : m_arena->text(QByteArray());
            const DocNode *bodyDoc = node->body ? this->build(node->body)
                                                : m_arena->text(";");
            if (node->needsBraces && node->body && ! node->body->isBlock) {
                bodyDoc = m_arena->concat(
                    m_arena->text("{"),
                    m_arena->indent(
                        m_arena->concat(m_arena->hardline(), bodyDoc)
                    ),
                    m_arena->hardline(),
                    m_arena->text("}")
                );
            }
            QVector<const DocNode * > parts;
            parts.append(
                m_arena->group(
                    m_arena->concat(
                        m_arena->text("if ("),
                        m_arena->indent(
                            m_arena->concat(m_arena->softline(), condDoc)
                        ),
                        m_arena->softline(),
                        m_arena->text(")")
                    )
                )
            );
            if (node->body && (node->body->isBlock || node->needsBraces)) {
                parts.append(m_arena->text(" "));
                parts.append(bodyDoc);
            } else {
                parts.append(
                    m_arena->indent(
                        m_arena->concat(m_arena->hardline(), bodyDoc)
                    )
                );
            }
            if (node->elseBranch) {
                parts.append(m_arena->text(" else "));
                parts.append(this->build(node->elseBranch));
            }
            return m_arena->concat(parts);
        }
        case AstNode::ForStmt:
        case AstNode::WhileStmt:
        case AstNode::SwitchStmt: {
            if (node->isDoWhile || ! node->body) {
                return m_arena->verbatim(
                    m_source.mid(
                        node->startOffset,
                        node->endOffset - node->startOffset
                    )
                );
            }
            const QByteArray kw = node->kind == AstNode::ForStmt
                ? QByteArray("for (")
                : (node->kind == AstNode::SwitchStmt ? QByteArray("switch (") : QByteArray("while ("));
            const DocNode *condDoc = node->cond ? this->build(node->cond)
                                                : m_arena->text(QByteArray());
            const DocNode *bodyDoc = this->build(node->body);
            if (node->needsBraces && ! node->body->isBlock) {
                bodyDoc = m_arena->concat(
                    m_arena->text("{"),
                    m_arena->indent(
                        m_arena->concat(m_arena->hardline(), bodyDoc)
                    ),
                    m_arena->hardline(),
                    m_arena->text("}")
                );
            }
            QVector<const DocNode * > parts;
            parts.append(
                m_arena->group(
                    m_arena->concat(
                        m_arena->text(kw),
                        m_arena->indent(
                            m_arena->concat(m_arena->softline(), condDoc)
                        ),
                        m_arena->softline(),
                        m_arena->text(")")
                    )
                )
            );
            if (node->body->isBlock || node->needsBraces) {
                parts.append(m_arena->text(" "));
                parts.append(bodyDoc);
            } else {
                parts.append(
                    m_arena->indent(
                        m_arena->concat(m_arena->hardline(), bodyDoc)
                    )
                );
            }
            return m_arena->concat(parts);
        }
        case AstNode::ExprStmt: {
            if ( ! node->inner) {
                return m_arena->text(";");
            }
            return m_arena->concat(
                this->build(node->inner),
                m_arena->text(";")
            );
        }
        case AstNode::ReturnStmt: {
            if ( ! node->inner) {
                return m_arena->text("return;");
            }
            return m_arena->concat(
                m_arena->text("return "),
                this->build(node->inner),
                m_arena->text(";")
            );
        }
        case AstNode::BinaryExpr: {
            return m_arena->group(
                m_arena->concat(
                    this->build(node->lhs),
                    m_arena->line(),
                    m_arena->text(node->opText + " "),
                    this->build(node->rhs)
                )
            );
        }
        case AstNode::TernaryExpr: {
            return m_arena->group(
                m_arena->concat(
                    this->build(node->cond),
                    m_arena->indent(
                        m_arena->concat(
                            m_arena->line(),
                            m_arena->text("? "),
                            this->build(node->body),
                            m_arena->line(),
                            m_arena->text(": "),
                            this->build(node->elseBranch)
                        )
                    )
                )
            );
        }
        case AstNode::CallExpr: {
            QVector<const DocNode * > argDocs;
            argDocs.reserve(node->items.size());
            for (int i = 0; i < node->items.size(); ++i) {
                argDocs.append(this->build(node->items.at(i)));
            }
            const DocNode *sep = m_arena->concat(
                m_arena->text(","),
                m_arena->line()
            );
            return m_arena->group(
                m_arena->concat(
                    this->build(node->lhs),
                    m_arena->text("("),
                    m_arena->indent(
                        m_arena->concat(
                            m_arena->softline(),
                            m_arena->join(sep, argDocs)
                        )
                    ),
                    m_arena->softline(),
                    m_arena->text(")")
                )
            );
        }
        case AstNode::MemberExpr: {
            return m_arena->group(
                m_arena->concat(
                    this->build(node->lhs),
                    m_arena->softline(),
                    m_arena->text(node->opText),
                    this->build(node->rhs)
                )
            );
        }
        case AstNode::UnaryExpr: {
            return m_arena->concat(
                m_arena->text(
                    this->joinedTokenSlice(node->opToken, node->opToken)
                ),
                node->opToken >= 0
                    && m_symbols.at(node->opToken).token == NOT
                    ? m_arena->text(" ") : m_arena->text(QByteArray()),
                this->build(node->inner)
            );
        }
        case AstNode::ParenExpr: {
            return m_arena->group(
                m_arena->concat(
                    m_arena->text("("),
                    m_arena->indent(
                        m_arena->concat(
                            m_arena->softline(),
                            this->build(node->inner)
                        )
                    ),
                    m_arena->softline(),
                    m_arena->text(")")
                )
            );
        }
        default: {
            const QByteArray raw = node->firstToken >= 0
                ? this->joinedTokenSlice(node->firstToken, node->lastToken)
                : m_source.mid(
                    node->startOffset,
                    node->endOffset - node->startOffset
                );
            return m_arena->text(raw);
        }
        }
    }

    const QByteArray & m_source;
    const Symbols & m_symbols;
    const QVector<int > & m_lineStarts;
    const FormatFileConfig & m_config;
    DocArena *m_arena;
};

bool nodeStartLessThan(const AstNode *a, const AstNode *b)
{
    if (a->startOffset != b->startOffset) {
        return a->startOffset < b->startOffset;
    }
    return a->endOffset > b->endOffset;
}

class TransformDocBuilder
{
public:

    TransformDocBuilder(const AstTransformState & state,
                        const FormatDocument & document,
                        DocArena *arena)
        : m_state(state)
        , m_source(document.text())
        , m_arena(arena)
        , m_spanIndex(0)
        , m_cursor(0)
    {
    }

    const DocNode *build(const AstNode *root)
    {
        if ( ! root) {
            return m_arena->verbatim(m_source);
        }
        QVector<const DocNode * > parts;
        this->lowerNode(root, 0, m_source.size(), &parts);
        this->flushSpansAtEnd(&parts);
        return parts.size() == 1 ? parts.first() : m_arena->concat(parts);
    }

private:

    void advanceCursorTo(int limit, QVector<const DocNode * > *out)
    {
        const QVector<AstTransformState::SpanRewrite > & spans =
            m_state.spanRewrites;
        while (m_cursor < limit) {
            while (m_spanIndex < spans.size()
                && spans.at(m_spanIndex).from == m_cursor
                && spans.at(m_spanIndex).to == m_cursor
            ) {
                const QByteArray & bytes = spans.at(m_spanIndex).bytes;
                if ( ! bytes.isEmpty()) {
                    out->append(m_arena->verbatim(bytes));
                }
                ++m_spanIndex;
            }
            if (m_spanIndex < spans.size()
                && spans.at(m_spanIndex).from == m_cursor
                && spans.at(m_spanIndex).to > m_cursor
            ) {
                const QByteArray & bytes = spans.at(m_spanIndex).bytes;
                if ( ! bytes.isEmpty()) {
                    out->append(m_arena->verbatim(bytes));
                }
                m_cursor = spans.at(m_spanIndex).to;
                ++m_spanIndex;
                continue;
            }
            int nextStop = limit;
            if (m_spanIndex < spans.size()
                && spans.at(m_spanIndex).from < nextStop
            ) {
                nextStop = spans.at(m_spanIndex).from;
            }
            if (nextStop > m_cursor) {
                out->append(
                    m_arena->verbatim(
                        m_source.mid(m_cursor, nextStop - m_cursor)
                    )
                );
                m_cursor = nextStop;
            }
        }
        while (m_spanIndex < spans.size()
            && spans.at(m_spanIndex).from == m_cursor
            && spans.at(m_spanIndex).to == m_cursor
        ) {
            const QByteArray & bytes = spans.at(m_spanIndex).bytes;
            if ( ! bytes.isEmpty()) {
                out->append(m_arena->verbatim(bytes));
            }
            ++m_spanIndex;
        }
    }

    void flushSpansAtEnd(QVector<const DocNode * > *out)
    {
        this->advanceCursorTo(m_source.size(), out);
    }

    QVector<const AstNode * > orderedChildren(
            const AstNode *node, int rangeStart, int rangeEnd) const
    {
        QVector<const AstNode * > children;
        const AstNode *singles[] =
            {
                node->cond,
                node->body,
                node->elseBranch,
                node->lhs,
                node->rhs,
                node->inner
            };
        for (int i = 0; i < 6; ++i) {
            const AstNode *c = singles[i];
            if (c
                && c->startOffset >= rangeStart
                && c->endOffset <= rangeEnd
            ) {
                children.append(c);
            }
        }
        for (int i = 0; i < node->children.size(); ++i) {
            const AstNode *c = node->children.at(i);
            if (c
                && c->startOffset >= rangeStart
                && c->endOffset <= rangeEnd
            ) {
                children.append(c);
            }
        }
        for (int i = 0; i < node->items.size(); ++i) {
            const AstNode *c = node->items.at(i);
            if (c
                && c->startOffset >= rangeStart
                && c->endOffset <= rangeEnd
            ) {
                children.append(c);
            }
        }
        for (int i = 0; i < node->initializers.size(); ++i) {
            const AstNode *c = node->initializers.at(i);
            if (c
                && c->startOffset >= rangeStart
                && c->endOffset <= rangeEnd
            ) {
                children.append(c);
            }
        }
        qSort(children.begin(), children.end(), nodeStartLessThan);
        return children;
    }

    void lowerNode(
            const AstNode *node,
            int rangeStart,
            int rangeEnd,
            QVector<const DocNode * > *out)
    {
        const QVector<const AstNode * > children = this->orderedChildren(
            node,
            rangeStart,
            rangeEnd
        );

        QVector<const DocNode * > nodeParts;
        for (int i = 0; i < children.size(); ++i) {
            const AstNode *child = children.at(i);
            if (child->endOffset <= m_cursor) {
                continue;
            }
            if (child->startOffset > m_cursor) {
                this->advanceCursorTo(child->startOffset, &nodeParts);
            }
            if (child->endOffset <= m_cursor) {
                continue;
            }
            const int childStart = qMax(child->startOffset, m_cursor);
            if (childStart == child->startOffset) {
                this->lowerNode(
                    child,
                    child->startOffset,
                    child->endOffset,
                    &nodeParts
                );
            } else {
                this->advanceCursorTo(child->endOffset, &nodeParts);
            }
        }

        if (m_cursor < rangeEnd) {
            this->advanceCursorTo(rangeEnd, &nodeParts);
        }

        if (node->kind == AstNode::NamespaceDecl
            && m_state.namespaceLabels.contains(node)
        ) {
            const QByteArray label = m_state.namespaceLabels.value(node);
            const int replaceEnd = m_state.namespaceLabelReplaceEnd.value(
                node,
                m_cursor
            );
            nodeParts.append(m_arena->text(label));
            if (replaceEnd > m_cursor) {
                m_cursor = replaceEnd;
                while (m_spanIndex < m_state.spanRewrites.size()
                    && m_state.spanRewrites.at(m_spanIndex).to <= m_cursor
                ) {
                    ++m_spanIndex;
                }
            }
        }

        if (nodeParts.size() == 1) {
            out->append(nodeParts.first());
        } else if ( ! nodeParts.isEmpty()) {
            out->append(m_arena->concat(nodeParts));
        }
    }

    const AstTransformState & m_state;
    const QByteArray & m_source;
    DocArena *m_arena;
    int m_spanIndex;
    int m_cursor;
};

} // namespace

/*!
    @class AstPrinter
    @internal

    Lowers a syntax tree (`AstNode`) and its attached comments into a layout IR
    (`DocNode`) and formats it in one pass via `DocPrinter`.
*/

/*!
    Converts @a root into a `DocNode` layout tree inside @a arena using
    @a source, @a symbols, @a lineStarts, and @a config.
*/
const DocNode *AstPrinter::buildDoc(
        const AstNode *root,
        const QByteArray & source,
        const Symbols & symbols,
        const QVector<int > & lineStarts,
        const FormatFileConfig & config,
        DocArena *arena)
{
    if ( ! root || ! arena) {
        return Q_NULLPTR;
    }
    AstDocBuilder builder(source, symbols, lineStarts, config, arena);
    return builder.build(root);
}

/*!
    Formats @a root in one layout-and-print pass and returns the result bytes.
*/
QByteArray AstPrinter::print(
        const AstNode *root,
        const QByteArray & source,
        const Symbols & symbols,
        const QVector<int > & lineStarts,
        const FormatFileConfig & config,
        const DocPrinter::Options & printerOptions)
{
    DocArena arena;
    const DocNode *doc = AstPrinter::buildDoc(
        root,
        source,
        symbols,
        lineStarts,
        config,
        &arena
    );
    return DocPrinter::print(doc, printerOptions);
}

/*!
    Converts @a root and transform state @a state for @a document into a
    `DocNode` layout tree inside @a arena.
*/
const DocNode *AstPrinter::buildDoc(
        const AstNode *root,
        const AstTransformState & state,
        const FormatDocument & document,
        DocArena *arena)
{
    if ( ! arena) {
        return Q_NULLPTR;
    }
    TransformDocBuilder builder(state, document, arena);
    return builder.build(root);
}

/*!
    Lowers @a root with @a state for @a document into Doc IR and prints it in
    one pass.
*/
QByteArray AstPrinter::print(
        const AstNode *root,
        const AstTransformState & state,
        const FormatDocument & document,
        const DocPrinter::Options & printerOptions)
{
    DocArena arena;
    const DocNode *doc = AstPrinter::buildDoc(
        root,
        state,
        document,
        &arena
    );
    return DocPrinter::print(doc, printerOptions);
}
