// Copyright (C) 2015 The XD Company Ltd.
// SPDX-License-Identifier: Apache-2.0

#include "./format-file-config.h"

#include "./parser/condition-context.h"
#include "./parser/scope-conditions.h"
#include "./rules/template-object.h"

#include "config-any-space.h"
#include "config-cache.h"
#include "config-choice.h"
#include "config-code.h"
#include "config-condition.h"
#include "config-scopes.h"
#include "config-name.h"
#include "config-numbers.h"
#include "config-regexp.h"
#include "config-word.h"

#include <QtCore/QFileInfo>
#include <QtCore/QRegularExpression>
#include <QtCore/qfunction.h>

#include <limits>

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
      a `.formatrc`;
    * `wrapChainAt` (`[Structure]`): where an operator chain that does not
      fit one line breaks before its operators (see FormatExpressionLayout):
      a condition of the same names, which may compare `spaceUsed` (the
      share of `wrapMax` used up to where an operator starts) with
      percents, such as
      `(spaceUsed > 40% & is-non-header) | (spaceUsed > 30% & is-header)`;
      the operators before it stay on the chain's first line.
      `spaceUsed > 40%` without a `.formatrc`;
    * `wrapArgsAt` (`[Structure]`): where a call whose arguments do not fit
      one line breaks after its `(`, one argument per line (see
      FormatExpressionLayout): a condition like `wrapChainAt`, where `spaceUsed`
      is the share of `wrapMax` used up to the `(`. It
      follows `wrapChainAt` until a `.formatrc` sets it, since an argument
      list is a chain of commas;
    * `wrapSignatureAt` (`[Structure]`): `wrapArgsAt` for the parameter
      list of a declaration or definition (a function, a method, or a
      constructor), which past `wrapMax` starts on the next line where it
      holds (see FormatExpressionLayout); the other `wrapArgs` keys still
      apply there. `spaceUsed > cfg.wrapArgsAt * 0.875` without a
      `.formatrc`: past 87.5% of the share `wrapArgsAt` gives at the place;
    * `wrapArgsOver` (`[Structure]`): how many arguments, or parameters, a
      list holds at most before it may always break, whatever
      `wrapArgsAt` says (see FormatExpressionLayout).
      A condition can instead decide per target using its argument count,
      remaining space, and scope; 3 without a `.formatrc`;
    * `wrapConditionsOver` (`[Structure]`): how many of the `&&` and `||`
      operands of an `if` or `while` the range that `wrapConditionsPer`
      names holds once the condition breaks (see FormatConditionLayout);
      past that number, every operator of the range breaks.
      A condition that holds gives 1, and one that does not gives 0
      (as many as fit); a 0 for the whole condition leaves it as it is.
      `spaceRemaining` is the share of `wrapMax` left after the range's `(`,
      or after the operand that may end a line.
      `spaceRemaining < 20% || count(cfg.wrapConditionsPer) > 2 ? 1 : 2`
      without a `.formatrc`;
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
    * `operatorPadding` (`[Structure]`): extra indentation steps for a
      wrapped call in an operator chain; every placement but `middle`
      takes one step without a `.formatrc`;
    * `wrapSubChain` (`[Structure]`): how a member after one joined to a
      closing-bracket line breaks: `always`, `wrap-begin`, `wrap-end`, or
      `never`; `always` without a `.formatrc`;
    * `target` (`[Structure]`): `long` (the default) keeps the line breaks
      that the code already has where they fit, and `any` lays the code out
      anew, as if each expression sat on one line;
      the command line's `--any-code` sets `any`;
    * `wrapMistakes` (`[Structure]`): how a list the user broke over lines
      is laid out anew when one of its lines strays from the average count
      of items per line (see FormatExpressionLayout): `average`, that count per
      line; a number, such as `cfg.joinArgs`, that many per line; or `never`,
      the user's lines stay. A condition picks it at each list, such as
      `is(array) ? average : never`, the default, where `is()` asks the list
      in question (see FormatConditionObject);
    * `compact` (`[Structure]`): `always` lets a call that ends the
      condition of an `if` or a `while`, its `(` on the keyword's line, close
      the condition right after its own `)`, as `)) {` (see
      FormatExpressionLayout); `never` (the
      default) closes the condition on a line of its own, which changes fewer
      lines of existing code;
    * `bodyBraces` (`[Structure]`): `true` (the default) lets
      a body that closes in a chain of brackets move to
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
static const char defaultWrapChainAtText[] = "spaceUsed > 40%";

/*!
    Spells the default `[Structure] wrapSignatureAt` (see FormatFileConfig):
    past 87.5% of the share `[Structure] wrapArgsAt` gives at the place.
*/
static const char defaultWrapSignatureAtText[] =
    "spaceUsed > cfg.wrapArgsAt * 0.875";

/*!
    Spells the default `[Structure] wrapConditionsOver` (see
    FormatFileConfig): one operand per line once less than a fifth of
    `wrapMax` is left on the line or the targeted range holds over two
    operands, else two.
*/
static const char defaultWrapConditionsOverText[] =
    "spaceRemaining < 20% || count(cfg.wrapConditionsPer) > 2 ? 1 : 2";

/*!
    Spells the default `[Structure] operatorPadding` (see
    FormatFileConfig): a step for a call wrapped at the start of an
    operator chain, and none where the line puts the operator in the middle.
*/
static const char defaultOperatorPaddingText[] =
    "cfg.operatorPlacement == \"middle\" ? 0 : 1";

/*!
    Spells the default `[Structure] wrapMistakes` (see FormatFileConfig): a
    brace list takes the average count of items per line, any other list
    keeps the user's lines.
*/
static const char defaultWrapMistakesText[] =
    "is(array) ? \"average\" : \"never\"";

/*!
    Spells the default `[Lint] byRefVariable` (see FormatFileConfig): a
    warning while the project targets C++11, where some compilers mishandle
    a non-const reference local, and nothing otherwise.
*/
static const char defaultByRefVariableText[] =
    "hasStd(\"C++11\") ? warn : ignore";

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
    return QStringList() << QString(QLL("isNewClass"))
        << QString(QLL("isNeeded"))
        << QString(QLL("hasSuperTypedef"));
}

QStringList FormatFileConfig::superChoices()
{
    return QStringList() << QString(QLL("always"))
        << QString(QLL("never"))
        << QString(QLL("keep-as-is"))
        << QString(QLL("if-needed"));
}

/*!
    Builds the levels a `[Lint]` key picks from: `ignore`, `warn`, and
    `error`.
*/
QStringList FormatFileConfig::lintLevels()
{
    return QStringList() << QString(QLL("ignore"))
        << QString(QLL("warn"))
        << QString(QLL("error"));
}

static bool extensionPattern(const QString &text, QRegularExpression *pattern)
{
    QString value = text.trimmed();
    if (value.startsWith(QLL("regexp/"))
        && value.endsWith(QLatin1Char('/'))
    ) {
        value = value.mid(7, value.size() - 8);
    } else if (value.startsWith(QLatin1Char('/'))
        && value.endsWith(QLatin1Char('/'))
        && value.size() >= 2
    ) {
        value = value.mid(1, value.size() - 2);
    } else if (value.startsWith(QLL("regexp:"))) {
        value.remove(0, 7);
    } else if (value.contains(QLatin1Char('^'))
        || value.contains(QLatin1Char('$'))
        || value.contains(QLatin1Char('|'))
        || value.contains(QLatin1Char('('))
        || value.contains(QLatin1Char('\\'))
        || value.contains(QLatin1Char('?'))
        || value.contains(QLatin1Char('{'))
        || value.contains(QLL(".*"))
        || value.contains(QLL(".+"))
    ) {
    } else {
        return false;
    }
    *pattern = QRegularExpression(
        value,
        QRegularExpression::CaseInsensitiveOption
    );
    return true;
}

/*!
    Builds the items of the list @a text: separated by commas, with or
    without brackets around them (`[a, b]`), each trimmed, and without the
    double or single quotes around it.
*/
QStringList FormatFileConfig::listItems(const QString &text)
{
    QString value = text.trimmed();
    if (value.startsWith(QLatin1Char('['))
        && value.endsWith(QLatin1Char(']'))
    ) {
        value = value.mid(1, value.size() - 2);
    }
    QStringList items = value.split(
        QLatin1Char(','),
        QString::SkipEmptyParts
    );
    for (int i = 0; i < items.size(); ++i) {
        QString item = items.at(i).trimmed();
        if (item.size() >= 2
            && ((item.startsWith(QLatin1Char('"'))
                && item.endsWith(QLatin1Char('"')))
                || (item.startsWith(QLatin1Char('\''))
                    && item.endsWith(QLatin1Char('\'')))
            )
        ) {
            item = item.mid(1, item.size() - 2);
        }
        items[i] = item;
    }
    return items;
}

static QStringList extensionList(const QString &text)
{
    QStringList extensions = FormatFileConfig::listItems(text);
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

    inline FormatNameListMatcher(
            const QString & pattern,
            const QString & description,
            bool allowsEmpty)
        : m_pattern(pattern), m_description(description)
        , m_allowsEmpty(allowsEmpty)
    {
    }

    inline bool match(const QString &value,
                      QString *matched) const Q_DECL_OVERRIDE_IL
    {
        const QStringList names = FormatFileConfig::listItems(value);
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

    inline QString describe() const Q_DECL_OVERRIDE_IL
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
    Matches a call replacement or an expression that builds one.
*/
class FormatReplacementMatcher : public Config::ValueMatcher
{
    typedef Config::ValueMatcher super;
public:

    bool match(const QString & value, QString *matched) const Q_DECL_OVERRIDE;
    QString describe() const Q_DECL_OVERRIDE;
    QString explain(const QString & value) const Q_DECL_OVERRIDE;
};

/*!
    Matches the replacement expression @a value into @a matched.
*/
bool FormatReplacementMatcher::match(
        const QString & value, QString *matched) const
{
    if ( ! this->explain(value).isEmpty()) {
        *matched = QString();
        return false;
    }
    *matched = value.trimmed();
    return true;
}

/*!
    Describes the accepted replacement expression.
*/
QString FormatReplacementMatcher::describe() const
{
    return QString(QLL("a call name, template, or replacement expression"));
}

/*!
    Describes why the replacement expression @a value does not compile.
*/
QString FormatReplacementMatcher::explain(const QString & value) const
{
    const QSharedPointer<Config::Scopes::Scope > scope =
        FormatTemplateObject::argumentScope(value);
    Q_UNUSED(scope);
    const QStringList words;
    const Config::Evaluator::CompileContext context(
        FormatReplaceTypesContext().names(),
        &words,
        Config::NameMismatchError
    );
    Config::Condition::compile(value, &context);
    if (context.lastCompiled()) {
        return QString();
    }
    QString matched;
    if (Config::Name(true).match(value, &matched)) {
        return QString();
    }
    return context.lastErrorForCompile();
}

class FormatSchema : public Config::Schema
{
    Q_DISABLE_COPY(FormatSchema)
public:

    FormatSchema();
};

/*!
    Builds the schema of the settings the tool takes, each at its default,
    which its code compiles over the names of the tool's conditions (see
    Config::Code); a choice spells its word in double quotes, and the matcher
    of its key checks it afterwards (see Config::Choice).
*/
FormatSchema::FormatSchema()
{
    const Config::Matchers spacings = Config::Matchers()
        << Config::AnySpace::create()
        << Config::Word::create(QString(QLL("keep-as-is")))
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
    this->setNames(
        FormatScopeConditions::names() + FormatFileConfig::superConditionNames()
    );
    Config::Section general(QString(QLL("General")));
    general.addKey(
        QString(QLL("extensions")),
        Config::Matchers() << QSharedPointer<const Config::ValueMatcher >(
            new FormatExtensionsMatcher
        ),
        FormatFileConfig::defaultExtensions()
    );
    this->addSection(general);
    this->addSection(types);
    Config::Section structure(QString(QLL("Structure")));
    structure.addKey(
        QString(QLL("typedefSuper")),
        Config::Matchers() << QSharedPointer<const Config::ValueMatcher >(
            new Config::Choice(
                FormatFileConfig::superConditionNames(),
                FormatFileConfig::superChoices()
            )
        ),
        FormatFileConfig::defaultTypedefSuper()
    );
    structure.addKey(
        QString(QLL("replaceSuperSites")),
        Config::Matchers() << Config::Boolean::create(),
        FormatFileConfig::defaultReplaceSuperSites()
    );
    structure.addKey(
        QString(QLL("wrapBegin")),
        Config::Matchers() << Config::Numbers::create(20, 200),
        QString(QLL("75"))
    );
    structure.addKey(
        QString(QLL("wrapMax")),
        Config::Matchers() << Config::Numbers::create(20, 200, 1),
        QString(QLL("80"))
    );
    structure.addKey(
        QString(QLL("wrapChainAt")),
        Config::Matchers() << Config::Boolean::create(),
        Config::Code(defaultWrapChainAtText),
        QHash<QString, QString >(),
        Q_NULLPTR,
        true
    );
    structure.addFollower(
        QString(QLL("wrapArgsAt")),
        QString(QLL("wrapChainAt"))
    );
    structure.addKey(
        QString(QLL("wrapSignatureAt")),
        Config::Matchers() << Config::Boolean::create(),
        Config::Code(defaultWrapSignatureAtText),
        QHash<QString, QString >(),
        Q_NULLPTR,
        true
    );
    structure.addKey(
        QString(QLL("wrapArgsOver")),
        Config::Matchers() << Config::Boolean::create()
            << Config::Numbers::create(
                1,
                std::numeric_limits<qint64 >::max(),
                1
            ),
        QString(QLL("3")),
        QHash<QString, QString >(),
        &FormatConditionContext::setCountObjects
    );
    structure.addKey(
        QString(QLL("wrapConditionsOver")),
        Config::Matchers() << Config::Numbers::create(
            0,
            std::numeric_limits<qint64 >::max(),
            1
        ),
        Config::Code(defaultWrapConditionsOverText),
        QHash<QString, QString >(),
        &FormatConditionContext::setCountObjects
    );
    structure.addKey(
        QString(QLL("wrapConditionsPer")),
        QStringList() << QString(QLL("line"))
            << QString(QLL("pair"))
            << QString(QLL("pair-bundle"))
            << QString(QLL("pair-root")),
        QString(QLL("pair-bundle"))
    );
    structure.addKey(
        QString(QLL("joinArgs")),
        Config::Matchers() << Config::Numbers::create(
            1,
            std::numeric_limits<qint64 >::max(),
            1
        ),
        QString(QLL("1")),
        QHash<QString, QString >(),
        &FormatConditionContext::setCountObjects
    );
    structure.addKey(
        QString(QLL("operatorPadding")),
        Config::Matchers() << Config::Numbers::create(
            0,
            std::numeric_limits<qint64 >::max(),
            1
        ),
        Config::Code(defaultOperatorPaddingText),
        QHash<QString, QString >(),
        &FormatConditionContext::setCountObjects
    );
    structure.addKey(
        QString(QLL("operatorPlacement")),
        QStringList() << QString(QLL("start")) << QString(QLL("middle")),
        QString(QLL("start"))
    );
    structure.addKey(
        QString(QLL("wrapSubChain")),
        QStringList() << QString(QLL("always"))
            << QString(QLL("wrap-begin"))
            << QString(QLL("wrap-end"))
            << QString(QLL("never")),
        QString(QLL("always"))
    );
    structure.addKey(
        QString(QLL("target")),
        QStringList() << QString(QLL("long")) << QString(QLL("any")),
        QString(QLL("long"))
    );
    structure.addKey(
        QString(QLL("compact")),
        QStringList() << QString(QLL("never")) << QString(QLL("always")),
        QString(QLL("never"))
    );
    structure.addKey(
        QString(QLL("wrapMistakes")),
        Config::Matchers() << QSharedPointer<const Config::ValueMatcher >(
            new Config::Choice(
                FormatScopeConditions::names(),
                FormatFileConfig::wrapMistakesWords()
            )
        ),
        Config::Code(defaultWrapMistakesText)
    );
    structure.addKey(
        QString(QLL("bodyBraces")),
        Config::Matchers() << Config::Boolean::create(),
        QString(QLL("true"))
    );
    Config::Section comments(QString(QLL("Comments")));
    comments.extendPrefixed(structure, QString(QLL("wrap")));
    comments.addKey(
        QString(QLL("wrapExtendsForKeywords")),
        Config::Matchers() << Config::Numbers::create(0, 100, 1),
        QString(QLL("20"))
    );
    comments.addKey(
        QString(QLL("wrapEmptyLimit")),
        Config::Matchers() << Config::Numbers::create(0, 100, 1),
        QString(QLL("8"))
    );
    comments.addKey(
        QString(QLL("wrapComma")),
        QStringList() << QString(QLL("soft")) << QString(QLL("hard")),
        QString(QLL("soft"))
    );
    comments.addKey(
        QString(QLL("target")),
        QStringList() << QString(QLL("long")) << QString(QLL("any")),
        QString(QLL("long"))
    );
    QHash<QString, QString > signs;
    signs.insert(QString(QLL("at")), QString(QLL("@")));
    signs.insert(QString(QLL("backslash")), QString(QLL("\\")));
    comments.addKey(
        QString(QLL("docTagSign")),
        QStringList() << QString(QLL("@")) << QString(QLL("\\")),
        QString(QLL("@")),
        signs
    );
    this->addSection(comments);
    Config::Section relations(QString(QLL("Relations")));
    relations.addKey(
        QString(QLL("thisMode")),
        Config::Matchers() << Config::Boolean::create(),
        Config::Code(defaultThisModeText)
    );
    this->addSection(relations);
    this->addSection(structure);
    Config::Section replaceTypes(QString(QLL("Replace.types")));
    replaceTypes.addKey(
        QString(QLL("target")),
        Config::Matchers() << Config::Boolean::create(),
        QString(QLL("isNew"))
    );
    replaceTypes.addKey(
        QString(QLL("action")),
        QStringList() << QString(QLL("replace")) << QString(QLL("report")),
        QString(QLL("replace"))
    );
    replaceTypes.setOpen(
        Config::Matchers() << QSharedPointer<const Config::ValueMatcher >(
                new FormatReplacementMatcher
        ) << Config::Word::create(QString(QLL("keep-as-is")))
    );
    this->addSection(replaceTypes);
    Config::Section code(QString(QLL("Code")));
    code.addKey(
        QString(QLL("nameMismatch")),
        QStringList() << QString(QLL("warn"))
            << QString(QLL("error"))
            << QString(QLL("critical")),
        QString(QLL("warn"))
    );
    code.addKey(
        QString(QLL("compilers")),
        Config::Matchers() << QSharedPointer<const Config::ValueMatcher >(
            new FormatNameListMatcher(
                QString(QLL("^[A-Za-z0-9+._-]+$")),
                QString(QLL("a list of qmake specs, such as win32-msvc2010, or nothing")),
                true
            )
        ),
        QString()
    );
    this->addSection(code);
    Config::Section lint(QString(QLL("Lint")));
    lint.addKey(
        QString(QLL("standards")),
        Config::Matchers() << QSharedPointer<const Config::ValueMatcher >(
            new FormatNameListMatcher(
                QString(QLL("^[A-Za-z0-9+]+$")),
                QString(QLL("a list of C++ standards, such as c++11 or [c++11, c++17]")),
                false
            )
        ),
        QString(QLL("c++11"))
    );
    lint.addKey(
        QString(QLL("byRefVariable")),
        Config::Matchers() << QSharedPointer<const Config::ValueMatcher >(
            new Config::Choice(QStringList(), FormatFileConfig::lintLevels())
        ),
        Config::Code(defaultByRefVariableText)
    );
    lint.addKey(
        QString(QLL("inlineOverride")),
        Config::Matchers() << Config::Name::create()
            << Config::Word::create(QString())
            << QSharedPointer<const Config::ValueMatcher >(
                new Config::Choice(QStringList(), QStringList())
            ),
        QString(QLL("override"))
    );
    this->addSection(lint);
}

/*!
    Builds the schema of the settings once, which schema() answers and the
    pieces that resolve() converts with ask (see FormatSchema).
*/
Q_GLOBAL_STATIC(FormatSchema, sharedSchema)

/*!
    Answers the schema of the settings, which it builds at the first call and
    every later call answers (see FormatSchema).
*/
QRef<const Config::Schema > FormatFileConfig::schema()
{
    return sharedSchema();
}

/*!
    Resolves @a program, which a setting @a key (`<section>.<key>`) compiled,
    at the place that @a context answers (see Config::Condition::resolve()),
    into the value of the type that the setting takes, as its
    matchers in the schema convert it (see Config::Schema::convert()):
    whether a condition holds, the number a count gives, or the word a
    choice picks. The context records whether resolution and conversion
    passed; a missing term or an invalid result leaves its default to decide.
*/
QVariant FormatFileConfig::resolve(
        const QString & key,
        const QStringList & program,
        const Config::Evaluator::Context *context)
{
    return FormatFileConfig::schema()->resolve(key, program, context);
}

/*!
    Builds the count that @a count, the number of a count setting (such as
    `joinArgs`, which takes any qint64 from 1 on), stands for as an int:
    past the int range, that range's end, which no list reaches.
*/
int FormatFileConfig::countOf(const QVariant & count)
{
    if (count.type() == QVariant::String) {
        const QList<qint64 > numbers = Config::Numbers::parse(count.toString());
        if ( ! numbers.isEmpty()) {
            return int(qBound(qint64(std::numeric_limits<int >::min()), numbers.first(), qint64(std::numeric_limits<int >::max())));
        }
    }
    return int(qBound(qreal(std::numeric_limits<int >::min()), count.toDouble(), qreal(std::numeric_limits<int >::max())));
}

/*!
    Resolves @a program, which the setting @a key compiled, at the place
    that @a context answers. Keeps values that do not depend on the place.
*/
QVariant FormatFileConfig::resolveCached(
        const QString & key,
        const QStringList & program,
        const Config::Evaluator::Context *context) const
{
    bool found = false;
    QVariant value = this->m_cache.get(key, program, &found);
    if (found) {
        context->markResolved();
        return value;
    }
    const Config::CacheScope scope(key);
    Q_UNUSED(scope);
    value = FormatFileConfig::schema()->resolve(key, program, context);
    if (value.isValid()) {
        this->m_cache.add(key, program, value);
    }
    return value;
}

/*!
    Resolves @a key from these settings at @a context.
*/
QVariant FormatFileConfig::resolveSetting(
        const QString & key, const Config::Evaluator::Context *context) const
{
    if (Config::Cache::isResolving(key)) {
        context->markResolveFailed(
            QString(QLL("recursive setting %1")).arg(key)
        );
        return QVariant();
    }
    const QRef<const Config::Schema > schema = FormatFileConfig::schema();
    const Config::Key field = schema->key(key);
    const QString text = settings.value(schema, key);
    if ( ! schema->isExpression(key, text, context)) {
        for (int i = 0; i < field.matchers.size(); ++i) {
            QString matched;
            if (field.matchers.at(i)->matchLiteral(text, &matched)) {
                const QVariant literal =
                    field.matchers.at(i)->settingValue(matched, context);
                if (literal.isValid()) {
                    context->markResolved();
                } else {
                    context->markResolveFailed(
                        QString(QLL("cannot resolve setting %1")).arg(key)
                    );
                }
                return literal;
            }
        }
        const QVariant literal = schema->convert(key, text);
        if (literal.isValid()) {
            context->markResolved();
            return literal;
        }
    }
    const QStringList program = schema->compile(key, text, context);
    if ( ! context->lastCompiled()) {
        context->markResolveFailed(context->lastErrorForCompile());
        return QVariant();
    }
    return this->resolveCached(key, program, context);
}

bool FormatFileConfig::hasExpression(const QString & key) const
{
    if (settings.values.isEmpty()) {
        return false;
    }
    const QRef<const Config::Schema > schema = FormatFileConfig::schema();
    const Config::Key field = schema->key(key);
    const QString text = settings.value(*schema, key);
    const Config::Evaluator::CompileContext context;
    bool expression = schema->isExpression(key, text, &context);
    if ( ! expression) {
        expression = field.allowsCode();
        for (int i = 0; i < field.matchers.size(); ++i) {
            QString matched;
            if (field.matchers.at(i)->matchLiteral(text, &matched)) {
                expression = false;
                break;
            }
        }
        if (expression && schema->convert(key, text).isValid()) {
            expression = false;
        }
    }
    return expression;
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
    FormatScopeConditions or, when @a words is given, the choice of those
    words (see Config::Condition::compile()); a text that does not compile
    leaves
    @a program at its default, and is logged. A name that differs from a
    known one in case alone is handled as @a mismatch says; for `critical`,
    returns false with @a error, and the file keeps its text.
*/
static bool compileCondition(
        const QString & text,
        QStringList *program,
        Config::NameMismatch mismatch = Config::NameMismatchWarn,
        QString *error = Q_NULLPTR,
        const QStringList *words = Q_NULLPTR)
{
    const Config::Evaluator::CompileContext context(
        FormatScopeConditions::names(), words, mismatch
    );
    const QStringList compiled = Config::Condition::compile(text, &context);
    if ( ! context.lastCompiled()) {
        if (context.mismatched()
            && mismatch == Config::NameMismatchCritical
        ) {
            *error = context.lastErrorForCompile();
            program->clear();
            return false;
        }
        reportCondition(context.lastErrorForCompile(), context.mismatched());
        return true;
    }
    *program = compiled;
    return true;
}

/*!
    Compiles the setting @a key of @a parser into @a program: the program
    that schema() compiled for the key's default, which is the key's value
    when no file sets it (see Config::Code), else the condition the value
    holds (see compileCondition()).
*/
static bool readProgram(
        const Config::Parser & parser,
        const QString & key,
        QStringList * program,
        Config::NameMismatch mismatch,
        QString * error,
        const QStringList * words = Q_NULLPTR)
{
    const Config::Key schemaKey = parser.schema().key(key);
    const QString value = parser.rawValue(key);
    const QStringList defaultProgram = parser.schema().programFor(key);
    if ( ! defaultProgram.isEmpty() && value == schemaKey.defaultValue) {
        *program = defaultProgram;
        return true;
    }
    return compileCondition(value, program, mismatch, error, words);
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
    Reads the command line's settings @a config into @a parser, after any
    `.formatrc` it read: the `--config-add` files, then the `-c` assignments;
    returns false, with @a error, for a setting outside of the schema.
*/
bool FormatFileConfig::readConfig(
        Config::Parser *parser, const FormatConfig & config, QString *error)
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
bool FormatFileConfig::load(
        const QString & path,
        const FormatConfig & config,
        FormatFileConfig *fileConfig,
        QString *error)
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
bool FormatFileConfig::readParser(
        const QString &path,
        const FormatConfig &config,
        Config::Parser *parser,
        QString *error)
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
        if ( ! item.startsWith(QLL("n:"))
            && ! item.startsWith(QLL("op:"))
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
    Builds the columns of @a value, a wrap column setting such as
    `wrapBegin = 75 | 51`, in order (see Config::Numbers::parse()); its
    schema keeps each one within the int range.
*/
static QList<int > columnsOf(const QString & value)
{
    const QList<qint64 > numbers = Config::Numbers::parse(value);
    QList<int > columns;
    for (int i = 0; i < numbers.size(); ++i) {
        columns << int(numbers.at(i));
    }
    return columns;
}

/*!
    Compiles the setting @a key of @a parser into @a program and handles
    name mismatches as @a mismatch specifies.
*/
static bool compileSetting(
        const Config::Parser & parser,
        const QString & key,
        QStringList *program,
        Config::NameMismatch mismatch,
        QString *error)
{
    const Config::Evaluator::CompileContext context(
        FormatScopeConditions::names(), Q_NULLPTR, mismatch
    );
    const QStringList compiled = parser.schema().compile(
        key,
        parser.rawValue(key),
        &context
    );
    if ( ! context.lastCompiled()) {
        if (context.mismatched()
            && mismatch == Config::NameMismatchCritical
        ) {
            *error = context.lastErrorForCompile();
            program->clear();
            return false;
        }
        reportCondition(context.lastErrorForCompile(), context.mismatched());
        return true;
    }
    *program = compiled;
    return true;
}

/*!
    Reads the setting @a key of @a parser, which takes a count or a
    condition: a count, or math that gives one (such as `1 + 7 - 3`), goes
    to @a count, as the setting converts it (see FormatFileConfig::resolve()),
    and clears @a program; any other condition is compiled into @a program,
    which then decides in place of the count. Returns false, with @a error,
    when a name mismatch fails it under @a mismatch `critical`.
*/
static bool readCountOrCondition(
        const Config::Parser & parser,
        const QString & key,
        int *count,
        QStringList *program,
        Config::NameMismatch mismatch,
        QString *error)
{
    const Config::Key schemaKey = parser.schema().key(key);
    const QString value = parser.rawValue(key);
    const QStringList defaultProgram = parser.schema().programFor(key);
    if ( ! defaultProgram.isEmpty() && value == schemaKey.defaultValue) {
        *program = defaultProgram;
        return true;
    }
    bool isNumber = false;
    const qint64 number = value.toLongLong(&isNumber);
    if (isNumber) {
        *count = FormatFileConfig::countOf(number);
        program->clear();
        return true;
    }
    if ( ! compileSetting(parser, key, program, mismatch, error)) {
        *count = 0;
        return false;
    }
    if (isConstant(*program)) {
        const Config::Evaluator::Context context(false);
        const QVariant constant =
            FormatFileConfig::resolve(key, *program, &context);
        if (context.lastResolvePassed()) {
            *count = FormatFileConfig::countOf(constant);
            program->clear();
        }
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
static bool checkCondition(
        QString *text,
        const QStringList & names,
        const QString & defaultValue,
        Config::NameMismatch mismatch,
        QString *error)
{
    const Config::Evaluator::CompileContext context(
        names,
        Q_NULLPTR,
        mismatch
    );
    Config::Condition::compile(*text, &context);
    if (context.lastCompiled() || ! context.mismatched()) {
        return true;
    }
    if (mismatch == Config::NameMismatchCritical) {
        *error = context.lastErrorForCompile();
        return false;
    }
    reportCondition(context.lastErrorForCompile(), true);
    *text = defaultValue;
    return true;
}

class FormatSettingsContext : public FormatConditionContext
{
public:

    inline explicit FormatSettingsContext(const FormatFileConfig *config)
        : m_config(config) {}
    inline const FormatFileConfig *fileConfig() const Q_DECL_OVERRIDE_IL
    { return m_config; }
private:
    const FormatFileConfig *m_config;
};

/*!
    Converts the validated values in @a parser to @a fileConfig. The
    conditions handle a name that differs from a known one in case alone as
    `[Code] nameMismatch` says (see Config::NameMismatch);
    returns false, with @a error, for such a name under `critical`.
*/
bool FormatFileConfig::fromParser(
        const Config::Parser &parser,
        FormatFileConfig *fileConfig,
        QString *error)
{
    fileConfig->settings = parser.settings();
    const FormatSettingsContext settingsContext(fileConfig);
    const Config::Evaluator::Context *previousContext = parser.setContext(
        &settingsContext
    );
    Q_DEFER { parser.setContext(previousContext); };
    const Config::NameMismatch mismatch = Config::Condition::nameMismatch(parser
        .value(QString(QLL("code.nameMismatch"))));
    fileConfig->nameMismatch = mismatch;
    fileConfig->standards = listItems(parser.value(
        QString(QLL("lint.standards"))
    ));
    fileConfig->compilers = listItems(parser.value(
        QString(QLL("code.compilers"))
    ));
    fileConfig->inlineOverride = FormatFileConfig::inlineMarker(
        parser.rawValue(QString(QLL("lint.inlineOverride"))),
        *fileConfig
    );
    fileConfig->byRefVariable = FormatFileConfig::lintLevel(
        parser.rawValue(QString(QLL("lint.byRefVariable"))),
        *fileConfig
    );
    fileConfig->extensions = parser.value(QString(QLL("general.extensions")));
    fileConfig->typedefSuper = parser.rawValue(
        QString(QLL("structure.typedefSuper"))
    );
    fileConfig->replaceSuperSites = parser.rawValue(
        QString(QLL("structure.replaceSuperSites"))
    );
    fileConfig->replaceTypesTarget = parser.rawValue(
        QString(QLL("replace.types.target"))
    );

    // Reports the mismatches of the conditions that rules compile later;
    // those rules then use the names meant.
    const QStringList superChoices = FormatFileConfig::superChoices();
    const QStringList superNames = FormatFileConfig::superConditionNames()
        + FormatScopeConditions::names();
    const Config::Evaluator::CompileContext superContext(
        superNames, &superChoices, mismatch
    );
    Config::Condition::compile(fileConfig->typedefSuper, &superContext);
    if ( ! superContext.lastCompiled() && superContext.mismatched()) {
        if (mismatch == Config::NameMismatchCritical) {
            *error = superContext.lastErrorForCompile();
            return false;
        }
        reportCondition(superContext.lastErrorForCompile(), true);
        fileConfig->typedefSuper = FormatFileConfig::defaultTypedefSuper();
    }
    if ( ! checkCondition(
            &fileConfig->replaceSuperSites,
            superNames,
            FormatFileConfig::defaultReplaceSuperSites(),
            mismatch,
            error)
        || ! checkCondition(
            &fileConfig->replaceTypesTarget,
            FormatReplaceTypesContext().names(),
            parser.schema().key(QString(QLL("replace.types.target")))
                .defaultValue,
            mismatch,
            error)
    ) {
        return false;
    }
    superContext.setMismatchMode(Config::NameMismatchIgnore);
    fileConfig->typedefSuperProgram =
        Config::Condition::compile(fileConfig->typedefSuper, &superContext);
    const Config::Evaluator::CompileContext sitesContext(superNames);
    fileConfig->replaceSuperSitesProgram = Config::Condition::compile(
        fileConfig->replaceSuperSites,
        &sitesContext
    );
    fileConfig->replaceTypesAction = parser.value(
        QString(QLL("replace.types.action"))
    );
    fileConfig->replaceTypes = FormatFileConfig::expandPrevious(parser
        .openEntries(QString(QLL("replace.types"))));
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
        = columnsOf(parser.value(QString(QLL("structure.wrapBegin"))));
    fileConfig->wrapMax = parser.value(QString(QLL("structure.wrapMax")))
        .toInt();
    const int scopes = Config::Scopes::count();
    // Lays out one operand per line where a condition misses a value.
    fileConfig->wrapConditionsOver = 1;
    const bool counted = readCountOrCondition(
        parser,
        QString(QLL("structure.wrapArgsOver")),
        &fileConfig->wrapArgsOver,
        &fileConfig->wrapArgsOverCondition,
        mismatch,
        error)
        && readCountOrCondition(
            parser,
            QString(QLL("structure.wrapConditionsOver")),
            &fileConfig->wrapConditionsOver,
            &fileConfig->wrapConditionsOverCondition,
            mismatch,
            error)
        && readCountOrCondition(
            parser,
            QString(QLL("structure.operatorPadding")),
            &fileConfig->operatorPadding,
            &fileConfig->operatorPaddingCondition,
            mismatch,
            error)
        && readCountOrCondition(
            parser,
            QString(QLL("structure.joinArgs")),
            &fileConfig->joinArgs,
            &fileConfig->joinArgsCondition,
            mismatch,
            error
        );
    Config::Scopes::popUntil(scopes);
    if ( ! counted) {
        return false;
    }
    fileConfig->operatorsInMiddle =
        parser.value(QString(QLL("structure.operatorPlacement")))
        == QLL("middle");
    fileConfig->wrapSubChain = parser.value(
        QString(QLL("structure.wrapSubChain"))
    );
    fileConfig->wrapConditionsPer = parser.value(
        QString(QLL("structure.wrapConditionsPer"))
    );
    fileConfig->commentWrapBegin
        = columnsOf(parser.value(QString(QLL("comments.wrapBegin"))));
    fileConfig->commentWrapMax = parser
        .value(QString(QLL("comments.wrapMax")))
        .toInt();
    fileConfig->wrapExtendsForKeywords
        = parser.value(QString(QLL("comments.wrapExtendsForKeywords"))).toInt();
    fileConfig->wrapEmptyLimit = parser
        .value(QString(QLL("comments.wrapEmptyLimit")))
        .toInt();
    fileConfig->wrapComma = parser.value(QString(QLL("comments.wrapComma")))
        == QLL("hard")
        ? WrapCommaHard : WrapCommaSoft;
    fileConfig->commentTarget = parser.value(QString(QLL("comments.target")))
        == QLL("any")
        ? CommentTargetAny : CommentTargetLong;
    fileConfig->codeTarget = parser.value(QString(QLL("structure.target")))
        == QLL("any")
        ? CodeTargetAny : CodeTargetLong;
    fileConfig->compact = parser.value(QString(QLL("structure.compact")))
        == QLL("always")
        ? CompactAlways : CompactNever;
    fileConfig->docTagSign = parser.value(QString(QLL("comments.docTagSign")))
        == QLL("\\")
        ? '\\' : '@';
    const QStringList words = FormatFileConfig::wrapMistakesWords();
    return readProgram(
        parser,
        QString(QLL("structure.bodyBraces")),
        &fileConfig->bodyBraces,
        mismatch,
        error)
        && readProgram(
            parser,
            QString(QLL("relations.thisMode")),
            &fileConfig->thisMode,
            mismatch,
            error)
        && readProgram(
            parser,
            QString(QLL("structure.wrapChainAt")),
            &fileConfig->wrapChainAt,
            mismatch,
            error)
        && readProgram(
            parser,
            QString(QLL("structure.wrapArgsAt")),
            &fileConfig->wrapArgsAt,
            mismatch,
            error)
        && readProgram(
            parser,
            QString(QLL("structure.wrapSignatureAt")),
            &fileConfig->wrapSignatureAt,
            mismatch,
            error)
        && readProgram(
            parser,
            QString(QLL("structure.wrapMistakes")),
            &fileConfig->wrapMistakes,
            mismatch,
            error,
            &words
        );
}

namespace {

/*!
    Answers a `[Lint]` key's choice from the settings it is given, as far as
    they are read: `hasStd()` from `[Lint] standards`, and `hasCompiler()`
    from `[Code] compilers` (see FormatConditionContext).
*/
class FormatLintContext : public FormatConditionContext
{
public:

    inline explicit FormatLintContext(const FormatFileConfig *config)
        : m_config(config) {}

    inline const FormatFileConfig *fileConfig() const Q_DECL_OVERRIDE_IL
    {
        return m_config;
    }

private:
    const FormatFileConfig *m_config;
};

/*!
    Resolves the `[Lint]` choice @a value into @a word, where `hasStd()`
    and `hasCompiler()` answer from @a config,
    as the setting @a key (`<section>.<key>`) converts it (see resolve());
    logs a choice that cannot pick one, naming that key, and returns false
    for such a choice.
*/
static bool resolveLintChoice(
        const QString & value,
        const FormatFileConfig & config,
        const QString & key,
        QString *word)
{
    const FormatLintContext context(&config);
    context.setMismatchMode(Config::NameMismatchWarn);
    QString error;
    const QStringList program = FormatFileConfig::schema()->compile(
        key,
        value,
        &context
    );
    if (context.lastCompiled()) {
        const QVariant resolved =
            FormatFileConfig::resolve(key, program, &context);
        if (context.lastResolvePassed()) {
            *word = resolved.toString();
            return true;
        }
        error = QString(QLL("cannot evaluate condition: %1")).arg(value);
    } else {
        error = context.lastErrorForCompile();
    }
    qWarning("Invalid %s: %s", qPrintable(key), qPrintable(error.isEmpty() ? value : error));
    *word = QString();
    return false;
}

} // namespace

/*!
    Builds the marker that the `[Lint] inlineOverride` value @a value names:
    a name, nothing, or a choice of texts in double quotes, where `hasStd()`
    and `hasCompiler()` answer from @a config, such as
    `hasCompiler("win32-msvc2010") ? "Q_DECL_OVERRIDE_IL" : "override"`. A
    choice that cannot pick one is logged, and gives `override`.
*/
QString FormatFileConfig::inlineMarker(
        const QString & value, const FormatFileConfig & config)
{
    const QString text = value.trimmed();
    QString matched;
    if (text.isEmpty() || Config::Name(false).match(text, &matched)) {
        return text;
    }
    QString marker;
    if ( ! resolveLintChoice(
        text,
        config,
        QString(QLL("lint.inlineOverride")),
        &marker
    )) {
        return QString(QLL("override"));
    }
    return marker.trimmed();
}

/*!
    Lists the words a `[Structure] wrapMistakes` choice may pick beside a
    number: `average` and `never`.
*/
QStringList FormatFileConfig::wrapMistakesWords()
{
    return QStringList() << QString(QLL("average")) << QString(QLL("never"));
}

/*!
    Builds the level that the `[Lint]` choice @a value picks (see
    lintLevels()), where `hasStd()` and `hasCompiler()` answer from
    @a config; a choice that cannot pick one is logged, and gives `warn`.
*/
FormatFileConfig::LintLevel FormatFileConfig::lintLevel(
        const QString & value, const FormatFileConfig & config)
{
    QString level;
    if ( ! resolveLintChoice(
        value,
        config,
        QString(QLL("lint.byRefVariable")),
        &level
    )) {
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
        const QString & value, const QString & defaultValue)
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
