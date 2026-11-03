// Copyright (C) 2015 The XD Company Ltd.
// SPDX-License-Identifier: Apache-2.0

#ifndef PARSER_CONDITION_LAYOUT_H
#define PARSER_CONDITION_LAYOUT_H

#include <QtCore/QByteArray>
#include <QtCore/QList>

class FormatDocument;
class FormatScopeConditions;

class FormatConditionLayout
{
public:
    FormatConditionLayout(const FormatDocument *document,
                          FormatScopeConditions *conditions);

    QByteArray layoutCondition(
            int open,
            int close,
            int indent,
            int column) const;
    QByteArray layoutPlain(int open, int close) const;
    QList<QByteArray > layoutHead(int first, int last, int indent) const;
    QList<int > operators(int open, int close) const;

private:
    QByteArray group(int open, int close, int indent, int column,
                     bool isCondition) const;
    QByteArray atom(int first, int last, int indent, int column,
                    bool *isComplex) const;
    QList<QByteArray > headLines(
            int first, int last, int column, int indent,
            int wrap, int tier, bool startsStatement) const;

    const FormatDocument *m_document;
    FormatScopeConditions *m_conditions;
};

#endif // PARSER_CONDITION_LAYOUT_H
