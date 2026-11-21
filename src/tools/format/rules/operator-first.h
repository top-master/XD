// Copyright (C) 2015 The XD Company Ltd.
// SPDX-License-Identifier: Apache-2.0

#ifndef RULES_OPERATOR_FIRST_H
#define RULES_OPERATOR_FIRST_H

#include "../format-named-rule.h"

class FormatOperatorFirstRule : public FormatNamedRule
{
    typedef FormatNamedRule super;
public:
    inline FormatOperatorFirstRule()
        : super("operator-first",
                "Breaks a long expression before an operator, not after it.") {}
    void run(FormatDocument *document) const Q_DECL_OVERRIDE;

protected:
    inline virtual bool forMembers() const { return false; }
};

class FormatChainMembersFirstRule : public FormatOperatorFirstRule
{
    typedef FormatOperatorFirstRule super;
public:
    inline QByteArray name() const Q_DECL_OVERRIDE_IL
    { return QByteArray("chain-members-first"); }
    inline QString summary() const Q_DECL_OVERRIDE_IL
    { return QString::fromLatin1("Joins member access after closing brackets when it fits."); }

protected:
    inline bool forMembers() const Q_DECL_OVERRIDE_IL { return true; }
};

#endif // RULES_OPERATOR_FIRST_H
