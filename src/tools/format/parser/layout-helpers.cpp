// Copyright (C) 2015 The XD Company Ltd.
// SPDX-License-Identifier: Apache-2.0

#include "./layout-helpers.h"

#include "../format-document.h"
#include "./code-helpers.h"
#include "./condition-context.h"
#include "./expression-layout.h"
#include "./scope-conditions.h"
#include "../format-file-config.h"

class FormatOffsetContext : public FormatConditionContext
{
public:
    inline FormatOffsetContext(const FormatDocument *document, int offset, int token)
        : m_document(document), m_offset(offset), m_token(token) {}

    inline QStringList onNames() const Q_DECL_OVERRIDE_IL
    { return FormatScopeConditions::names(); }

    inline const FormatFileConfig *fileConfig() const Q_DECL_OVERRIDE_IL
    { return &m_document->configAt(m_offset); }

    bool predicate(const QString & name, bool *value) const Q_DECL_OVERRIDE
    {
        Config::Cache::skip();
        if (name == QLL("isNew")) {
            *value = m_document->isLineSelected(m_document->lineOf(m_offset));
            return true;
        }
        if (name == QLL("isHeader") || name == QLL("isNonHeader")) {
            *value = m_document->isHeader() == (name == QLL("isHeader"));
            return true;
        }
        if (name == QLL("always") || name == QLL("never")) {
            *value = name == QLL("always");
            return true;
        }
        if (m_token >= m_document->symbols().size()) {
            return false;
        }
        FormatScopeConditions conditions(m_document);
        return conditions.nameHolds(m_token, name, value);
    }

    bool argumentCount(int depth, int argument, qreal *count) const Q_DECL_OVERRIDE
    {
        if (m_token >= m_document->symbols().size()) {
            return false;
        }
        FormatScopeConditions conditions(m_document);
        *count = conditions.argumentCount(m_token, depth, argument);
        return true;
    }

    bool value(qreal *count) const Q_DECL_OVERRIDE
    {
        return this->argumentCount(0, 0, count);
    }

    bool spaceRemaining(qreal *percent) const Q_DECL_OVERRIDE
    {
        if (m_token >= m_document->symbols().size()) {
            return false;
        }
        FormatScopeConditions conditions(m_document);
        *percent = conditions.spaceRemaining(m_token);
        return true;
    }

    bool isTarget(const QString & kind, bool *value) const Q_DECL_OVERRIDE
    {
        if (m_token >= m_document->symbols().size()) {
            return false;
        }
        FormatScopeConditions conditions(m_document);
        return conditions.targetIs(m_token, kind, value);
    }

private:
    const FormatDocument *m_document;
    int m_offset;
    int m_token;
};

/*!
    Builds the column the code wrap rules break a line at: the first
    `[Structure] wrapBegin` column, never past `[Structure] wrapMax`; a line
    wider than it does not fit. Uses the settings at @a offset.
*/
int wrapColumn(const FormatDocument *document, int offset)
{
    const FormatFileConfig & fileConfig = document->configAt(offset);
    const int maximum = wrapMaxAt(document, offset);
    int begin = fileConfig.wrapBegin.isEmpty()
        ? maximum : fileConfig.wrapBegin.first();
    const QString key = QString(QLL("structure.wrapBegin"));
    begin = FormatFileConfig::countOf(settingValueAt(document, offset, key, begin));
    return qMin(begin, maximum);
}

/*!
    Builds the column no line may pass, `[Structure] wrapMax` at @a offset;
    at least 1, hence a share of it never divides by zero.
*/
int wrapMaxAt(const FormatDocument *document, int offset)
{
    const FormatFileConfig &config = document->configAt(offset);
    const QString key = QString(QLL("structure.wrapMax"));
    const QVariant value = settingValueAt(document, offset, key, config.wrapMax);
    return qMax(1, FormatFileConfig::countOf(value));
}

QVariant settingValueAt(const FormatDocument *document, int offset,
                        const QString & key, const QVariant & fallback)
{
    const FormatFileConfig &config = document->configAt(offset);
    if ( ! config.hasExpression(key)) {
        return fallback;
    }
    const int token = document->symbolAfter(offset);
    if (token < document->symbols().size()
        && document->symbols().at(token).from == offset
    ) {
        FormatScopeConditions conditions(document);
        const QVariant value = conditions.settingAt(key, token);
        if (value.isValid()) {
            return value;
        }
    } else {
        const FormatOffsetContext context(document, offset, token);
        const QVariant value = config.resolveSetting(key, &context);
        if (value.isValid()) {
            return value;
        }
    }
    return fallback;
}

bool operatorsInMiddleAt(const FormatDocument *document, int offset)
{
    const bool middle = document->configAt(offset).operatorsInMiddle;
    return settingValueAt(document, offset, QString(QLL("structure.operatorPlacement")),
        QString(middle ? QLL("middle") : QLL("start"))).toString() == QLL("middle");
}

bool codeTargetAnyAt(const FormatDocument *document, int offset)
{
    const bool any = document->configAt(offset).codeTarget == FormatFileConfig::CodeTargetAny;
    return settingValueAt(document, offset, QString(QLL("structure.target")),
        QString(any ? QLL("any") : QLL("long"))).toString() == QLL("any");
}

bool compactAlwaysAt(const FormatDocument *document, int offset)
{
    const bool always = document->configAt(offset).compact == FormatFileConfig::CompactAlways;
    return settingValueAt(document, offset, QString(QLL("structure.compact")),
        QString(always ? QLL("always") : QLL("never"))).toString() == QLL("always");
}

/*!
    Returns the column @a steps (4 spaces each) past @a indent.
*/
int continuationIndent(int indent, int steps)
{
    return indent + 4 * steps;
}

/*!
    Builds spaces up to the column @a steps past @a indent.
*/
QByteArray continuationPad(int indent, int steps)
{
    return QByteArray(continuationIndent(indent, steps), ' ');
}

/*!
    Tells whether @a token is a binary operator that may start a line.
*/
bool isBinaryOperator(Token token)
{
    switch (token) {
    case PLUS: case MINUS: case STAR: case SLASH: case PERCENT:
    case LTLT: case GTGT: case AND: case OR: case HAT:
    case ANDAND: case OROR: case EQEQ: case NE:
    case LANGLE: case RANGLE: case LE: case GE:
    case QUESTION: case COLON:
        return true;
    default:
        return false;
    }
}

/*!
    Tells whether the token at @a index of @a document is inside the
    parentheses of a control statement.
*/
bool isControlHead(const FormatDocument *document, int index)
{
    for (int i = index - 1; i >= 0; --i) {
        const Token token = document->token(i);
        if (token == RBRACE
            && document->match(i) >= 0
            && document->braceScope(document->match(i))
                == FormatDocument::InitScope
        ) {
            i = document->match(i);
            continue;
        }
        if (token == LBRACE || token == RBRACE) {
            break;
        }
        if ((token == RPAREN
                || token == RBRACK)
            && document->match(i) >= 0
        ) {
            i = document->match(i);
            continue;
        }
        if (token == LPAREN && document->match(i) >= index) {
            const Token keyword = document->token(i - 1);
            if (keyword == IF || keyword == WHILE || keyword == FOR) {
                return true;
            }
        }
    }
    return false;
}

/*!
    Finds the preceding operator when the operand before @a next in
    @a document is a call or parenthesized group in the same binary or
    conditional chain.
*/
int precedingChainOperator(
        const FormatDocument *document, int next, bool respectPlacement)
{
    const Token token = document->token(next);
    const int close = next - 1;
    if ( ! isBinaryOperator(token) || document->token(close) != RPAREN) {
        return -1;
    }
    const int open = document->match(close);
    int first = open;
    // Include the qualified name of a call in the operand.
    if (document->token(first - 1) == IDENTIFIER) {
        --first;
        while ((document->token(first - 1) == DOT || document->token(first - 1) == ARROW
                || document->token(first - 1) == SCOPE)
            && (document->token(first - 2) == IDENTIFIER
                || document->token(first - 2) == THIS)
        ) {
            first -= 2;
        }
    }
    const int previous = first - 1;
    const Token lead = document->token(previous);
    const bool conditional = lead == QUESTION && token == COLON;
    const int rank = operatorPrecedence(token);
    const bool controlHead = isControlHead(document, next);
    const bool binary = rank > 0
        && ! (controlHead && (token == ANDAND || token == OROR))
        && operatorPrecedence(lead) == rank
        && ( ! respectPlacement
            || ! operatorsInMiddleAt(document, document->symbols().at(next).from))
        && isOperandEnd(document->token(previous - 1));
    return conditional || binary ? previous : -1;
}

/*!
    Tells whether @a text holds only closing brackets and whitespace.
*/
bool isClosingLine(const QByteArray & text)
{
    const QByteArray line = text.trimmed();
    if (line.isEmpty()) {
        return false;
    }
    for (int i = 0; i < line.size(); ++i) {
        const char character = line.at(i);
        if (character != ')' && character != ']' && character != '}'
            && character != ' ' && character != '\t'
        ) {
            return false;
        }
    }
    return true;
}

/*!
    Tells whether the closing @a bracket of @a document opened on an earlier
    line than its own.
*/
bool closesEarlierLine(const FormatDocument *document, int bracket)
{
    const Token token = document->token(bracket);
    const int open = document->match(bracket);
    return (token == RPAREN || token == RBRACK || token == RBRACE)
        && open >= 0
        && document->symbolLine(open) < document->symbolLine(bracket);
}

/*!
    Tells whether a line of the bracket group from the token @a open of
    @a document to its closing token @a close, which opens on a line indented
    by @a indent, sits at another indent than its brackets give it.

    The padding of a line counts one step (4 spaces)
    per bracket that a line above it left open, and taken away by the
    bracket's closing one on a later line, from that closing line on.
    @a extraPad adds steps while the outer bracket remains open.
    Inside such a bracket, an argument that goes on past the end of its
    line adds one more step to its next lines, until the `,` that ends it,
    unless that line left a bracket open, closes a bracket opened on an
    earlier line, ends with a string literal, or starts with a string literal
    (see startsWithStringLiteral()). Adjacent string literals take that extra
    step; a bracket opened while an argument had that step keeps it for its
    own lines. A line that starts with a
    binary operator, right below another one that does, sits at that
    line's indent; hence the operator lines of one chain line up.
*/
bool hasMisplacedLines(const FormatDocument *document, int open, int close,
                       int indent, int extraPad)
{
    const int firstLine = document->symbolLine(open);
    const int lastLine = document->symbolLine(close);

    // Lists the brackets open at the current line: each one's closing token,
    // and whether the argument its content is at has the extra step.
    QList<QPair<int, bool > > ranges;
    int k = open;
    bool aboveLeadsWithOperator = false;
    int aboveIndent = -1;
    for (int l = firstLine; l <= lastLine; ++l) {
        // Closes the ranges whose closing brackets lead the line.
        while (k <= close && document->symbolLine(k) == l
            && ! ranges.isEmpty() && ranges.last().first == k
        ) {
            ranges.removeLast();
            ++k;
        }

        // Checks the line's indent: an operator line below another one
        // takes its indent, any other line the one its brackets give it.
        const int lead = document->firstSymbolOfLine(l);
        const bool leadsWithOperator = lead >= 0
            && isBinaryOperator(document->token(lead));
        if (l > firstLine && lead > open && lead <= close) {
            int wanted = aboveIndent;
            if ( ! leadsWithOperator
                || ! aboveLeadsWithOperator
                || aboveIndent < 0
            ) {
                int steps = ranges.isEmpty() ? 0 : extraPad;
                for (int r = 0; r < ranges.size(); ++r) {
                    steps += ranges.at(r).second ? 2 : 1;
                }
                wanted = continuationIndent(indent, steps);
            }
            if (textWidth(document->indentOf(l)) != wanted) {
                return true;
            }
            aboveIndent = wanted;
        }
        aboveLeadsWithOperator = leadsWithOperator;

        // Opens and closes the ranges of the rest of the line.
        bool opened = false;
        int last = -1;
        for (; k <= close && document->symbolLine(k) == l; ++k) {
            const Token token = document->token(k);
            const int other = document->match(k);
            last = k;
            if ((token == LPAREN || token == LBRACK || token == LBRACE)
                && other > k && document->symbolLine(other) > l
            ) {
                ranges.append(qMakePair(other, false));
                opened = true;
            } else if ( ! ranges.isEmpty() && ranges.last().first == k) {
                ranges.removeLast();
            }
        }

        // Gives the argument that goes on past the line its extra step, or
        // takes it away at its `,`.
        if (last < 0 || ranges.isEmpty()) {
            continue;
        }
        const Token end = document->token(last);
        if (end == COMMA) {
            ranges.last().second = false;
        } else if (end == STRING_LITERAL
            && startsWithStringLiteral(document, last + 1)
        ) {
            ranges.last().second = true;
        } else if ( ! opened && ! closesEarlierLine(document, last)
            && end != STRING_LITERAL
            && lead >= 0 && ! startsWithStringLiteral(document, lead)
        ) {
            ranges.last().second = true;
        }
    }
    return false;
}

/*!
    Tells whether the token @a token of @a document starts a string literal:
    a `"` string, or one with an encoding or raw prefix (`u8"`, `u"`, `U"`,
    `L"`, `R"`, and the like), which the tokenizer reads as a name right
    before the string.
*/
bool startsWithStringLiteral(const FormatDocument *document, int token)
{
    if (token < 0 || token >= document->symbols().size()) {
        return false;
    }
    if (document->token(token) == STRING_LITERAL) {
        return true;
    }
    static const char *const prefixes[] = {
        "u8", "u", "U", "L", "R", "u8R", "uR", "UR", "LR"
    };
    if (document->token(token) != IDENTIFIER
        || token + 1 >= document->symbols().size()
        || document->token(token + 1) != STRING_LITERAL
        || document->symbols().at(token + 1).from != document->symbolEnd(token)
    ) {
        return false;
    }
    const QByteArray name = document->lexem(token);
    for (size_t i = 0; i < sizeof(prefixes) / sizeof(prefixes[0]); ++i) {
        if (name == prefixes[i]) {
            return true;
        }
    }
    return false;
}

/*!
    Builds the text of @a document from the start of the token @a first up to
    the end of the token @a last, as the file spells it.
*/
QByteArray sourceText(const FormatDocument *document, int first, int last)
{
    if (last < first) {
        return QByteArray();
    }
    const int from = document->symbols().at(first).from;
    return document->text().mid(from, document->symbolEnd(last) - from);
}

/*!
    Finds the first `?` of a conditional expression among the tokens from
    @a first up to @a last of @a document, outside of every bracket pair,
    and its matching `:`. Returns false when either token is absent.
*/
bool findTernary(const FormatDocument *document, int first, int last,
                 int *question, int *colon)
{
    *question = -1;
    *colon = -1;
    int depth = 0;
    // Match the outer conditional while skipping nested brackets.
    for (int k = first; k <= last; ++k) {
        const Token token = document->token(k);
        if ((token == LPAREN
                || token == LBRACK
                || token == LBRACE)
            && document->match(k) > k
        ) {
            k = document->match(k);
        } else if (token == QUESTION) {
            if (*question < 0) {
                *question = k;
            }
            ++depth;
        } else if (token == COLON && depth > 0 && --depth == 0) {
            *colon = k;
            return true;
        }
    }
    return false;
}

/*!
    Tells whether a `<` after @a token opens template arguments: after a
    name, a cast keyword, or `template`.
*/
static bool opensTemplate(Token token)
{
    return token == IDENTIFIER || token == STATIC_CAST || token == DYNAMIC_CAST
        || token == CONST_CAST || token == REINTERPRET_CAST || token == TEMPLATE;
}

/*!
    Builds the index of the `>` that closes the template arguments opened by
    the `<` at @a angle of @a document, before @a end; -1 when a `(`, `&&`,
    `||`, `?` or `;` comes first, hence the `<` compares.
*/
int templateClose(const FormatDocument *document, int angle, int end)
{
    int depth = 0;
    for (int i = angle; i < end; ++i) {
        const Token token = document->token(i);
        if (token == LPAREN
            || token == ANDAND
            || token == OROR
            || token == QUESTION
            || token == SEMIC
        ) {
            return -1;
        }
        if (token == LANGLE) {
            ++depth;
        } else if (token == RANGLE) {
            --depth;
        } else if (token == GTGT) {
            depth -= 2;
        }
        if (depth <= 0) {
            return i;
        }
    }
    return -1;
}

/*!
    Builds the indexes of the operators that break the tokens @a first up to
    @a last of @a document: those of the loosest rank that @a rankOf gives,
    from @a fromRank on, outside of nested brackets and template arguments,
    and that are no unary sign (one that starts the range or follows no end
    of an operand). Gives that rank to @a rank, unless it is null. Builds an
    empty list when no operator ranks, or when a brace stands at the
    range's own level.

    @a rankOf gives each operator its rank, the loosest 0, and -1 to a
    token that does not break; hence a rule breaks first where its tokens
    hold together least.
*/
QList<int > loosestOperators(
        const FormatDocument *document, int first, int last,
        int (*rankOf)(Token), int fromRank, int *rank)
{
    QList<int > found;
    int best = -1;
    for (int k = first; k <= last; ++k) {
        const Token token = document->token(k);
        if (token == LBRACE) {
            return QList<int >();
        }
        if ((token == LPAREN || token == LBRACK)
            && document->match(k) > k
        ) {
            k = document->match(k);
            continue;
        }
        if (token == LANGLE && k > first
            && opensTemplate(document->token(k - 1))
        ) {
            const int close = templateClose(document, k, last + 1);
            if (close > k) {
                k = close;
                continue;
            }
        }
        const int tokenRank = rankOf(token);
        if (tokenRank < fromRank
            || (best >= 0 && tokenRank > best)
            || k == first
            || ! isOperandEnd(document->token(k - 1))
        ) {
            continue;
        }
        if (tokenRank < best || best < 0) {
            found.clear();
            best = tokenRank;
        }
        found << k;
    }
    if (rank) {
        *rank = best;
    }
    return found;
}

/*!
    Tells whether the token at @a token of @a document stands at or past
    `[Structure] wrapArgsAt`: whether its condition (see
    FormatScopeConditions::holds()) holds, its `spaceUsed` being the share
    of `[Structure] wrapMax` the line uses up to right after the token.
    A list may break after such a `(`; a value may move below such an `=`.
*/
bool reachesWrapArgsAt(const FormatDocument *document,
                       FormatScopeConditions *conditions, int token)
{
    const FormatFileConfig &fileConfig =
        document->configAt(document->symbols().at(token).from);
    return conditions->holds(
        QString(QLL("structure.wrapArgsAt")),
        fileConfig.wrapArgsAt,
        token,
        FormatFileConfig::schema()
            ->programFor(QString(QLL("structure.wrapChainAt")))
    );
}

/*!
    Ranks @a token among the comparison operators, from the loosest:
    `==` and `!=`, then `<`, `>`, `<=` and `>=`; -1 for any other token.
*/
int comparisonRank(Token token)
{
    switch (token) {
    case EQEQ:
    case NE:
        return 0;
    case LANGLE:
    case RANGLE:
    case LE:
    case GE:
        return 1;
    default:
        return -1;
    }
}

/*!
    Builds the lines of the operand from the token @a first up to @a last
    of @a document, written from @a column of a line indented by @a indent:
    one line while it fits `[Structure] wrapMax`, hence a comparison stays
    whole on a line of its own where it can; else broken before each of its
    loosest comparison operators (see loosestOperators()), each such line
    @a step columns past @a indent.
*/
QList<QByteArray > comparisonLines(
        const FormatDocument *document, int first,
        int last, int column, int indent, int step)
{
    const QByteArray text = sourceText(document, first, last);
    const QList<int > operators = loosestOperators(
        document, first, last, &comparisonRank, 0, Q_NULLPTR
    );
    if (column + textWidth(text)
        <= wrapMaxAt(document, document->symbols().at(first).from)
        || operators.isEmpty()
    ) {
        return QList<QByteArray >() << text;
    }
    const QByteArray pad(indent + step, ' ');
    QList<QByteArray > lines;
    lines << sourceText(document, first, operators.first() - 1);
    for (int k = 0; k < operators.size(); ++k) {
        const int end = k + 1 < operators.size()
            ? operators.at(k + 1) - 1 : last;
        lines << pad + sourceText(document, operators.at(k), end);
    }
    return lines;
}

/*!
    Builds the first and last token of each argument between the brackets at
    @a open and @a close of @a document, split at the commas outside of
    nested brackets and template arguments (a `<` after a name, closed
    before any `(`, `&&`, `||`, `?` or `;`; any other `<` compares); an
    empty list for no argument. A trailing comma (`{ a, b, }`) ends the last
    argument, and adds no empty one.
*/
QList<QPair<int, int > > splitArguments(
        const FormatDocument *document,
        int open, int close)
{
    QList<QPair<int, int > > arguments;
    if (close <= open + 1) {
        return arguments;
    }
    int from = open + 1;
    for (int i = open + 1; i < close; ++i) {
        const Token token = document->token(i);
        if ((token == LPAREN || token == LBRACK || token == LBRACE)
            && document->match(i) > i
        ) {
            i = document->match(i);
            continue;
        }
        if (token == LANGLE && document->token(i - 1) == IDENTIFIER) {
            i = qMax(i, templateClose(document, i, close));
            continue;
        }
        if (token == COMMA) {
            arguments << qMakePair(from, i - 1);
            from = i + 1;
        }
    }

    // Leaves out the empty item after a trailing comma (`{ a, b, }`).
    if (from <= close - 1) {
        arguments << qMakePair(from, close - 1);
    }
    return arguments;
}

/*!
    Builds the lines of a conditional expression whose part before the `?`
    is @a head, whose `?` part is @a middle, and whose `:` part is @a tail,
    written from the column @a column of a line indented by @a indent, with
    @a trailing characters after it on its last line: one line when that
    fits the @a wrap column; else tries a break at `:` aligned under `?`,
    then at `?`, then at both signs on continuation lines.
*/
QList<QByteArray > ternaryLines(
        const QByteArray & head, const QByteArray & middle,
        const QByteArray & tail, int indent, int column, int trailing,
        int wrap)
{
    const QByteArray oneLine = head + ' ' + middle + ' ' + tail;
    if (column + textWidth(oneLine) + trailing <= wrap) {
        return QList<QByteArray >() << oneLine;
    }
    const QByteArray colonPad(column + textWidth(head) + 1, ' ');
    if (column + textWidth(head) + 1 + textWidth(middle) <= wrap
        && textWidth(colonPad) + textWidth(tail) + trailing <= wrap
    ) {
        return QList<QByteArray >() << head + ' ' + middle << colonPad + tail;
    }
    const QByteArray pad = continuationPad(indent);
    const QByteArray rest = pad + middle + ' ' + tail;
    if (textWidth(rest) + trailing <= wrap) {
        return QList<QByteArray >() << head << rest;
    }
    return QList<QByteArray >() << head << pad + middle << pad + tail;
}

/*!
    Tells whether the `(` at @a paren of @a document opens the parameter
    list of a function's declaration or definition, at file, namespace or
    class or block scope: its name follows a type, or it is a constructor,
    a destructor, or an operator, such as `operator==`, `operator()`,
    or a conversion (`operator bool`, `operator QString`).
*/
bool isDeclarationParen(
        const FormatDocument *document,
        int paren)
{
    const FormatDocument::Scope scope = document->scopeOf(paren);
    if (scope != FormatDocument::FileScope
        && scope != FormatDocument::NamespaceScope
        && scope != FormatDocument::ClassScope
        && scope != FormatDocument::FunctionScope
        && scope != FormatDocument::BlockScope
    ) {
        return false;
    }

    // Finds where the name starts: at the `operator` keyword of an operator,
    // whose symbol or conversion type (up to 16 tokens) comes between it and
    // the `(`; a `;`, a brace, or a `:` ends the search, as does a bracket
    // other than the `()` of `operator()`.
    int name = paren - 1;
    bool isOperator = false;
    for (int k = paren - 1; k >= qMax(1, paren - 16); --k) {
        const Token token = document->token(k);
        if (token == OPERATOR) {
            name = k;
            isOperator = true;
            break;
        }
        if (token == SEMIC
            || token == LBRACE
            || token == RBRACE
            || token == COLON
            || ((token == LPAREN || token == RPAREN) && k < paren - 2)
        ) {
            break;
        }
    }
    if ( ! isOperator && document->token(name) != IDENTIFIER) {
        return false;
    }
    if ( ! isOperator && document->token(name - 1) == TILDE) {
        return true;
    }
    const int unqualified = name;
    while (name >= 2
        && document->token(name - 1) == SCOPE
        && document->token(name - 2) == IDENTIFIER
    ) {
        name -= 2;
    }
    const Token typeEnd = document->token(name - 1);
    if (isTypeEnd(typeEnd) || typeEnd == AND) {
        return true;
    }

    // A qualified name with no type before it: an operator, such as a
    // conversion, or a constructor, whose name repeats its class's.
    if (name < unqualified) {
        return isOperator
            || document->lexem(paren - 1) == document->lexem(paren - 3);
    }
    const bool afterAccess = document->isAccessLabel(name - 1);
    return scope == FormatDocument::ClassScope
        && (typeEnd == SEMIC || typeEnd == LBRACE || typeEnd == RBRACE || afterAccess
            || typeEnd == EXPLICIT || typeEnd == INLINE);
}

/*!
    Tells whether the `:` at @a index of @a document belongs to a `?` of its
    own statement, rather than starting an initializer list or a label.
*/
static bool isTernaryColon(const FormatDocument *document, int index)
{
    for (int k = index - 1; k >= 0; --k) {
        const Token token = document->token(k);
        if ((token == RPAREN || token == RBRACK)
            && document->match(k) >= 0
        ) {
            k = document->match(k);
            continue;
        }
        if (token == QUESTION) {
            return true;
        }
        if (token == SEMIC || token == LBRACE || token == RBRACE) {
            return false;
        }
    }
    return false;
}

/*!
    Places the operators on either side of a multiline operand at the same
    indentation. A call closes on its last argument. A parenthesized
    operand keeps its own closing layout.
*/
bool layoutChainOperand(
        FormatDocument *document, FormatScopeConditions *conditions, int next)
{
    const int close = next - 1;
    if ( ! closesEarlierLine(document, close)) {
        return false;
    }
    const int previous = precedingChainOperator(document, next);
    if (previous < 1) {
        return false;
    }
    const int first = previous + 1;
    const int open = document->match(close);
    const int from = document->symbolEnd(previous - 1);
    const int end = document->symbols().at(next).from;
    if (document->hasCommentBetween(from, end)
        || document->spansDirective(from, end)
    ) {
        return false;
    }
    const int line = document->symbolLine(previous);
    const bool startsLine = document->firstSymbolOfLine(line) == previous;
    const bool conditional = document->token(previous) == QUESTION;
    const QByteArray pad = startsLine
        ? document->indentOf(line)
        : continuationPad(textWidth(document->indentOf(line)),
                          conditional ? 1 : conditions->operatorPadding(previous));
    const QByteArray newline = document->text().mid(
        document->lineEnd(line) - 1,
        1
    ) == "\r"
        ? "\r\n" : "\n";
    const QByteArray gap = newline + pad;
    const int closeEnd = document->symbolEnd(close);
    if (first < open) {
        const bool keepInline = ! startsLine && ! conditional;
        const int layoutIndent = keepInline
            ? textWidth(document->indentOf(line))
            : textWidth(pad);
        FormatExpressionLayout layout(document, conditions, layoutIndent);
        const int lineStart = document->lineStart(line);
        const int operatorColumn = keepInline
            ? textWidth(document->text().mid(lineStart, from - lineStart)) + 1
            : textWidth(pad);
        const int column = operatorColumn
            + document->lexem(previous).size()
            + 1;
        const QByteArray flat = layout.oneLine(first, close);
        QByteArray operand = flat;
        const int limit = conditional
            ? wrapMaxAt(document, from)
            : wrapColumn(document, from);
        if (column + textWidth(flat) > limit) {
            const int extraPad = keepInline
                ? conditions->operatorPadding(previous) : 0;
            operand = layout.layoutList(
                first,
                open,
                close,
                column,
                true,
                0,
                extraPad
            );
            if (newline == "\r\n") {
                operand.replace("\r\n", "\n");
                operand.replace("\n", newline);
            }
        }
        const QByteArray leadGap = keepInline ? QByteArray(" ") : gap;
        const QByteArray replacement = leadGap
            + document->lexem(previous)
            + ' '
            + operand
            + gap;
        if (replacement != document->text().mid(from, end - from)) {
            document->replace(from, end - from, replacement);
        }
    } else if (document->text().mid(closeEnd, end - closeEnd) != gap) {
        document->replace(closeEnd, end - closeEnd, gap);
    }
    return true;
}

/*!
    Joins the line that the operator at @a index of @a document leads to the
    line before it, when that one holds only closing brackets and the joined
    line fits `wrapMax`. The `&&` and `||` operators, a prefix `++` or
    `--`, a `:` without its `?`, a block's `}`, and the head of a control
    statement (whose body follows it) keep their lines. Returns whether the
    edit was queued.
*/
bool joinsClosingLine(FormatDocument *document, int index)
{
    const Token token = document->token(index);
    // Keeps logical operators on their own continuation lines.
    if (token == ANDAND || token == OROR) {
        return false;
    }
    const Token next = document->token(index + 1);
    const bool isPostfix = (token == INCR || token == DECR)
        && (next == SEMIC || next == RPAREN || next == RBRACK || next == COMMA);
    const bool isTight = isPostfix || token == DOT || token == ARROW
        || token == DOTSTAR || token == ARROW_STAR;
    // Joins expression operators, leaving labels and prefix increments alone.
    if ( ! isTight && ! isAssignmentOperator(token)
        && ( ! isBinaryOperator(token)
            || (token == COLON && ! isTernaryColon(document, index)))
    ) {
        return false;
    }
    const int line = document->symbolLine(index);
    const int before = index - 1;
    // Requires adjacent lines with only closing brackets on the first one,
    // and leaves comments and directives alone.
    if (line < 2
        || document->firstSymbolOfLine(line) != index
        || document->lastSymbolOfLine(line) == index
        || document->symbolEndLine(before) != line - 1
        || document->isDirectiveLine(line)
        || document->isDirectiveLine(line - 1)
        || ! isClosingLine(document->lineText(line - 1))
        || document->hasCommentBetween(
            document->symbolEnd(before),
            document->lineEnd(line)
        )
    ) {
        return false;
    }
    const int open = document->match(before);
    const Token closer = document->token(before);
    // Joins initializer braces, leaving block braces on their own lines.
    if (closer == RBRACE
        && (open < 0 || document->braceScope(open) != FormatDocument::InitScope)
    ) {
        return false;
    }
    // Keeps a control statement's body separate from its closing parenthesis.
    if (closer == RPAREN && open > 0) {
        const Token keyword = document->token(open - 1);
        if (keyword == IF
            || keyword == WHILE
            || keyword == FOR
            || keyword == SWITCH
            || keyword == CATCH
        ) {
            return false;
        }
    }

    // Moves the operator line's text up behind the closing brackets.
    const QByteArray space = isTight ? QByteArray() : QByteArray(" ");
    const QByteArray text = document->lineText(line).trimmed();
    const int previousStart = document->lineStart(line - 1);
    const int previousEnd = document->symbolEnd(before);
    const QByteArray joined = document->text().mid(
        previousStart,
        previousEnd - previousStart
    ) + space + text;
    // Keeps the break when the joined line would exceed `wrapMax`.
    if (textWidth(joined)
            > wrapMaxAt(document, document->lineStart(line))
    ) {
        return false;
    }
    const int from = document->lineStart(line);
    const int lineEnd = document->lineEnd(line);
    const bool hasNewline = lineEnd < document->text().size();
    return document->replace(
        previousEnd,
        from - previousEnd,
        space + text + (hasNewline ? "\n" : ""),
        from, lineEnd - from + (hasNewline ? 1 : 0), QByteArray()
    );
}

/*!
    Tells whether the layout keeps the lines the user broke the list over,
    from the token @a open of @a document to its closing token @a close,
    whose items are @a arguments: the settings @a fileConfig take
    `[Structure] target` as `long`, and the lines look meant:

    * each item sits whole on one line;
    * none of them passes the wrap column (see wrapColumn());
    * no item of a later line starts past the end of the line above, which
      leaves room the user did not use;
    * each line holds the average count of items per line, rounded, or the
      last line fewer, as no more items may follow; unless
      `[Structure] wrapMistakes`, asked at the list through @a conditions,
      picks `never`.

    For a line that strays from that average, @a joinCount gets the count of
    items per line `wrapMistakes` lays the list out with: the average for
    `average`, else the number it gives; 0 leaves them to `joinArgs`.
*/
bool keepsUserLines(
        const FormatDocument *document,
        int open,
        int close,
        const QList<QPair<int, int > > & arguments,
        FormatScopeConditions *conditions,
        const FormatFileConfig & fileConfig,
        int *joinCount)
{
    *joinCount = 0;
    const int firstLine = document->symbolLine(open);
    const int lastLine = document->symbolLine(close);
    if (codeTargetAnyAt(document, document->symbols().at(open).from)) {
        return false;
    }

    // Takes a line past the wrap column as lines that do not fit.
    for (int l = firstLine; l <= lastLine; ++l) {
        if (textWidth(document->lineText(l))
            > wrapColumn(document, document->lineStart(l))
        ) {
            return false;
        }
    }

    // Counts the items of each line, and finds one past the line above.
    QList<int > counts;
    int line = -1;
    for (int i = 0; i < arguments.size(); ++i) {
        const QPair<int, int > & item = arguments.at(i);
        const int from = document->symbols().at(item.first).from;
        const int itemLine = document->lineOf(from);
        if (itemLine != document->symbolEndLine(item.second)) {
            return false;
        }
        const int column = textWidth(
            document->text().mid(
                document->lineStart(itemLine),
                from - document->lineStart(itemLine)
            )
        );
        if (itemLine > firstLine
            && column > textWidth(document->lineText(itemLine - 1).trimmed())
                + textWidth(document->indentOf(itemLine - 1))
        ) {
            return false;
        }
        if (itemLine == line) {
            ++counts.last();
        } else {
            counts << 1;
            line = itemLine;
        }
    }

    // Takes a line whose count strays from the average as a mistake, which
    // `wrapMistakes` lays out anew with its count per line.
    const QVariant mistakes = conditions->resolveAt(
        QString(QLL("structure.wrapMistakes")),
        fileConfig.wrapMistakes,
        open
    );
    const bool isNumber = mistakes.type() == QVariant::Double;
    if ( ! isNumber && mistakes.toString() != QLL("average")) {
        return true;
    }
    const int average = qRound(qreal(arguments.size()) / counts.size());
    for (int i = 0; i < counts.size(); ++i) {
        const bool isShortLast = i + 1 == counts.size()
            && counts.at(i) < average;
        if (counts.at(i) != average && ! isShortLast) {
            *joinCount = isNumber
                ? qMax(1, FormatFileConfig::countOf(mistakes))
                : average;
            return false;
        }
    }
    return true;
}

/*!
    Tells whether the user broke the list from the token @a open of
    @a document to its closing token @a close, whose items are
    @a arguments, the way the C++ rules wrap a parameter list: its first
    item on the line of the opening, each later line starting right under
    that item, and the closing bracket on the line of the last item.
*/
bool isAlignedUnderFirst(
        const FormatDocument *document,
        int open,
        int close,
        const QList<QPair<int, int > > & arguments)
{
    const int line = document->symbolLine(open);
    const int first = arguments.first().first;
    if (document->symbolLine(first) != line
        || document->symbolLine(arguments.last().second)
            != document->symbolLine(close)
    ) {
        return false;
    }
    const int column = textWidth(
        document->text().mid(
            document->lineStart(line),
            document->symbols().at(first).from - document->lineStart(line)
        )
    );
    for (int i = 1; i < arguments.size(); ++i) {
        const int itemLine = document->symbolLine(arguments.at(i).first);
        const bool startsLine = itemLine
            != document->symbolLine(arguments.at(i - 1).second);
        if (startsLine
            && textWidth(document->indentOf(itemLine)) != column
        ) {
            return false;
        }
    }
    return true;
}

/*!
    Finds a logical operator before the call at @a open whose break keeps
    both resulting lines within wrapMax. Gives its token and the indentation
    of its continuation line; leaves comments, directives and already split
    calls to their own layouts.
*/
bool logicalBreakBeforeCall(
        const FormatDocument *document, int open,
        int *operatorIndex, QByteArray *indent)
{
    *operatorIndex = -1;
    indent->clear();
    const int close = document->match(open);
    const int line = document->symbolLine(open);
    if (close <= open || document->symbolEndLine(close) != line
        || document->isDirectiveLine(line)
    ) {
        return false;
    }
    const int lineFirst = document->firstSymbolOfLine(line);
    int op = open - 1;
    while (op > lineFirst) {
        const Token token = document->token(op);
        if (token == ANDAND || token == OROR) {
            break;
        }
        if (token == RPAREN || token == RBRACK) {
            const int match = document->match(op);
            if (match < lineFirst) {
                return false;
            }
            op = match - 1;
            continue;
        }
        if (token == COMMA || token == SEMIC || token == QUESTION
            || token == COLON || token == EQ || token == LBRACE
            || token == RBRACE || operatorPrecedence(token) > 0
        ) {
            return false;
        }
        --op;
    }
    if (op <= lineFirst || (document->token(op) != ANDAND
            && document->token(op) != OROR)
    ) {
        return false;
    }
    const int from = document->symbolEnd(op - 1);
    const int at = document->symbols().at(op).from;
    const int end = contentEnd(document, line);
    if (document->hasCommentBetween(from, end)
        || document->spansDirective(from, end)
    ) {
        return false;
    }
    const QByteArray prefix = document->text().mid(
        document->lineStart(line), from - document->lineStart(line)
    );
    const QByteArray tail = document->text().mid(at, end - at);
    const QByteArray padding = continuationPad(
        textWidth(document->indentOf(line))
    );
    const int limit = wrapMaxAt(document, at);
    if (textWidth(prefix) > limit
        || textWidth(padding) + textWidth(tail) > limit
    ) {
        return false;
    }
    *operatorIndex = op;
    *indent = padding;
    return true;
}

/*!
    Builds the `{` of the one-line function body that holds the list at
    @a paren of @a document, when that body fits the wrap column (see
    wrapColumn()) on a line of its own, at the indent of its head's line;
    -1 for any other list. Such a body opens and closes on the list's line,
    right after the function's head.
*/
int inlineFunctionBody(
        const FormatDocument *document,
        int paren)
{
    // Finds the brace that holds the list.
    int brace = -1;
    for (int k = paren - 1; k > 0 && brace < 0; --k) {
        const Token token = document->token(k);
        if (token == RBRACE && document->match(k) >= 0) {
            k = document->match(k);
        } else if (token == LBRACE && document->match(k) > paren) {
            brace = k;
        }
    }
    const int line = document->symbolLine(paren);
    if (brace < 1
        || document->braceScope(brace) != FormatDocument::FunctionScope
        || document->symbolLine(brace) != line
        || document->symbolLine(brace - 1) != line
        || document->symbolLine(document->match(brace)) != line
    ) {
        return -1;
    }

    // Measures the body on a line of its own.
    const int from = document->symbols().at(brace).from;
    const QByteArray text = document->text().mid(
        from, document->symbolEnd(document->match(brace)) - from
    );
    const int width = textWidth(document->indentOf(line)) + textWidth(text);
    if (width > wrapColumn(document, from)) {
        return -1;
    }
    return brace;
}

/*!
    Tells whether the `(` at @a paren of @a document opens the arguments of
    a call inside a function body: a name comes before it, other than
    `connect` and `disconnect`.
*/
bool isFunctionCallParen(
        const FormatDocument *document,
        int paren)
{
    if (document->token(paren - 1) != IDENTIFIER) {
        return false;
    }
    const FormatDocument::Scope scope = document->scopeOf(paren);
    if (scope != FormatDocument::FunctionScope
        && scope != FormatDocument::BlockScope
        && scope != FormatDocument::InitScope
    ) {
        return false;
    }
    const QByteArray name = document->lexem(paren - 1);
    return name != "connect" && name != "disconnect";
}

/*!
    Builds the index of the `if`, `while`, `for`, or `switch` whose head holds
    the `(` at @a paren of @a document, or -1 when it sits in no such head.
*/
int controlKeywordFor(
        const FormatDocument *document, int paren)
{
    for (int k = paren - 1; k > 0; --k) {
        const Token token = document->token(k);
        const int match = document->match(k);
        if ((token == RPAREN || token == RBRACK) && match >= 0) {
            k = match;
            continue;
        }
        if (token == SEMIC || token == LBRACE || token == RBRACE) {
            return -1;
        }
        if (token == LPAREN) {
            const Token keyword = document->token(k - 1);
            if (keyword == IF || keyword == WHILE || keyword == FOR
                || keyword == SWITCH
            ) {
                return k - 1;
            }
        }
    }
    return -1;
}

/*!
    Finds a statement starting on @a line of @a document and gives
    its first token to @a first and its `;` to @a last. Returns false unless
    the statement ends with its only top-level `;` on the same line or
    the closing line of a nested expression,
    holds no comment, and is no control statement.
*/
bool findWholeStatement(
        const FormatDocument *document, int line,
        int *first, int *last)
{
    *first = document->firstSymbolOfLine(line);
    *last = document->lastSymbolOfLine(line);
    if (*first < 0) {
        return false;
    }

    // Takes only a statement that starts the line and ends on it.
    if (*first > 0) {
        const Token before = document->token(*first - 1);
        if (before != SEMIC
            && before != LBRACE
            && before != RBRACE
            && before != COLON
        ) {
            return false;
        }
    }
    const Token lead = document->token(*first);
    if (lead == IF || lead == ELSE || lead == WHILE || lead == FOR || lead == DO
        || lead == SWITCH || lead == CASE
    ) {
        return false;
    }
    const int end = statementEnd(document, *first);
    if (end >= *first && document->token(end) == SEMIC
        && end == document->lastSymbolOfLine(document->symbolLine(end)))
    {
        *last = end;
    }
    bool hasBinaryOperator = false;
    // Skip nested expressions while checking the statement boundary.
    for (int i = *first; i <= *last; ++i) {
        const Token token = document->token(i);
        if ((token == LPAREN || token == LBRACK || token == LBRACE)
            && document->match(i) > i
        ) {
            i = document->match(i);
            if (i > *last) {
                if (token == LBRACE
                    && document->braceScope(document->match(i))
                        != FormatDocument::InitScope
                ) {
                    return false;
                }
                *last = document->lastSymbolOfLine(document->symbolEndLine(i));
            }
            continue;
        }
        hasBinaryOperator |= operatorPrecedence(token) > 0 && i > *first
            && isOperandEnd(document->token(i - 1));
        if ((token == SEMIC && i < *last) || token == RBRACE) {
            return false;
        }
    }
    const int from = document->lineStart(line);
    const int to = document->lineEnd(document->symbolEndLine(*last));
    return (hasBinaryOperator || document->symbolLine(*last) == line)
        && document->token(*last) == SEMIC
        && ! document->hasCommentBetween(from, to)
        && ! document->spansDirective(from, to);
}

/*!
    Moves the braced body of the control statement whose keyword is at
    @a keyword of @a document, and whose `{` is at @a brace, off its line,
    when it sits on that line whole (`if (x) { a(); }`): the statements go
    one step (4 spaces) past the keyword's line, and the `}` to the line
    after them, at the keyword's indent. An empty body stays on its line,
    and so does one that a directive splits, or that has a brace or a
    comment of its own.
*/
bool expandInlineBody(
        FormatDocument *document, int keyword,
        int brace)
{
    const int close = document->match(brace);
    const int line = document->symbolLine(brace);
    if (close <= brace + 1 || document->symbolLine(close) != line
        || document->symbolLine(keyword) != line
        || document->hasCommentBetween(
            document->symbolEnd(brace),
            document->symbols().at(close).from
        )
    ) {
        return false;
    }
    for (int k = brace + 1; k < close; ++k) {
        if (document->token(k) == LBRACE) {
            return false;
        }
    }
    const QByteArray indent = document->indentOf(line);
    const int from = document->symbolEnd(brace);
    const int last = document->symbolEnd(close - 1);
    return document->replace(
        from, document->symbols().at(brace + 1).from - from,
        "\n" + continuationPad(indent.size()),
        last, document->symbols().at(close).from - last, "\n" + indent
    );
}

/*!
    Builds the index of the first token of the body that the control keyword
    at @a keyword of @a document governs, or -1 when that token is no control
    keyword (or the keyword governs no body of its own, as `else if` and the
    `while` of a `do` loop do not).
*/
int controlBodyStart(
        const FormatDocument *document,
        int keyword)
{
    const Token token = document->token(keyword);
    if (token == ELSE) {
        return document->token(keyword + 1) == IF ? -1 : keyword + 1;
    }
    if (token == DO) {
        return keyword + 1;
    }
    bool isLoopMacro = false;
    if (token == IDENTIFIER) {
        const QByteArray name = document->lexem(keyword);
        isLoopMacro = name == "foreach" || name == "Q_FOREACH";
    }
    if (token != IF && token != FOR && token != WHILE && ! isLoopMacro) {
        return -1;
    }
    if (document->token(keyword + 1) != LPAREN
        || document->match(keyword + 1) < 0
    ) {
        return -1;
    }

    // Leaves out the `while` that ends a `do` loop.
    if (token == WHILE && document->token(keyword - 1) == RBRACE) {
        const int opener = document->match(keyword - 1);
        if (opener > 0 && document->token(opener - 1) == DO) {
            return -1;
        }
    }
    const int body = document->match(keyword + 1) + 1;
    return body < document->symbols().size() ? body : -1;
}

/*!
    Queues the braces around the one-statement body starting at the token
    @a body of @a document, whose control statement starts at @a keyword and
    whose head ends at the token @a head.
*/
void braceStatementBody(
        FormatDocument *document, int keyword,
        int head, int body)
{
    const int end = statementEnd(document, body);
    if (end < 0) {
        return;
    }
    const int headLine = document->symbolEndLine(head);
    const int endLine = document->symbolEndLine(end);
    for (int line = headLine; line <= endLine; ++line) {
        if (document->isDirectiveLine(line)) {
            document->report(document->symbols().at(keyword).from, "control-braces",
                             QString(QLL("brace the body by hand: a directive splits it")));
            return;
        }
    }

    // Keeps a one-line statement on its line: `if (x) { return; }`.
    const Token after = document->token(end + 1);
    const bool followedOnLine = end + 1 < document->symbols().size()
        && document->symbolLine(end + 1) == endLine;
    if (document->symbolLine(body) == headLine) {
        if (followedOnLine && after != ELSE && after != WHILE
            && (document->isHeader() || document->hasCommentBetween(
                document->symbolEnd(end), document->symbols().at(end + 1).from
            ))
        ) {
            return;
        }
        if (document->isHeader()) {
            document->replace(
                document->symbolEnd(head), 0, " {",
                document->symbolEnd(end), 0, " }"
            );
            return;
        }

        // Moves the body to a line of its own, and closes it on the next.
        const QByteArray indent =
            document->indentOf(document->symbolLine(keyword));
        const int from = document->symbolEnd(head);
        const int endFrom = document->symbolEnd(end);
        const bool movesNext = followedOnLine
            && after != ELSE
            && after != WHILE;
        const int endLength = movesNext
            ? document->symbols().at(end + 1).from - endFrom : 0;
        const QByteArray endText = "\n" + indent + "}"
            + (movesNext ? "\n" + indent : QByteArray());
        document->replace(
            from, document->symbols().at(body).from - from,
            " {\n" + continuationPad(indent.size()),
            endFrom, endLength, endText
        );
        return;
    }

    // Closes a body on lines of its own with a brace at the keyword's indent;
    // an `else` or the `while` of a `do` joins that brace's line.
    if (followedOnLine) {
        return;
    }
    const QByteArray indent = document->indentOf(document->symbolLine(keyword));
    const bool joins = (after == ELSE || (after == WHILE && document->token(keyword) == DO))
        && ! document->hasCommentBetween(
            document->symbolEnd(end),
            document->symbols().at(end + 1).from
        ) && ! document->spansDirective(
            document->symbolEnd(end),
            document->symbols().at(end + 1).from
        );
    int closeFrom = contentEnd(document, endLine);
    int closeLength = 0;
    QByteArray closeText = "\n" + indent + "}";
    if (joins) {
        closeFrom = document->symbolEnd(end);
        closeLength = document->symbols().at(end + 1).from - closeFrom;
        closeText += ' ';
    }
    document->replace(
        document->symbolEnd(head), 0, " {",
        closeFrom, closeLength, closeText
    );
}

/*!
    Tells whether the `}` at @a close of @a document stands on a line that
    holds only a chain of closing brackets, such as `} }`, `) }`, or
    `); }`: another closing bracket before it, and nothing else on its line
    but closing brackets and `;`.
*/
bool closesBracketChain(const FormatDocument *document, int close)
{
    const int line = document->symbolLine(close);
    const int first = document->firstSymbolOfLine(line);
    const int last = document->lastSymbolOfLine(line);
    if (first < 0 || first >= close) {
        return false;
    }
    for (int k = first; k <= last; ++k) {
        const Token token = document->token(k);
        if (token != RBRACE
            && token != RPAREN
            && token != RBRACK
            && token != SEMIC
        ) {
            return false;
        }
    }
    const Token start = document->token(first);
    return start == RBRACE || start == RPAREN || start == RBRACK;
}

static bool groupKeepsIndent(const FormatDocument *document, int group)
{
    const Token token = document->token(group);
    if (token == LBRACE) {
        const FormatDocument::Scope scope = document->braceScope(group);
        return scope == FormatDocument::InitScope
            || scope == FormatDocument::EnumScope;
    }
    if (token == LBRACK) {
        return true;
    }
    const Token opener = document->token(group - 1);
    return opener == IDENTIFIER || opener == RANGLE || opener == GTGT
        || opener == RBRACK || opener == RPAREN || opener == STATIC_CAST
        || opener == SIZEOF;
}

/*!
    Builds the line that starts the statement holding the token at @a from
    of @a document: the first line after the `;`, `{` or `}` before it that
    is no preprocessor line. Directive lines and matched brackets on the way
    are passed over, since a directive ends no statement.
*/
static int statementLine(const FormatDocument *document, int from)
{
    int k = from;
    for (; k >= 0; --k) {
        if (document->isDirectiveLine(document->symbolLine(k))) {
            continue;
        }
        const Token token = document->token(k);
        if ((token == RPAREN || token == RBRACK)
            && document->match(k) >= 0
        ) {
            k = document->match(k);
            continue;
        }
        if (token == SEMIC || token == LBRACE || token == RBRACE) {
            break;
        }
    }
    int start = k + 1;
    while (start + 1 < document->symbols().size()
        && document->isDirectiveLine(document->symbolLine(start))
    ) {
        ++start;
    }
    return document->symbolLine(start);
}

/*!
    Finds the group that the token at @a index of @a document belongs to:
    the innermost `(` or `[` still open before it, or a braced initializer,
    else the start of its statement. Past a preprocessor line, only a
    matching open bracket that spans it makes a group; otherwise the
    statement's start is found past the directive lines (see
    statementLine()). Returns that bracket's index, or -1 for the statement,
    and gives the line that opened the group to @a base; sets
    @a keepsIndent for a group whose lines align under its first item.
*/
int continuationGroup(const FormatDocument *document, int index, int *base,
                   bool *keepsIndent)
{
    *base = -1;
    *keepsIndent = false;
    int k = index - 1;
    for (; k >= 0; --k) {
        const Token token = document->token(k);
        if (document->isDirectiveLine(document->symbolLine(k))) {
            for (int group = k - 1; group >= 1; --group) {
                if (document->isDirectiveLine(document->symbolLine(group))) {
                    continue;
                }
                const Token groupToken = document->token(group);
                if ((groupToken == LPAREN
                        || groupToken == LBRACK
                        || groupToken == LBRACE)
                    && document->match(group) >= index
                ) {
                    *keepsIndent = groupKeepsIndent(document, group);
                    if (groupToken != LBRACE || *keepsIndent) {
                        *base = document->symbolLine(group);
                        return group;
                    }
                }
            }
            *base = statementLine(document, k);
            return -1;
        }
        if ((token == RPAREN
                || token == RBRACK)
            && document->match(k) >= 0
        ) {
            k = document->match(k);
            continue;
        }
        if (token == LPAREN || token == LBRACK) {
            *keepsIndent = groupKeepsIndent(document, k);
            *base = document->symbolLine(k);
            return k;
        }
        if (token == LBRACE
            && document->braceScope(k) != FormatDocument::FunctionScope
            && document->braceScope(k) != FormatDocument::BlockScope
        ) {
            *keepsIndent = groupKeepsIndent(document, k);
        }
        if (token == SEMIC || token == LBRACE || token == RBRACE) {
            *base = document->symbolLine(k + 1);
            return -1;
        }
    }
    *base = document->symbolLine(0);
    return -1;
}

bool isForCondition(const FormatDocument *document, int group, int index)
{
    if (document->token(group - 1) != FOR) {
        return false;
    }
    int semicolons = 0;
    for (int k = group + 1; k < index; ++k) {
        if (document->isDirectiveLine(document->symbolLine(k))) {
            continue;
        }
        const Token token = document->token(k);
        if ((token == LPAREN || token == LBRACK || token == LBRACE)
            && document->match(k) > k
        ) {
            k = document->match(k);
        } else if (token == SEMIC) {
            ++semicolons;
        }
    }
    return semicolons == 1;
}

int conditionSteps(const FormatDocument *document, int group, int base,
                          int index)
{
    int steps = 0;
    int bracket = group;
    while (bracket >= 1 && document->token(bracket) == LPAREN) {
        if (document->symbolLine(bracket) == base) {
            ++steps;
        }
        const Token opener = document->token(bracket - 1);
        if (opener == IF || opener == WHILE
            || (opener == FOR && isForCondition(document, bracket, index))
        ) {
            if (bracket != group
                && document->symbolLine(bracket) != base
                && steps > 0
            ) {
                --steps;
            }
            return bracket == group ? 0 : steps;
        }
        int outerBase = -1;
        bool keepsIndent = false;
        bracket = continuationGroup(document, bracket, &outerBase, &keepsIndent);
        if (keepsIndent) {
            return 0;
        }
    }
    return 0;
}

/*!
    Builds how tightly the operator @a token that leads a line binds: as
    operatorPrecedence() ranks it, with a `?` loosest of all.
*/
int leadRank(Token token)
{
    return token == QUESTION ? 0 : operatorPrecedence(token);
}
