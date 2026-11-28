// Copyright (C) 2015 The XD Company Ltd.
// SPDX-License-Identifier: Apache-2.0

#include "./ast-node.h"

/*!
    @struct AstNode
    @internal

    Represents one structural declaration, statement, expression, or directive
    in the parsed syntax tree together with its byte span, token span, and
    attached comments.
*/

/*!
    Appends @a child to this node's ordered child list and links its parent.
*/
void AstNode::addChild(AstNode *child)
{
    if ( ! child) {
        return;
    }
    child->parent = this;
    if (this->children.isEmpty()
        || this->children.last()->startOffset <= child->startOffset
    ) {
        this->children.append(child);
        return;
    }
    int insertAt = this->children.size();
    while (insertAt > 0
        && this->children.at(insertAt - 1)->startOffset > child->startOffset
    ) {
        --insertAt;
    }
    this->children.insert(insertAt, child);
}

/*!
    Records byte offsets @a startOff .. @a endOff and token bounds @a firstTok
    .. @a lastTok on this node.
*/
void AstNode::setSpan(int startOff, int endOff, int firstTok, int lastTok)
{
    this->startOffset = startOff;
    this->endOffset = endOff;
    this->firstToken = firstTok;
    this->lastToken = lastTok;
}

/*!
    @class AstArena
    @internal

    Owns every `AstNode` and `AstComment` allocated for one parse pass.
*/
AstArena::AstArena()
{
    m_nodes.reserve(128);
    m_comments.reserve(64);
}

AstArena::~AstArena()
{
    this->clear();
}

AstNode *AstArena::createNode(AstNode::Kind kind)
{
    AstNode *node = new AstNode(kind);
    m_nodes.append(node);
    return node;
}

AstComment *AstArena::createComment()
{
    AstComment *comment = new AstComment();
    m_comments.append(comment);
    return comment;
}

void AstArena::clear()
{
    qDeleteAll(m_nodes);
    qDeleteAll(m_comments);
    m_nodes.clear();
    m_comments.clear();
}
