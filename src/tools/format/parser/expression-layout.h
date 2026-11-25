// Copyright (C) 2015 The XD Company Ltd.
// SPDX-License-Identifier: Apache-2.0

#ifndef PARSER_EXPRESSION_LAYOUT_H
#define PARSER_EXPRESSION_LAYOUT_H

#include <QtCore/QByteArray>
#include <QtCore/QList>
#include <QtCore/QPair>

class FormatDocument;
class FormatScopeConditions;

class FormatExpressionLayout
{
public:
    FormatExpressionLayout(
            const FormatDocument *document,
            FormatScopeConditions *conditions,
            int indent,
            bool stepRoot = true);

    QByteArray layout(
            int first,
            int last,
            int column,
            int trailing,
            bool forceMath = false);
    QByteArray layoutList(
            int first,
            int open,
            int close,
            int column,
            bool keepsLines = false,
            int joinCount = 0,
            int extraPad = 0,
            bool closeWithLastItem = false);
    QList<QByteArray > layoutDeclaration(
            int open,
            const QByteArray & prefix,
            const QByteArray & suffix,
            bool breaksAfterOpen,
            bool wrapping,
            const QByteArray & pad,
            int limit,
            bool *fits);
    QByteArray oneLine(int first, int last) const;
    bool hasChain(int first, int last) const;
    bool hasOuterChain(int first, int last) const;
    inline bool brokeUngrouped() const { return m_brokeUngrouped; }

private:
    struct LineLayout {
        QByteArray firstLead;
        QByteArray nextLead;
        QByteArray suffix;
        int limit;
        bool preserveText;
        bool wrapItems;
    };
    QList<int > chainOperators(int first, int last) const;
    int operandCall(int first, int last) const;
    bool operatorLinesFit(
            int first,
            int last,
            const QList<int > & operators,
            int column,
            int trailing) const;
    QByteArray layoutChain(
            int first,
            int last,
            const QList<int > & operators,
            int column,
            int trailing,
            bool forceMath);
    QByteArray layoutChainRest(
            int first,
            int last,
            const QList<int > & operators,
            int column,
            int trailing,
            bool keepsLeading,
            int stepsIn,
            bool byLine,
            const QByteArray & head,
            bool forceMath = false);
    int chainStepsIn(int headFirst,
                     int headLast,
                     const QByteArray & head) const;
    QByteArray layoutGroup(int open, int close, int column, int trailing);
    QByteArray layoutArray(
            int open,
            int close,
            const QList<QPair<int, int > > & items,
            bool keepsGroups,
            int firstItem,
            int joinCount);
    QByteArray listLines(
            int open,
            int close,
            const QList<QPair<int, int > > & items,
            bool keepsGroups,
            int firstItem,
            int joinCount,
            const LineLayout & layout,
            bool *fits);
    QByteArray itemText(int first, int last, int column, int trailing);
    bool breaksOnlyBetweenLiterals(int first, int last) const;
    bool mayBreakList(int open, int itemCount) const;
    bool isListOpen(int open) const;
    QList<QList<int > > lineGroups(const QList<int > & starts) const;
    QByteArray pad() const;
    bool fits(
            int column,
            const QByteArray & text,
            int trailing,
            int token) const;

    const FormatDocument *m_document;
    FormatScopeConditions *m_conditions;
    int m_indent;
    int m_paddingLevel;
    bool m_stepRoot;
    bool m_brokeUngrouped;
};

#endif // PARSER_EXPRESSION_LAYOUT_H
