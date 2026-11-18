// Copyright (C) 2015 The XD Company Ltd.
// SPDX-License-Identifier: Apache-2.0

#ifndef FORMAT_FILE_CONFIG_H
#define FORMAT_FILE_CONFIG_H

#include "./format-config.h"
#include "./format-spacing.h"

#include "config-parser.h"

#include <QtCore/QList>
#include <QtCore/QString>

class FormatFileConfig
{
public:
    enum WrapComma { WrapCommaSoft, WrapCommaHard };
    enum CommentTarget { CommentTargetLong, CommentTargetAny };
    enum CodeTarget { CodeTargetLong, CodeTargetAny };

    inline FormatFileConfig()
        : refSuffix(" "), moveSuffix(FormatSpacing::keep())
        , wrapMax(80), wrapArgsOver(3), wrapConditionsOver(1), joinArgs(1), commentWrapMax(80), wrapExtendsForKeywords(20), wrapEmptyLimit(8)
        , wrapComma(WrapCommaSoft), commentTarget(CommentTargetLong), codeTarget(CodeTargetLong)
        , docTagSign('@')
    {
        wrapBegin << 75;
        commentWrapBegin << 75;
        thisMode = FormatFileConfig::defaultThisMode();
        wrapChainAt = FormatFileConfig::defaultWrapChainAt();
        wrapArgsAt = FormatFileConfig::defaultWrapChainAt();
        wrapConditionsOverCondition =
            FormatFileConfig::defaultWrapConditionsOver();
        bodyBraces << QString::fromLatin1("always");
        extensions = FormatFileConfig::defaultExtensions();
        typedefSuper = FormatFileConfig::defaultTypedefSuper();
        replaceSuperSites = FormatFileConfig::defaultReplaceSuperSites();
        wrapSubChain = QString::fromLatin1("always");
        wrapConditionsPer = QString::fromLatin1("pair-bundle");
    }

    static QString fileName();
    static QString defaultExtensions();
    static QStringList superConditionNames();
    static QStringList superChoices();
    bool acceptsFile(const QString &path) const;
    static Config::Schema schema();
    static QString defaultTypedefSuper();
    static QString defaultReplaceSuperSites();
    static QStringList defaultThisMode();
    static QStringList defaultWrapChainAt();
    static QStringList defaultWrapConditionsOver();
    static bool readConfig(Config::Parser *parser, const FormatConfig & config,
                           QString *error);
    static bool load(const QString & path, const FormatConfig & config,
                     FormatFileConfig *fileConfig, QString *error);
    static bool readParser(const QString &path, const FormatConfig &config,
                           Config::Parser *parser, QString *error);
    static void fromParser(
            const Config::Parser &parser,
            FormatFileConfig *fileConfig);

    FormatSpacing refSuffix;
    FormatSpacing moveSuffix;
    QList<int > wrapBegin;
    int wrapMax;
    int wrapArgsOver;
    QStringList wrapArgsOverCondition;
    int wrapConditionsOver;
    QStringList wrapConditionsOverCondition;
    QString wrapConditionsPer;
    int joinArgs;
    QStringList joinArgsCondition;
    QString wrapSubChain;
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
    QString extensions;
    QString typedefSuper;
    QString replaceSuperSites;

private:
    static FormatSpacing toSpacing(
            const QString & value,
            const QString & defaultValue);
};

#endif // FORMAT_FILE_CONFIG_H
