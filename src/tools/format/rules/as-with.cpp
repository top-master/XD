// Copyright (C) 2015 The XD Company Ltd.
// SPDX-License-Identifier: Apache-2.0

#include "./as-with.h"

#include <QtCore/QList>
#include <QtCore/QRegularExpression>
#include <QtCore/QString>

/*!
    Matches the wording `as with`, which the mappings write `just like`.
*/
Q_GLOBAL_STATIC_WITH_ARGS(QRegularExpression, asWithPattern, (QString(QLL("\\b([Aa])s with\\b"))))

/*!
    Builds \a text with `as with` written as `just like`.
*/
static QString mapAsWith(const QString & text, const void *)
{
    QString result = text;
    QRegularExpressionMatchIterator it = asWithPattern()->globalMatch(text);
    QList<QRegularExpressionMatch > matches;
    while (it.hasNext()) {
        matches.prepend(it.next());
    }
    for (int i = 0; i < matches.size(); ++i) {
        const QRegularExpressionMatch & found = matches.at(i);
        const bool upper = found.captured(1) == QLL("A");
        result.replace(found.capturedStart(), found.capturedLength(),
                       upper ? QString(QLL("Just like")) : QString(QLL("just like")));
    }
    return result;
}

/*!
    \class FormatAsWithRule
    \internal

    Writes `just like` for the wording `as with` in comments, as the
    comment mappings ask.
*/

/*!
    Builds the rule, with mapAsWith() as its map.
*/
FormatAsWithRule::FormatAsWithRule()
    : super("as-with", "Writes \"just like\" for \"as with\" in comments.", mapAsWith)
{
}
