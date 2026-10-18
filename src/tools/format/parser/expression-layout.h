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
            const FormatDocument *document, FormatScopeConditions *conditions,
            int indent);

    QByteArray layout(int first, int last, int column, int trailing);
    QByteArray layoutList(int first, int open, int close, int column);
    QByteArray oneLine(int first, int last) const;
    bool hasChain(int first, int last) const;
    bool hasOuterChain(int first, int last) const;
    inline bool brokeUngrouped() const { return m_brokeUngrouped; }

private:
    QList<int > chainOperators(int first, int last) const;
    QByteArray layoutChain(
            int first, int last, const QList<int > & operators, int column,
            int trailing);
    QByteArray layoutChainRest(
            int first, int last, const QList<int > & operators,
            int column, int trailing, bool keepsLeading);
    QByteArray layoutGroup(int open, int close, int column, int trailing);
    QByteArray listLines(
            int open, int close, const QList<QPair<int, int > > & items,
            bool keepsGroups);
    QByteArray itemText(int first, int last, int column, int trailing);
    bool breaksOnlyBetweenLiterals(int first, int last) const;
    bool mayBreakList(int open, int itemCount) const;
    bool isListOpen(int open) const;
    QList<QList<int > > lineGroups(const QList<int > & starts) const;
    QByteArray pad() const;
    bool fits(int column, const QByteArray & text, int trailing) const;

    const FormatDocument *m_document;
    FormatScopeConditions *m_conditions;
    int m_indent;
    int m_paddingLevel;
    int m_wrap;
    int m_wrapMax;
    bool m_brokeUngrouped;
};

#endif // PARSER_EXPRESSION_LAYOUT_H
