// Copyright (C) 2015 The XD Company Ltd.
// SPDX-License-Identifier: Apache-2.0

#include "./scope-conditions.h"

#include "../format-document.h"
#include "./class-helpers.h"
#include "./code-helpers.h"
#include "./condition-context.h"
#include "./layout-helpers.h"

#include "config-condition.h"

#include <QtCore/QFile>
#include <QtCore/QFileInfo>
#include <QtCore/QList>

/*!
    @class FormatScopeConditions
    @internal

    Tells which names of a setting's condition (see Config::Condition) hold
    at a token of a document, for any rule that a setting steers: the names
    come from the scope that holds the token, which is the member function
    whose body holds it, else the class body that does, and from the token
    itself (see names()).

    The scope of each body, and the members of each class, are read once and
    cached; hence a rule asks for each token it formats.
*/

/*!
    Builds the conditions of @a document, which must stay alive as long as
    the conditions are in use.
*/
FormatScopeConditions::FormatScopeConditions(const FormatDocument *document)
    : m_document(document), m_reservedLine(-1), m_reservedColumn(0)
{
}

/*!
    Builds the names a condition takes, each one true at a token when:

    * `always`: always, and `never`: never;
    * `already-existing`: the token is a member name that already follows
      `this->`;
    * `is-base-member`: the token is a member name that a base class
      declares, and the scope's own class does not;
    * `is-template`: the scope's function, or its class, is a template;
    * `is-outline`: the scope's function is defined outside its class body;
    * `is-header`: the file is a header (see FormatDocument::isHeader()),
      and `is-non-header`: it is not.
*/
QStringList FormatScopeConditions::names()
{
    return QStringList() << QString(QLL("always")) << QString(QLL("never"))
        << QString(QLL("already-existing")) << QString(QLL("is-base-member"))
        << QString(QLL("is-template")) << QString(QLL("is-outline"))
        << QString(QLL("is-header")) << QString(QLL("is-non-header"))
        << QString(QLL("is-signature"));
}

namespace {

/*!
    Answers what a condition asks for (see FormatConditionContext) at the
    argument list that holds a token, or at the `(` of an `if` or a `while`
    for a condition's operands, each answer only when a term asks for it:
    the names (see FormatScopeConditions::nameHolds()), `value`, the
    argument counts, `spaceRemaining`, and the percent value of the percent
    terms, which is 100 less `spaceRemaining`.

    A caller that already knows `value` or `spaceRemaining` passes it in
    @a count or @a spaceRemaining, else null; @a hasPercent unset leaves the
    percent terms without a value.
*/
class ArgumentContext : public FormatConditionContext
{
    typedef FormatConditionContext super;
public:
    inline ArgumentContext(FormatScopeConditions *scope, int token,
                           int target, bool condition, const int *count,
                           const qreal *spaceRemaining, bool hasPercent)
        : m_scope(scope), m_token(token), m_target(target)
        , m_condition(condition), m_count(count)
        , m_spaceRemaining(spaceRemaining), m_hasPercent(hasPercent)
    {
    }

    bool predicate(const QString &name, bool *value) const Q_DECL_OVERRIDE;
    bool percent(qreal *value) const Q_DECL_OVERRIDE;
    bool value(qreal *value) const Q_DECL_OVERRIDE;
    bool argumentCount(int depth, int argument,
                       qreal *count) const Q_DECL_OVERRIDE;
    bool spaceRemaining(qreal *percent) const Q_DECL_OVERRIDE;

private:
    FormatScopeConditions *m_scope;
    int m_token;
    int m_target;
    bool m_condition;
    const int *m_count;
    const qreal *m_spaceRemaining;
    bool m_hasPercent;
};

bool ArgumentContext::predicate(const QString &name, bool *value) const
{
    return this->m_scope->nameHolds(this->m_token, name, value);
}

bool ArgumentContext::percent(qreal *value) const
{
    qreal space = 0;
    if ( ! this->m_hasPercent || ! this->spaceRemaining(&space)) {
        return false;
    }
    *value = 100 - space;
    return true;
}

bool ArgumentContext::value(qreal *value) const
{
    *value = this->m_count ? *this->m_count
                           : this->m_scope->argumentCount(this->m_token, 0);
    return true;
}

bool ArgumentContext::argumentCount(
        int depth, int argument,
        qreal *count) const
{
    *count = this->m_condition
        ? this->m_scope->conditionArgumentCount(
            this->m_token,
            depth,
            argument
        )
        : this->m_scope->argumentCount(this->m_token, depth, argument);
    return true;
}

bool ArgumentContext::spaceRemaining(qreal *percent) const
{
    *percent = this->m_spaceRemaining
        ? *this->m_spaceRemaining
        : this->m_scope->spaceRemaining(this->m_target);
    return true;
}

} // namespace

/*!
    Tells whether the condition that Config::Condition::compile() made
    @a program of holds at the token @a token.
*/
bool FormatScopeConditions::holds(const QStringList & program, int token)
{
    const ArgumentContext context(
        this, token, this->argumentTarget(token),
        false, Q_NULLPTR, Q_NULLPTR, true
    );
    return Config::Condition::evaluate(program, QSet<QString >(), &context);
}

/*!
    Tells whether the condition @a program holds at the token @a token, with
    @a percent as the value of its percent terms (see
    Config::Condition::evaluate()), negative where the use site has none.
    When a term misses what it needs (Config::Condition logs that), the
    default condition @a fallback decides instead; the formatting goes on.
*/
bool FormatScopeConditions::holds(
        const QStringList & program, int token, qreal percent,
        const QStringList & fallback)
{
    const qreal space = 100 - percent;
    const ArgumentContext context(
        this, token, this->argumentTarget(token),
        false, Q_NULLPTR,
        percent >= 0 ? &space : Q_NULLPTR,
        percent >= 0
    );
    bool ok = true;
    const bool result = Config::Condition::evaluate(
        program, QSet<QString >(), &context, &ok
    );
    if (ok) {
        return result;
    }
    return Config::Condition::evaluate(
        fallback,
        QSet<QString >(),
        &context
    );
}

/*!
    Tells in @a value whether the name @a name of names() holds at the token
    @a token, in the form without case, `_` and `-` that
    Config::Condition::compile() gives it; returns false for a name that is
    not one of names(). It finds only what that one name needs, hence a
    condition costs only the names it reaches.
*/
bool FormatScopeConditions::nameHolds(
        int token, const QString & name,
        bool *value)
{
    if (name == QLL("always") || name == QLL("never")) {
        *value = name == QLL("always");
        return true;
    }
    if (name == QLL("isHeader") || name == QLL("isNonHeader")) {
        *value = this->m_document->isHeader() == (name == QLL("isHeader"));
        return true;
    }
    if (name == QLL("isSignature")) {
        const int target = this->argumentTarget(token);
        *value = target >= 0 && isDeclarationParen(m_document, target);
        return true;
    }
    if (name == QLL("isOutline") || name == QLL("isTemplate")) {
        const FormatMemberScope scope = this->scopeOf(token);
        *value = name == QLL("isOutline") ? scope.isOutline : scope.isTemplate;
        return true;
    }
    if (name != QLL("isBaseMember") && name != QLL("alreadyExisting")) {
        return false;
    }

    // Names what holds for a member name.
    *value = false;
    if (this->m_document->token(token) != IDENTIFIER) {
        return true;
    }
    const FormatMemberScope scope = this->scopeOf(token);
    const QByteArray member = this->m_document->lexem(token);
    if (name == QLL("isBaseMember")) {
        *value = scope.baseMembers.contains(member);
        return true;
    }
    *value = (scope.members.names.contains(member)
        || scope.baseMembers.contains(member))
        && token >= 2 && this->m_document->token(token - 1) == ARROW
        && this->m_document->token(token - 2) == THIS;
    return true;
}

/*!
    Builds the names of names() that hold at the token @a token, for a
    caller that wants all of them at once.
*/
QSet<QString > FormatScopeConditions::holding(int token)
{
    QSet<QString > holding;
    const QStringList all = FormatScopeConditions::names();
    for (int i = 0; i < all.size(); ++i) {
        const QString name = Config::Condition::canonicalName(all.at(i));
        bool value = false;
        if (this->nameHolds(token, name, &value) && value) {
            holding << all.at(i);
        }
    }
    return holding;
}

/*!
    Builds the scope of the token @a token: the member function whose body
    holds it (see functionScope()), else the class body that holds it, else
    a scope without a class.
*/
FormatMemberScope FormatScopeConditions::scopeOf(int token)
{
    const int body = this->enclosingFunctionBody(token);
    if (body >= 0) {
        const FormatMemberScope scope = this->functionScope(body);
        if ( ! scope.className.isEmpty()) {
            return scope;
        }
    }
    FormatMemberScope scope;
    const int classBody = enclosingClassBody(this->m_document, token);
    if (classBody < 0) {
        return scope;
    }
    scope.body = classBody;
    scope.className = classNameOf(this->m_document, classBody);
    if (scope.className.isEmpty()) {
        return scope;
    }
    scope.members = this->membersOf(scope.className);
    scope.baseMembers = this->baseMembersOf(scope.members);
    scope.isTemplate = scope.members.isTemplate;
    return scope;
}

/*!
    Builds the scope of the member function whose body's `{` is at @a brace:
    its class from `<Class>::<member>(` in the head of an outline
    definition, else the class body it sits in; its parameters; and whether
    it is static, an outline, and a template. The scope has no class for a
    body that is no member function's.
*/
FormatMemberScope FormatScopeConditions::functionScope(int brace)
{
    if (this->m_scopes.contains(brace)) {
        return this->m_scopes.value(brace);
    }
    const FormatDocument *document = this->m_document;
    FormatMemberScope scope;
    scope.body = brace;
    const FormatDocument::Scope outer = document->scopeOf(brace);
    scope.isOutline = outer == FormatDocument::FileScope
        || outer == FormatDocument::NamespaceScope;
    if (scope.isOutline || outer == FormatDocument::ClassScope) {
        QByteArray className;
        for (int k = document->headStart(brace); k + 1 < brace; ++k) {
            const Token token = document->token(k);
            scope.isTemplate = scope.isTemplate || token == TEMPLATE;
            scope.isStatic = scope.isStatic || token == STATIC;
            if (token == LPAREN && document->match(k) > k) {
                k = document->match(k);
                continue;
            }
            if ( ! scope.isOutline) {
                if (scope.params < 0
                    && token == IDENTIFIER
                    && document->token(k + 1) == LPAREN
                    && ! isMacroName(document->lexem(k))
                ) {
                    scope.memberToken = k;
                    scope.params = k + 1;
                }
                continue;
            }

            // Steps over a class template's arguments: `W<T>::run(`.
            int scopeToken = k + 1;
            if (token == IDENTIFIER
                && document->token(scopeToken) == LANGLE
            ) {
                int depth = 0;
                for (; scopeToken < brace; ++scopeToken) {
                    const Token angle = document->token(scopeToken);
                    depth += angle == LANGLE ? 1 : 0;
                    depth -= angle == RANGLE ? 1 : (angle == GTGT ? 2 : 0);
                    if (depth <= 0) {
                        break;
                    }
                }
                ++scopeToken;
                scope.isTemplate = scope.isTemplate || depth == 0;
            }
            if (token != IDENTIFIER
                || document->token(scopeToken) != SCOPE
            ) {
                continue;
            }
            int member = scopeToken + 1;
            if (document->token(member) == TILDE) {
                ++member;
            }
            if (scope.params < 0 && document->token(member) == IDENTIFIER
                && document->token(member + 1) == LPAREN
                && document->token(member + 2) != STAR
            ) {
                className = document->lexem(k);
                scope.memberToken = member;
                scope.params = member + 1;
            }
        }
        if ( ! scope.isOutline) {
            className = classNameOf(
                document,
                enclosingClassBody(document, brace)
            );
        }
        if (scope.params >= 0 && document->match(scope.params) >= 0) {
            scope.className = className;
        }
    }
    if ( ! scope.className.isEmpty()) {
        scope.members = this->membersOf(scope.className);
        scope.baseMembers = this->baseMembersOf(scope.members);
        scope.isTemplate = scope.isTemplate || scope.members.isTemplate;
    }
    this->m_scopes.insert(brace, scope);
    return scope;
}

/*!
    Builds the index of the `{` of the outermost function body that holds
    the token @a token, or -1 when none does: a lambda's body, and a block or
    a braced initializer inside the body, count as part of its function's,
    while a class or a namespace body ends the search.
*/
int FormatScopeConditions::enclosingFunctionBody(int token) const
{
    const FormatDocument *document = this->m_document;
    int found = -1;
    int depth = 0;
    for (int k = token - 1; k >= 0; --k) {
        const Token kind = document->token(k);
        if (kind == RBRACE) {
            ++depth;
            continue;
        }
        if (kind != LBRACE) {
            continue;
        }
        if (depth > 0) {
            --depth;
            continue;
        }
        const FormatDocument::Scope scope = document->braceScope(k);
        if (scope == FormatDocument::ClassScope
            || scope == FormatDocument::NamespaceScope
        ) {
            break;
        }
        if (scope == FormatDocument::FunctionScope) {
            found = k;
        }
    }
    return found;
}

/*!
    Builds the members of the class named @a className: from its body in the
    document, else in a header named after the document's file (with a `.h`,
    `_p.h`, or `.hpp` suffix), else in one the file quotes in an `#include`;
    each header is looked for next to the file, then in the include folders
    (see FormatDocument::findInclude()).
*/
FormatClassMembers FormatScopeConditions::membersOf(
        const QByteArray & className)
{
    if (this->m_classes.contains(className)) {
        return this->m_classes.value(className);
    }
    const FormatDocument *document = this->m_document;
    FormatClassMembers members;
    const int brace = findClassBody(document, className);
    if (brace >= 0) {
        members = collectMembers(document, brace, className);
    } else if ( ! document->path().isEmpty()) {
        // Looks for the class in the headers named after the file, then in
        // the ones the file includes.
        const QFileInfo info(document->path());
        QStringList headers;
        const char *const suffixes[] = { ".h", "_p.h", ".hpp" };
        for (size_t s = 0; s < sizeof(suffixes) / sizeof(suffixes[0]); ++s) {
            const QString suffix = QString::fromLatin1(suffixes[s]);
            headers << info.completeBaseName() + suffix;
        }
        headers << document->quotedIncludes();
        for (int h = 0; h < headers.size(); ++h) {
            QFile header(document->findInclude(headers.at(h)));
            if (header.fileName().isEmpty()
                || ! header.open(QIODevice::ReadOnly)
            ) {
                continue;
            }
            const FormatDocument other(header.readAll(), header.fileName());
            const int found = findClassBody(&other, className);
            if (found >= 0) {
                members = collectMembers(&other, found, className);
                break;
            }
        }
    }
    this->m_classes.insert(className, members);
    return members;
}

/*!
    Builds the names of the members that the bases of the class whose
    members are @a members declare, and the bases of those in turn, up to 8
    levels deep; a base the conditions find no body of adds none, and the
    class's own members are left out.
*/
QSet<QByteArray > FormatScopeConditions::baseMembersOf(
        const FormatClassMembers & members)
{
    QSet<QByteArray > names;
    QSet<QByteArray > seen;
    QList<QByteArray > queue = members.bases;
    for (int depth = 0; depth < 8 && ! queue.isEmpty(); ++depth) {
        QList<QByteArray > next;
        for (int b = 0; b < queue.size(); ++b) {
            if (seen.contains(queue.at(b))) {
                continue;
            }
            seen.insert(queue.at(b));
            const FormatClassMembers base = this->membersOf(queue.at(b));
            names.unite(base.names);
            next << base.bases;
        }
        queue = next;
    }
    names.subtract(members.names);
    return names;
}

namespace {
static bool isArgumentTarget(const FormatDocument *document, int open)
{
    if (open < 1
        || document->token(open) != LPAREN
        || document->match(open) <= open
    ) {
        return false;
    }
    const Token before = document->token(open - 1);
    return before == IDENTIFIER || before == RPAREN || before == RBRACK
        || before == RANGLE || before == GTGT || before == IF || before == WHILE;
}

static QList<QPair<int, int > > targetItems(const FormatDocument *document,
                                             int open)
{
    const int close = document->match(open);
    const Token before = document->token(open - 1);
    if (before != IF && before != WHILE && isArgumentTarget(document, open)) {
        return splitArguments(document, open, close);
    }
    QList<QPair<int, int > > items;
    int first = open + 1;
    for (int i = first; i < close; ++i) {
        const Token token = document->token(i);
        if ((token == LPAREN || token == LBRACK || token == LBRACE)
            && document->match(i) > i
        ) {
            i = document->match(i);
        } else if (token == QUESTION) {
            return QList<QPair<int, int > >() << qMakePair(open + 1, close - 1);
        } else if (token == ANDAND || token == OROR) {
            items << qMakePair(first, i - 1);
            first = i + 1;
        }
    }
    if (first < close) {
        items << qMakePair(first, close - 1);
    }
    return items;
}

static int enclosingArgumentTarget(const FormatDocument *document, int token)
{
    for (int i = token - 1; i >= 1; --i) {
        if (isArgumentTarget(document, i) && document->match(i) >= token) {
            return i;
        }
    }
    return -1;
}

static int enclosingConditionTarget(const FormatDocument *document, int token)
{
    for (int i = token - 1; i >= 1; --i) {
        if (document->token(i) != LPAREN || document->match(i) < token) {
            continue;
        }
        if (isArgumentTarget(document, i) || targetItems(document, i).size() > 1) {
            return i;
        }
    }
    return -1;
}

static void directCalls(const FormatDocument *document, int first, int last,
                        QList<int > *calls)
{
    for (int i = first; i <= last; ++i) {
        if (isArgumentTarget(document, i)) {
            calls->append(i);
            i = document->match(i);
        }
    }
}
}

/*!
    Counts arguments of calls reached from the target: zero selects the
    target, negative depths select parents, and positive depths select calls
    within its arguments. A positive argument selects that one-based item;
    zero selects all items. Returns -1 when no target exists at that depth.
*/
int FormatScopeConditions::argumentTarget(int token) const
{
    if (isArgumentTarget(m_document, token)) {
        return token;
    }
    if (isArgumentTarget(m_document, token + 1)) {
        return token + 1;
    }
    return enclosingArgumentTarget(m_document, token);
}

qreal FormatScopeConditions::remainingSpace(int token, bool direct) const
{
    const int open = direct ? token : this->argumentTarget(token);
    if (open < 0) {
        return 0;
    }
    const int line = m_document->symbolLine(open);
    const QByteArray head = m_document->text().mid(
        m_document->lineStart(line),
        m_document->symbolEnd(open) - m_document->lineStart(line)
    );
    const int maximum = qMax(1, m_document->configAt(
        m_document->symbols().at(open).from
    ).wrapMax);
    return 100.0 * (maximum - textWidth(head)) / maximum;
}

/*!
    Marks the line that holds the token @a token as used up to the column
    @a column, for a rule that lays the line out anew: spaceRemaining() at a
    token of that line then counts from that column. A later reserve() moves
    the mark to its own line.
*/
void FormatScopeConditions::reserve(int token, int column)
{
    m_reservedLine = m_document->symbolLine(token);
    m_reservedColumn = column;
}

/*!
    Builds the share of `[Structure] wrapMax`, in percents, that is left on
    the line of the token @a token: past the column that reserve() marked on
    that line, else past the token's end. A token past `wrapMax` leaves a
    negative share, and a token of no line leaves none.
*/
qreal FormatScopeConditions::spaceRemaining(int token) const
{
    if (token < 0) {
        return 0;
    }
    if (m_reservedLine >= 0
        && m_document->symbolLine(token) == m_reservedLine
    ) {
        const int maximum = qMax(
            1,
            m_document->configAt(
                m_document->symbols().at(token).from
            ).wrapMax
        );
        return 100.0 * (maximum - m_reservedColumn) / maximum;
    }
    return this->remainingSpace(token, true);
}

int FormatScopeConditions::argumentCount(
        int token, int depth, int argument) const
{
    return this->countTargets(this->argumentTarget(token), depth, argument, false);
}

int FormatScopeConditions::conditionArgumentCount(
        int token, int depth, int argument) const
{
    if (token < 0 || token >= m_document->symbols().size()
        || m_document->token(token) != LPAREN || m_document->match(token) <= token
    ) {
        return -1;
    }
    return this->countTargets(token, depth, argument, true);
}

int FormatScopeConditions::countTargets(
        int target, int depth, int argument, bool condition) const
{
    if (target < 0 || argument < 0) {
        return -1;
    }
    while (depth < 0) {
        target = condition ? enclosingConditionTarget(m_document, target)
                           : enclosingArgumentTarget(m_document, target);
        if (target < 0) {
            return -1;
        }
        ++depth;
    }
    QList<int > targets;
    targets << target;
    for (int level = 0; level < depth; ++level) {
        QList<int > next;
        for (int k = 0; k < targets.size(); ++k) {
            const int open = targets.at(k);
            const QList<QPair<int, int > > items =
                targetItems(m_document, open);
            for (int i = 0; i < items.size(); ++i) {
                if (level == 0 && argument > 0 && i + 1 != argument) {
                    continue;
                }
                directCalls(
                    m_document,
                    items.at(i).first,
                    items.at(i).second,
                    &next
                );
            }
        }
        if (next.isEmpty()) {
            return -1;
        }
        targets = next;
    }
    int count = 0;
    for (int i = 0; i < targets.size(); ++i) {
        const int open = targets.at(i);
        const QList<QPair<int, int > > items =
            targetItems(m_document, open);
        if (items.size() == 1 && m_document->token(items.first().first) == VOID
            && items.first().first == items.first().second
            && isDeclarationParen(m_document, open)
        ) {
            continue;
        }
        count += items.size();
    }
    return count;
}

/*!
    Evaluates the argument-count setting at @a token, for a list of
    @a count arguments; `spaceRemaining` is measured from the target's
    column, only when the setting reaches it.
*/
bool FormatScopeConditions::argsOver(int token, int count)
{
    return this->countOver(token, count, Q_NULLPTR, false);
}

/*!
    Evaluates the argument-count setting at @a token, as the argsOver()
    above does, with @a spaceRemaining as `spaceRemaining`.
*/
bool FormatScopeConditions::argsOver(
        int token, int count,
        qreal spaceRemaining)
{
    return this->countOver(token, count, &spaceRemaining, false);
}

bool FormatScopeConditions::conditionsOver(int token, int count)
{
    return this->countOver(token, count, Q_NULLPTR, true);
}

bool FormatScopeConditions::conditionsOver(int token, int count,
                                           qreal spaceRemaining)
{
    return this->countOver(token, count, &spaceRemaining, true);
}

/*!
    Builds how many operands of the condition whose `(` is at @a token, of
    @a count operands, a line may hold by `[Structure] wrapConditionsOver`,
    asked at an operand that ends a line, with @a spaceRemaining as the
    share of `wrapMax` left after it: 0 for as many as fit. A condition that
    holds gives 1, one that does not gives 0, and one that misses a value
    gives the setting's number.
*/
int FormatScopeConditions::conditionsPerLine(
        int token, int count,
        qreal spaceRemaining)
{
    const FormatFileConfig &fileConfig = m_document->configAt(
        m_document->symbols().at(token).from
    );
    const int fallback = qMax(1, fileConfig.wrapConditionsOver);
    if (fileConfig.wrapConditionsOverCondition.isEmpty()) {
        return fallback;
    }
    const ArgumentContext context(
        this, token, token, true, &count,
        &spaceRemaining, true
    );
    const QSharedPointer<Config::Scopes::Scope > scope =
        FormatConditionContext::pushCountScope();
    qreal limit = 0;
    if ( ! Config::Condition::value(
            fileConfig.wrapConditionsOverCondition,
            QSet<QString >(),
            &context,
            &limit
        )
    ) {
        return fallback;
    }
    return qMax(0, int(limit));
}

bool FormatScopeConditions::countOver(int token, int count,
                                      const qreal *spaceRemaining,
                                      bool condition)
{
    const FormatFileConfig &fileConfig = m_document->configAt(
        m_document->symbols().at(token).from
    );
    const QStringList &program = condition ? fileConfig.wrapConditionsOverCondition
                                           : fileConfig.wrapArgsOverCondition;
    const int limit = condition ? fileConfig.wrapConditionsOver : fileConfig.wrapArgsOver;
    if (program.isEmpty()) {
        return count > limit;
    }
    const int target = condition ? token : this->argumentTarget(token);
    const ArgumentContext context(
        this, token, target, condition, &count,
        spaceRemaining, true
    );
    const QSharedPointer<Config::Scopes::Scope > scope =
        FormatConditionContext::pushCountScope();
    bool ok = true;
    const bool result = Config::Condition::evaluate(
        program, QSet<QString >(), &context, &ok
    );
    return ok ? result : count > limit;
}
