// Copyright (C) 2015 The XD Company Ltd.
// SPDX-License-Identifier: Apache-2.0

#ifndef FORMAT_RULES_H
#define FORMAT_RULES_H

#include "./format-document.h"
#include "./format-growth.h"
#include "./format-named-rule.h"

#include <QtCore/QList>
#include <QtCore/QString>

QList<FormatRule * > formatCreateRules();
void formatShiftContinuations(FormatDocument *document,
                              const QList<FormatGrowth > & growths);
bool formatIsInXdTree(const QString & path);

#endif // FORMAT_RULES_H
