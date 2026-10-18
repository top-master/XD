// Copyright (C) 2015 The XD Company Ltd.
// SPDX-License-Identifier: Apache-2.0

#ifndef FORMAT_SPACING_H
#define FORMAT_SPACING_H

#include "config-replacement.h"

#include <QtCore/QByteArray>
#include <QtCore/QRegularExpression>
#include <QtCore/QString>

/*!
    \internal

    Carries one spacing choice: `keepAsIs` when the spacing found stays, else
    `text`, the spaces and/or tabs to write. A choice by regular expression
    (see isRegExp()) gives the `pattern` whose matches on each line the
    setting governs take the `replacement`, and its `keepAsIs` and `text`
    name the fallback for a line without a match.

    A null spacing (see isNull(), the one the default constructor builds)
    is no choice at all: FormatDocument::onSetting() returns one for a line
    that a replacement took, which the caller then skips.
*/
class FormatSpacing
{
public:
    inline FormatSpacing() : keepAsIs(false), m_null(true) {}
    inline explicit FormatSpacing(const QByteArray & spacing)
        : keepAsIs(false), text(spacing), m_null(false)
    {
    }

    static inline FormatSpacing keep()
    {
        FormatSpacing spacing = FormatSpacing(QByteArray());
        spacing.keepAsIs = true;
        return spacing;
    }

    inline bool isNull() const { return m_null; }
    inline bool isRegExp() const { return ! pattern.pattern().isEmpty(); }

    bool keepAsIs;
    QByteArray text;
    QRegularExpression pattern;
    Config::Replacement replacement;

private:
    bool m_null;
};

#endif // FORMAT_SPACING_H
