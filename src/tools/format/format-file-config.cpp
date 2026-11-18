// Copyright (C) 2015 The XD Company Ltd.
// SPDX-License-Identifier: Apache-2.0

#include "./format-file-config.h"

#include "./parser/condition-context.h"
#include "./parser/scope-conditions.h"

#include "config-any-space.h"
#include "config-choice.h"
#include "config-condition.h"
#include "config-scopes.h"
#include "config-name.h"
#include "config-numbers.h"
#include "config-regexp.h"
#include "config-word.h"

#include <QtCore/QFileInfo>
#include <QtCore/QRegularExpression>

/*!
    @class FormatFileConfig
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
    * `docTagSign` (`[Comments]`): the sign a new doc-comment writes its
      commands with, `@` (the default) or `\` (also spelled `at` and
      `backslash`), see FormatDocTagSignRule;
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
      FormatArgsWrapRule): a condition like `wrapChainAt`, where a percent
      holds when the column of the `(` is past that share of `wrapMax`. It
      follows `wrapChainAt` until a `.formatrc` sets it, since an argument
      list is a chain of commas;
    * `wrapSignatureAt` (`[Structure]`): `wrapArgsAt` for the parameter
      list of a declaration or definition (a function, a method, or a
      constructor), which past `wrapMax` starts on the next line where it
      holds (see FormatParamWrapRule); the other `wrapArgs` keys still
      apply there. `cfg.wrapArgsAt * 0.875` without a `.formatrc`, which
      holds past 87.5% of the percent `wrapArgsAt` gives at the place;
    * `wrapArgsOver` (`[Structure]`): how many arguments, or parameters, a
      list holds at most before it may always break, whatever
      `wrapArgsAt` says (see FormatArgsWrapRule and FormatParamWrapRule).
      A condition can instead decide per target using its argument count,
      remaining space, and scope; 3 without a `.formatrc`;
    * `wrapConditionsOver` (`[Structure]`): how many of the `&&` and `||`
      operands of an `if` or `while` the range that `wrapConditionsPer`
      names holds once the condition breaks (see FormatConditionLayout);
      past that number, every operator of the range breaks. A condition
      that holds gives 1, and one that does not gives 0 (as many as fit); a
      0 for the whole condition leaves it as it is. `spaceRemaining` is the
      share of `wrapMax` left after the range's `(`, or after the operand
      that may end a line. `spaceRemaining < 20% ? 1 : 2` without a
      `.formatrc`;
    * `wrapConditionsPer` (`[Structure]`): the range that
      `wrapConditionsOver` counts: `line`, the operands of each line;
      `pair`, those of the closest pair of parentheses, or of one argument
      when the operands are a call's argument; `pair-bundle`, those of the
      closest parentheses of a call, else of the `if` or `while`, nested
      groups included; or `pair-root`, every operand of the `if` or
      `while`. `pair-bundle` without a `.formatrc`;
    * `joinArgs` (`[Structure]`): how many arguments of a broken list may
      share a line, where the list would else give each argument a line of
      its own (see FormatExpressionLayout); a condition, evaluated at each
      argument that ends such a line (see FormatDocument::joinArgsAt()),
      may instead give the number, as in `hasMoreArgs() ? 2 : 1`; 1 without
      a `.formatrc`;
    * `wrapSubChain` (`[Structure]`): how a member after one joined to a
      closing-bracket line breaks: `always`, `wrap-begin`, `wrap-end`, or
      `never`; `always` without a `.formatrc`;
    * `target` (`[Structure]`): `long` (the default) keeps the line breaks
      that the code already has where they fit, and `any` lays the code out
      anew, as if each expression sat on one line (see FormatLineJoinRule);
      the command line's `--any-code` sets `any`;
    * `bodyBraces` (`[Structure]`): `true` (the default) lets
      FormatBodyBracesRule give a body that closes in a chain of brackets
      its own lines, and `false` leaves such a body as it is; a condition
      selects the bodies where the rule applies.

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
    Spells the default `[Relations] thisMode` (see FormatFileConfig).
*/
static const char defaultThisModeText[] = "is-base-member | already-existing | (is-outline & is-header)";

/*!
    Spells the default `[Structure] typedefSuper` (see FormatFileConfig).
*/
static const char defaultTypedefSuperText[] =
    "isNewClass() && isNeeded() ? always : never";

/*!
    Spells the default `[Structure] replaceSuperSites` (see FormatFileConfig).
*/
static const char defaultReplaceSuperSitesText[] =
    "isNewClass() && hasSuperTypedef()";

/*!
    Spells the default `[Structure] wrapChainAt`, which `[Structure]
    wrapArgsAt` follows (see FormatFileConfig).
*/
static const char defaultWrapChainAtText[] = "40%";

/*!
    Spells the default `[Structure] wrapSignatureAt` (see FormatFileConfig):
    87.5% of the percent `[Structure] wrapArgsAt` gives at the place.
*/
static const char defaultWrapSignatureAtText[] = "cfg.wrapArgsAt * 0.875";

/*!
    Spells the default `[Structure] wrapConditionsOver` (see
    FormatFileConfig): one operand per line once less than a fifth of
    `wrapMax` is left on the line, else two.
*/
static const char defaultWrapConditionsOverText[] =
    "spaceRemaining < 20% ? 1 : 2";

/*!
    Spells the default `[Lint] byRefVariable` (see FormatFileConfig): a
    warning while the project targets C++11, where some compilers mishandle
    a non-const reference local, and nothing otherwise.
*/
static const char defaultByRefVariableText[] = "hasStd(\"C++11\") ? warn : ignore";

/*!
    Builds the name of the settings files the tool reads in each folder.
*/
QString FormatFileConfig::fileName()
{
    return QString(QLL(".formatrc"));
}

QString FormatFileConfig::defaultExtensions()
{
    return QString(QLL("h,hh,hpp,hxx,h++,c,cc,cpp,cxx,c++,m,mm"));
}

QStringList FormatFileConfig::superConditionNames()
{
    return QStringList() << QString(QLL("isNewClass")) << QString(QLL("isNeeded"))
        << QString(QLL("hasSuperTypedef"));
}

/*!
    Builds the names that `[Replace.types] target` may use: `isNew`, which
    holds on a line `-L` or `--git` selects (every line, without them).
*/
QStringList FormatFileConfig::replaceTargetNames()
{
    return QStringList() << QString(QLL("isNew"));
}

QStringList FormatFileConfig::superChoices()
{
    return QStringList() << QString(QLL("always")) << QString(QLL("never"))
        << QString(QLL("keep-as-is")) << QString(QLL("if-needed"));
}

/*!
    Builds the levels a `[Lint]` key picks from: `ignore`, `warn`, and
    `error`.
*/
QStringList FormatFileConfig::lintLevels()
{
    return QStringList() << QString(QLL("ignore")) << QString(QLL("warn"))
        << QString(QLL("error"));
}

static bool extensionPattern(const QString &text, QRegularExpression *pattern)
{
    QString value = text.trimmed();
    if (value.startsWith(QLL("regexp/")) && value.endsWith(QLatin1Char('/'))) {
        value = value.mid(7, value.size() - 8);
    } else if (value.startsWith(QLatin1Char('/')) && value.endsWith(QLatin1Char('/'))
               && value.size() >= 2) {
        value = value.mid(1, value.size() - 2);
    } else if (value.startsWith(QLL("regexp:"))) {
        value.remove(0, 7);
    } else if (value.contains(QLatin1Char('^')) || value.contains(QLatin1Char('$'))
               || value.contains(QLatin1Char('|')) || value.contains(QLatin1Char('('))
               || value.contains(QLatin1Char('\\')) || value.contains(QLatin1Char('?'))
               || value.contains(QLatin1Char('{')) || value.contains(QLL(".*"))
               || value.contains(QLL(".+"))) {
    } else {
        return false;
    }
    *pattern = QRegularExpression(value, QRegularExpression::CaseInsensitiveOption);
    return true;
}

/*!
    Builds the items of the list @a text: separated by commas, with or
    without brackets around them (`[a, b]`), each trimmed, and without the
    double or single quotes around it.
*/
static QStringList listItems(const QString &text)
{
    QString value = text.trimmed();
    if (value.startsWith(QLatin1Char('[')) && value.endsWith(QLatin1Char(']'))) {
        value = value.mid(1, value.size() - 2);
    }
    QStringList items = value.split(QLatin1Char(','), QString::SkipEmptyParts);
    for (int i = 0; i < items.size(); ++i) {
        QString item = items.at(i).trimmed();
        if (item.size() >= 2
            && ((item.startsWith(QLatin1Char('"')) && item.endsWith(QLatin1Char('"')))
                || (item.startsWith(QLatin1Char('\'')) && item.endsWith(QLatin1Char('\''))))
        ) {
            item = item.mid(1, item.size() - 2);
        }
        items[i] = item;
    }
    return items;
}

static QStringList extensionList(const QString &text)
{
    QStringList extensions = listItems(text);
    for (int i = 0; i < extensions.size(); ++i) {
        QString extension = extensions.at(i);
        if (extension.startsWith(QLL("*."))) {
            extension.remove(0, 2);
        } else if (extension.startsWith(QLatin1Char('.'))) {
            extension.remove(0, 1);
        }
        extensions[i] = extension.toLower();
    }
    return extensions;
}

class FormatExtensionsMatcher : public Config::ValueMatcher
{
public:
    bool match(const QString &value, QString *matched) const Q_DECL_OVERRIDE
    {
        QRegularExpression pattern;
        if (extensionPattern(value, &pattern)) {
            if ( ! pattern.isValid()) {
                return false;
            }
        } else {
            const QStringList extensions = extensionList(value);
            if (extensions.isEmpty() && value.trimmed() != QLL("[]")) {
                return false;
            }
            for (int i = 0; i < extensions.size(); ++i) {
                if (extensions.at(i).isEmpty()
                    || extensions.at(i).contains(QLatin1Char('/'))
                    || extensions.at(i).contains(QLatin1Char('['))
                    || extensions.at(i).contains(QLatin1Char(']'))
                ) {
                    return false;
                }
            }
        }
        *matched = value;
        return true;
    }

    QString describe() const Q_DECL_OVERRIDE
    {
        return QString(QLL("a list of extensions or a /regular expression/"));
    }
};

/*!
    Matches a list of names (see listItems()), such as the C++ standards a
    project may target, `c++11` or `[c++11, c++17]`, each name matching a
    pattern; an empty list matches only where it is allowed.
*/
class FormatNameListMatcher : public Config::ValueMatcher
{
public:
    inline FormatNameListMatcher(const QString & pattern,
                                 const QString & description,
                                 bool allowsEmpty)
        : m_pattern(pattern), m_description(description)
        , m_allowsEmpty(allowsEmpty)
    {
    }

    bool match(const QString &value, QString *matched) const Q_DECL_OVERRIDE
    {
        const QStringList names = listItems(value);
        const QRegularExpression pattern(m_pattern);
        if (names.isEmpty() && ! m_allowsEmpty) {
            return false;
        }
        for (int i = 0; i < names.size(); ++i) {
            if ( ! pattern.match(names.at(i)).hasMatch()) {
                return false;
            }
        }
        *matched = value;
        return true;
    }

    QString describe() const Q_DECL_OVERRIDE
    {
        return m_description;
    }

private:
    QString m_pattern;
    QString m_description;
    bool m_allowsEmpty;
};

bool FormatFileConfig::acceptsFile(const QString &path) const
{
    const QFileInfo info(path);
    QRegularExpression pattern;
    if (extensionPattern(this->extensions, &pattern)) {
        return pattern.match(info.fileName()).hasMatch()
            || pattern.match(info.suffix()).hasMatch();
    }
    return extensionList(this->extensions).contains(info.suffix().toLower());
}

/*!
    Builds the schema of the settings the tool takes, each at its default.
*/
Config::Schema FormatFileConfig::schema()
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
    Config::Section general(QString(QLL("General")));
    general.addKey(QString(QLL("extensions")),
                   Config::Matchers() << QSharedPointer<const Config::ValueMatcher >(
                       new FormatExtensionsMatcher),
                   FormatFileConfig::defaultExtensions());
    schema.addSection(general);
    schema.addSection(types);
    Config::Section structure(QString(QLL("Structure")));
    structure.addKey(QString(QLL("typedefSuper")),
                     Config::Matchers() << QSharedPointer<const Config::ValueMatcher >(
                         new Config::Choice(FormatFileConfig::superConditionNames(),
                                            FormatFileConfig::superChoices())),
                     FormatFileConfig::defaultTypedefSuper());
    structure.addKey(QString(QLL("replaceSuperSites")),
                     Config::Matchers() << Config::Condition::create(
                         FormatFileConfig::superConditionNames()),
                     FormatFileConfig::defaultReplaceSuperSites());
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
    structure
        .addFollower(QString(QLL("wrapArgsAt")), QString(QLL("wrapChainAt")));
    structure.addKey(QString(QLL("wrapSignatureAt")),
                     Config::Matchers()
                         << Config::Condition::create(
                             FormatScopeConditions::names(),
                             true
                         ),
                     QString::fromLatin1(defaultWrapSignatureAtText));
    structure.addKey(QString(QLL("wrapArgsOver")),
                     Config::Matchers() << Config::Numbers::create(
                         1,
                         100,
                         1
                     ) << Config::Condition::create(
                         FormatScopeConditions::names(),
                         true,
                         &FormatConditionContext::setCountObjects
                     ),
                     QString(QLL("3")));
    structure.addKey(QString(QLL("wrapConditionsOver")),
                     Config::Matchers() << Config::Numbers::create(
                         1,
                         100,
                         1
                     ) << Config::Condition::create(
                         FormatScopeConditions::names(),
                         true,
                         &FormatConditionContext::setCountObjects
                     ),
                     QString::fromLatin1(defaultWrapConditionsOverText));
    structure.addKey(QString(QLL("wrapConditionsPer")),
                     QStringList() << QString(QLL("line"))
                         << QString(QLL("pair"))
                         << QString(QLL("pair-bundle"))
                         << QString(QLL("pair-root")),
                     QString(QLL("pair-bundle")));
    structure.addKey(QString(QLL("joinArgs")),
                     Config::Matchers() << Config::Numbers::create(
                         1,
                         100,
                         1
                     ) << Config::Condition::create(
                         FormatScopeConditions::names(),
                         true,
                         &FormatConditionContext::setCountObjects
                     ),
                     QString(QLL("1")));
    structure.addKey(QString(QLL("wrapSubChain")),
                     QStringList() << QString(QLL("always"))
                         << QString(QLL("wrap-begin"))
                         << QString(QLL("wrap-end"))
                         << QString(QLL("never")),
                     QString(QLL("always")));
    structure.addKey(QString(QLL("target")),
                     QStringList() << QString(QLL("long")) << QString(QLL("any")),
                     QString(QLL("long")));
    structure.addKey(QString(QLL("bodyBraces")),
                     Config::Matchers() << Config::Condition::create(
                         FormatScopeConditions::names()
                     ),
                     QString(QLL("true")));
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
    QHash<QString, QString > signs;
    signs.insert(QString(QLL("at")), QString(QLL("@")));
    signs.insert(QString(QLL("backslash")), QString(QLL("\\")));
    comments.addKey(
        QString(QLL("docTagSign")),
        QStringList() << QString(QLL("@")) << QString(QLL("\\")),
        QString(QLL("@")), signs
    );
    schema.addSection(comments);
    Config::Section relations(QString(QLL("Relations")));
    relations.addKey(QString(QLL("thisMode")),
                     Config::Matchers() << Config::Condition::create(FormatScopeConditions::names()),
                     QString::fromLatin1(defaultThisModeText));
    schema.addSection(relations);
    schema.addSection(structure);
    Config::Section replaceTypes(QString(QLL("Replace.types")));
    replaceTypes.addKey(QString(QLL("target")),
                        Config::Matchers() << Config::Condition::create(
                            FormatFileConfig::replaceTargetNames()),
                        QString(QLL("isNew")));
    replaceTypes.addKey(QString(QLL("action")),
                        QStringList() << QString(QLL("replace")) << QString(QLL("report")),
                        QString(QLL("replace")));
    replaceTypes.setOpen(Config::Matchers() << Config::Name::create(true)
                             << Config::Word::create(QString(QLL("keep-as-is"))));
    schema.addSection(replaceTypes);
    Config::Section code(QString(QLL("Code")));
    code.addKey(QString(QLL("nameMismatch")),
                QStringList() << QString(QLL("warn")) << QString(QLL("error"))
                    << QString(QLL("critical")),
                QString(QLL("warn")));
    code.addKey(QString(QLL("compilers")),
                Config::Matchers() << QSharedPointer<const Config::ValueMatcher >(
                    new FormatNameListMatcher(
                        QString(QLL("^[A-Za-z0-9+._-]+$")),
                        QString(QLL("a list of qmake specs, such as win32-msvc2010, or nothing")),
                        true
                    )),
                QString());
    schema.addSection(code);
    Config::Section lint(QString(QLL("Lint")));
    lint.addKey(QString(QLL("standards")),
                Config::Matchers() << QSharedPointer<const Config::ValueMatcher >(
                    new FormatNameListMatcher(
                        QString(QLL("^[A-Za-z0-9+]+$")),
                        QString(QLL("a list of C++ standards, such as c++11 or [c++11, c++17]")),
                        false
                    )),
                QString(QLL("c++11")));
    lint.addKey(QString(QLL("byRefVariable")),
                Config::Matchers() << QSharedPointer<const Config::ValueMatcher >(
                    new Config::Choice(QStringList(), FormatFileConfig::lintLevels())),
                QString::fromLatin1(defaultByRefVariableText));
    lint.addKey(QString(QLL("inlineOverride")),
                Config::Matchers() << Config::Name::create()
                    << Config::Word::create(QString())
                    << QSharedPointer<const Config::ValueMatcher >(
                        new Config::Choice(QStringList(), QStringList())),
                QString(QLL("override")));
    schema.addSection(lint);
    return schema;
}

/*!
    Logs @a error, a condition that did not compile; @a mismatched tells a
    name mismatch, which `[Code] nameMismatch = error` logs as an error.
*/
static void reportCondition(const QString & error, bool mismatched)
{
    if (mismatched) {
        qCritical("Invalid format condition: %s", qPrintable(error));
        return;
    }
    qWarning("Invalid format condition: %s", qPrintable(error));
}

/*!
    Compiles the condition @a text into @a program, over the names of
    FormatScopeConditions, and over percents when @a takesPercents is set;
    a condition that does not compile leaves @a program at its default, and
    is logged. A name that differs from a known one in case alone is
    handled as @a mismatch says; for `critical`, returns false with
    @a error, and the file keeps its text.
*/
static bool compileCondition(const QString & text, QStringList *program,
                             bool takesPercents = false,
                             Config::Condition::NameMismatch mismatch
                                 = Config::Condition::NameMismatchWarn,
                             QString *error = Q_NULLPTR)
{
    QString reason;
    QStringList compiled;
    bool mismatched = false;
    if ( ! Config::Condition::compile(
            text,
            FormatScopeConditions::names(),
            &compiled,
            &reason,
            takesPercents,
            mismatch,
            &mismatched
        )
    ) {
        if (mismatched
            && mismatch == Config::Condition::NameMismatchCritical
        ) {
            *error = reason;
            return false;
        }
        reportCondition(reason, mismatched);
        return true;
    }
    *program = compiled;
    return true;
}

/*!
    Builds the default `[Structure] typedefSuper`, for a file whose
    `.formatrc` files leave it unset.
*/
QString FormatFileConfig::defaultTypedefSuper()
{
    return QString::fromLatin1(defaultTypedefSuperText);
}

/*!
    Builds the default `[Structure] replaceSuperSites`, for files no
    `.formatrc` sets it for.
*/
QString FormatFileConfig::defaultReplaceSuperSites()
{
    return QString::fromLatin1(defaultReplaceSuperSitesText);
}

/*!
    Builds the program of the default `[Relations] thisMode`, for settings
    that no `.formatrc` set.
*/
QStringList FormatFileConfig::defaultThisMode()
{
    QStringList program;
    compileCondition(QString::fromLatin1(defaultThisModeText), &program);
    return program;
}

/*!
    Builds the program of the default `[Structure] wrapChainAt`, which
    `[Structure] wrapArgsAt` follows, for settings that no `.formatrc` set,
    and for a use site that sets no percent.
*/
QStringList FormatFileConfig::defaultWrapChainAt()
{
    QStringList program;
    compileCondition(QString::fromLatin1(defaultWrapChainAtText), &program, true);
    return program;
}

/*!
    Builds the program of the default `[Structure] wrapSignatureAt`, for
    settings that no `.formatrc` set, and where a condition misses a value.
*/
QStringList FormatFileConfig::defaultWrapSignatureAt()
{
    QStringList program;
    compileCondition(
        QString::fromLatin1(defaultWrapSignatureAtText),
        &program,
        true
    );
    return program;
}

/*!
    Builds the program of the default `[Structure] wrapConditionsOver`, for
    settings that no `.formatrc` set.
*/
QStringList FormatFileConfig::defaultWrapConditionsOver()
{
    QStringList program;
    compileCondition(
        QString::fromLatin1(defaultWrapConditionsOverText),
        &program,
        true
    );
    return program;
}

/*!
    Reads the command line's settings @a config into @a parser, after any
    `.formatrc` it read: the `--config-add` files, then the `-c` assignments;
    returns false, with @a error, for a setting outside of the schema.
*/
bool FormatFileConfig::readConfig(
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
    Reads the settings of the file at @a path into @a fileConfig: the
    `.formatrc` files from its project's root down to its folder (or the
    `--config-file` of @a config instead), then the rest of @a config;
    returns false, with @a error naming the file, the key, and the values it
    takes, for a setting the schema does not take.
*/
bool FormatFileConfig::load(const QString & path, const FormatConfig & config,
                          FormatFileConfig *fileConfig, QString *error)
{
    Config::Parser parser(FormatFileConfig::schema());
    if ( ! FormatFileConfig::readParser(path, config, &parser, error)) {
        return false;
    }
    QString reason;
    if ( ! FormatFileConfig::fromParser(parser, fileConfig, &reason)) {
        *error = QString(QLL("%1: [Code] nameMismatch = critical: %2"))
            .arg(path, reason);
        return false;
    }
    return true;
}

/*!
    Reads the settings for @a path and @a config into @a parser, reporting
    validation errors in @a error.
*/
bool FormatFileConfig::readParser(const QString &path, const FormatConfig &config,
                                Config::Parser *parser, QString *error)
{
    if ( ! config.file.isEmpty()) {
        if ( ! parser->readFile(config.file, error)) {
            return false;
        }
    } else if ( ! path.isEmpty()
        && ! parser->readLevels(path, FormatFileConfig::fileName(), error)
    ) {
        return false;
    }
    if ( ! FormatFileConfig::readConfig(parser, config, error)) {
        return false;
    }
    return true;
}

/*!
    Tells whether @a program, which Config::Condition::compile() made, is a
    constant: numbers and signs alone, no name and no call.
*/
static bool isConstant(const QStringList & program)
{
    if (program.isEmpty() || program.first() != QLL("@expression")) {
        return false;
    }
    for (int i = 1; i < program.size(); ++i) {
        const QString & item = program.at(i);
        if ( ! item.startsWith(QLL("n:")) && ! item.startsWith(QLL("op:"))
            && ! item.startsWith(QLL("jfalse:"))
            && ! item.startsWith(QLL("jtrue:"))
            && ! item.startsWith(QLL("jelse:"))
            && ! item.startsWith(QLL("goto:"))
        ) {
            return false;
        }
    }
    return true;
}

/*!
    Reads @a value, a setting that takes a count or a condition: a count,
    or math that gives one (such as `1 + 7 - 3`), goes to @a count and
    clears @a program; any other condition is compiled into @a program,
    which then decides in place of the count. Returns false, with @a error,
    when a name mismatch fails it under @a mismatch `critical`.
*/
static bool readCountOrCondition(
        const QString & value, int *count,
        QStringList *program,
        Config::Condition::NameMismatch mismatch, QString *error)
{
    bool isNumber = false;
    const int number = value.toInt(&isNumber);
    if (isNumber) {
        *count = number;
        program->clear();
        return true;
    }
    const QSharedPointer<Config::Scopes::Scope > scope =
        FormatConditionContext::pushCountScope();
    if ( ! compileCondition(value, program, true, mismatch, error)) {
        return false;
    }
    qreal constant = 0;
    if (isConstant(*program)
        && Config::Condition::value(
            *program,
            QSet<QString >(),
            Q_NULLPTR,
            &constant
        )
    ) {
        *count = int(constant);
        program->clear();
    }
    return true;
}

/*!
    Builds @a entries, the lines of `[Replace.types]` in their order, with
    each `&` of a key expanded: a key that starts with `&` takes the key of
    the latest line that does not, in place of that `&`. A second `&` in a
    key sets what a later `&` stands for to the expanded text before it, and
    drops out; hence `&::Inner&::make*` under `::A` is `::A::Inner::make*`,
    and a later `&` stands for `::A::Inner`.
*/
QList<QPair<QString, QString > > FormatFileConfig::expandPrevious(
        const QList<QPair<QString, QString > > & entries)
{
    QList<QPair<QString, QString > > expanded;
    QString previous;
    for (int i = 0; i < entries.size(); ++i) {
        QString key = entries.at(i).first;
        if ( ! key.startsWith(QLatin1Char('&'))) {
            previous = key;
        } else {
            key = previous + key.mid(1);
            const int second = key.indexOf(
                QLatin1Char('&'),
                previous.size()
            );
            if (second >= 0) {
                previous = key.left(second);
                key.remove(second, 1);
            }
        }
        expanded.append(qMakePair(key, entries.at(i).second));
    }
    return expanded;
}

/*!
    Checks the condition @a text over @a names, as @a mismatch says (see
    compileCondition()), for a setting that a rule compiles later: on a
    name mismatch, `error` puts @a defaultValue in @a text, and `critical`
    returns false with @a error.
*/
static bool checkCondition(QString *text, const QStringList & names,
                           const QString & defaultValue,
                           Config::Condition::NameMismatch mismatch,
                           QString *error)
{
    QStringList program;
    QString reason;
    bool mismatched = false;
    if (Config::Condition::compile(
            *text,
            names,
            &program,
            &reason,
            false,
            mismatch,
            &mismatched
        )
        || ! mismatched
    ) {
        return true;
    }
    if (mismatch == Config::Condition::NameMismatchCritical) {
        *error = reason;
        return false;
    }
    reportCondition(reason, true);
    *text = defaultValue;
    return true;
}

/*!
    Converts the validated values in @a parser to @a fileConfig. The
    conditions handle a name that differs from a known one in case alone as
    `[Code] nameMismatch` says (see Config::Condition::NameMismatch);
    returns false, with @a error, for such a name under `critical`.
*/
bool FormatFileConfig::fromParser(
        const Config::Parser &parser,
        FormatFileConfig *fileConfig, QString *error)
{
    const Config::Condition::NameMismatch mismatch =
        Config::Condition::nameMismatch(parser.value(QString(QLL("code.nameMismatch"))));
    fileConfig->nameMismatch = mismatch;
    fileConfig->standards = listItems(parser.value(QString(QLL("lint.standards"))));
    fileConfig->compilers = listItems(parser.value(QString(QLL("code.compilers"))));
    fileConfig->inlineOverride = FormatFileConfig::inlineMarker(
        parser.value(QString(QLL("lint.inlineOverride"))),
        fileConfig->standards,
        fileConfig->compilers
    );
    fileConfig->byRefVariable = FormatFileConfig::lintLevel(
        parser.value(QString(QLL("lint.byRefVariable"))),
        fileConfig->standards,
        fileConfig->compilers
    );
    fileConfig->extensions = parser.value(QString(QLL("general.extensions")));
    fileConfig->typedefSuper = parser.value(QString(QLL("structure.typedefSuper")));
    fileConfig->replaceSuperSites = parser.value(QString(QLL("structure.replaceSuperSites")));
    fileConfig->replaceTypesTarget = parser.value(
        QString(QLL("replace.types.target"))
    );

    // Reports the mismatches of the conditions that rules compile later;
    // those rules then use the names meant.
    bool mismatched = false;
    QString reason;
    if ( ! Config::Choice::check(
            fileConfig->typedefSuper,
            FormatFileConfig::superConditionNames(),
            FormatFileConfig::superChoices(),
            mismatch,
            &reason,
            &mismatched
        )
        && mismatched
    ) {
        if (mismatch == Config::Condition::NameMismatchCritical) {
            *error = reason;
            return false;
        }
        reportCondition(reason, true);
        fileConfig->typedefSuper = FormatFileConfig::defaultTypedefSuper();
    }
    if ( ! checkCondition(
            &fileConfig->replaceSuperSites,
            FormatFileConfig::superConditionNames(),
            FormatFileConfig::defaultReplaceSuperSites(),
            mismatch,
            error
        )
        || ! checkCondition(
            &fileConfig->replaceTypesTarget,
            FormatFileConfig::replaceTargetNames(),
            parser.schema().key(QString(QLL("replace.types.target")))
                .defaultValue,
            mismatch,
            error
        )
    ) {
        return false;
    }
    fileConfig->replaceTypesAction = parser.value(
        QString(QLL("replace.types.action"))
    );
    fileConfig->replaceTypes = FormatFileConfig::expandPrevious(
        parser.openEntries(QString(QLL("replace.types"))));
    const Config::Schema &schema = parser.schema();
    const QString refSuffix = QString(QLL("types.refSuffix"));
    const QString moveSuffix = QString(QLL("types.moveSuffix"));
    fileConfig->refSuffix = FormatFileConfig::toSpacing(
        parser.value(refSuffix),
        schema.key(refSuffix).defaultValue
    );
    fileConfig->moveSuffix = FormatFileConfig::toSpacing(
        parser.value(moveSuffix),
        schema.key(moveSuffix).defaultValue
    );
    fileConfig->wrapBegin
        = Config::Numbers::parse(parser.value(QString(QLL("structure.wrapBegin"))));
    fileConfig->wrapMax = parser.value(QString(QLL("structure.wrapMax"))).toInt();
    const int scopes = Config::Scopes::count();
    // Lays out one operand per line where a condition misses a value.
    fileConfig->wrapConditionsOver = 1;
    const bool counted = readCountOrCondition(
        parser.value(QString(QLL("structure.wrapArgsOver"))),
        &fileConfig->wrapArgsOver,
        &fileConfig->wrapArgsOverCondition, mismatch, error
    )
        && readCountOrCondition(
            parser.value(QString(QLL("structure.wrapConditionsOver"))),
            &fileConfig->wrapConditionsOver,
            &fileConfig->wrapConditionsOverCondition, mismatch, error
        )
        && readCountOrCondition(
            parser.value(QString(QLL("structure.joinArgs"))),
            &fileConfig->joinArgs, &fileConfig->joinArgsCondition, mismatch,
            error
        );
    Config::Scopes::popUntil(scopes);
    if ( ! counted) {
        return false;
    }
    fileConfig->wrapSubChain = parser.value(QString(QLL("structure.wrapSubChain")));
    fileConfig->wrapConditionsPer = parser.value(
        QString(QLL("structure.wrapConditionsPer"))
    );
    fileConfig->commentWrapBegin
        = Config::Numbers::parse(parser.value(QString(QLL("comments.wrapBegin"))));
    fileConfig->commentWrapMax = parser
        .value(QString(QLL("comments.wrapMax")))
        .toInt();
    fileConfig->wrapExtendsForKeywords
        = parser.value(QString(QLL("comments.wrapExtendsForKeywords"))).toInt();
    fileConfig->wrapEmptyLimit = parser
        .value(QString(QLL("comments.wrapEmptyLimit")))
        .toInt();
    fileConfig->wrapComma = parser.value(QString(QLL("comments.wrapComma"))) == QLL("hard")
        ? WrapCommaHard : WrapCommaSoft;
    fileConfig->commentTarget = parser.value(QString(QLL("comments.target"))) == QLL("any")
        ? CommentTargetAny : CommentTargetLong;
    fileConfig->codeTarget = parser.value(QString(QLL("structure.target"))) == QLL("any")
        ? CodeTargetAny : CodeTargetLong;
    fileConfig->docTagSign = parser.value(QString(QLL("comments.docTagSign"))) == QLL("\\")
        ? '\\' : '@';
    return compileCondition(
        parser.value(QString(QLL("structure.bodyBraces"))),
        &fileConfig->bodyBraces,
        false,
        mismatch,
        error
    )
        && compileCondition(
            parser.value(QString(QLL("relations.thisMode"))),
            &fileConfig->thisMode,
            false,
            mismatch,
            error
        )
        && compileCondition(
            parser.value(QString(QLL("structure.wrapChainAt"))),
            &fileConfig->wrapChainAt,
            true,
            mismatch,
            error
        )
        && compileCondition(
            parser.value(QString(QLL("structure.wrapArgsAt"))),
            &fileConfig->wrapArgsAt,
            true,
            mismatch,
            error
        )
        && compileCondition(
            parser.value(QString(QLL("structure.wrapSignatureAt"))),
            &fileConfig->wrapSignatureAt,
            true,
            mismatch,
            error
        );
}

namespace {

/*!
    Answers `hasStd()` of a `[Lint]` key's choice from the C++ standards the
    project targets, `[Lint] standards`, and `hasCompiler()` from the
    compilers it builds with, `[Code] compilers`, both in any case.
*/
class FormatLintContext : public FormatConditionContext
{
    typedef FormatConditionContext super;
public:
    inline FormatLintContext(const QStringList & standards,
                             const QStringList & compilers)
        : m_standards(standards), m_compilers(compilers) {}

    inline bool hasStandard(const QString & standard,
                            bool *value) const Q_DECL_OVERRIDE_IL
    {
        *value = m_standards.contains(standard, Qt::CaseInsensitive);
        return true;
    }

    inline bool hasCompiler(const QString & compiler,
                            bool *value) const Q_DECL_OVERRIDE_IL
    {
        *value = m_compilers.contains(compiler, Qt::CaseInsensitive);
        return true;
    }

private:
    QStringList m_standards;
    QStringList m_compilers;
};

} // namespace

/*!
    Builds the marker that the `[Lint] inlineOverride` value @a value names:
    a name, nothing, or a choice of texts in double quotes, where `hasStd()`
    answers from @a standards and `hasCompiler()` from @a compilers, such as
    `hasCompiler("win32-msvc2010") ? "Q_DECL_OVERRIDE_IL" : "override"`. A
    choice that cannot pick one is logged, and gives `override`.
*/
QString FormatFileConfig::inlineMarker(
        const QString & value, const QStringList & standards,
        const QStringList & compilers)
{
    const QString text = value.trimmed();
    QString matched;
    if (text.isEmpty() || Config::Name(false).match(text, &matched)) {
        return text;
    }
    const FormatLintContext context(standards, compilers);
    QString marker;
    QString error;
    if ( ! Config::Choice::resolve(text, QStringList(), QStringList(),
                                   &context, &marker, &error)
    ) {
        qWarning("Invalid [Lint] inlineOverride: %s", qPrintable(error));
        return QString(QLL("override"));
    }
    return marker.trimmed();
}

/*!
    Builds the level that the `[Lint]` choice @a value picks (see
    lintLevels()), where `hasStd()` answers from @a standards and
    `hasCompiler()` from @a compilers; a choice that cannot pick one is
    logged, and gives `warn`.
*/
FormatFileConfig::LintLevel FormatFileConfig::lintLevel(
        const QString & value, const QStringList & standards,
        const QStringList & compilers)
{
    const FormatLintContext context(standards, compilers);
    QString level;
    QString error;
    if ( ! Config::Choice::resolve(value, QStringList(), FormatFileConfig::lintLevels(),
                                   &context, &level, &error)
    ) {
        qWarning("Invalid [Lint] level: %s", qPrintable(error));
        return LintWarn;
    }
    if (level == QLL("ignore")) {
        return LintIgnore;
    }
    return level == QLL("error") ? LintError : LintWarn;
}

/*!
    Builds the spacing of @a value, one of the values schema() lets a
    spacing key take: `keep-as-is`, the spaces and/or tabs to write, or a
    `regexp/<pattern>/<replacement>/`, whose spacing where the pattern
    matches nothing is the fallback after its `|` (empty for no space), or
    without a `|` the key's @a defaultValue.
*/
FormatSpacing FormatFileConfig::toSpacing(
        const QString & value,
        const QString & defaultValue)
{
    QString head;
    QString fallback;
    bool hasFallback = false;
    if (Config::RegExp::split(value, &head, &fallback, &hasFallback)) {
        FormatSpacing spacing = FormatFileConfig::toSpacing(hasFallback
                ? fallback : defaultValue,
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
