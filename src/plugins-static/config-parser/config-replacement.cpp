// Copyright (C) 2015 The XD Company Ltd.
// SPDX-License-Identifier: Apache-2.0

#include "./config-replacement.h"

/*!
    \class Config::Replacement
    \internal

    Builds the text that replaces each match of a regular expression, from
    a replacement written with group references: `\0` stands for the whole
    match, `\1` to `\99` for the pattern's groups (for the pattern
    `(abc.*)(xyz.*)`, `\2\1` swaps its two groups), and `\\` for one
    backslash; every other character stays as it is.

    It keeps the replacement as pieces, each a literal text and the group
    that follows it (-1 for none), read once by parse().
*/

/*!
    Parses \a text, a replacement for a pattern of \a groupCount groups, into
    \a replacement; returns false for a reference to a group the pattern does
    not have.

    A reference takes a second digit only while the two-digit number still
    names a group; hence `\10` is the tenth group of a pattern that has ten,
    and the first group followed by a `0` otherwise.
*/
bool Config::Replacement::parse(const QString & text, int groupCount,
                                Config::Replacement *replacement)
{
    Config::Replacement parsed;
    QString literal;
    for (int i = 0; i < text.size(); ++i) {
        const QChar c = text.at(i);
        if (c != QLatin1Char('\\') || i + 1 >= text.size()) {
            literal += c;
            continue;
        }
        const QChar next = text.at(i + 1);
        if (next == QLatin1Char('\\')) {
            literal += QLatin1Char('\\');
            ++i;
            continue;
        }
        if (!next.isDigit()) {
            literal += c;
            continue;
        }

        // Reads the group number, taking a second digit while the number
        // still names a group.
        int group = next.digitValue();
        ++i;
        if (i + 1 < text.size() && text.at(i + 1).isDigit()
            && group * 10 + text.at(i + 1).digitValue() <= groupCount
        ) {
            group = group * 10 + text.at(i + 1).digitValue();
            ++i;
        }
        if (group > groupCount) {
            return false;
        }
        parsed.m_literals << literal;
        parsed.m_groups << group;
        literal.clear();
    }
    parsed.m_literals << literal;
    parsed.m_groups << -1;
    *replacement = parsed;
    return true;
}

/*!
    Builds \a subject with each match of \a pattern replaced by this
    replacement, its group references filled in from that match.
*/
QString Config::Replacement::apply(const QRegularExpression & pattern,
                                   const QString & subject) const
{
    QString result;
    int last = 0;
    QRegularExpressionMatchIterator matches = pattern.globalMatch(subject);
    while (matches.hasNext()) {
        const QRegularExpressionMatch match = matches.next();
        result += subject.midRef(last, match.capturedStart() - last);
        for (int i = 0; i < this->m_literals.size(); ++i) {
            result += this->m_literals.at(i);
            if (this->m_groups.at(i) >= 0) {
                result += match.capturedRef(this->m_groups.at(i));
            }
        }
        last = match.capturedEnd();
    }
    result += subject.midRef(last);
    return result;
}
