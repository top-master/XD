// Copyright (C) 2015 The XD Company Ltd.
// SPDX-License-Identifier: Apache-2.0

#ifndef FORMAT_RULE_H
#define FORMAT_RULE_H

#include <QtCore/QByteArray>
#include <QtCore/QString>

class FormatDocument;

class FormatRule
{
public:
    virtual ~FormatRule();

    virtual QByteArray name() const = 0;
    virtual QString summary() const = 0;
    virtual void run(FormatDocument *document) const = 0;
};

#endif // FORMAT_RULE_H
