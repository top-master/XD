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
    enum CodeTarget { CodeTargetLong, CodeTargetAny };

    inline FormatSettings()
        : refSuffix(" "), moveSuffix(FormatSpacing::keep())
        , wrapMax(80), wrapArgsOver(3), commentWrapMax(80), wrapExtendsForKeywords(20), wrapEmptyLimit(8)
        , wrapComma(WrapCommaSoft), commentTarget(CommentTargetLong), codeTarget(CodeTargetLong)
        , docTagSign('@')
    {
        wrapBegin << 75;
        commentWrapBegin << 75;
        thisMode = FormatSettings::defaultThisMode();
        wrapChainAt = FormatSettings::defaultWrapChainAt();
        wrapArgsAt = FormatSettings::defaultWrapChainAt();
        bodyBraces << QString::fromLatin1("always");
    }

    static QString fileName();
    static Config::Schema schema();
    static QStringList defaultThisMode();
    static QStringList defaultWrapChainAt();
    static bool readConfig(Config::Parser *parser, const FormatConfig & config,
                           QString *error);
    static bool load(const QString & path, const FormatConfig & config,
                     FormatSettings *settings, QString *error);
    static bool readParser(const QString &path, const FormatConfig &config,
                           Config::Parser *parser, QString *error);
    static void fromParser(const Config::Parser &parser, FormatSettings *settings);

    FormatSpacing refSuffix;
    FormatSpacing moveSuffix;
    QList<int > wrapBegin;
    int wrapMax;
    int wrapArgsOver;
    QStringList wrapArgsOverCondition;
    QList<int > commentWrapBegin;
    int commentWrapMax;
    int wrapExtendsForKeywords;
    int wrapEmptyLimit;
    WrapComma wrapComma;
    CommentTarget commentTarget;
    CodeTarget codeTarget;
    QStringList bodyBraces;
    char docTagSign;
    QStringList thisMode;
    QStringList wrapChainAt;
    QStringList wrapArgsAt;

private:
    static FormatSpacing toSpacing(
            const QString & value,
            const QString & defaultValue);
};

#endif // FORMAT_SETTINGS_H
