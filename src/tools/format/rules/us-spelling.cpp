// Copyright (C) 2015 The XD Company Ltd.
// SPDX-License-Identifier: Apache-2.0

#include "./us-spelling.h"

#include <QtCore/QRegularExpression>
#include <QtCore/QString>

/*!
    Builds \a text with each British spelling of a common word written the
    US way, keeping an upper-case first letter.
*/
static QString mapSpelling(const QString & text, const void *)
{
    static const char *const pairs[][2] = {
        { "colour", "color" }, { "colours", "colors" }, { "behaviour", "behavior" },
        { "behaviours", "behaviors" }, { "favour", "favor" }, { "honour", "honor" },
        { "neighbour", "neighbor" }, { "centre", "center" }, { "licence", "license" },
        { "cancelled", "canceled" }, { "cancelling", "canceling" },
        { "travelled", "traveled" }, { "modelled", "modeled" }, { "labelled", "labeled" },
        { "signalled", "signaled" }, { "initialise", "initialize" },
        { "initialised", "initialized" }, { "initialises", "initializes" },
        { "initialising", "initializing" }, { "initialisation", "initialization" },
        { "organise", "organize" }, { "organised", "organized" }, { "recognise", "recognize" },
        { "recognised", "recognized" }, { "realise", "realize" }, { "realised", "realized" },
        { "optimise", "optimize" }, { "optimised", "optimized" }, { "serialise", "serialize" },
        { "serialised", "serialized" }, { "normalise", "normalize" },
        { "normalised", "normalized" }, { "finalise", "finalize" }, { "analyse", "analyze" },
        { "analysed", "analyzed" }, { "catalogue", "catalog" }, { "dialogue", "dialog" },
        { "grey", "gray" }, { "whilst", "while" }, { "amongst", "among" },
        { "closeable", "closable" }
    };
    QString result = text;
    for (size_t p = 0; p < sizeof(pairs) / sizeof(pairs[0]); ++p) {
        const QString british = QString::fromLatin1(pairs[p][0]);
        const QRegularExpression word(
            QLL("\\b") + british + QLL("\\b"),
            QRegularExpression::CaseInsensitiveOption
        );
        int at = 0;
        for (;;) {
            const QRegularExpressionMatch found = word.match(result, at);
            if ( ! found.hasMatch()) {
                break;
            }
            QString us = QString::fromLatin1(pairs[p][1]);
            if (result.at(found.capturedStart()).isUpper()) {
                us[0] = us.at(0).toUpper();
            }
            result.replace(found.capturedStart(), found.capturedLength(), us);
            at = found.capturedStart() + us.size();
        }
    }
    return result;
}

/*!
    \class FormatUsSpellingRule
    \internal

    Spells common words of comments the US way (`color`, `initialize`).
*/

/*!
    Builds the rule, with mapSpelling() as its map.
*/
FormatUsSpellingRule::FormatUsSpellingRule()
    : super("us-spelling", "Spells common words the US way in comments.", mapSpelling)
{
}
