// Copyright (C) 2015 The XD Company Ltd.
// SPDX-License-Identifier: Apache-2.0

#include "./format-document.h"
#include "./parser/comment-helpers.h"
#include "./parser/condition-context.h"
#include "./parser/layout-helpers.h"
#include "./parser/scope-conditions.h"

#include "./lexer/format-lexer.h"

#include "config-cache.h"
#include "config-condition.h"

#include <QtCore/QDir>
#include <QtCore/QFileInfo>
#include <QtCore/QMap>
#include <QtCore/QRegularExpression>
#include <algorithm>

/*!
    @class FormatDocument
    @internal

    Holds one file under formatting as text plus its lexed tokens, and
    queues the edits and issues that the formatting rules find.

    The tokens come from FormatLexer::tokenize(), which keeps every token's
    offset into the text and emits comments as tokens too; hence the document
    lists comments() directly without external preprocessor state.
*/

/*!
    Builds the document of @a text, the content of the file at @a path (which
    rules that look at the file's folder use, and which may be empty).
*/
FormatDocument::FormatDocument(const QByteArray & text, const QString & path)
    : m_text(text), m_path(path), m_language(FormatLanguage::Cpp)
    , m_inlineConfigured(false), m_pass(0), m_agentMode(false)
    , m_commentChanged(false)
{
    this->analyze();
}

namespace {

/*!
    Builds how many entries of @a ranges, ordered by their first member,
    start at or before @a offset; the last of them is the one that may hold
    @a offset.
*/
template <typename T >
int startsUpTo(const QVector<QPair<int, T > > & ranges, int offset)
{
    int low = 0;
    int high = ranges.size();
    while (low < high) {
        const int middle = low + (high - low) / 2;
        if (ranges.at(middle).first <= offset) {
            low = middle + 1;
        } else {
            high = middle;
        }
    }
    return low;
}

} // namespace

/*!
    Returns the settings active at @a offset in the document.
*/
const FormatFileConfig &FormatDocument::configAt(int offset) const
{
    const int low = startsUpTo(m_fileConfigRegions, offset);
    return low > 0 ? m_fileConfigRegions.at(low - 1).second : m_fileConfig;
}

namespace {

/*!
    Answers the functions of a `[Structure] joinArgs` condition (see
    FormatConditionContext) at one argument of a list that a rule lays out.
*/
class JoinArgsContext : public FormatConditionContext
{
    typedef FormatConditionContext super;
public:

    inline JoinArgsContext(
            FormatScopeConditions *conditions,
            int open,
            int argument,
            int last,
            bool isWrapping)
        : m_conditions(conditions), m_open(open), m_argument(argument)
        , m_last(last), m_isWrapping(isWrapping)
    {
    }

    bool argumentCount(int depth,
                       int argument,
                       qreal *count) const Q_DECL_OVERRIDE;

    inline QStringList onNames() const Q_DECL_OVERRIDE_IL
    {
        return FormatScopeConditions::names();
    }

    inline bool predicate(const QString &name,
                          bool *value) const Q_DECL_OVERRIDE_IL
    {
        Config::Cache::skip();
        return m_conditions->nameHolds(m_open, name, value);
    }

    inline bool value(qreal *value) const Q_DECL_OVERRIDE_IL
    {
        *value = this->count();
        return true;
    }

    inline bool spaceRemaining(qreal *percent) const Q_DECL_OVERRIDE_IL
    {
        *percent = m_conditions->spaceRemaining(m_last);
        return true;
    }

    inline bool hasMoreArgs(bool *value) const Q_DECL_OVERRIDE_IL
    {
        *value = m_argument + 1 < this->count();
        return true;
    }

    inline bool isWrapping(bool *value) const Q_DECL_OVERRIDE_IL
    {
        *value = m_isWrapping;
        return true;
    }

    inline bool isTarget(const QString & kind,
                         bool *value) const Q_DECL_OVERRIDE_IL
    {
        return m_conditions->targetIs(m_open, kind, value);
    }

    inline const FormatFileConfig *fileConfig() const Q_DECL_OVERRIDE_IL
    {
        const FormatDocument *document = m_conditions->document();
        return &document->configAt(document->symbols().at(m_open).from);
    }

private:

    int count() const;

    FormatScopeConditions *m_conditions;
    int m_open;
    int m_argument;
    int m_last;
    bool m_isWrapping;
};

/*!
    Counts the arguments of the list in question, each time it is asked.
*/
int JoinArgsContext::count() const
{
    const FormatDocument *document = this->m_conditions->document();
    return splitArguments(document, this->m_open, document->match(this->m_open))
        .size();
}

/*!
    Counts as FormatScopeConditions::argumentCount() does, except that a
    positive depth without an argument selects the argument in question; at
    depth 1, an argument that holds no call takes no arguments.
*/
bool JoinArgsContext::argumentCount(int depth, int argument, qreal *count) const
{
    if (depth > 0 && argument == 0) {
        argument = this->m_argument + 1;
    }
    *count = this->m_conditions->argumentCount(
        this->m_open,
        depth,
        argument
    );
    if (depth == 1 && *count < 0) {
        *count = 0;
    }
    return true;
}

} // namespace

/*!
    Builds how many arguments of the list opened at the token @a open may
    share a line, by `[Structure] joinArgs` at the argument @a argument
    (zero-based), which a rule lays out to end at the column @a column, and
    which would get a line of its own when @a isWrapping is set: the line
    may hold that argument and the ones before it on the line, up to the
    number this builds, and the argument after it when the number is larger.

    A plain number applies only where @a isWrapping is set; elsewhere the
    list keeps one argument per line of its own. A condition decides at
    every argument instead (see FormatConditionContext for the functions it
    calls). It reserves the line up to the argument's `,`, which a later
    argument follows, hence its `spaceRemaining` counts the space after that
    `,`. A condition that misses a value gives 1.
*/
int FormatDocument::joinArgsAt(
        int open,
        int argument,
        int column,
        bool isWrapping) const
{
    const FormatFileConfig &fileConfig = this->configAt(
        this->symbols().at(open).from
    );
    if (fileConfig.joinArgsCondition.isEmpty()) {
        return isWrapping ? qMax(1, fileConfig.joinArgs) : 1;
    }
    const QList<QPair<int, int > > items =
        splitArguments(this, open, this->match(open));
    if (argument < 0 || argument >= items.size()) {
        return 1;
    }

    // Reserves the line up to the argument's `,`.
    const int last = items.at(argument).second;
    const bool hasComma = this->token(last + 1) == COMMA;
    FormatScopeConditions conditions(this);
    conditions.reserve(last, column + (hasComma ? 1 : 0));

    const JoinArgsContext context(
        &conditions,
        open,
        argument,
        last,
        isWrapping
    );
    const QSharedPointer<Config::Scopes::Scope > scope =
        FormatConditionContext::pushCountScope();
    const QVariant count = FormatFileConfig::resolve(
        QString(QLL("structure.joinArgs")),
        fileConfig.joinArgsCondition,
        &context
    );
    if ( ! context.lastResolvePassed()) {
        return 1;
    }
    return qMax(1, FormatFileConfig::countOf(count));
}

/*!
    Resolves the comment configuration over @a config, reporting errors in
    @a error.
*/
bool FormatDocument::setInlineConfiguration(
        const FormatConfig &config, QString *error)
{
    const QList<FormatConfigDirective > directives = configDirectives(this);
    if (directives.isEmpty()) {
        m_fileConfigRegions.clear();
        m_inlineRanges.clear();
        m_disabledRanges.clear();
        m_inlineConfigured = false;
        m_inlineError.clear();
        m_ruleCaches.clear();
        return true;
    }
    QVector<QPair<int, int > > disabledRanges;
    bool disabledActive = false;
    int disabledStart = -1;
    for (int i = 0; i < directives.size(); ++i) {
        const FormatConfigDirective &directive = directives.at(i);
        if (directive.kind == FormatConfigDirective::Off) {
            if ( ! disabledActive) {
                disabledStart = directive.offset;
                disabledActive = true;
            }
        } else if (directive.kind == FormatConfigDirective::On
            && disabledActive
        ) {
            disabledActive = false;
            disabledRanges.append(
                qMakePair(disabledStart, directive.endOffset)
            );
        }
    }
    if (disabledActive) {
        disabledRanges.append(qMakePair(disabledStart, m_text.size() + 1));
    }
    QVector<bool > matched(directives.size(), false);
    QList<int > openings;
    for (int i = 0; i < directives.size(); ++i) {
        if (directives.at(i).kind == FormatConfigDirective::Begin) {
            openings.append(i);
        } else if (directives.at(i).kind == FormatConfigDirective::End
            && ! openings.isEmpty()
        ) {
            matched[openings.takeLast()] = true;
            matched[i] = true;
        }
    }
    Config::Parser base(FormatFileConfig::schema());
    if ( ! FormatFileConfig::readParser(m_path, config, &base, error)) {
        return false;
    }
    Config::LayeredParser layers(base);
    QVector<QPair<int, FormatFileConfig > > regions;
    QVector<QPair<int, int > > protectedRanges = disabledRanges;
    int firstComment = 0;
    for (int i = 0; i < directives.size(); ++i) {
        const FormatConfigDirective &directive = directives.at(i);
        while (firstComment < m_comments.size()
            && m_comments.at(firstComment).offset + m_comments.at(firstComment).length
                <= directive.offset
        ) {
            ++firstComment;
        }
        for (int j = firstComment; j < m_comments.size(); ++j) {
            const FormatComment &comment = m_comments.at(j);
            if (comment.offset > directive.endOffset) {
                break;
            }
            if (comment.offset <= directive.endOffset
                && comment.offset + comment.length > directive.offset
            ) {
                protectedRanges.append(
                    qMakePair(comment.offset, comment.offset + comment.length)
                );
            }
        }
    }
    std::sort(protectedRanges.begin(), protectedRanges.end());
    QVector<QPair<int, int > > mergedRanges;
    for (int i = 0; i < protectedRanges.size(); ++i) {
        if ( ! mergedRanges.isEmpty()
            && protectedRanges.at(i).first <= mergedRanges.last().second
        ) {
            mergedRanges.last().second =
                qMax(mergedRanges.last().second, protectedRanges.at(i).second);
        } else {
            mergedRanges.append(protectedRanges.at(i));
        }
    }
    for (int i = 0; i < directives.size(); ++i) {
        const FormatConfigDirective &directive = directives.at(i);
        if (directive.kind == FormatConfigDirective::Off
            || directive.kind == FormatConfigDirective::On
            || directive.kind == FormatConfigDirective::Invalid
        ) {
            continue;
        }
        if (directive.kind == FormatConfigDirective::Begin) {
            if ( ! matched.at(i)) {
                continue;
            }
            if ( ! layers.pushLines(
                directive.lines,
                QString(QLL("Format rule")),
                error
            )) {
                return false;
            }
        } else if (directive.kind == FormatConfigDirective::End) {
            if ( ! matched.at(i)) {
                continue;
            }
            if ( ! layers.popLines(error)) {
                return false;
            }
        } else {
            if ( ! layers.addLines(
                directive.lines,
                QString(QLL("Format rule")),
                error
            )) {
                return false;
            }
        }
        FormatFileConfig resolved;
        QString reason;
        if ( ! FormatFileConfig::fromParser(layers, &resolved, &reason)) {
            *error = QString(QLL("Format rule: [Code] nameMismatch = critical: %1"))
                .arg(reason);
            return false;
        }
        regions.append(qMakePair(directive.endOffset, resolved));
    }
    m_fileConfigRegions = regions;
    m_inlineRanges = mergedRanges;
    m_disabledRanges = disabledRanges;
    m_inlineConfigured = ! directives.isEmpty();
    m_inlineError.clear();
    m_ruleCaches.clear();
    return true;
}

/*!
    Reports unmatched comment configuration markers, including unmatched
    openings when @a verbose is set.
*/
QList<FormatIssue > FormatDocument::inlineWarnings(bool verbose) const
{
    const QList<FormatConfigDirective > directives = configDirectives(this);
    QList<int > openings;
    QList<FormatIssue > warnings;
    for (int i = 0; i < directives.size(); ++i) {
        const FormatConfigDirective &directive = directives.at(i);
        if (verbose && this->isFormattingDisabled(directive.offset)) {
            if (directive.kind == FormatConfigDirective::Add
                || directive.kind == FormatConfigDirective::Begin
            ) {
                const int line = this->lineOf(directive.offset);
                warnings.append(
                    FormatIssue(
                        line,
                        "format-warning",
                        QString(
                            QLL("#: format warning: configuration applied inside an off region at line %1.")
                        ).arg(line)
                    )
                );
            }
        }
        if (directive.kind == FormatConfigDirective::Invalid) {
            const int line = this->lineOf(directive.offset);
            warnings.append(FormatIssue(
                line,
                "format-warning",
                QString(QLL("#: format warning: invalid config detected at line %1. "
                    "Maybe `@formatter: off` was meant.")).arg(line)
            ));
        } else if (directive.kind == FormatConfigDirective::Begin) {
            openings.append(i);
        } else if (directive.kind == FormatConfigDirective::End) {
            if (openings.isEmpty()) {
                const int line = this->lineOf(directive.offset);
                warnings.append(
                    FormatIssue(
                        line,
                        "format-warning",
                        QString(
                            QLL("#: format warning: unmatched \"Format rule end.\" at line %1")
                        ).arg(line)
                    )
                );
            } else {
                openings.removeLast();
            }
        }
    }
    if (verbose) {
        for (int i = 0; i < openings.size(); ++i) {
            const int line = this->lineOf(directives.at(openings.at(i)).offset);
            warnings.append(FormatIssue(
                line,
                "format-warning",
                QString(QLL("#: format warning: consider replacing \"Format rule begin:\" with "
                    "\"Format rule:\" at line %1 (since it has no \"Format rule end.\" comment)."))
                .arg(line)
            ));
        }
    }
    return warnings;
}

/*!
    Tokenizes the text, then lists its line starts, its
    code tokens, and its comments.
*/
void FormatDocument::analyze()
{
    this->m_symbols.clear();
    this->m_comments.clear();
    this->m_lineStarts.clear();

    // Finds the line starts.
    this->m_lineStarts.append(0);
    for (int i = 0; i < this->m_text.size(); ++i) {
        if (this->m_text.at(i) == '\n') {
            this->m_lineStarts.append(i + 1);
        }
    }

    m_language = FormatLanguageInfo::detect(this->m_path, this->m_text);
    const FormatCommentStyle commentStyle =
        FormatLanguageInfo::commentStyle(m_language);
    const Symbols all = qMove(FormatLexer::tokenize(
        this->m_text,
        FormatLexer::Options(true, m_language)
    ));
    int previousCodeLine = 0;
    for (int i = 0; i < all.size(); ++i) {
        const Symbol & symbol = all.at(i);
        if (symbol.len <= 0
            || symbol.token == WHITESPACE
            || symbol.token == PP_WHITESPACE
            || symbol.token == NOTOKEN
            || symbol.token == PP_NEWLINE
            || symbol.token == NEWLINE
        ) {
            continue;
        }
        if (symbol.token == CPP_COMMENT || symbol.token == C_COMMENT) {
            this->appendComment(symbol, previousCodeLine);
            continue;
        }
        if (commentStyle.supportsTripleDocstring
            && symbol.token == STRING_LITERAL
            && symbol.len >= 6
            && this->lineOf(symbol.from) > previousCodeLine
        ) {
            const char q = this->m_text.at(symbol.from);
            if ((q == '"'
                    || q == '\'')
                && this->m_text.mid(symbol.from, 3) == QByteArray(3, q)
            ) {
                FormatComment docComment;
                docComment.offset = symbol.from;
                docComment.length = symbol.len;
                docComment.kind = FormatComment::BlockDoc;
                docComment.trailing = false;
                this->m_comments.append(docComment);
                previousCodeLine = this->lineOf(
                    symbol.from + symbol.len - 1
                );
                continue;
            }
        }
        this->m_symbols.append(symbol);
        previousCodeLine = this->lineOf(symbol.from + symbol.len - 1);
    }

    this->analyzeLines();
    this->analyzeBrackets();
}

/*!
    Parses the analyzed token and comment streams into an AST in @a arena and
    marks protected comments.
*/
AstParseResult FormatDocument::buildAst(AstArena *arena) const
{
    AstParseResult ast = AstParser::parse(
        m_text,
        m_symbols,
        m_comments,
        m_lineStarts,
        m_directiveLines,
        m_matches,
        m_language,
        arena
    );
    for (int i = 0; i < ast.comments.size(); ++i) {
        ast.comments.at(i)->isProtected = this->isProtectedComment(i);
    }
    return ast;
}

/*!
    Lists the comment token @a symbol, which is trailing when it starts on
    @a previousCodeLine, the line where the code token before it ends (0
    when there is none before it).
*/
void FormatDocument::appendComment(const Symbol & symbol, int previousCodeLine)
{
    FormatComment comment;
    comment.offset = symbol.from;
    comment.length = symbol.len;
    const QByteArray head = this->m_text.mid(symbol.from, 4);
    if (symbol.token == CPP_COMMENT) {
        if (head.startsWith('#')) {
            comment.kind = FormatComment::Line;
        } else {
            const bool doc = head.startsWith("///") || head.startsWith("//!");
            comment.kind = doc && ! head.startsWith("////")
                ? FormatComment::LineDoc : FormatComment::Line;
        }
    } else {
        const bool doc = head.startsWith("/**") || head.startsWith("/*!");
        comment.kind = doc && head != "/**/" ? FormatComment::BlockDoc
                                             : FormatComment::Block;
    }
    comment.trailing = previousCodeLine > 0
        && this->lineOf(symbol.from) == previousCodeLine;
    this->m_comments.append(comment);
}

/*!
    Lists, for each line, its first and its last code token, and whether it
    belongs to a preprocessor directive (a line that starts with `#`, and each
    line that a backslash at the end of the line before joins to it).
*/
void FormatDocument::analyzeLines()
{
    const int lines = this->lineCount();
    this->m_lineFirstSymbols.fill(-1, lines + 1);
    this->m_lineLastSymbols.fill(-1, lines + 1);
    this->m_directiveLines.fill(false, lines + 1);

    // Finds the directive lines.
    bool continued = false;
    for (int line = 1; line <= lines; ++line) {
        const QByteArray text = this->lineText(line);
        const QByteArray trimmed = text.trimmed();
        this->m_directiveLines[line] = continued || trimmed.startsWith('#');
        continued = this->m_directiveLines.at(line) && text.endsWith('\\');
    }

    // Finds each line's first and last code token.
    for (int i = 0; i < this->m_symbols.size(); ++i) {
        const int line = this->symbolLine(i);
        if (this->m_lineFirstSymbols.at(line) < 0) {
            this->m_lineFirstSymbols[line] = i;
        }
        this->m_lineLastSymbols[this->symbolEndLine(i)] = i;
    }
}

/*!
    Pairs each bracket of the code with the one that closes it, and finds the
    scope each brace opens and the scope that holds each token; the tokens of
    the preprocessor directives take no part.
*/
void FormatDocument::analyzeBrackets()
{
    const int count = this->m_symbols.size();
    this->m_matches.fill(-1, count);
    this->m_scopes.fill(FileScope, count);
    this->m_braceScopes.fill(FileScope, count);
    QVector<int > open;
    QVector<int > scopes;
    for (int i = 0; i < count; ++i) {
        const Token token = this->m_symbols.at(i).token;
        this->m_scopes[i] = scopes.isEmpty() ? int(FileScope) : scopes.last();
        if (this->isDirectiveLine(this->symbolLine(i))) {
            continue;
        }
        if (token == LPAREN || token == LBRACK || token == LBRACE) {
            if (token == LBRACE) {
                const Scope scope = this->classifyBrace(
                    i,
                    Scope(this->m_scopes.at(i))
                );
                this->m_braceScopes[i] = scope;
                scopes.append(scope);
            }
            open.append(i);
            continue;
        }
        if (token != RPAREN && token != RBRACK && token != RBRACE) {
            continue;
        }

        // Closes the nearest open bracket of the same kind; one left open in
        // between (such as in an `#if` branch) stays unmatched.
        const Token opening = token == RPAREN
            ? LPAREN : (token == RBRACK ? LBRACK : LBRACE);
        int k = open.size() - 1;
        while (k >= 0 && this->m_symbols.at(open.at(k)).token != opening) {
            --k;
        }
        if (k < 0) {
            continue;
        }
        this->m_matches[i] = open.at(k);
        this->m_matches[open.at(k)] = i;
        while (open.size() > k) {
            if (this->m_symbols.at(open.last()).token == LBRACE) {
                scopes.removeLast();
            }
            open.removeLast();
        }
        this->m_scopes[i] = scopes.isEmpty() ? int(FileScope) : scopes.last();
    }
}

/*!
    Builds the index in symbols() where the head of the token at @a index
    starts: the first token after the end of the statement before it (a `;`,
    a brace, an access label such as `public:`, or a preprocessor
    directive), stepping over bracket groups.
*/
int FormatDocument::headStart(int index) const
{
    int start = index;
    while (start > 0) {
        const Token before = this->token(start - 1);
        if (before == RBRACE) {
            const int opener = this->match(start - 1);
            if (opener >= 0 && this->braceScope(opener) == InitScope) {
                start = opener;
                continue;
            }
        }
        if (before == SEMIC
            || before == LBRACE
            || before == RBRACE
            || this->isAccessLabel(start - 1)
            || this->isDirectiveLine(this->symbolLine(start - 1))
        ) {
            break;
        }
        const int opener = this->match(start - 1);
        start = (before == RPAREN
                || before == RBRACK)
            && opener >= 0
            ? opener : start - 1;
    }
    return start;
}

/*!
    Tells whether the token at @a colon of symbols() is the `:` of an access
    label, such as `public:` or `public slots:`, which ends the declarations
    before it as a `;` would.
*/
bool FormatDocument::isAccessLabel(int colon) const
{
    if (colon < 1 || this->token(colon) != COLON) {
        return false;
    }
    switch (this->token(colon - 1)) {
    case PUBLIC:
    case PRIVATE:
    case PROTECTED:
    case SLOTS:
    case SIGNALS:
    case Q_SLOTS_TOKEN:
    case Q_SIGNALS_TOKEN:
        return true;
    default:
        return false;
    }
}

/*!
    Builds the scope that the brace at @a index of symbols() opens, inside the
    scope @a enclosing, from the tokens before it up to the end of the
    statement before them.
*/
FormatDocument::Scope FormatDocument::classifyBrace(
        int index, Scope enclosing) const
{
    if (enclosing == EnumScope || enclosing == InitScope) {
        return InitScope;
    }

    const int start = this->headStart(index);

    // Reads the head's tokens, leaving out the template parameters and the
    // parenthesized groups.
    bool hasParen = false;
    bool hasAssign = false;
    bool hasClassKey = false;
    bool hasEnum = false;
    bool hasNamespace = false;
    bool hasExtern = false;
    bool hasColon = false;
    for (int i = start; i < index; ++i) {
        const Token token = this->token(i);
        if (token == LPAREN) {
            hasParen = true;
            i = qMax(i, this->match(i));
        } else if (token == TEMPLATE && this->token(i + 1) == LANGLE) {
            int depth = 0;
            for (++i; i < index; ++i) {
                const Token inner = this->token(i);
                depth += inner == LANGLE
                    ? 1 : (inner == RANGLE ? -1 : (inner == GTGT ? -2 : 0));
                if (depth <= 0) {
                    break;
                }
            }
        } else if (token == CLASS || token == STRUCT || token == UNION) {
            hasClassKey = true;
        } else if (token == ENUM) {
            hasEnum = true;
        } else if (token == NAMESPACE) {
            hasNamespace = true;
        } else if (token == EQ) {
            hasAssign = true;
        } else if (token == COLON) {
            hasColon = true;
        } else if (token == EXTERN
            && this->token(i + 1) == STRING_LITERAL
        ) {
            hasExtern = true;
        }
    }

    // Tells the scope by the head.
    const Token last = this->token(index - 1);
    if (hasNamespace || (hasExtern && ! hasParen)) {
        return NamespaceScope;
    }
    if (hasEnum) {
        return EnumScope;
    }
    if (last == RPAREN && this->match(index - 1) >= 0) {
        const Token keyword = this->token(this->match(index - 1) - 1);
        if (keyword == IF
            || keyword == FOR
            || keyword == WHILE
            || keyword == SWITCH
            || keyword == CATCH
        ) {
            return BlockScope;
        }
        return FunctionScope;
    }
    if (last == ELSE || last == DO || last == TRY) {
        return BlockScope;
    }
    if (hasAssign
        || last == RETURN
        || last == COMMA
        || last == LPAREN
        || last == LBRACK
    ) {
        return InitScope;
    }
    if (hasClassKey) {
        return ClassScope;
    }
    if (last == IDENTIFIER && hasParen && hasColon) {
        return InitScope;
    }
    if (hasParen) {
        return FunctionScope;
    }

    // Takes a brace after a name for a braced initializer, and any other one
    // for a plain block.
    return last == IDENTIFIER ? InitScope : BlockScope;
}

/*!
    Builds the 1-based number of the line that holds @a offset.
*/
int FormatDocument::lineOf(int offset) const
{
    int low = 0;
    int high = this->m_lineStarts.size() - 1;
    while (low < high) {
        const int middle = (low + high + 1) / 2;
        if (this->m_lineStarts.at(middle) <= offset) {
            low = middle;
        } else {
            high = middle - 1;
        }
    }
    return low + 1;
}

/*!
    Builds the offset of the first character of the 1-based @a line.
*/
int FormatDocument::lineStart(int line) const
{
    return this->m_lineStarts.at(qBound(1, line, this->lineCount()) - 1);
}

/*!
    Builds the offset of the newline that ends the 1-based @a line, or the
    text's size for a last line with no newline.
*/
int FormatDocument::lineEnd(int line) const
{
    if (line >= this->lineCount()) {
        return this->m_text.size();
    }
    return this->m_lineStarts.at(line) - 1;
}

/*!
    Builds the text of the 1-based @a line, without its newline (and without
    a carriage return before it).
*/
QByteArray FormatDocument::lineText(int line) const
{
    const int start = this->lineStart(line);
    QByteArray text = this->m_text.mid(start, this->lineEnd(line) - start);
    if (text.endsWith('\r')) {
        text.chop(1);
    }
    return text;
}

/*!
    Builds the spaces and tabs that start the 1-based @a line.
*/
QByteArray FormatDocument::indentOf(int line) const
{
    const QByteArray text = this->lineText(line);
    int i = 0;
    while (i < text.size() && (text.at(i) == ' ' || text.at(i) == '\t')) {
        ++i;
    }
    return text.left(i);
}

/*!
    Builds the offset right after the token at @a index of symbols().
*/
int FormatDocument::symbolEnd(int index) const
{
    const Symbol & symbol = this->m_symbols.at(index);
    return symbol.from + symbol.len;
}

/*!
    Builds the 1-based number of the line where the token at @a index
    of symbols() starts.
*/
int FormatDocument::symbolLine(int index) const
{
    return this->lineOf(this->m_symbols.at(index).from);
}

/*!
    Builds the 1-based number of the line where the token at @a index of
    symbols() ends, which differs from symbolLine() for a token that spans
    lines (such as a raw string).
*/
int FormatDocument::symbolEndLine(int index) const
{
    return this->lineOf(this->symbolEnd(index) - 1);
}

/*!
    Builds the text of the token at @a index of symbols().
*/
QByteArray FormatDocument::lexem(int index) const
{
    const Symbol & symbol = this->m_symbols.at(index);
    return this->m_text.mid(symbol.from, symbol.len);
}

/*!
    Builds the kind of the token at @a index of symbols(), or `NOTOKEN` for
    an index outside of them.
*/
Token FormatDocument::token(int index) const
{
    if (index < 0 || index >= this->m_symbols.size()) {
        return NOTOKEN;
    }
    return this->m_symbols.at(index).token;
}

/*!
    Builds the index of the bracket that pairs with the bracket at @a index
    of symbols() (a parenthesis, a square bracket, or a brace), or -1 when
    the bracket has no pair.
*/
int FormatDocument::match(int index) const
{
    if (index < 0 || index >= this->m_matches.size()) {
        return -1;
    }
    return this->m_matches.at(index);
}

/*!
    Builds the index in symbols() of the first code token that starts at or
    after @a offset, or the count of symbols() when none does.
*/
int FormatDocument::symbolAfter(int offset) const
{
    int low = 0;
    int high = this->m_symbols.size();
    while (low < high) {
        const int middle = (low + high) / 2;
        if (this->m_symbols.at(middle).from < offset) {
            low = middle + 1;
        } else {
            high = middle;
        }
    }
    return low;
}

/*!
    Builds the scope that holds the token at @a index of symbols(); for a
    brace, that is the scope around the one it opens or closes.
*/
FormatDocument::Scope FormatDocument::scopeOf(int index) const
{
    return Scope(this->m_scopes.at(index));
}

/*!
    Builds the scope that the brace at @a index of symbols() opens.
*/
FormatDocument::Scope FormatDocument::braceScope(int index) const
{
    return Scope(this->m_braceScopes.at(index));
}

/*!
    Builds the index in symbols() of the first code token that starts on the
    1-based @a line, or -1 when none does.
*/
int FormatDocument::firstSymbolOfLine(int line) const
{
    if (line < 1 || line >= this->m_lineFirstSymbols.size()) {
        return -1;
    }
    return this->m_lineFirstSymbols.at(line);
}

/*!
    Builds the index in symbols() of the last code token that ends on the
    1-based @a line, or -1 when none does.
*/
int FormatDocument::lastSymbolOfLine(int line) const
{
    if (line < 1 || line >= this->m_lineLastSymbols.size()) {
        return -1;
    }
    return this->m_lineLastSymbols.at(line);
}

/*!
    Tells whether the 1-based @a line belongs to a preprocessor directive.
*/
bool FormatDocument::isDirectiveLine(int line) const
{
    if (line < 1 || line >= this->m_directiveLines.size()) {
        return false;
    }
    return this->m_directiveLines.at(line);
}

/*!
    Tells whether a line of a preprocessor directive lies between the offsets
    @a from and @a to, the lines that hold them included; an edit that joins
    or moves code across such a line would break the directive.
*/
bool FormatDocument::spansDirective(int from, int to) const
{
    const int last = this->lineOf(qMax(from, to));
    for (int line = this->lineOf(qMin(from, to)); line <= last; ++line) {
        if (this->isDirectiveLine(line)) {
            return true;
        }
    }
    return false;
}

/*!
    Builds the text of the comment at @a index of comments().
*/
QByteArray FormatDocument::commentText(int index) const
{
    const FormatComment & comment = this->m_comments.at(index);
    return this->m_text.mid(comment.offset, comment.length);
}

/*!
    Builds the 1-based number of the line where the comment at @a index of
    comments() starts.
*/
int FormatDocument::commentLine(int index) const
{
    return this->lineOf(this->m_comments.at(index).offset);
}

/*!
    Builds the 1-based number of the line where the comment at @a index
    of comments() ends.
*/
int FormatDocument::commentEndLine(int index) const
{
    const FormatComment & comment = this->m_comments.at(index);
    return this->lineOf(comment.offset + comment.length - 1);
}

/*!
    Tells whether the comment at @a index carries an inline setting.
*/
bool FormatDocument::isInlineConfigComment(int index) const
{
    const int offset = m_comments.at(index).offset;
    const int low = startsUpTo(m_inlineRanges, offset);
    return low > 0 && offset < m_inlineRanges.at(low - 1).second;
}

/*!
    Tells whether the comment at @a index is protected from formatting.
*/
bool FormatDocument::isProtectedComment(int index) const
{
    if (this->isInlineConfigComment(index)) {
        return true;
    }
    const QByteArray text = this->commentText(index);
    return text.startsWith("#!")
        || text.contains("TRACE/")
        || text.contains("$QT_BEGIN_LICENSE")
        || text.contains("SPDX-License-Identifier:")
        || text.startsWith("/****");
}

/*!
    Builds the index in comments() of the first comment that starts at or
    after @a offset, or the count of comments() when none does.
*/
int FormatDocument::commentAfter(int offset) const
{
    int low = 0;
    int high = this->m_comments.size();
    while (low < high) {
        const int middle = low + (high - low) / 2;
        if (this->m_comments.at(middle).offset < offset) {
            low = middle + 1;
        } else {
            high = middle;
        }
    }
    return low;
}

/*!
    Tells whether a comment starts between the offsets @a from and @a to.
*/
bool FormatDocument::hasCommentBetween(int from, int to) const
{
    if (to <= from) {
        return false;
    }
    const int first = this->commentAfter(from);
    return first < this->m_comments.size()
        && this->m_comments.at(first).offset < to;
}

/*!
    Tells whether a protected comment starts between the offsets @a from
    and @a to.
*/
bool FormatDocument::hasProtectedCommentBetween(int from, int to) const
{
    if (to <= from) {
        return false;
    }
    for (int i = this->commentAfter(from); i < this->m_comments.size() && this->m_comments.at(i).offset < to; ++i)
    {
        if (this->isProtectedComment(i)) {
            return true;
        }
    }
    return false;
}

/*!
    Tells whether any comment partially crosses either boundary of the
    half-open byte range [@a from, @a to).
*/
bool FormatDocument::overlapsComment(int from, int to) const
{
    const int first = this->commentAfter(from);
    if (first > 0) {
        const FormatComment & previous = this->m_comments.at(first - 1);
        if (previous.offset < from
            && previous.offset + previous.length > from
        ) {
            return true;
        }
    }
    for (int i = first; i < this->m_comments.size() && this->m_comments.at(i).offset < to; ++i)
    {
        const FormatComment & entry = this->m_comments.at(i);
        if (entry.offset + entry.length > to) {
            return true;
        }
    }
    return false;
}

/*!
    Tells whether the text between the offsets @a from and @a to is only
    spaces, tabs, carriage returns, and newlines.
*/
bool FormatDocument::isOnlyWhitespace(int from, int to) const
{
    for (int i = from; i < to; ++i) {
        const char c = this->m_text.at(i);
        if (c != ' ' && c != '\t' && c != '\r' && c != '\n') {
            return false;
        }
    }
    return true;
}

/*!
    Tells whether [@a from, @a to) holds only horizontal whitespace (plus line
    breaks when @a allowNewlines is set) and unprotected comments.
*/
bool FormatDocument::isCodeWhitespace(
        int from, int to, bool allowNewlines) const
{
    int comment = this->commentAfter(from);
    if (comment > 0
        && this->m_comments.at(comment - 1).offset + this->m_comments.at(
            comment - 1
        ).length
            > from
    ) {
        --comment;
    }
    int offset = from;
    while (offset < to) {
        while (comment < this->m_comments.size()
            && this->m_comments.at(comment).offset + this->m_comments.at(comment).length
                <= offset
        ) {
            ++comment;
        }
        if (comment < this->m_comments.size()
            && this->m_comments.at(comment).offset <= offset
            && ! this->isProtectedComment(comment)
        ) {
            const int end = qMin(
                to,
                this->m_comments.at(comment).offset
                    + this->m_comments.at(comment).length
            );
            while (offset < end) {
                const char c = this->m_text.at(offset);
                if ( ! allowNewlines && (c == '\r' || c == '\n')) {
                    return false;
                }
                ++offset;
            }
            continue;
        }
        const char c = this->m_text.at(offset);
        if (c != ' '
            && c != '\t'
            && ( ! allowNewlines
                || (c != '\r'
                    && c != '\n')
            )
        ) {
            return false;
        }
        ++offset;
    }
    return true;
}

/*!
    Builds the slice [@a from, @a to) with unprotected comments hidden from
    code layout: when @a keepWidth is set, comment bytes become spaces so
    column widths stay intact; otherwise unprotected comments are omitted.
*/
QByteArray FormatDocument::codeSlice(int from, int to, bool keepWidth) const
{
    from = qBound(0, from, this->m_text.size());
    to = qBound(from, to, this->m_text.size());
    int comment = this->commentAfter(from);
    if (comment > 0
        && this->m_comments.at(comment - 1).offset + this->m_comments.at(
            comment - 1
        ).length
            > from
    ) {
        --comment;
    }
    if (comment >= this->m_comments.size()
        || this->m_comments.at(comment).offset >= to
    ) {
        return this->m_text.mid(from, to - from);
    }

    QByteArray result;
    result.reserve(to - from);
    int offset = from;
    while (offset < to) {
        while (comment < this->m_comments.size()
            && this->m_comments.at(comment).offset + this->m_comments.at(comment).length
                <= offset
        ) {
            ++comment;
        }
        if (comment >= this->m_comments.size()
            || this->m_comments.at(comment).offset >= to
        ) {
            result += this->m_text.mid(offset, to - offset);
            break;
        }
        const FormatComment & entry = this->m_comments.at(comment);
        if (offset < entry.offset) {
            result += this->m_text.mid(offset, entry.offset - offset);
            offset = entry.offset;
        }
        const int end = qMin(to, entry.offset + entry.length);
        if (this->isProtectedComment(comment)) {
            result += this->m_text.mid(offset, end - offset);
            offset = end;
            continue;
        }
        if (keepWidth) {
            for (; offset < end; ++offset) {
                const char c = this->m_text.at(offset);
                result += (c == '\r' || c == '\n') ? c : ' ';
            }
            continue;
        }
        const bool internalNewline =
            this->m_text.mid(offset, end - offset).contains('\n');
        if ( ! internalNewline
            && end < to
            && ! result.isEmpty()
            && ! QByteArray(" \t\r\n([{").contains(
                result.at(result.size() - 1))
            && ! QByteArray(" \t\r\n)]},;").contains(this->m_text.at(end))
        ) {
            result += ' ';
        }
        if (internalNewline) {
            for (int k = offset; k < end; ++k) {
                const char c = this->m_text.at(k);
                if (c == '\r' || c == '\n') {
                    result += c;
                }
            }
        }
        offset = end;
        if ( ! internalNewline
            && ! result.isEmpty()
            && (result.endsWith(' ')
                || result.endsWith('\t'))
        ) {
            while (offset < to
                && (this->m_text.at(offset) == ' '
                    || this->m_text.at(offset) == '\t')
            ) {
                ++offset;
            }
        }
    }
    return result;
}

/*!
    Builds the 1-based @a line with unprotected comments replaced by spaces,
    keeping line width unchanged while hiding comment text from line checks.
*/
QByteArray FormatDocument::codeLineText(int line) const
{
    if (line < 1 || line > this->m_lineStarts.size()) {
        return QByteArray();
    }
    const int start = this->lineStart(line);
    int end = this->lineEnd(line);
    if (end > start && this->m_text.at(end - 1) == '\r') {
        --end;
    }
    return this->codeSlice(start, end, true);
}

/*!
    Limits the edits and the issues to the 1-based, inclusive line @a ranges;
    an empty list selects the whole text.
*/
void FormatDocument::setLineRanges(const QVector<QPair<int, int > > & ranges)
{
    this->m_ranges = ranges;
}

/*!
    Tells whether the 1-based @a line is selected for formatting.
*/
bool FormatDocument::isLineSelected(int line) const
{
    if ( ! m_disabledRanges.isEmpty()
        && this->isFormattingDisabled(this->lineStart(line))
        && this->isFormattingDisabled(this->lineEnd(line))
    ) {
        return false;
    }
    if (this->m_ranges.isEmpty()) {
        return true;
    }
    for (int i = 0; i < this->m_ranges.size(); ++i) {
        if (line >= this->m_ranges.at(i).first
            && line <= this->m_ranges.at(i).second
        ) {
            return true;
        }
    }
    return false;
}

/*!
    Tells whether @a offset is inside a disabled formatting range.
*/
bool FormatDocument::isFormattingDisabled(int offset) const
{
    const int low = startsUpTo(m_disabledRanges, offset);
    return low > 0 && offset < m_disabledRanges.at(low - 1).second;
}

bool FormatDocument::isOffsetEditable(int offset, int length) const
{
    if ( ! this->isLineSelected(this->lineOf(offset))) {
        return false;
    }
    int low = 0;
    int high = m_inlineRanges.size();
    while (low < high) {
        const int middle = low + (high - low) / 2;
        if (m_inlineRanges.at(middle).second <= offset) {
            low = middle + 1;
        } else {
            high = middle;
        }
    }
    if (low < m_inlineRanges.size()) {
        const QPair<int, int > & range = m_inlineRanges.at(low);
        if ((length == 0
                && offset > range.first
                && offset < range.second)
            || (length > 0
                && offset < range.second
                && offset + length > range.first)
        ) {
            return false;
        }
    }
    return true;
}

/*!
    Tells whether [@a offset, @a offset + @a length) lies on a selected line
    and outside disabled or inline-configuration ranges.
*/
bool FormatDocument::isSpanEditable(int offset, int length) const
{
    if (offset < 0 || length < 0 || offset > m_text.size() - length) {
        return false;
    }
    if (this->isFormattingDisabled(offset)
        || (length > 0
            && this->isFormattingDisabled(offset + length - 1))
    ) {
        return false;
    }
    return this->isOffsetEditable(offset, length);
}

/*!
    Replaces the buffer with @a text after code formatting finishes and
    optionally refreshes tokens and comments once for comment checks.
*/
void FormatDocument::resetTextAndAnalyze(
        const QByteArray & text, bool reanalyze)
{
    if (m_text == text) {
        return;
    }
    m_text = text;
    m_ruleCaches.clear();
    m_commentChanged = false;
    if (reanalyze) {
        this->analyze();
        if (text.contains("Format rule") || text.contains('@')) {
            QString ignoredError;
            this->setInlineConfiguration(m_config, &ignoredError);
        }
    }
}

void FormatDocument::spliceTextKeepTokens(
        int offset, int length, const QByteArray & text)
{
    if (offset < 0 || length < 0 || offset > m_text.size() - length) {
        return;
    }
    if (m_text.mid(offset, length) == text) {
        return;
    }

    const int line = this->lineOf(offset);
    const int oldLineStart = this->lineStart(line);
    const int oldEnd = offset + length;
    const int deltaBytes = text.size() - length;
    const int removedNewlines = m_text.mid(offset, length).count('\n');
    const int deltaLines = text.count('\n') - removedNewlines;

    m_text.replace(offset, length, text);

    // Updates line starts around `[offset, offset + text.size())` without
    // scanning the full file again.
    QVector<int > insertedStarts;
    for (int i = 0; i < text.size(); ++i) {
        if (text.at(i) == '\n') {
            insertedStarts.append(offset + i + 1);
        }
    }
    if (removedNewlines > 0 && line < m_lineStarts.size()) {
        const int eraseCount = qMin(
            removedNewlines,
            m_lineStarts.size() - line
        );
        m_lineStarts.remove(line, eraseCount);
    }
    for (int i = 0; i < insertedStarts.size(); ++i) {
        m_lineStarts.insert(line + i, insertedStarts.at(i));
    }
    for (int k = line + insertedStarts.size(); k < m_lineStarts.size(); ++k) {
        m_lineStarts[k] += deltaBytes;
    }

    if (deltaLines != 0) {
        const int symLinePos = (offset == oldLineStart && deltaLines > 0)
            ? line : qMin(line + 1, m_lineFirstSymbols.size());
        if (deltaLines > 0) {
            m_lineFirstSymbols.insert(symLinePos, deltaLines, -1);
            m_lineLastSymbols.insert(symLinePos, deltaLines, -1);
            m_directiveLines.insert(symLinePos, deltaLines, false);
        } else {
            const int eraseAt = qMin(line + 1, m_lineFirstSymbols.size());
            const int count = qMin(
                -deltaLines,
                m_lineFirstSymbols.size() - eraseAt
            );
            if (count > 0) {
                m_lineFirstSymbols.remove(eraseAt, count);
                m_lineLastSymbols.remove(eraseAt, count);
                m_directiveLines.remove(eraseAt, count);
            }
        }
        const int shiftLine = (offset == oldLineStart) ? line - 1 : line;
        for (int r = 0; r < m_ranges.size(); ++r) {
            QPair<int, int > *range = &(m_ranges[r]);
            if (shiftLine < range->first) {
                range->first += deltaLines;
                range->second += deltaLines;
            } else if (shiftLine <= range->second) {
                range->second += deltaLines;
            }
        }
        FormatDocument::shiftIssues(&m_issues, shiftLine, deltaLines);
        FormatDocument::shiftIssues(&m_requests, shiftLine, deltaLines);
    }

    for (int s = 0; s < m_symbols.size(); ++s) {
        Symbol *sym = &(m_symbols[s]);
        sym->source = m_text;
        if (sym->from >= oldEnd || (length == 0 && sym->from >= offset)) {
            sym->from += deltaBytes;
            sym->lineNum += deltaLines;
        }
    }

    for (int c = 0; c < m_comments.size(); ++c) {
        FormatComment *comment = &(m_comments[c]);
        if (comment->offset >= oldEnd
            || (length == 0
                && comment->offset >= offset)
        ) {
            comment->offset += deltaBytes;
        } else if (comment->offset <= offset
            && oldEnd <= comment->offset + comment->length
        ) {
            comment->length += deltaBytes;
        }
    }

    for (int i = 0; i < m_fileConfigRegions.size(); ++i) {
        if (m_fileConfigRegions.at(i).first >= oldEnd
            || (length == 0
                && m_fileConfigRegions.at(i).first >= offset)
        ) {
            m_fileConfigRegions[i].first += deltaBytes;
        }
    }
    QVector<QPair<int, int > > *inlineLists[] =
        { &m_inlineRanges, &m_disabledRanges };
    for (int l = 0; l < 2; ++l) {
        for (int i = 0; i < inlineLists[l]->size(); ++i) {
            QPair<int, int > *range = &((*inlineLists[l])[i]);
            if (range->first > offset
                || (range->first == offset
                    && length == 0)
            ) {
                range->first += deltaBytes;
            }
            if (range->second >= oldEnd) {
                range->second += deltaBytes;
            }
        }
    }
    m_ruleCaches.clear();
    m_commentChanged = true;
}

/*!
    Rewrites the @a length bytes inside a comment at @a offset with @a text
    without queueing delayed edits or re-lexing C++ code tokens.
*/
bool FormatDocument::rewriteCommentSpan(
        int offset, int length, const QByteArray & text)
{
    if ( ! this->isOffsetEditable(offset, length)) {
        return false;
    }
    if (m_text.mid(offset, length) == text) {
        return false;
    }
    this->spliceTextKeepTokens(offset, length, text);
    return true;
}

/*!
    Moves a trailing comment removed at @a removeFrom .. @a removeLength onto
    its own line at @a insertOffset as @a leadingLine without re-lexing code.
*/
bool FormatDocument::promoteTrailingComment(
        int removeFrom,
        int removeLength,
        int insertOffset,
        const QByteArray & leadingLine)
{
    if (insertOffset > removeFrom
        || ! this->isOffsetEditable(removeFrom, removeLength)
        || ! this->isOffsetEditable(insertOffset, 0)
    ) {
        return false;
    }
    int movedIndex = -1;
    for (int i = 0; i < m_comments.size(); ++i) {
        const FormatComment & c = m_comments.at(i);
        if (c.offset >= removeFrom
            && c.offset < removeFrom + removeLength
        ) {
            movedIndex = i;
            break;
        }
    }
    if (movedIndex < 0) {
        return false;
    }
    const FormatComment::Kind movedKind = m_comments.at(movedIndex).kind;
    this->spliceTextKeepTokens(removeFrom, removeLength, QByteArray());

    // Parks the removed entry at `insertOffset` before splicing the
    // leading line so later offset shifts do not double-count it, then
    // places it in order.
    m_comments[movedIndex].offset = insertOffset;
    m_comments[movedIndex].length = 0;
    this->spliceTextKeepTokens(insertOffset, 0, leadingLine);

    int ws = 0;
    while (ws < leadingLine.size()
        && (leadingLine.at(ws) == ' '
            || leadingLine.at(ws) == '\t')
    ) {
        ++ws;
    }
    int cleanLen = leadingLine.size() - ws;
    while (cleanLen > 0
        && (leadingLine.at(ws + cleanLen - 1) == '\n'
            || leadingLine.at(ws + cleanLen - 1) == '\r')
    ) {
        --cleanLen;
    }
    m_comments[movedIndex].offset = insertOffset + ws;
    m_comments[movedIndex].length = cleanLen;
    m_comments[movedIndex].kind = movedKind;
    m_comments[movedIndex].trailing = false;
    while (movedIndex > 0
        && m_comments.at(movedIndex - 1).offset
            > m_comments.at(movedIndex).offset
    ) {
        m_comments.swap(movedIndex - 1, movedIndex);
        --movedIndex;
    }
    return true;
}

/*!
    Records an issue of @a rule at @a offset that the rule cannot fix on its
    own, with @a message, which prints to the error output when @a isError
    is set; an issue on a line that is not selected is dropped.
*/
void FormatDocument::report(
        int offset,
        const char *rule,
        const QString & message,
        bool isError)
{
    const int line = this->lineOf(offset);
    if (this->isFormattingDisabled(offset)
        || ! this->isLineSelected(line)
    ) {
        return;
    }
    FormatIssue issue(line, QByteArray(rule), message);
    issue.isError = isError;
    this->m_issues.append(issue);
}

/*!
    Asks the agent (in agent mode, see agentMode()) to handle @a message for
    the @a lineCount lines from the one at @a offset, as the rule @a rule
    names, each line within @a columnLimit columns when it is not 0; a
    request already asked for the same lines is not asked again.
*/
void FormatDocument::request(
        int offset,
        const char *rule,
        const QString & message,
        int lineCount,
        int columnLimit)
{
    const int line = this->lineOf(offset);
    if ( ! this->m_agentMode
        || this->isFormattingDisabled(offset)
        || ! this->isLineSelected(line)
    ) {
        return;
    }
    const FormatIssue asked(
        line,
        QByteArray(rule),
        message,
        line + qMax(lineCount, 1) - 1,
        columnLimit
    );
    for (int i = 0; i < this->m_requests.size(); ++i) {
        const FormatIssue & other = this->m_requests.at(i);
        if (other.line == asked.line
            && other.rule == asked.rule
            && other.message == asked.message
        ) {
            return;
        }
    }
    this->m_requests.append(asked);
}

/*!
    Drops the issues after the first @a count ones.
*/
void FormatDocument::truncateIssues(int count)
{
    while (this->m_issues.size() > count) {
        this->m_issues.removeLast();
    }
}

/*!
    Starts the pass @a pass of a rule over the document (0 for the first
    one); the first pass also forgets the lines that the previous rule's
    replacements took (see onSetting()).
*/
void FormatDocument::setPass(int pass)
{
    this->m_pass = pass;
    if (pass == 0) {
        this->m_replacedLines.clear();
    }
}

/*!
    Builds the spacing to follow at the token at @a index for @a setting; a
    null spacing (see FormatSpacing::isNull()) means a replacement by
    regular expression took the token's line (stored into @a replacedLine when
    non-null), hence the caller skips it.
*/
FormatSpacing FormatDocument::onSetting(
        const FormatSpacing & setting, int index, QByteArray *replacedLine)
{
    if ( ! setting.isRegExp()) {
        return setting;
    }
    FormatSpacing fallback = setting;
    fallback.pattern = QRegularExpression();
    fallback.replacement = Config::Replacement();
    const int line = this->symbolLine(index);
    if (this->m_replacedLines.contains(line)) {
        return FormatSpacing();
    }
    if (this->m_pass > 0) {
        return fallback;
    }
    const QByteArray text = this->lineText(line);
    const QString decoded = QString::fromUtf8(text);
    if ( ! setting.pattern.match(decoded).hasMatch()) {
        return fallback;
    }
    const QString replaced = setting.replacement.apply(
        setting.pattern,
        decoded
    );
    if (replaced != decoded && replacedLine) {
        *replacedLine = replaced.toUtf8();
    }
    this->m_replacedLines.insert(line);
    return FormatSpacing();
}

/*!
    Moves @a issues by @a delta lines for an edit on the line @a line: an
    issue below it moves whole, and one whose range holds that line grows
    or shrinks with it.
*/
void FormatDocument::shiftIssues(
        QList<FormatIssue > *issues, int line, int delta)
{
    for (int k = 0; k < issues->size(); ++k) {
        FormatIssue *issue = &((*issues)[k]);
        if (issue->line > line) {
            issue->line += delta;
            issue->endLine += delta;
        } else if (issue->endLine >= line) {
            issue->endLine = qMax(issue->line, issue->endLine + delta);
        }
    }
}

/*!
    Tells whether the document is a header: its file name ends in `.h`,
    `.hh`, `.hpp`, `.hxx` or `.h++`, or has no suffix at all, like the
    forwarding headers (`QString`); a document without a path counts as one.
*/
bool FormatDocument::isHeader() const
{
    const QString suffix = QFileInfo(this->m_path).suffix().toLower();
    return suffix.isEmpty()
        || suffix == QLL("h")
        || suffix == QLL("hh")
        || suffix == QLL("hpp")
        || suffix == QLL("hxx")
        || suffix == QLL("h++");
}

/*!
    Finds the header that the quoted include @a name names, the way a
    compiler does: next to the document's file first, then in each folder of
    includePaths(), in order; an empty string when none holds it.
*/
QString FormatDocument::findInclude(const QString & name) const
{
    QStringList folders;
    if ( ! this->m_path.isEmpty()) {
        folders << QFileInfo(this->m_path).absolutePath();
    }
    folders << this->m_includePaths;
    for (int i = 0; i < folders.size(); ++i) {
        const QString candidate = QDir(folders.at(i)).filePath(name);
        if (QFileInfo(candidate).isFile()) {
            return QDir::cleanPath(candidate);
        }
    }
    return QString();
}

/*!
    Lists the names that the quoted `#include` lines of the document name,
    in order (`"./widget.h"` gives `./widget.h`).
*/
QStringList FormatDocument::quotedIncludes() const
{
    QStringList names;
    const QRegularExpression include(
        QString(QLL("^\\s*#\\s*include\\s*\"([^\"]+)\"")),
        QRegularExpression::MultilineOption
    );
    QRegularExpressionMatchIterator found = include.globalMatch(
        QString::fromUtf8(this->m_text)
    );
    while (found.hasNext()) {
        names << found.next().captured(1);
    }
    return names;
}
