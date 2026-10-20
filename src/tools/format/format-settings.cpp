// Copyright (C) 2015 The XD Company Ltd.
// SPDX-License-Identifier: Apache-2.0

#include "./format-settings.h"

#include "./parser/scope-conditions.h"

#include "config-any-space.h"
#include "config-condition.h"
#include "config-numbers.h"
#include "config-regexp.h"
#include "config-word.h"

/*!
    @class FormatSettings
    @internal

    Carries the choices of a project's `.formatrc` files for one file under
    formatting, where the rule files leave a choice to the project:

    * `refSuffix` (`[Types]`): the spacing after the `&` of a named
      reference declaration, `Type & name` for a space;
    * `moveSuffix` (`[Types]`): the same after the `&&` of a named rvalue
      reference declaration;
    * `wrapBegin` (`[Structure]`): the columns a long line wraps at, tried in
      order, such as `75 | 51`; the code rules take the first one (see
      wrapColumn()); `75` without a `.formatrc`;
    * `wrapMax` (`[Structure]`): the column no line may pass, for the code
      rules that wrap a line; 80 without a `.formatrc`;
    * `commentWrapBegin` and `commentWrapMax` (`[Comments]`, as `wrapBegin`
      and `wrapMax`): the same for comments (see FormatCommentWidthRule);
      `[Comments]` extends every key of `[Structure]` whose name starts with
      `wrap` (`wrapArgsAt` too, which no comment rule reads yet), hence each
      follows its `[Structure]` key until a `.formatrc` overrides it in
      `[Comments]`;
    * `wrapExtendsForKeywords` and `wrapEmptyLimit` (`[Comments]`): how many
      columns past `wrapMax` a comment line may run to end on a code keyword,
      and how many free columns the break before it would leave at least
      (see FormatCommentWidthRule); 20 and 8 without a `.formatrc`;
    * `wrapComma` (`[Comments]`): `soft` (the default) lets a line break
      after a word's `,` only within `wrapEmptyLimit` columns of the wrap
      column, and `hard` within the 25 columns of the other punctuation;
    * `target` (`[Comments]`): `long` (the default) rewraps only a comment
      paragraph that has a line past the limit, and `any` also one whose
      lines leave room unused (see FormatCommentWidthRule); the command line's
      `--any-comment` sets `any`;
    * `thisMode` (`[Relations]`): where a member function reaches a member of
      its own class, or of a base class, through `this->`: a condition (see
      Config::Condition) of the names FormatScopeConditions::names() lists,
      compiled for FormatScopeConditions::holds();
      `is-base-member | already-existing | (is-outline & is-header)` without
      a `.formatrc` (see FormatOutlineThisRule);
    * `wrapChainAt` (`[Structure]`): where an operator chain that does not
      fit one line breaks before its operators (see FormatExpressionLayout):
      a condition of the same names and of percents, such as
      `(40% & is-non-header) | (30% & is-header)`, where a percent holds
      when the column where an operator starts is past that share of
      `wrapMax`; the operators before it stay on the chain's first line.
      `40%` without a `.formatrc`;
    * `wrapArgsAt` (`[Structure]`): where a call whose arguments do not fit
      one line breaks after its `(`, one argument per line (see
      FormatArgsWrapRule): a condition of the same names and of percents,
      such as `(40% & is-non-header) | (30% & is-header)`, where a percent
      holds when the column of the `(` is past that share of `wrapMax`;
      `40%` without a `.formatrc`.

    Each spacing key takes any run of spaces and/or tabs to write there
    (`" "` for one space, `"\t"` for a tab, `""` or `none` for no space),
    `keep-as-is` (the spacing found stays), or
    `regexp/<pattern>/<replacement>/` (see Config::RegExp); FormatSpacing
    carries the choice. Without a `.formatrc`, `refSuffix` is a space, as the
    C++ rules ask, and `moveSuffix` keeps what it finds.

    The `.formatrc` files apply level by level (see
    Config::Scanner::levelFiles()), then the FormatConfig of the command line.
*/

/*!
    Spells the default `[Relations] thisMode` (see FormatSettings).
*/
static const char defaultThisModeText[] = "is-base-member | already-existing | (is-outline & is-header)";

/*!
    Spells the default `[Structure] wrapChainAt`, which `[Structure]
    wrapArgsAt` follows (see FormatSettings).
*/
static const char defaultWrapChainAtText[] = "40%";

/*!
    Builds the name of the settings files the tool reads in each folder.
*/
QString FormatSettings::fileName()
{
    return QString(QLL(".formatrc"));
}

/*!
    Builds the schema of the settings the tool takes, each at its default.
*/
Config::Schema FormatSettings::schema()
{
    const Config::Matchers spacings = Config::Matchers()
        << Config::AnySpace::create() << Config::Word::create(QString(QLL("keep-as-is")))
        << Config::RegExp::create();
    QHash<QString, QString > aliases;
    aliases.insert(QString(QLL("none")), QString());
    Config::Section types(QString(QLL("Types")));
    types.addKey(
        QString(QLL("refSuffix")),
        spacings,
        QString(QLL(" ")),
        aliases
    );
    types.addKey(
        QString(QLL("moveSuffix")),
        spacings,
        QString(QLL("keep-as-is")),
        aliases
    );
    Config::Schema schema;
    schema.addSection(types);
    Config::Section structure(QString(QLL("Structure")));
    structure.addKey(QString(QLL("wrapBegin")),
                     Config::Matchers() << Config::Numbers::create(20, 200), QString(QLL("75")));
    structure.addKey(QString(QLL("wrapMax")),
                     Config::Matchers() << Config::Numbers::create(
                         20,
                         200,
                         1
                     ), QString(QLL("80")));
    structure.addKey(QString(QLL("wrapChainAt")),
                     Config::Matchers()
                         << Config::Condition::create(
                             FormatScopeConditions::names(),
                             true
                         ),
                     QString::fromLatin1(defaultWrapChainAtText));
    structure.addFollower(QString(QLL("wrapArgsAt")), QString(QLL("wrapChainAt")));
    Config::Section comments(QString(QLL("Comments")));
    comments.extendPrefixed(structure, QString(QLL("wrap")));
    comments.addKey(QString(QLL("wrapExtendsForKeywords")),
                    Config::Matchers() << Config::Numbers::create(
                        0,
                        100,
                        1
                    ), QString(QLL("20")));
    comments.addKey(QString(QLL("wrapEmptyLimit")),
                    Config::Matchers() << Config::Numbers::create(
                        0,
                        100,
                        1
                    ), QString(QLL("8")));
    comments.addKey(QString(QLL("wrapComma")),
                    QStringList() << QString(QLL("soft")) << QString(QLL("hard")),
                    QString(QLL("soft")));
    comments.addKey(QString(QLL("target")),
                    QStringList() << QString(QLL("long")) << QString(QLL("any")),
                    QString(QLL("long")));
    schema.addSection(comments);
    Config::Section relations(QString(QLL("Relations")));
    relations.addKey(QString(QLL("thisMode")),
                     Config::Matchers() << Config::Condition::create(FormatScopeConditions::names()),
                     QString::fromLatin1(defaultThisModeText));
    schema.addSection(relations);
    schema.addSection(structure);
    return schema;
}

/*!
    Builds the program of the default `[Relations] thisMode`, for settings
    that no `.formatrc` set.
*/
QStringList FormatSettings::defaultThisMode()
{
    QStringList program;
    QString error;
    Config::Condition::compile(
        QString::fromLatin1(defaultThisModeText),
        FormatScopeConditions::names(),
        &program,
        &error
    );
    return program;
}

/*!
    Builds the program of the default `[Structure] wrapChainAt`, which
    `[Structure] wrapArgsAt` follows, for settings that no `.formatrc` set,
    and for a use site that sets no percent.
*/
QStringList FormatSettings::defaultWrapChainAt()
{
    QStringList program;
    QString error;
    Config::Condition::compile(
        QString::fromLatin1(defaultWrapChainAtText),
        FormatScopeConditions::names(),
        &program,
        &error,
        true
    );
    return program;
}

/*!
    Reads the command line's settings @a config into @a parser, after any
    `.formatrc` it read: the `--config-add` files, then the `-c` assignments;
    returns false, with @a error, for a setting outside of the schema.
*/
bool FormatSettings::readConfig(
        Config::Parser *parser, const FormatConfig & config,
        QString *error)
{
    for (int i = 0; i < config.adds.size(); ++i) {
        if ( ! parser->readFile(config.adds.at(i), error)) {
            return false;
        }
    }
    for (int i = 0; i < config.assignments.size(); ++i) {
        QString reason;
        if ( ! parser->assign(config.assignments.at(i), &reason)) {
            *error = QLL("-c ") + reason;
            return false;
        }
    }
    return true;
}

/*!
    Reads the settings of the file at @a path into @a settings: the
    `.formatrc` files from its project's root down to its folder (or the
    `--config-file` of @a config instead), then the rest of @a config;
    returns false, with @a error naming the file, the key, and the values it
    takes, for a setting the schema does not take.
*/
bool FormatSettings::load(const QString & path, const FormatConfig & config,
                          FormatSettings *settings, QString *error)
{
    const Config::Schema schema = FormatSettings::schema();
    Config::Parser parser(schema);
    if ( ! config.file.isEmpty()) {
        if ( ! parser.readFile(config.file, error)) {
            return false;
        }
    } else if ( ! path.isEmpty()
        && ! parser.readLevels(path, FormatSettings::fileName(), error)
    ) {
        return false;
    }
    if ( ! FormatSettings::readConfig(&parser, config, error)) {
        return false;
    }
    const QString refSuffix = QString(QLL("types.refSuffix"));
    const QString moveSuffix = QString(QLL("types.moveSuffix"));
    settings->refSuffix = FormatSettings::toSpacing(
        parser.value(refSuffix),
        schema.key(refSuffix).defaultValue
    );
    settings->moveSuffix = FormatSettings::toSpacing(
        parser.value(moveSuffix),
        schema.key(moveSuffix).defaultValue
    );
    settings->wrapBegin
        = Config::Numbers::parse(parser.value(QString(QLL("structure.wrapBegin"))));
    settings->wrapMax = parser.value(QString(QLL("structure.wrapMax"))).toInt();
    settings->commentWrapBegin
        = Config::Numbers::parse(parser.value(QString(QLL("comments.wrapBegin"))));
    settings->commentWrapMax = parser.value(QString(QLL("comments.wrapMax"))).toInt();
    settings->wrapExtendsForKeywords
        = parser.value(QString(QLL("comments.wrapExtendsForKeywords"))).toInt();
    settings->wrapEmptyLimit = parser.value(QString(QLL("comments.wrapEmptyLimit"))).toInt();
    settings->wrapComma = parser.value(QString(QLL("comments.wrapComma"))) == QLL("hard")
        ? WrapCommaHard : WrapCommaSoft;
    settings->commentTarget = parser.value(QString(QLL("comments.target"))) == QLL("any")
        ? CommentTargetAny : CommentTargetLong;
    Config::Condition::compile(
        parser.value(QString(QLL("relations.thisMode"))),
        FormatScopeConditions::names(),
        &settings->thisMode,
        error
    );
    Config::Condition::compile(
        parser.value(QString(QLL("structure.wrapChainAt"))),
        FormatScopeConditions::names(),
        &settings->wrapChainAt,
        error,
        true
    );
    Config::Condition::compile(
        parser.value(QString(QLL("structure.wrapArgsAt"))),
        FormatScopeConditions::names(),
        &settings->wrapArgsAt,
        error,
        true
    );
    return true;
}

/*!
    Builds the spacing of @a value, one of the values schema() lets a
    spacing key take: `keep-as-is`, the spaces and/or tabs to write, or a
    `regexp/<pattern>/<replacement>/`, whose spacing where the pattern
    matches nothing is the fallback after its `|` (empty for no space), or
    without a `|` the key's @a defaultValue.
*/
FormatSpacing FormatSettings::toSpacing(
        const QString & value,
        const QString & defaultValue)
{
    QString head;
    QString fallback;
    bool hasFallback = false;
    if (Config::RegExp::split(value, &head, &fallback, &hasFallback)) {
        FormatSpacing spacing = FormatSettings::toSpacing(
            hasFallback ? fallback : defaultValue,
            defaultValue
        );
        Config::RegExp::parse(head, &spacing.pattern, &spacing.replacement);
        return spacing;
    }
    if (value == QLL("keep-as-is")) {
        return FormatSpacing::keep();
    }
    return FormatSpacing(value.toLatin1());
}
