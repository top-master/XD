// Copyright (C) 2015 The XD Company Ltd.
// SPDX-License-Identifier: Apache-2.0

#ifndef FORMAT_FILE_CONFIG_H
#define FORMAT_FILE_CONFIG_H

#include "./format-config.h"
#include "./format-spacing.h"

#include "config-condition.h"
#include "config-parser.h"

#include <QtCore/QList>
#include <QtCore/QPair>
#include <QtCore/QString>

class FormatFileConfig
{
public:
    enum WrapComma { WrapCommaSoft, WrapCommaHard };
    enum CommentTarget { CommentTargetLong, CommentTargetAny };
    enum CodeTarget { CodeTargetLong, CodeTargetAny };
    enum Compact { CompactNever, CompactAlways };
    enum LintLevel { LintIgnore, LintWarn, LintError };

    inline FormatFileConfig()
        : refSuffix(" "), moveSuffix(FormatSpacing::keep())
        , wrapMax(80), wrapArgsOver(3), wrapConditionsOver(1), joinArgs(1), commentWrapMax(80), wrapExtendsForKeywords(20), wrapEmptyLimit(8)
        , wrapComma(WrapCommaSoft), commentTarget(CommentTargetLong), codeTarget(CodeTargetLong)
        , compact(CompactNever)
        , docTagSign('@'), nameMismatch(Config::Condition::NameMismatchWarn)
        , byRefVariable(LintWarn)
    {
        wrapBegin << 75;
        commentWrapBegin << 75;
        thisMode = FormatFileConfig::defaultThisMode();
        wrapChainAt = FormatFileConfig::defaultWrapChainAt();
        wrapArgsAt = FormatFileConfig::defaultWrapChainAt();
        wrapSignatureAt = FormatFileConfig::defaultWrapSignatureAt();
        wrapConditionsOverCondition =
            FormatFileConfig::defaultWrapConditionsOver();
        bodyBraces << QString::fromLatin1("always");
        wrapMistakes = FormatFileConfig::defaultWrapMistakes();
        extensions = FormatFileConfig::defaultExtensions();
        typedefSuper = FormatFileConfig::defaultTypedefSuper();
        replaceSuperSites = FormatFileConfig::defaultReplaceSuperSites();
        wrapSubChain = QString::fromLatin1("always");
        wrapConditionsPer = QString::fromLatin1("pair-bundle");
        replaceTypesTarget = QString::fromLatin1("isNew");
        replaceTypesAction = QString::fromLatin1("replace");
        standards << QString::fromLatin1("c++11");
        inlineOverride = QString::fromLatin1("override");
    }

    static QString fileName();
    static QString defaultExtensions();
    static QStringList superConditionNames();
    static QStringList superChoices();
    static QStringList lintLevels();
    static QStringList defaultWrapMistakes();
    static QStringList wrapMistakesWords();
    static LintLevel lintLevel(
            const QString & value,
            const FormatFileConfig & config);
    static QString inlineMarker(
            const QString & value,
            const FormatFileConfig & config);
    static QStringList replaceTargetNames();
    bool acceptsFile(const QString &path) const;
    static Config::Schema schema();
    static bool answer(const QString & key, const QStringList & program,
                       const Config::Evaluator::Context *context,
                       QVariant *value);
    static QString defaultTypedefSuper();
    static QString defaultReplaceSuperSites();
    static QStringList defaultThisMode();
    static QStringList defaultWrapChainAt();
    static QStringList defaultWrapSignatureAt();
    static QStringList defaultWrapConditionsOver();
    static bool readConfig(Config::Parser *parser, const FormatConfig & config,
                           QString *error);
    static bool load(const QString & path, const FormatConfig & config,
                     FormatFileConfig *fileConfig, QString *error);
    static bool readParser(const QString &path, const FormatConfig &config,
                           Config::Parser *parser, QString *error);
    static bool fromParser(
            const Config::Parser &parser,
            FormatFileConfig *fileConfig, QString *error);
    static QList<QPair<QString, QString > > expandPrevious(
            const QList<QPair<QString, QString > > & entries);

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
    QStringList wrapMistakes;
    Compact compact;
    QStringList bodyBraces;
    char docTagSign;
    QStringList thisMode;
    QStringList wrapChainAt;
    QStringList wrapArgsAt;
    QStringList wrapSignatureAt;
    QString extensions;
    QString typedefSuper;
    QString replaceSuperSites;
    QList<QPair<QString, QString > > replaceTypes;
    QString replaceTypesTarget;
    QString replaceTypesAction;
    Config::Condition::NameMismatch nameMismatch;
    LintLevel byRefVariable;
    QStringList standards;
    QStringList compilers;
    QString inlineOverride;

private:
    static FormatSpacing toSpacing(
            const QString & value,
            const QString & defaultValue);
};

#endif // FORMAT_FILE_CONFIG_H
