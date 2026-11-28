// Copyright (C) 2015 The XD Company Ltd.
// SPDX-License-Identifier: Apache-2.0

#include "./doc-node.h"

/*!
    @class DocArena
    @internal

    Allocates and owns immutable layout command nodes (`DocNode`) for one
    formatting run, reusing leaf singletons (`line`, `softline`, `breakParent`,
    `trim`, and empty text) so layout trees remain compact and leak-free.
*/

/*!
    Initializes an empty arena and pre-allocates reusable leaf commands.
*/
DocArena::DocArena()
{
    m_nodes.reserve(256);

    m_emptyText = this->allocNode(DocNode::Text);

    DocNode *lineNode = this->allocNode(DocNode::Line);
    m_lineNode = lineNode;

    DocNode *soft = this->allocNode(DocNode::Line);
    soft->soft = true;
    m_softlineNode = soft;

    DocNode *hardNoBreak = this->allocNode(DocNode::Line);
    hardNoBreak->hard = true;
    m_hardlineNoBreakNode = hardNoBreak;

    DocNode *literalNoBreak = this->allocNode(DocNode::Line);
    literalNoBreak->hard = true;
    literalNoBreak->literal = true;
    m_literalNoBreakNode = literalNoBreak;

    m_breakParentNode = this->allocNode(DocNode::BreakParent);
    m_lineSuffixBoundaryNode = this->allocNode(DocNode::LineSuffixBoundary);
    m_trimNode = this->allocNode(DocNode::Trim);
}

/*!
    Releases all `DocNode` instances owned by this arena.
*/
DocArena::~DocArena()
{
    qDeleteAll(m_nodes);
}

DocNode *DocArena::allocNode(DocNode::Kind kind)
{
    DocNode *node = new DocNode(kind);
    m_nodes.append(node);
    return node;
}

/*!
    Returns a literal text node holding @a bytes.
*/
const DocNode *DocArena::text(const QByteArray & bytes)
{
    if (bytes.isEmpty()) {
        return m_emptyText;
    }
    DocNode *node = this->allocNode(DocNode::Text);
    node->text = bytes;
    return node;
}

/*!
    Returns a verbatim span that prints @a bytes without re-indenting lines.
*/
const DocNode *DocArena::verbatim(const QByteArray & bytes)
{
    if (bytes.isEmpty()) {
        return m_emptyText;
    }
    DocNode *node = this->allocNode(DocNode::Verbatim);
    node->text = bytes;
    return node;
}

/*!
    Concatenates @a parts into one sequence node, skipping null or empty nodes.
*/
const DocNode *DocArena::concat(const QVector<const DocNode * > & parts)
{
    QVector<const DocNode * > filtered;
    filtered.reserve(parts.size());
    for (int i = 0; i < parts.size(); ++i) {
        const DocNode *part = parts.at(i);
        if ( ! part || part == m_emptyText) {
            continue;
        }
        if (part->kind == DocNode::Concat) {
            filtered += part->parts;
        } else {
            filtered.append(part);
        }
    }
    if (filtered.isEmpty()) {
        return m_emptyText;
    }
    if (filtered.size() == 1) {
        return filtered.at(0);
    }
    DocNode *node = this->allocNode(DocNode::Concat);
    node->parts = filtered;
    return node;
}

const DocNode *DocArena::concat(const DocNode *a, const DocNode *b)
{
    QVector<const DocNode * > parts;
    parts.reserve(2);
    parts.append(a);
    parts.append(b);
    return this->concat(parts);
}

const DocNode *DocArena::concat(
        const DocNode *a, const DocNode *b, const DocNode *c)
{
    QVector<const DocNode * > parts;
    parts.reserve(3);
    parts.append(a);
    parts.append(b);
    parts.append(c);
    return this->concat(parts);
}

const DocNode *DocArena::concat(
        const DocNode *a,
        const DocNode *b,
        const DocNode *c,
        const DocNode *d)
{
    QVector<const DocNode * > parts;
    parts.reserve(4);
    parts.append(a);
    parts.append(b);
    parts.append(c);
    parts.append(d);
    return this->concat(parts);
}

const DocNode *DocArena::concat(
        const DocNode *a,
        const DocNode *b,
        const DocNode *c,
        const DocNode *d,
        const DocNode *e)
{
    QVector<const DocNode * > parts;
    parts.reserve(5);
    parts.append(a);
    parts.append(b);
    parts.append(c);
    parts.append(d);
    parts.append(e);
    return this->concat(parts);
}

const DocNode *DocArena::concat(
        const DocNode *a,
        const DocNode *b,
        const DocNode *c,
        const DocNode *d,
        const DocNode *e,
        const DocNode *f)
{
    QVector<const DocNode * > parts;
    parts.reserve(6);
    parts.append(a);
    parts.append(b);
    parts.append(c);
    parts.append(d);
    parts.append(e);
    parts.append(f);
    return this->concat(parts);
}

/*!
    Increases indentation by one level around @a contents.
*/
const DocNode *DocArena::indent(const DocNode *contents)
{
    if ( ! contents || contents == m_emptyText) {
        return m_emptyText;
    }
    DocNode *node = this->allocNode(DocNode::Indent);
    node->contents = contents;
    return node;
}

/*!
    Aligns @a contents by @a spaces spaces relative to the enclosing indent.
*/
const DocNode *DocArena::alignWidth(int spaces, const DocNode *contents)
{
    if ( ! contents || contents == m_emptyText) {
        return m_emptyText;
    }
    if (spaces == 0) {
        return contents;
    }
    DocNode *node = this->allocNode(DocNode::Align);
    node->alignKind = spaces < 0 ? DocNode::AlignDedent : DocNode::AlignWidth;
    node->alignWidth = spaces;
    node->contents = contents;
    return node;
}

/*!
    Appends @a str literally to the indentation stack while
    printing @a contents.
*/
const DocNode *DocArena::alignString(
        const QByteArray & str, const DocNode *contents)
{
    if ( ! contents || contents == m_emptyText) {
        return m_emptyText;
    }
    if (str.isEmpty()) {
        return contents;
    }
    DocNode *node = this->allocNode(DocNode::Align);
    node->alignKind = DocNode::AlignString;
    node->text = str;
    node->contents = contents;
    return node;
}

/*!
    Decreases indentation by one level while printing @a contents.
*/
const DocNode *DocArena::dedent(const DocNode *contents)
{
    return this->alignWidth(-1, contents);
}

/*!
    Resets alignment back to the nearest marked root while printing @a contents.
*/
const DocNode *DocArena::dedentToRoot(const DocNode *contents)
{
    if ( ! contents || contents == m_emptyText) {
        return m_emptyText;
    }
    DocNode *node = this->allocNode(DocNode::Align);
    node->alignKind = DocNode::AlignResetRoot;
    node->contents = contents;
    return node;
}

/*!
    Marks the current indentation as the root anchor while printing @a contents.
*/
const DocNode *DocArena::markAsRoot(const DocNode *contents)
{
    if ( ! contents || contents == m_emptyText) {
        return m_emptyText;
    }
    DocNode *node = this->allocNode(DocNode::Align);
    node->alignKind = DocNode::AlignMarkRoot;
    node->contents = contents;
    return node;
}

/*!
    Tries @a contents flat on one line unless broken or forced via @a shouldBreak.
*/
const DocNode *DocArena::group(
        const DocNode *contents, bool shouldBreak, const QByteArray & id)
{
    DocNode *node = this->allocNode(DocNode::Group);
    node->contents = contents ? contents : m_emptyText;
    node->breakState = shouldBreak ? DocNode::BreakExplicit
                                   : DocNode::BreakNone;
    node->id = id;
    return node;
}

/*!
    Picks the first state in @a states that fits within print width,
    falling back to the final state when none fits flat.
*/
const DocNode *DocArena::conditionalGroup(
        const QVector<const DocNode * > & states, const QByteArray & id)
{
    if (states.isEmpty()) {
        return m_emptyText;
    }
    DocNode *node = this->allocNode(DocNode::Group);
    node->contents = states.at(0) ? states.at(0) : m_emptyText;
    node->expandedStates = states;
    node->id = id;
    return node;
}

/*!
    Packs alternating content and line-break @a parts greedily onto lines.
*/
const DocNode *DocArena::fill(const QVector<const DocNode * > & parts)
{
    if (parts.isEmpty()) {
        return m_emptyText;
    }
    DocNode *node = this->allocNode(DocNode::Fill);
    node->parts = parts;
    return node;
}

/*!
    Selects @a breakContents when the enclosing group (or group @a groupId)
    breaks, and @a flatContents when it prints flat.
*/
const DocNode *DocArena::ifBreak(
        const DocNode *breakContents,
        const DocNode *flatContents,
        const QByteArray & groupId)
{
    DocNode *node = this->allocNode(DocNode::IfBreak);
    node->breakContents = breakContents ? breakContents : m_emptyText;
    node->flatContents = flatContents ? flatContents : m_emptyText;
    node->id = groupId;
    return node;
}

/*!
    Indents @a contents only when group @a groupId breaks
    (or fits when @a negate is true).
*/
const DocNode *DocArena::indentIfBreak(
        const DocNode *contents, const QByteArray & groupId, bool negate)
{
    if ( ! contents || contents == m_emptyText) {
        return m_emptyText;
    }
    DocNode *node = this->allocNode(DocNode::IndentIfBreak);
    node->contents = contents;
    node->id = groupId;
    node->negate = negate;
    return node;
}

const DocNode *DocArena::line()
{
    return m_lineNode;
}

const DocNode *DocArena::softline()
{
    return m_softlineNode;
}

const DocNode *DocArena::hardline()
{
    return this->concat(m_hardlineNoBreakNode, m_breakParentNode);
}

const DocNode *DocArena::literalline()
{
    return this->concat(m_literalNoBreakNode, m_breakParentNode);
}

const DocNode *DocArena::hardlineWithoutBreakParent()
{
    return m_hardlineNoBreakNode;
}

const DocNode *DocArena::literallineWithoutBreakParent()
{
    return m_literalNoBreakNode;
}

/*!
    Defers @a contents until right before the following line break.
*/
const DocNode *DocArena::lineSuffix(const DocNode *contents)
{
    if ( ! contents || contents == m_emptyText) {
        return m_emptyText;
    }
    DocNode *node = this->allocNode(DocNode::LineSuffix);
    node->contents = contents;
    return node;
}

const DocNode *DocArena::lineSuffixBoundary()
{
    return m_lineSuffixBoundaryNode;
}

const DocNode *DocArena::breakParent()
{
    return m_breakParentNode;
}

const DocNode *DocArena::trim()
{
    return m_trimNode;
}

const DocNode *DocArena::label(const QByteArray & name, const DocNode *contents)
{
    if ( ! contents || contents == m_emptyText) {
        return m_emptyText;
    }
    DocNode *node = this->allocNode(DocNode::Label);
    node->text = name;
    node->contents = contents;
    return node;
}

/*!
    Inserts @a separator between every adjacent pair of @a items.
*/
const DocNode *DocArena::join(
        const DocNode *separator, const QVector<const DocNode * > & items)
{
    QVector<const DocNode * > parts;
    parts.reserve(qMax(0, items.size() * 2 - 1));
    for (int i = 0; i < items.size(); ++i) {
        if (i > 0 && separator && separator != m_emptyText) {
            parts.append(separator);
        }
        parts.append(items.at(i));
    }
    return this->concat(parts);
}

/*!
    Discards allocated nodes and resets cached singleton leaves.
*/
void DocArena::clear()
{
    qDeleteAll(m_nodes);
    m_nodes.clear();

    m_emptyText = this->allocNode(DocNode::Text);

    DocNode *lineNode = this->allocNode(DocNode::Line);
    m_lineNode = lineNode;

    DocNode *soft = this->allocNode(DocNode::Line);
    soft->soft = true;
    m_softlineNode = soft;

    DocNode *hardNoBreak = this->allocNode(DocNode::Line);
    hardNoBreak->hard = true;
    m_hardlineNoBreakNode = hardNoBreak;

    DocNode *literalNoBreak = this->allocNode(DocNode::Line);
    literalNoBreak->hard = true;
    literalNoBreak->literal = true;
    m_literalNoBreakNode = literalNoBreak;

    m_breakParentNode = this->allocNode(DocNode::BreakParent);
    m_lineSuffixBoundaryNode = this->allocNode(DocNode::LineSuffixBoundary);
    m_trimNode = this->allocNode(DocNode::Trim);
}
