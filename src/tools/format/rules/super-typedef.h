// Copyright (C) 2015 The XD Company Ltd.
// SPDX-License-Identifier: Apache-2.0

#ifndef RULES_SUPER_TYPEDEF_H
#define RULES_SUPER_TYPEDEF_H

#include "../format-named-rule.h"

#include <QtCore/QByteArray>

class FormatSuperTypedefRule : public FormatNamedRule
{
    typedef FormatNamedRule super;
public:
    inline FormatSuperTypedefRule()
        : super("super-typedef",
                "Gives each class with a base a private typedef <Base> super;.")
    {
    }

    void run(FormatDocument *document) const Q_DECL_OVERRIDE;

private:
    QByteArray baseName(const FormatDocument *document, int brace,
                        bool *isStruct) const;
    bool hasSuper(const FormatDocument *document, int brace) const;
};

#endif // RULES_SUPER_TYPEDEF_H
