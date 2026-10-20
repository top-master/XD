// Copyright (C) 2015 The XD Company Ltd.
// SPDX-License-Identifier: Apache-2.0

#ifndef FORMAT_SETTINGS_H
#define FORMAT_SETTINGS_H

#include "./format-config.h"
#include "./format-spacing.h"

#include "config-parser.h"

#include <QtCore/QList>
#include <QtCore/QString>

class FormatSettings
{
public:
    enum WrapComma { WrapCommaSoft, WrapCommaHard };
    enum CommentTarget { CommentTargetLong, CommentTargetAny };

    inline FormatSettings()
        : refSuffix(" "), moveSuffix(FormatSpacing::keep())
        , wrapMax(80), commentWrapMax(80), wrapExtendsForKeywords(20), wrapEmptyLimit(8)
        , wrapComma(WrapCommaSoft), commentTarget(CommentTargetLong)
    {
        wrapBegin << 75;
        commentWrapBegin << 75;
        thisMode = FormatSettings::defaultThisMode();
        wrapChainAt = FormatSettings::defaultWrapChainAt();
        wrapArgsAt = FormatSettings::defaultWrapChainAt();
    }

    static QString fileName();
    static Config::Schema schema();
    static QStringList defaultThisMode();
    static QStringList defaultWrapChainAt();
    static bool readConfig(Config::Parser *parser, const FormatConfig & config,
                           QString *error);
    static bool load(const QString & path, const FormatConfig & config,
                     FormatSettings *settings, QString *error);

    FormatSpacing refSuffix;
    FormatSpacing moveSuffix;
    QList<int > wrapBegin;
    int wrapMax;
    QList<int > commentWrapBegin;
    int commentWrapMax;
    int wrapExtendsForKeywords;
    int wrapEmptyLimit;
    WrapComma wrapComma;
    CommentTarget commentTarget;
    QStringList thisMode;
    QStringList wrapChainAt;
    QStringList wrapArgsAt;

private:
    static FormatSpacing toSpacing(
            const QString & value,
            const QString & defaultValue);
};

#endif // FORMAT_SETTINGS_H
