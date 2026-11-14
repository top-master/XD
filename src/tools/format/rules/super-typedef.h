// Copyright (C) 2015 The XD Company Ltd.
// SPDX-License-Identifier: Apache-2.0

#ifndef RULES_SUPER_TYPEDEF_H
#define RULES_SUPER_TYPEDEF_H

#include "../format-named-rule.h"

#include <QtCore/QByteArray>

class FormatSuperTypedefRule : public FormatNamedRule
{
    typedef FormatNamedRule super;
    friend class FormatSuperContext;
public:
    inline FormatSuperTypedefRule()
        : super("super-typedef",
                "Adds a configured super typedef and uses it for base qualifiers.")
    {
    }

    void run(FormatDocument *document) const Q_DECL_OVERRIDE;

private:
    QByteArray baseName(const FormatDocument *document, int brace,
                        bool *isStruct) const;
    bool hasSuper(const FormatDocument *document, int brace) const;
    bool superIsBase(const FormatDocument *document, int brace,
                     const QByteArray &base) const;
    bool isNewClass(const FormatDocument *document, int brace) const;
    bool isNeeded(const FormatDocument *document, int brace,
                  const QByteArray &base) const;
    bool wantsTypedef(const FormatDocument *document, int brace,
                      const QByteArray &base) const;
    bool willHaveSuper(const FormatDocument *document, int brace,
                       const QByteArray &base, const FormatConfig &config) const;
    void replaceSites(FormatDocument *document) const;
};

#endif // RULES_SUPER_TYPEDEF_H
