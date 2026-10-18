// Copyright (C) 2015 The XD Company Ltd.
// SPDX-License-Identifier: Apache-2.0

#ifndef RULES_INLINE_KEYWORD_H
#define RULES_INLINE_KEYWORD_H

#include "../format-named-rule.h"

class FormatInlineKeywordRule : public FormatNamedRule
{
    typedef FormatNamedRule super;
public:
    inline FormatInlineKeywordRule()
        : super("inline-keyword",
                "Spells out inline on in-class definitions, as the class does.")
    {
    }

    void run(FormatDocument *document) const Q_DECL_OVERRIDE;

private:
    int declarationStart(const FormatDocument *document, int body, bool *hasInline,
                         bool *exempt) const;
};

#endif // RULES_INLINE_KEYWORD_H
