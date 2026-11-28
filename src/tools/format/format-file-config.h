// Copyright (C) 2015 The XD Company Ltd.
// SPDX-License-Identifier: Apache-2.0

#ifndef FORMAT_FILE_CONFIG_H
#define FORMAT_FILE_CONFIG_H

#include "./format-config.h"
#include "./format-spacing.h"

#include "config-cache.h"
#include "config-condition.h"
#include "config-parser.h"

#include <QtCore/QHash>
#include <QtCore/QList>
#include <QtCore/QPair>
#include <QtCore/QSharedPointer>
#include <QtCore/QString>
#include <QtCore/QVariant>

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
        , docTagSign('@'), nameMismatch(Config::NameMismatchWarn)
        , byRefVariable(LintWarn)
    {
        wrapBegin << 75;
        commentWrapBegin << 75;
        thisMode = FormatFileConfig::schema()
            ->programFor(QString::fromLatin1("relations.thisMode"));
        wrapChainAt = FormatFileConfig::schema()
            ->programFor(QString::fromLatin1("structure.wrapChainAt"));
        wrapArgsAt = FormatFileConfig::schema()
            ->programFor(QString::fromLatin1("structure.wrapChainAt"));
        wrapSignatureAt = FormatFileConfig::schema()
            ->programFor(QString::fromLatin1("structure.wrapSignatureAt"));
        wrapConditionsOverCondition = FormatFileConfig::schema()
            ->programFor(QString::fromLatin1("structure.wrapConditionsOver"));
        bodyBraces << QString::fromLatin1("always");
        wrapMistakes = FormatFileConfig::schema()
            ->programFor(QString::fromLatin1("structure.wrapMistakes"));
        extensions = FormatFileConfig::defaultExtensions();
        typedefSuper = FormatFileConfig::defaultTypedefSuper();
        replaceSuperSites = FormatFileConfig::defaultReplaceSuperSites();
        operatorsInMiddle = false;
        operatorPadding = 0;
        operatorPaddingCondition = FormatFileConfig::schema()
            ->programFor(QString::fromLatin1("structure.operatorPadding"));
        wrapSubChain = QString::fromLatin1("always");
        wrapConditionsPer = QString::fromLatin1("pair-bundle");
        replaceTypesTarget = QString::fromLatin1("isNew");
        replaceTypesAction = QString::fromLatin1("replace");
        standards << QString::fromLatin1("c++11");
        inlineOverride = QString::fromLatin1("override");
    }

    static QString fileName();
    static QString defaultExtensions();
    static QStringList listItems(const QString & text);
    static QStringList superConditionNames();
    static QStringList superChoices();
    static QStringList lintLevels();
    static QStringList wrapMistakesWords();
    static LintLevel lintLevel(
            const QString & value, const FormatFileConfig & config);
    static QString inlineMarker(
            const QString & value, const FormatFileConfig & config);
    static FormatSpacing toSpacing(
            const QString & value, const QString & defaultValue);
    bool acceptsFile(const QString &path) const;
    static QRef<const Config::Schema > schema();
    static QVariant resolve(const QString & key,
                            const QStringList & program,
                            const Config::Evaluator::Context *context);
    static int countOf(const QVariant & count);
    QVariant resolveCached(const QString & key,
                           const QStringList & program,
                           const Config::Evaluator::Context *context) const;
    QVariant resolveSetting(const QString & key,
                            const Config::Evaluator::Context *context) const;
    bool hasExpression(const QString & key) const;
    static QString defaultTypedefSuper();
    static QString defaultReplaceSuperSites();
    static bool readConfig(Config::Parser *parser,
                           const FormatConfig & config,
                           QString *error);
    static bool load(
            const QString & path,
            const FormatConfig & config,
            FormatFileConfig *fileConfig,
            QString *error);
    static bool readParser(
            const QString &path,
            const FormatConfig &config,
            Config::Parser *parser,
            QString *error);
    static bool fromParser(const Config::Parser &parser,
                           FormatFileConfig *fileConfig,
                           QString *error);
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
    bool operatorsInMiddle;
    Config::Settings settings;
    int operatorPadding;
    QStringList operatorPaddingCondition;
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
    QStringList typedefSuperProgram;
    QString replaceSuperSites;
    QStringList replaceSuperSitesProgram;
    QList<QPair<QString, QString > > replaceTypes;
    QString replaceTypesTarget;
    QString replaceTypesAction;
    Config::NameMismatch nameMismatch;
    LintLevel byRefVariable;
    QStringList standards;
    QStringList compilers;
    QString inlineOverride;

private:
    mutable Config::Cache m_cache;
};

#endif // FORMAT_FILE_CONFIG_H
