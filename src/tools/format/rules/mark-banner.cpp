// Copyright (C) 2015 The XD Company Ltd.
// SPDX-License-Identifier: Apache-2.0

#include "./mark-banner.h"

#include "../format-document.h"
#include "../parser/comment-helpers.h"

#include <QtCore/QList>
#include <QtCore/QRegularExpression>
#include <QtCore/QString>

/*!
    Matches a divider comment: a rule of dashes, equal signs, or stars
    around an optional label.
*/
Q_GLOBAL_STATIC_WITH_ARGS(QRegularExpression, bannerPattern, (QString(QLL(
        "^[-=*#~_+/]{3,}\\s*(.*?)\\s*[-=*#~_+/]*$"))))

/*!
    @class FormatMarkBannerRule
    @internal

    Writes a section divider as `// MARK: <label>.`, never as a rule of
    dashes, equal signs, or stars around the label.
*/

/*!
    Rewrites a one-line comment of the shape `---- <label> ----` as a `MARK:`
    line, and reports one with no label.
*/
void FormatMarkBannerRule::runGroup(
        FormatDocument *document,
        const QList<int > & group) const
{
    if (group.size() != 1) {
        return;
    }
    const int index = group.first();
    const FormatComment & comment = document->comments().at(index);
    if (comment.trailing || comment.isDoc()
        || document->commentLine(index) != document->commentEndLine(index)
    ) {
        return;
    }
    const QString text = QString::fromUtf8(commentLines(document, index).first().text);
    if (text.contains(QLL("-*-"))) {
        return;
    }
    const QRegularExpressionMatch found = bannerPattern()->match(text);
    if ( ! found.hasMatch()) {
        return;
    }
    QString label = found.captured(1);
    if (label.isEmpty()) {
        document->report(comment.offset, "mark-banner",
                         QString(QLL("drop the decorative rule, or label it as // MARK: <label>.")));
        return;
    }
    while (label.endsWith(QLatin1Char('.'))
        || label.endsWith(QLatin1Char(':'))
    ) {
        label.chop(1);
    }
    document->replace(comment.offset, comment.length, "// MARK: "
        + label.toUtf8()
        + '.'
    );
}
