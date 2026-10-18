// Copyright (C) 2015 The XD Company Ltd.
// SPDX-License-Identifier: Apache-2.0

#ifndef RULES_NO_LAMBDA_H
#define RULES_NO_LAMBDA_H

#include "../format-named-rule.h"

class FormatNoLambdaRule : public FormatNamedRule
{
    typedef FormatNamedRule super;
public:
    inline FormatNoLambdaRule()
        : super("no-lambda",
                "Reports lambdas in XD's tree, and nested lambdas elsewhere.")
    {
    }

    void run(FormatDocument *document) const Q_DECL_OVERRIDE;
};

#endif // RULES_NO_LAMBDA_H
