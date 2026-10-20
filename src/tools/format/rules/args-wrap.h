// Copyright (C) 2015 The XD Company Ltd.
// SPDX-License-Identifier: Apache-2.0

#ifndef RULES_ARGS_WRAP_H
#define RULES_ARGS_WRAP_H

#include "../format-named-rule.h"

#include <QtCore/QByteArray>
#include <QtCore/QList>
#include <QtCore/QPair>

class FormatArgsWrapRule : public FormatNamedRule
{
    typedef FormatNamedRule super;
public:
    inline FormatArgsWrapRule()
        : super("args-wrap",
                "Breaks a wrapped call after its (, one argument per line.")
    {
    }

    void run(FormatDocument *document) const Q_DECL_OVERRIDE;

private:
    bool isCallParen(const FormatDocument *document, int paren) const;
    bool isInControlHead(const FormatDocument *document, int paren) const;
    int oneLineEnd(const FormatDocument *document, int open, int close,
                     const QList<QPair<int, int > > & arguments) const;
    bool isBrokenAfterOpen(const FormatDocument *document, int open, int close,
                           int limit) const;
    QByteArray argumentText(const FormatDocument *document, int first, int last,
                            const QByteArray & pad) const;
};

#endif // RULES_ARGS_WRAP_H
