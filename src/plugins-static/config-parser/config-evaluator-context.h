// Copyright (C) 2015 The XD Company Ltd.
// SPDX-License-Identifier: Apache-2.0

#ifndef CONFIG_EVALUATOR_CONTEXT_H
#define CONFIG_EVALUATOR_CONTEXT_H

#include <QtCore/QString>

namespace Config {

namespace Evaluator {

class Context
{
public:
    virtual ~Context() {}
    virtual bool predicate(const QString &name, bool *value) const;
    virtual bool percent(qreal *value) const;
};

} // Namespace `Evaluator`.

} // Namespace `Config`.

#endif // CONFIG_EVALUATOR_CONTEXT_H
