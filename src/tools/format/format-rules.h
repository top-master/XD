// Copyright (C) 2015 The XD Company Ltd.
// SPDX-License-Identifier: Apache-2.0

#ifndef FORMAT_RULES_H
#define FORMAT_RULES_H

#include "./format-document.h"
#include "./format-named-rule.h"

#include <QtCore/QList>
#include <QtCore/QString>

QList<FormatRule * > formatCreateRules();
bool formatIsInXdTree(const QString & path);

#endif // FORMAT_RULES_H
