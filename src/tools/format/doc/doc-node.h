// Copyright (C) 2015 The XD Company Ltd.
// SPDX-License-Identifier: Apache-2.0

#ifndef DOC_DOC_NODE_H
#define DOC_DOC_NODE_H

#include <QtCore/QByteArray>
#include <QtCore/QVector>

struct DocNode
{
    enum Kind {
        Text,
        Concat,
        Indent,
        Align,
        Group,
        Fill,
        IfBreak,
        IndentIfBreak,
        Line,
        LineSuffix,
        LineSuffixBoundary,
        BreakParent,
        Trim,
        Label,
        Verbatim
    };

    enum AlignKind {
        AlignWidth,
        AlignString,
        AlignDedent,
        AlignMarkRoot,
        AlignResetRoot
    };

    enum BreakState {
        BreakNone,
        BreakExplicit,
        BreakPropagated
    };

    inline DocNode(Kind nodeKind = Text)
        : kind(nodeKind)
        , alignKind(AlignWidth)
        , alignWidth(0)
        , breakState(BreakNone)
        , soft(false)
        , hard(false)
        , literal(false)
        , negate(false)
        , contents(Q_NULLPTR)
        , breakContents(Q_NULLPTR)
        , flatContents(Q_NULLPTR)
    {
    }

    Kind kind;
    AlignKind alignKind;
    int alignWidth;
    mutable BreakState breakState;
    bool soft;
    bool hard;
    bool literal;
    bool negate;

    QByteArray text;
    QByteArray id;
    const DocNode *contents;
    const DocNode *breakContents;
    const DocNode *flatContents;
    QVector<const DocNode * > parts;
    QVector<const DocNode * > expandedStates;
};

class DocArena
{
public:

    DocArena();
    ~DocArena();

    const DocNode *text(const QByteArray & bytes);
    const DocNode *verbatim(const QByteArray & bytes);
    const DocNode *concat(const QVector<const DocNode * > & parts);
    const DocNode *concat(const DocNode *a, const DocNode *b);
    const DocNode *concat(const DocNode *a, const DocNode *b, const DocNode *c);
    const DocNode *concat(
            const DocNode *a,
            const DocNode *b,
            const DocNode *c,
            const DocNode *d);
    const DocNode *concat(
            const DocNode *a,
            const DocNode *b,
            const DocNode *c,
            const DocNode *d,
            const DocNode *e);
    const DocNode *concat(
            const DocNode *a,
            const DocNode *b,
            const DocNode *c,
            const DocNode *d,
            const DocNode *e,
            const DocNode *f);
    const DocNode *indent(const DocNode *contents);
    const DocNode *alignWidth(int spaces, const DocNode *contents);
    const DocNode *alignString(const QByteArray & str, const DocNode *contents);
    const DocNode *dedent(const DocNode *contents);
    const DocNode *dedentToRoot(const DocNode *contents);
    const DocNode *markAsRoot(const DocNode *contents);
    const DocNode *group(const DocNode *contents,
                         bool shouldBreak = false,
                         const QByteArray & id = QByteArray());
    const DocNode *conditionalGroup(
            const QVector<const DocNode * > & states,
            const QByteArray & id = QByteArray());
    const DocNode *fill(const QVector<const DocNode * > & parts);
    const DocNode *ifBreak(const DocNode *breakContents,
                           const DocNode *flatContents = Q_NULLPTR,
                           const QByteArray & groupId = QByteArray());
    const DocNode *indentIfBreak(
            const DocNode *contents,
            const QByteArray & groupId,
            bool negate = false);
    const DocNode *line();
    const DocNode *softline();
    const DocNode *hardline();
    const DocNode *literalline();
    const DocNode *hardlineWithoutBreakParent();
    const DocNode *literallineWithoutBreakParent();
    const DocNode *lineSuffix(const DocNode *contents);
    const DocNode *lineSuffixBoundary();
    const DocNode *breakParent();
    const DocNode *trim();
    const DocNode *label(const QByteArray & name, const DocNode *contents);
    const DocNode *join(const DocNode *separator,
                        const QVector<const DocNode * > & items);

    void clear();

private:
    Q_DISABLE_COPY(DocArena)
    DocNode *allocNode(DocNode::Kind kind);

    QVector<DocNode * > m_nodes;
    const DocNode *m_emptyText;
    const DocNode *m_lineNode;
    const DocNode *m_softlineNode;
    const DocNode *m_hardlineNoBreakNode;
    const DocNode *m_literalNoBreakNode;
    const DocNode *m_breakParentNode;
    const DocNode *m_lineSuffixBoundaryNode;
    const DocNode *m_trimNode;
};

#endif // DOC_DOC_NODE_H
