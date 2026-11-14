// Copyright (C) 2015 The XD Company Ltd.
// SPDX-License-Identifier: Apache-2.0

#include "./layout-helpers.h"

#include "../format-document.h"
#include "./code-helpers.h"
#include "./scope-conditions.h"
#include "../format-settings.h"

/*!
    Builds the column the code wrap rules break a line at: the first
    `[Structure] wrapBegin` column, never past `[Structure] wrapMax`; a line
    wider than it does not fit. Uses the settings at @a offset when supplied.
*/
int wrapColumn(const FormatDocument *document, int offset)
{
    const FormatSettings & settings = offset < 0
        ? document->settings() : document->settingsAt(offset);
    const int begin = settings.wrapBegin.isEmpty()
        ? settings.wrapMax : settings.wrapBegin.first();
    return qMin(begin, settings.wrapMax);
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
    Finds a logical operator before the call at @a open whose break keeps
    both resulting lines within wrapMax. Gives its token and the indentation
    of its continuation line; leaves comments, directives and already split
    calls to their own layouts.
*/
bool logicalBreakBeforeCall(const FormatDocument *document, int open,
                            int *operatorIndex, QByteArray *indent)
{
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
    const int limit = qMax(1, document->settingsAt(at).wrapMax);
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
                       int indent)
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
                int steps = 0;
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
    Finds the one `?` of a conditional expression among the tokens from
    @a first up to @a last of @a document, outside of every bracket pair, and
    the first `:` after it; returns false when there is no such `?`, or more
    than one (a nested conditional expression), or no `:`.
*/
bool findTernary(const FormatDocument *document, int first, int last,
                 int *question, int *colon)
{
    *question = -1;
    *colon = -1;
    for (int k = first; k <= last; ++k) {
        const Token token = document->token(k);
        if ((token == LPAREN
                || token == LBRACK
                || token == LBRACE)
            && document->match(k) > k
        ) {
            k = document->match(k);
        } else if (token == QUESTION) {
            if (*question >= 0) {
                return false;
            }
            *question = k;
        } else if (token == COLON && *question >= 0 && *colon < 0) {
            *colon = k;
        }
    }
    return *question >= 0 && *colon >= 0;
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
static int templateClose(const FormatDocument *document, int angle, int end)
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
    FormatScopeConditions::holds()) holds, its percents comparing the column
    right after the token with `[Structure] wrapMax`. A list may break after
    such a `(`, and a value may move below such an `=`.
*/
bool reachesWrapArgsAt(const FormatDocument *document,
                       FormatScopeConditions *conditions, int token)
{
    const int lineStart = document->lineStart(document->symbolLine(token));
    const QByteArray head = document->text().mid(
        lineStart,
        document->symbolEnd(token) - lineStart
    );
    const FormatSettings &settings =
        document->settingsAt(document->symbols().at(token).from);
    const qreal percent = textWidth(head) * 100.0 / qMax(
        1,
        settings.wrapMax
    );
    return conditions->holds(
        settings.wrapArgsAt, token, percent,
        FormatSettings::defaultWrapChainAt()
    );
}

/*!
    Ranks @a token among the comparison operators, from the loosest:
    `==` and `!=`, then `<`, `>`, `<=` and `>=`; -1 for any other token.
*/
static int comparisonRank(Token token)
{
    static const Token ranks[][4] = {
        { EQEQ, NE, NE, NE },
        { LANGLE, RANGLE, LE, GE },
    };
    for (size_t r = 0; r < sizeof(ranks) / sizeof(ranks[0]); ++r) {
        for (int t = 0; t < 4; ++t) {
            if (ranks[r][t] == token) {
                return int(r);
            }
        }
    }
    return -1;
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
        <= document->settingsAt(document->symbols().at(first).from).wrapMax
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
    const Token label = document->token(name - 2);
    const bool afterAccess = typeEnd == COLON
        && (label == PUBLIC || label == PRIVATE || label == PROTECTED || label == SLOTS
            || label == SIGNALS);
    return scope == FormatDocument::ClassScope
        && (typeEnd == SEMIC || typeEnd == LBRACE || typeEnd == RBRACE || afterAccess
            || typeEnd == EXPLICIT || typeEnd == INLINE);
}
