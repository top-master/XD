// Copyright (C) 2015 The XD Company Ltd.
// SPDX-License-Identifier: Apache-2.0

#ifndef AST_AST_NODE_H
#define AST_AST_NODE_H

#include "../format-comment.h"
#include "../lexer/format-language.h"
#include "../lexer/format-token.h"

#include <QtCore/QByteArray>
#include <QtCore/QVector>

struct AstNode;

struct AstComment
{
    enum Placement {
        Unplaced = 0,
        Leading,
        Trailing,
        Dangling
    };

    inline AstComment()
        : index(-1)
        , offset(0)
        , length(0)
        , line(0)
        , endLine(0)
        , kind(FormatComment::Line)
        , trailingHint(false)
        , isProtected(false)
        , placement(Unplaced)
        , enclosingNode(Q_NULLPTR)
        , precedingNode(Q_NULLPTR)
        , followingNode(Q_NULLPTR)
        , edited(false)
        , printed(false)
    {
    }

    inline int endOffset() const { return offset + length; }

    int index;
    int offset;
    int length;
    int line;
    int endLine;
    FormatComment::Kind kind;
    bool trailingHint;
    bool isProtected;
    Placement placement;
    AstNode *enclosingNode;
    AstNode *precedingNode;
    AstNode *followingNode;
    QByteArray text;
    bool edited;
    mutable bool printed;
};

struct AstNode
{
    enum Kind {
        TranslationUnit = 0,
        Directive,
        NamespaceDecl,
        LinkageSpec,
        TemplateDecl,
        ClassDecl,
        AccessSpec,
        TypedefUsingDecl,
        FunctionDecl,
        ParamDecl,
        CtorInitializer,
        VarDecl,
        QtMacroDecl,
        CompoundStmt,
        IfStmt,
        ForStmt,
        WhileStmt,
        SwitchStmt,
        CaseStmt,
        TryStmt,
        ReturnStmt,
        ExprStmt,
        BinaryExpr,
        TernaryExpr,
        UnaryExpr,
        ParenExpr,
        CallExpr,
        MemberExpr,
        SubscriptExpr,
        InitListExpr,
        LambdaExpr,
        CastExpr,
        IdentExpr,
        LiteralExpr,
        RawSpan
    };

    inline explicit AstNode(Kind nodeKind = RawSpan)
        : kind(nodeKind)
        , startOffset(0)
        , endOffset(0)
        , firstToken(-1)
        , lastToken(-1)
        , openToken(-1)
        , closeToken(-1)
        , opToken(-1)
        , altToken(-1)
        , parent(Q_NULLPTR)
        , cond(Q_NULLPTR)
        , body(Q_NULLPTR)
        , elseBranch(Q_NULLPTR)
        , lhs(Q_NULLPTR)
        , rhs(Q_NULLPTR)
        , inner(Q_NULLPTR)
        , isBlock(false)
        , isDoWhile(false)
        , isEnum(false)
        , isForward(false)
        , isInline(false)
        , isConst(false)
        , isPrefix(false)
        , isRangeFor(false)
        , isConnectCall(false)
        , synthetic(false)
        , preserveVerbatim(false)
        , needsBraces(false)
    {
    }

    void addChild(AstNode *child);
    void setSpan(int startOff, int endOff, int firstTok = -1, int lastTok = -1);

    Kind kind;
    int startOffset;
    int endOffset;
    int firstToken;
    int lastToken;
    int openToken;
    int closeToken;
    int opToken;
    int altToken;

    QByteArray name;
    QByteArray opText;
    QByteArray transformedText;

    AstNode *parent;
    AstNode *cond;
    AstNode *body;
    AstNode *elseBranch;
    AstNode *lhs;
    AstNode *rhs;
    AstNode *inner;

    QVector<AstNode * > children;
    QVector<AstNode * > items;
    QVector<AstNode * > initializers;
    QVector<AstComment * > comments;

    bool isBlock;
    bool isDoWhile;
    bool isEnum;
    bool isForward;
    bool isInline;
    bool isConst;
    bool isPrefix;
    bool isRangeFor;
    bool isConnectCall;
    bool synthetic;
    bool preserveVerbatim;
    bool needsBraces;
};

class AstArena
{
public:

    AstArena();
    ~AstArena();

    AstNode *createNode(AstNode::Kind kind);
    AstComment *createComment();
    void clear();

private:
    Q_DISABLE_COPY(AstArena)
    QVector<AstNode * > m_nodes;
    QVector<AstComment * > m_comments;
};

#endif // AST_AST_NODE_H
