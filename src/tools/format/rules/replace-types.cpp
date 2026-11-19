// Copyright (C) 2015 The XD Company Ltd.
// SPDX-License-Identifier: Apache-2.0

#include "./replace-types.h"

#include "./args-array.h"
#include "../format-document.h"
#include "../format-file-config.h"
#include "../parser/condition-context.h"

#include "config-condition.h"
#include "config-scopes.h"
#include "config-types.h"

#include <QtCore/QByteArray>
#include <QtCore/QHash>
#include <QtCore/QRegExp>
#include <QtCore/QSet>
#include <QtCore/QString>
#include <QtCore/QStringList>

/*!
    @class FormatReplaceTypesRule
    @internal

    Turns each call, or construction, of a name that a key of
    `[Replace.types]` matches, where `target` holds. A key is a full name from
    the global scope, such as `::QLatin1String`, in which a `*` stands for any
    part of a name (see FormatFileConfig::expandPrevious() for the `&` of a
    key). Its value is one of two things, in which each `*` stands for what
    the key's `*` matched:

    * a name, such as `::QLL`, which renames the call and keeps all of its
      arguments as written;
    * a call template, such as `::MyClass::create($1)`, which turns the call
      into that call (see fillTemplate()).

    A result that calls a name another key matches turns again (see
    replacement()); hence lines chain. Each name is matched and written as
    the location sees it: the namespaces it sits in, and the ones a
    `using namespace` before it brings in, need no qualifier (see
    visibleScopes()). `action = report` reports every call instead.
*/

/*!
    Answers `isNew` of `[Replace.types] target` for one line of a document:
    it holds on a line that `-L` or `--git` selects.
*/
class FormatReplaceTypesContext : public FormatConditionContext
{
    typedef FormatConditionContext super;
public:
    inline FormatReplaceTypesContext(const FormatDocument *document, int line)
        : m_document(document), m_line(line) {}

    inline bool predicate(const QString &name,
                          bool *value) const Q_DECL_OVERRIDE_IL
    {
        if (name.compare(QLL("isNew"), Qt::CaseInsensitive) != 0) {
            return false;
        }
        *value = m_document->isLineSelected(m_line);
        return true;
    }

private:
    const FormatDocument *m_document;
    int m_line;
};

/*!
    Answers `args` for the call that a call template turns (see
    FormatTemplateObject): its arguments, as written.
*/
class FormatCallTemplateContext : public FormatConditionContext
{
    typedef FormatConditionContext super;
public:
    inline explicit FormatCallTemplateContext(FormatArgsArray *arguments)
        : m_arguments(arguments) {}

    inline FormatArgsArray *callArguments() const Q_DECL_OVERRIDE_IL
    {
        return m_arguments;
    }

private:
    FormatArgsArray *m_arguments;
};

/*!
    Builds the full name @a name without its leading `::`, if any.
*/
static QString fullName(const QString & name)
{
    return name.startsWith(QLL("::")) ? name.mid(2) : name;
}

/*!
    Builds the pattern of the key @a key: its full name, in which each `*`
    captures any part of a name.
*/
static QRegExp keyPattern(const QString & key)
{
    const QStringList pieces = fullName(key).split(QLatin1Char('*'));
    QStringList escaped;
    for (int i = 0; i < pieces.size(); ++i) {
        escaped << QRegExp::escape(pieces.at(i));
    }
    return QRegExp(escaped.join(QString(QLL("([A-Za-z0-9_]*)"))));
}

/*!
    Tells whether the last part of the key @a key (after its last `::`) may
    match the identifier @a last.
*/
static bool lastPartMatches(const QString & key, const QString & last)
{
    const QString name = fullName(key);
    const int scope = name.lastIndexOf(QLL("::"));
    return keyPattern(name.mid(scope < 0 ? 0 : scope + 2)).exactMatch(last);
}

static QString readName(const FormatDocument *document, int from, int *end)
{
    QString name;
    int k = from;
    if (document->token(k) == SCOPE) {
        name = QString(QLL("::"));
        ++k;
    }
    while (k < document->symbols().size()
        && document->token(k) == IDENTIFIER
    ) {
        name += QString::fromLatin1(document->lexem(k));
        *end = k;
        if (k + 2 < document->symbols().size()
            && document->token(k + 1) == SCOPE
            && document->token(k + 2) == IDENTIFIER
        ) {
            name += QLL("::");
            k += 2;
        } else {
            break;
        }
    }
    return name;
}
static QStringList visibleScopes(const FormatDocument *document, int index)
{
    QStringList names;
    QList<QStringList > usings;
    names << QString();
    usings << QStringList();
    QString pending;
    bool hasPending = false;
    for (int k = 0; k < index; ++k) {
        if (document->isDirectiveLine(document->symbolLine(k))) {
            continue;
        }
        const Token token = document->token(k);
        if (token == NAMESPACE && k + 1 < index
            && (k == 0 || document->token(k - 1) != USING)
        ) {
            int end = k;
            const QString name = document->token(k + 1) == IDENTIFIER
                ? readName(document, k + 1, &end) : QString();
            if (end + 1 < index && document->token(end + 1) == LBRACE) {
                pending = name.isEmpty() ? names.last()
                    : (names.last().isEmpty() ? name : names.last() + QLL("::") + name);
                hasPending = true;
                k = end;
            }
        } else if (token == USING
            && k + 2 < index
            && document->token(k + 1) == NAMESPACE
        ) {
            int end = k + 1;
            const QString name = readName(document, k + 2, &end);
            if (name.startsWith(QLL("::"))) {
                usings.last() << name.mid(2);
            } else if ( ! name.isEmpty()) {
                usings.last() << name;
                if ( ! names.last().isEmpty()) {
                    usings.last() << names.last() + QLL("::") + name;
                }
            }
            k = end;
        } else if (token == LBRACE) {
            names << (hasPending ? pending : names.last());
            usings << QStringList();
            hasPending = false;
        } else if (token == RBRACE && names.size() > 1) {
            names.removeLast();
            usings.removeLast();
        }
    }
    QStringList scopes;
    QStringList parts = names.last().isEmpty()
        ? QStringList() : names.last().split(QString(QLL("::")));
    while ( ! parts.isEmpty()) {
        scopes << parts.join(QString(QLL("::")));
        parts.removeLast();
    }
    scopes << QString();
    for (int i = 0; i < usings.size(); ++i) {
        for (int u = 0; u < usings.at(i).size(); ++u) {
            if ( ! scopes.contains(usings.at(i).at(u))) {
                scopes << usings.at(i).at(u);
            }
        }
    }
    return scopes;
}

/*!
    Builds the full names that the name @a written (as the source spells it)
    may stand for in a location that sees @a scopes.
*/
static QStringList candidates(const QString & written,
                              const QStringList & scopes)
{
    if (written.startsWith(QLL("::"))) {
        return QStringList() << written.mid(2);
    }
    QStringList names;
    for (int i = 0; i < scopes.size(); ++i) {
        names
            << (scopes.at(i).isEmpty() ? written
                                       : scopes.at(i) + QLL("::") + written);
    }
    return names;
}

/*!
    Finds the first mapping of @a mappings whose key matches one of the full
    names @a names, and gives what its first `*` matched to @a capture; -1
    for none, and for a mapping to `keep-as-is`.
*/
static int mappingOf(const QList<QPair<QString, QString > > & mappings,
                     const QStringList & names, QString *capture)
{
    for (int m = 0; m < mappings.size(); ++m) {
        QRegExp pattern = keyPattern(mappings.at(m).first);
        for (int n = 0; n < names.size(); ++n) {
            if ( ! pattern.exactMatch(names.at(n))) {
                continue;
            }
            if (mappings.at(m).second == QLL("keep-as-is")) {
                return -1;
            }
            *capture = pattern.captureCount() > 0 ? pattern.cap(1) : QString();
            return m;
        }
    }
    return -1;
}

/*!
    Builds the shortest spelling of the full name @a name in a location that
    sees @a scopes: the qualifier of a scope it sees is left out.
*/
static QString shortestName(const QString & name, const QStringList & scopes)
{
    const QStringList parts = fullName(name).split(QString(QLL("::")));
    for (int start = parts.size() - 1; start > 0; --start) {
        const QString prefix = QStringList(parts.mid(0, start))
            .join(QString(QLL("::")));
        if (scopes.contains(prefix)) {
            return QStringList(parts.mid(start)).join(QString(QLL("::")));
        }
    }
    return parts.join(QString(QLL("::")));
}

/*!
    Tells whether the `:` before the token at @a index of @a document ends the
    first branch of a `?:`: a `?` comes before that `:` in the same expression.
*/
static bool isTernaryElse(const FormatDocument *document, int index)
{
    for (int k = index - 2; k >= 0; --k) {
        const Token token = document->token(k);
        if ((token == RPAREN || token == RBRACK || token == RBRACE)
            && document->match(k) >= 0
        ) {
            k = document->match(k);
            continue;
        }
        if (token == QUESTION) {
            return true;
        }
        if (token == SEMIC || token == LBRACE || token == LPAREN
            || token == LBRACK || token == COMMA || token == COLON
        ) {
            return false;
        }
    }
    return false;
}

/*!
    Tells whether the token at @a index of @a document sits in the body of a
    function: the innermost `{` still open before it opens the body of a
    function, or a block within such a body.
*/
static bool inFunctionBody(const FormatDocument *document, int index)
{
    for (int k = index - 1; k >= 0; --k) {
        const Token token = document->token(k);
        if ((token == RBRACE || token == RPAREN || token == RBRACK)
            && document->match(k) >= 0
        ) {
            k = document->match(k);
            continue;
        }
        if (token == LBRACE) {
            const FormatDocument::Scope scope = document->braceScope(k);
            return scope == FormatDocument::FunctionScope
                || scope == FormatDocument::BlockScope;
        }
    }
    return false;
}

/*!
    Tells whether the call that starts at the token @a index of @a document
    is one, not a declaration: it stands in an expression (after an
    operator, an opening bracket, a comma, `return`, or the `:` of a `?:`),
    or starts a statement in a function's body. A constructor's declaration
    and a `new` are none.
*/
static bool isCall(const FormatDocument *document, int index)
{
    if (index < 1) {
        return false;
    }
    switch (document->token(index - 1)) {
    case EQ:
    case EQEQ:
    case NE:
    case LTLT:
    case PLUS:
    case PLUS_EQ:
    case MINUS:
    case NOT:
    case ANDAND:
    case OROR:
    case RETURN:
    case QUESTION:
    case LPAREN:
    case LBRACK:
    case COMMA:
        return true;
    case COLON:
        return isTernaryElse(document, index);
    case SEMIC:
    case LBRACE:
    case RBRACE:
        return inFunctionBody(document, index);
    default:
        return false;
    }
}

/*!
    Builds the arguments of the call whose parentheses are the tokens
    @a open and @a close of @a document, each as the source writes it.
*/
static QStringList callArguments(
        const FormatDocument *document, int open, int close)
{
    QStringList arguments;
    int first = open + 1;
    for (int k = open + 1; k <= close; ++k) {
        const Token token = document->token(k);
        if ((token == LPAREN || token == LBRACK || token == LBRACE)
            && document->match(k) > k && k < close
        ) {
            k = document->match(k);
            continue;
        }
        if (token == COMMA || k == close) {
            if (k > first) {
                const int from = document->symbols().at(first).from;
                arguments << QString::fromLatin1(
                    document->text().mid(
                        from, document->symbolEnd(k - 1) - from
                    )
                );
            }
            first = k + 1;
        }
    }
    return arguments;
}

/*!
    Splits the argument list @a inner (the text between a call's
    parentheses) at its commas outside brackets and quotes.
*/
static QStringList splitArguments(const QString & inner)
{
    QStringList arguments;
    if (inner.trimmed().isEmpty()) {
        return arguments;
    }
    int depth = 0;
    QChar quote;
    int first = 0;
    for (int k = 0; k <= inner.size(); ++k) {
        const QChar c = k < inner.size() ? inner.at(k)
                                         : QChar(QLatin1Char(','));
        if ( ! quote.isNull()) {
            if (c == QLatin1Char('\\')) {
                ++k;
            } else if (c == quote) {
                quote = QChar();
            }
            continue;
        }
        if (c == QLatin1Char('"') || c == QLatin1Char('\'')) {
            quote = c;
        } else if (c == QLatin1Char('(')
            || c == QLatin1Char('[')
            || c == QLatin1Char('{')
        ) {
            ++depth;
        } else if (c == QLatin1Char(')')
            || c == QLatin1Char(']')
            || c == QLatin1Char('}')
        ) {
            --depth;
        } else if (c == QLatin1Char(',') && depth == 0) {
            arguments << inner.mid(first, k - first).trimmed();
            first = k + 1;
        }
    }
    return arguments;
}

/*!
    Builds the values of the literal list @a text, the arguments that a call
    template passes to a member of `args`: each a C++ string literal or a
    whole number. Returns false for any other argument.
*/
static bool literalValues(const QString & text, QVariantList *values)
{
    QStringList pieces;
    QString piece;
    bool quoted = false;
    for (int k = 0; k < text.size(); ++k) {
        const QChar c = text.at(k);
        if (quoted && c == QLatin1Char('\\') && k + 1 < text.size()) {
            piece += text.mid(k, 2);
            ++k;
            continue;
        }
        if (c == QLatin1Char('"')) {
            quoted = ! quoted;
        } else if (c == QLatin1Char(',') && ! quoted) {
            pieces << piece.trimmed();
            piece.clear();
            continue;
        }
        piece += c;
    }
    if (quoted) {
        return false;
    }
    if ( ! piece.trimmed().isEmpty() || ! pieces.isEmpty()) {
        pieces << piece.trimmed();
    }
    for (int i = 0; i < pieces.size(); ++i) {
        const QString & literal = pieces.at(i);
        if (literal.size() >= 2
            && literal.startsWith(QLatin1Char('"'))
            && literal.endsWith(QLatin1Char('"'))
        ) {
            QString value = literal.mid(1, literal.size() - 2);
            value.replace(QLL("\\\""), QLL("\"")).replace(
                QLL("\\\\"),
                QLL("\\")
            );
            *values << value;
            continue;
        }
        bool isNumber = false;
        const int number = literal.toInt(&isNumber);
        if ( ! isNumber) {
            return false;
        }
        *values << number;
    }
    return true;
}

/*!
    Builds what the member @a name of @a object answers with @a values, at
    the place @a context stands for, to @a result; returns false when
    @a object offers no such member, or it has no answer there (such as
    `at()` past the arguments).
*/
static bool memberAnswer(Config::Object *object, const QString & name,
                         const QVariantList & values,
                         const FormatConditionContext *context,
                         QVariant *result)
{
    Config::Scopes::Member member;
    member.object = object;
    member.name = Config::Condition::canonicalName(name);
    return Config::Scopes::answer(member, context, values, result);
}

/*!
    Builds the object that the name @a name of the scopes answers, at the
    place @a context stands for, else null: a name whose answer is no
    Config::Object, such as a plain number, counts as no such name.
*/
static Config::Object *scopeObject(
        const QString & name,
        const FormatConditionContext *context)
{
    QVariant answer;
    const Config::Scopes::Member member = Config::Scopes::find(
        Config::Condition::canonicalName(name)
    );
    if (member.isNull()
        || member.minimumArguments > 0
        || ! Config::Scopes::answer(
            member,
            context,
            QVariantList(),
            &answer
        )
    ) {
        return Q_NULLPTR;
    }
    return Config::Types::object(answer);
}

/*!
    Builds the text that @a object answers at the steps of @a pattern from
    @a *position on, to @a result, and moves @a *position past them: each
    step is `[<index>]`, which calls `at(<index>)`, or `.<name>` and
    `.<name>(<literals>)`, which read or call that member. A step whose
    answer is another object goes on with that object's names. Returns
    false when a member has no answer, or the last answer is an object.
*/
static bool stepsText(const QString & pattern, int *position,
                      Config::Object *object,
                      const FormatConditionContext *context, QString *result)
{
    const QRegExp index(QString(QLL("^\\[(\\d+)\\]")));
    const QRegExp member(QString(QLL(
        "^\\.([A-Za-z_][A-Za-z0-9_]*)"
        "(\\(((?:[^\"()\\\\]|\\\\.|\"(?:[^\"\\\\]|\\\\.)*\")*)\\))?"
    )));
    QVariant answer;
    while (object) {
        const QString rest = pattern.mid(*position);
        QRegExp indexAt = index;
        QRegExp memberAt = member;
        QString name;
        QVariantList values;
        if (indexAt.indexIn(rest) == 0) {
            name = QString(QLL("at"));
            values << indexAt.cap(1).toInt();
            *position += indexAt.matchedLength();
        } else if (memberAt.indexIn(rest) == 0) {
            name = memberAt.cap(1);
            if ( ! literalValues(memberAt.cap(3), &values)) {
                return false;
            }
            *position += memberAt.matchedLength();
        } else {
            break;
        }
        if ( ! memberAnswer(object, name, values, context, &answer)) {
            return false;
        }
        object = Config::Types::object(answer);
    }
    if (object || ! answer.isValid()) {
        return false;
    }
    *result = answer.toString();
    return true;
}

/*!
    Fills the call template @a pattern with @a arguments, the call's own,
    which it reaches as `args`, a name of the global scope (see
    FormatTemplateObject and FormatArgsArray): `args[0]`, `args[1]`, and so
    on stand for one argument as written, and `args.<name>` or
    `args.<name>(<literals>)` for what that member of `args` answers, such
    as `args.join("<separator>")` for all of them, joined by the separator
    (each literal a C++ string literal or a whole number). Any other name of
    the scopes whose answer is an object leads on the same way, once a step
    follows it. `$1`, `$2`, and so on are short for `args[0]`, `args[1]`,
    and so on; `$*` and `$@`, outside double quotes, for `args.join(", ")`;
    and `$$` stands for a `$`. Returns false for an argument the call lacks,
    or a member the object does not offer.
*/
static bool fillTemplate(const QString & pattern, const QStringList & arguments,
                         QString *result)
{
    const QRegExp identifier(QString(QLL("^[A-Za-z_][A-Za-z0-9_]*")));
    FormatArgsArray args(arguments);
    const FormatCallTemplateContext context(&args);
    result->clear();
    bool quoted = false;
    for (int k = 0; k < pattern.size(); ++k) {
        const QString rest = pattern.mid(k);
        QRegExp identifierAt = identifier;
        QString shorthand;

        // Copies a quoted string as it is, but for the `$` shorthands of
        // a single argument.
        if (quoted
            && rest.startsWith(QLatin1Char('\\'))
            && rest.size() > 1
        ) {
            *result += rest.left(2);
            ++k;
            continue;
        }
        if (rest.startsWith(QLatin1Char('"'))) {
            quoted = ! quoted;
            *result += QLatin1Char('"');
            continue;
        }

        if (rest.startsWith(QLL("$$"))) {
            *result += QLatin1Char('$');
            ++k;
            continue;
        }
        if ( ! quoted
            && (rest.startsWith(QLL("$*")) || rest.startsWith(QLL("$@")))
        ) {
            shorthand = QString(QLL(".join(\", \")"));
            ++k;
        } else if (rest.startsWith(QLatin1Char('$'))
            && rest.size() > 1
            && rest.at(1).isDigit()
        ) {
            int end = 1;
            while (end < rest.size() && rest.at(end).isDigit()) {
                ++end;
            }
            shorthand = QLatin1Char('[')
                + QString::number(rest.mid(1, end - 1).toInt() - 1)
                + QLatin1Char(']');
            k += end - 1;
        }

        // Writes what `args` answers for a shorthand.
        if ( ! shorthand.isEmpty()) {
            int position = 0;
            QString text;
            if ( ! stepsText(
                    shorthand,
                    &position,
                    scopeObject(QString(QLL("args")), &context),
                    &context,
                    &text
                )
            ) {
                return false;
            }
            *result += text;
            continue;
        }

        // Writes what a name of the scopes answers, once a step follows it;
        // any other name stays as written.
        const QChar previous = k > 0 ? pattern.at(k - 1) : QChar();
        if (quoted
            || identifierAt.indexIn(rest) != 0
            || previous.isLetterOrNumber()
            || previous == QLatin1Char('_')
            || previous == QLatin1Char(':')
            || previous == QLatin1Char('.')
        ) {
            *result += pattern.at(k);
            continue;
        }
        const QString name = identifierAt.cap(0);
        int position = k + name.size();
        Config::Object *object = Q_NULLPTR;
        if (position < pattern.size()
            && (pattern.at(position) == QLatin1Char('[')
                || pattern.at(position) == QLatin1Char('.'))
        ) {
            object = scopeObject(name, &context);
        }
        if ( ! object) {
            *result += name;
            k = position - 1;
            continue;
        }
        QString text;
        if ( ! stepsText(pattern, &position, object, &context, &text)) {
            return false;
        }
        *result += text;
        k = position - 1;
    }
    return true;
}

/*!
    Builds what a call matched by mapping @a m of @a mappings, whose key's
    `*` matched @a capture, turns into, with @a arguments: the name to call,
    as a full name, to @a head, and the parenthesized arguments to @a call.
    A result that calls a name another key matches turns again, at most as
    often as there are mappings; hence mappings chain, and a cycle ends.
    Returns false for an argument a template needs that the call lacks.
*/
static bool replacement(
        const QList<QPair<QString, QString > > & mappings, int m,
        QString capture, QStringList arguments,
        QString *head, QString *call)
{
    for (int step = 0; step <= mappings.size(); ++step) {
        // Writes what the key's `*` matched in place of each `*` of the value,
        // but for the `$*` shorthand.
        QString value;
        const QString pattern = mappings.at(m).second;
        for (int k = 0; k < pattern.size(); ++k) {
            if (pattern.at(k) == QLatin1Char('*')
                && (k == 0 || pattern.at(k - 1) != QLatin1Char('$'))
            ) {
                value += capture;
            } else {
                value += pattern.at(k);
            }
        }
        const int paren = value.indexOf(QLatin1Char('('));
        if (paren < 0) {
            *head = value.trimmed();
            *call = QLatin1Char('(')
                + arguments.join(QString(QLL(", ")))
                + QLatin1Char(')');
        } else {
            QString filled;
            if ( ! fillTemplate(value, arguments, &filled)) {
                return false;
            }
            const int filledParen = filled.indexOf(QLatin1Char('('));
            *head = filled.left(filledParen).trimmed();
            *call = filled.mid(filledParen);
        }
        const int next = mappingOf(
            mappings,
            QStringList() << fullName(*head),
            &capture
        );
        if (next < 0 || next == m || ! call->endsWith(QLatin1Char(')'))) {
            return true;
        }
        arguments = splitArguments(call->mid(1, call->size() - 2));
        m = next;
    }
    return true;
}

/*!
    Writes, or reports, each call of a name that `[Replace.types]` maps,
    outside of preprocessor directives, where the location's `target` holds.
*/
void FormatReplaceTypesRule::run(FormatDocument *document) const
{
    QHash<QString, QStringList > programs;
    for (int i = 0; i + 2 < document->symbols().size(); ++i) {
        if (document->token(i) != IDENTIFIER
            || document->token(i + 1) != LPAREN
            || document->match(i + 1) <= i + 1
            || document->isDirectiveLine(document->symbolLine(i))
        ) {
            continue;
        }
        const int close = document->match(i + 1);
        const FormatFileConfig &config = document->configAt(
            document->symbols().at(i).from
        );
        const QString last = QString::fromLatin1(document->lexem(i));
        bool mayMatch = false;
        for (int m = 0; m < config.replaceTypes.size() && ! mayMatch; ++m) {
            mayMatch = lastPartMatches(
                config.replaceTypes.at(m).first,
                last
            );
        }
        if ( ! mayMatch) {
            continue;
        }

        // Finds the start of the name as written, `A::B::Name` or `::Name`.
        int start = i;
        while (start >= 2 && document->token(start - 1) == SCOPE
            && document->token(start - 2) == IDENTIFIER
        ) {
            start -= 2;
        }
        if (start >= 1 && document->token(start - 1) == SCOPE) {
            --start;
        }
        if ( ! isCall(document, start)) {
            continue;
        }
        int end = i;
        const QString written = readName(document, start, &end);
        const QStringList scopes = visibleScopes(document, start);
        QString capture;
        const int mapping = mappingOf(
            config.replaceTypes,
            candidates(written, scopes),
            &capture
        );
        if (mapping < 0) {
            continue;
        }

        // Checks `target` for this line.
        QStringList *program = &(programs[config.replaceTypesTarget]);
        QString error;
        if (program->isEmpty()
            && ! Config::Condition::compile(
                config.replaceTypesTarget,
                FormatFileConfig::replaceTargetNames(),
                program,
                &error,
                false,
                Config::Condition::NameMismatchIgnore
            )
        ) {
            continue;
        }
        const FormatReplaceTypesContext context(
            document,
            document->symbolLine(i)
        );
        QVariant wanted;
        if ( ! FormatFileConfig::resolve(
                QString(QLL("replace.types.target")),
                *program,
                &context,
                &wanted
            )
            || ! wanted.toBool()
        ) {
            continue;
        }

        const int from = document->symbols().at(start).from;
        QString head;
        QString call;
        if ( ! replacement(
                config.replaceTypes,
                mapping,
                capture,
                callArguments(document, i + 1, close),
                &head,
                &call
            )
        ) {
            document->report(from, "replace-types",
                             QString(QLL("%1 lacks an argument that its [Replace.types] "
                                         "template takes")).arg(written));
            continue;
        }
        const QString shortest = shortestName(head, scopes);
        const int to = document->symbolEnd(close);
        if (config.replaceTypesAction == QLL("replace")
            && document->replace(
                from,
                to - from,
                (shortest + call).toLatin1()
            )
        ) {
            i = close;
            continue;
        }
        document->report(from, "replace-types",
                         QString(QLL("write %1%2")).arg(shortest, call));
    }
}
