// Copyright (C) 2015 The XD Company Ltd.
// SPDX-License-Identifier: Apache-2.0

#include "./comment-attacher.h"

namespace {

enum CommentLinePosition {
    OwnLine = 0,
    EndOfLine,
    InlineRemaining
};

bool hasNewlineBeforeOnLine(const QByteArray & source, int offset)
{
    int i = qMin(offset, source.size()) - 1;
    while (i >= 0) {
        const char ch = source.at(i);
        if (ch == '\n' || ch == '\r') {
            return true;
        }
        if (ch != ' ' && ch != '\t') {
            return false;
        }
        --i;
    }
    return true;
}

bool hasNewlineAfterOnLine(const QByteArray & source, int endOffset)
{
    int i = qMax(0, endOffset);
    while (i < source.size()) {
        const char ch = source.at(i);
        if (ch == '\n' || ch == '\r') {
            return true;
        }
        if (ch != ' ' && ch != '\t') {
            return false;
        }
        ++i;
    }
    return true;
}

bool onlyTieBreakerCharsBetween(const QByteArray & source, int from, int to)
{
    const int safeFrom = qMax(0, from);
    const int safeTo = qMin(to, source.size());
    for (int i = safeFrom; i < safeTo; ++i) {
        const char ch = source.at(i);
        if (ch != ' '
            && ch != '\t'
            && ch != '\r'
            && ch != '\n'
            && ch != '('
            && ch != '['
            && ch != '{'
        ) {
            return false;
        }
    }
    return true;
}

CommentLinePosition classifyPosition(
        const QByteArray & source, const AstComment *comment)
{
    const bool ownLine = ! comment->trailingHint
        && hasNewlineBeforeOnLine(source, comment->offset);
    if (ownLine) {
        return OwnLine;
    }
    if (hasNewlineAfterOnLine(source, comment->endOffset())) {
        return EndOfLine;
    }
    return InlineRemaining;
}

void locateAstNeighborhood(
        AstNode *root,
        AstComment *comment,
        AstNode **enclosingOut,
        AstNode **precedingOut,
        AstNode **followingOut)
{
    AstNode *enclosing = root;
    AstNode *preceding = Q_NULLPTR;
    AstNode *following = Q_NULLPTR;

    const int commentStart = comment->offset;
    const int commentEnd = comment->endOffset();

    for (;;) {
        const QVector<AstNode * > & kids = enclosing->children;
        int low = 0;
        int high = kids.size();
        AstNode *nextEnclosing = Q_NULLPTR;
        preceding = Q_NULLPTR;
        following = Q_NULLPTR;

        while (low < high) {
            const int mid = low + (high - low) / 2;
            AstNode *child = kids.at(mid);
            if (child->startOffset <= commentStart
                && commentEnd <= child->endOffset
            ) {
                nextEnclosing = child;
                break;
            }
            if (child->endOffset <= commentStart) {
                preceding = child;
                low = mid + 1;
            } else {
                following = child;
                high = mid;
            }
        }

        if (nextEnclosing) {
            if (preceding == Q_NULLPTR || following == Q_NULLPTR) {
                const int idx = kids.indexOf(nextEnclosing);
                if (idx > 0 && ! preceding) {
                    preceding = kids.at(idx - 1);
                }
                if (idx >= 0 && idx + 1 < kids.size() && ! following) {
                    following = kids.at(idx + 1);
                }
            }
            enclosing = nextEnclosing;
            continue;
        }
        break;
    }

    *enclosingOut = enclosing;
    *precedingOut = preceding;
    *followingOut = following;
}

void assignComment(AstNode *target,
                   AstComment *comment,
                   AstComment::Placement placement)
{
    if ( ! target || ! comment) {
        return;
    }
    comment->placement = placement;
    target->comments.append(comment);
}

bool handleEmptyContainer(AstNode *enclosing, AstComment *comment)
{
    if ( ! enclosing || ! enclosing->children.isEmpty()) {
        return false;
    }
    if (enclosing->kind == AstNode::CompoundStmt
        || enclosing->kind == AstNode::ClassDecl
        || enclosing->kind == AstNode::NamespaceDecl
        || enclosing->kind == AstNode::InitListExpr
        || enclosing->kind == AstNode::TranslationUnit
    ) {
        assignComment(enclosing, comment, AstComment::Dangling);
        return true;
    }
    return false;
}

bool handleOwnLineSpecific(
        AstNode *enclosing,
        AstNode *preceding,
        AstNode *following,
        AstComment *comment)
{
    if (handleEmptyContainer(enclosing, comment)) {
        return true;
    }
    if (enclosing->kind == AstNode::IfStmt
        && following
        && following == enclosing->elseBranch
    ) {
        assignComment(following, comment, AstComment::Leading);
        return true;
    }
    if ((enclosing->kind == AstNode::IfStmt
            || enclosing->kind == AstNode::WhileStmt
            || enclosing->kind == AstNode::ForStmt)
        && following
        && following == enclosing->body
    ) {
        assignComment(following, comment, AstComment::Leading);
        return true;
    }
    Q_UNUSED(preceding);
    return false;
}

bool handleEndOfLineSpecific(
        AstNode *enclosing,
        AstNode *preceding,
        AstNode *following,
        AstComment *comment)
{
    if (handleEmptyContainer(enclosing, comment)) {
        return true;
    }
    if (preceding
        && preceding->kind == AstNode::IfStmt
        && following
        && following == enclosing->elseBranch
    ) {
        assignComment(preceding, comment, AstComment::Trailing);
        return true;
    }
    if (preceding
        && (enclosing->kind == AstNode::IfStmt
            || enclosing->kind == AstNode::WhileStmt
            || enclosing->kind == AstNode::ForStmt)
        && preceding == enclosing->cond
    ) {
        assignComment(preceding, comment, AstComment::Trailing);
        return true;
    }
    return false;
}

bool handleInlineSpecific(
        AstNode *enclosing,
        AstNode *preceding,
        AstNode *following,
        const QByteArray & source,
        AstComment *comment)
{
    if (handleEmptyContainer(enclosing, comment)) {
        return true;
    }
    if (preceding
        && following
        && onlyTieBreakerCharsBetween(
            source,
            preceding->endOffset,
            comment->offset)
    ) {
        assignComment(preceding, comment, AstComment::Trailing);
        return true;
    }
    return false;
}

} // namespace

/*!
    @class AstCommentAttacher
    @internal

    Attaches every comment in @a comments onto its enclosing, preceding, or
    following AST node in one binary-search pass with brace and control-flow
    tie breaking.
*/

/*!
    Attaches every comment in @a comments directly to nodes inside @a root using
    byte spans from @a source.
*/
void AstCommentAttacher::attach(
        AstNode *root,
        QVector<AstComment * > *comments,
        const QByteArray & source)
{
    if ( ! root || ! comments || comments->isEmpty()) {
        return;
    }

    for (int i = 0; i < comments->size(); ++i) {
        AstComment *comment = comments->at(i);
        if ( ! comment) {
            continue;
        }
        AstNode *enclosing = Q_NULLPTR;
        AstNode *preceding = Q_NULLPTR;
        AstNode *following = Q_NULLPTR;
        locateAstNeighborhood(
            root,
            comment,
            &enclosing,
            &preceding,
            &following
        );

        comment->enclosingNode = enclosing;
        comment->precedingNode = preceding;
        comment->followingNode = following;

        const CommentLinePosition linePos = classifyPosition(
            source,
            comment
        );

        // Applies tie breaking when text immediately precedes a block comment.
        if (linePos != OwnLine
            && preceding
            && following
            && ! comment->trailingHint
            && onlyTieBreakerCharsBetween(
                source,
                preceding->endOffset,
                comment->offset)
            && ! hasNewlineAfterOnLine(source, comment->endOffset())
        ) {
            assignComment(following, comment, AstComment::Leading);
            continue;
        }

        if (linePos == OwnLine) {
            if (handleOwnLineSpecific(
                enclosing,
                preceding,
                following,
                comment
            )) {
                continue;
            }
            if (following) {
                assignComment(following, comment, AstComment::Leading);
            } else if (preceding) {
                assignComment(preceding, comment, AstComment::Trailing);
            } else {
                assignComment(enclosing, comment, AstComment::Dangling);
            }
        } else if (linePos == EndOfLine) {
            if (handleEndOfLineSpecific(
                enclosing,
                preceding,
                following,
                comment
            )) {
                continue;
            }
            if (preceding) {
                assignComment(preceding, comment, AstComment::Trailing);
            } else if (following) {
                assignComment(following, comment, AstComment::Leading);
            } else {
                assignComment(enclosing, comment, AstComment::Dangling);
            }
        } else {
            if (handleInlineSpecific(
                enclosing,
                preceding,
                following,
                source,
                comment
            )) {
                continue;
            }
            if (following) {
                assignComment(following, comment, AstComment::Leading);
            } else if (preceding) {
                assignComment(preceding, comment, AstComment::Trailing);
            } else {
                assignComment(enclosing, comment, AstComment::Dangling);
            }
        }
    }
}
