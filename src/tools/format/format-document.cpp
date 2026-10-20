// Copyright (C) 2015 The XD Company Ltd.
// SPDX-License-Identifier: Apache-2.0

#include "./format-document.h"

#include "preprocessor.h"

#include <QtCore/QDir>
#include <QtCore/QFileInfo>
#include <QtCore/QRegularExpression>

/*!
    @class FormatDocument
    @internal

    Holds one file under formatting as text plus moc's tokens of it, and
    queues the edits and issues that the formatting rules find.

    The tokens come from Preprocessor::tokenize() in its preprocess-only mode
    with Preprocessor::keepComments set, which keeps every token's offset into
    the text and emits the comments as tokens too; hence the document lists
    comments() without a parser of its own. Edits apply only to the selected
    lines (see setLineRanges()), and applyEdits() tokenizes the new text again
    for the next rule.
*/

/*!
    Builds the document of @a text, the content of the file at @a path (which
    rules that look at the file's folder use, and which may be empty).
*/
FormatDocument::FormatDocument(const QByteArray & text, const QString & path)
    : m_text(text), m_path(path), m_pass(0), m_agentMode(false)
{
    this->analyze();
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

    // Splits moc's tokens into comments and code, leaving out whitespace,
    // newlines, and the tokens moc adds without text (such as
    // the `PP_IF` of an `#ifdef`).
    const Symbols all = qMove(Preprocessor::tokenize(this->m_text));
    int previousCodeLine = 0;
    for (int i = 0; i < all.size(); ++i) {
        const Symbol & symbol = all.at(i);
        if (symbol.len <= 0 || symbol.token == WHITESPACE || symbol.token == PP_WHITESPACE
            || symbol.token == NOTOKEN || symbol.token == PP_NEWLINE || symbol.token == NEWLINE
        ) {
            continue;
        }
        if (symbol.token == CPP_COMMENT || symbol.token == C_COMMENT) {
            this->appendComment(symbol, previousCodeLine);
            continue;
        }
        this->m_symbols.append(symbol);
        previousCodeLine = this->lineOf(symbol.from + symbol.len - 1);
    }

    this->analyzeLines();
    this->analyzeBrackets();
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
        const bool doc = head.startsWith("///") || head.startsWith("//!");
        comment.kind = doc && ! head.startsWith("////")
            ? FormatComment::LineDoc : FormatComment::Line;
    } else {
        const bool doc = head.startsWith("/**") || head.startsWith("/*!");
        comment.kind = doc && head != "/**/"
            ? FormatComment::BlockDoc : FormatComment::Block;
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
        this->m_scopes[i] = scopes.isEmpty()
            ? int(FileScope) : scopes.last();
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
        this->m_scopes[i] = scopes.isEmpty()
            ? int(FileScope) : scopes.last();
    }
}

/*!
    Builds the index in symbols() where the head of the token at @a index
    starts: the first token after the end of the statement before it (a `;`,
    a brace, or a preprocessor directive), stepping over bracket groups.
*/
int FormatDocument::headStart(int index) const
{
    int start = index;
    while (start > 0) {
        const Token before = this->token(start - 1);
        if (before == SEMIC || before == LBRACE || before == RBRACE
            || this->isDirectiveLine(this->symbolLine(start - 1))
        ) {
            break;
        }
        const int opener = this->match(start - 1);
        start = (before == RPAREN || before == RBRACK) && opener >= 0
            ? opener : start - 1;
    }
    return start;
}

/*!
    Builds the scope that the brace at @a index of symbols() opens, inside the
    scope @a enclosing, from the tokens before it up to the end of the
    statement before them.
*/
FormatDocument::Scope FormatDocument::classifyBrace(
        int index,
        Scope enclosing) const
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
                    ? 1
                    : (inner == RANGLE ? -1 : (inner == GTGT ? -2 : 0));
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
        if (keyword == IF || keyword == FOR || keyword == WHILE || keyword == SWITCH
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
    Tells whether the comment at \a index of comments() is one that no rule
    changes: a `TRACE/` record, or a license header (a Qt license block, or
    an `SPDX-License-Identifier:` tag, which tools read as it is).
*/
bool FormatDocument::isProtectedComment(int index) const
{
    const QByteArray text = this->commentText(index);
    return text.contains("TRACE/") || text.contains("$QT_BEGIN_LICENSE")
        || text.contains("SPDX-License-Identifier:") || text.startsWith("/****");
}

/*!
    Tells whether a comment starts between the offsets @a from and @a to.
*/
bool FormatDocument::hasCommentBetween(int from, int to) const
{
    for (int i = 0; i < this->m_comments.size(); ++i) {
        const int offset = this->m_comments.at(i).offset;
        if (offset >= from && offset < to) {
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
    Queues the replacement of the @a length bytes at @a offset with @a text,
    and returns whether it was queued: an edit on a line that is not selected,
    or one that overlaps an edit queued before it, is dropped (a later pass of
    the rule may make it once the queued edits are applied).
*/
bool FormatDocument::replace(int offset, int length, const QByteArray & text)
{
    if ( ! this->isLineSelected(this->lineOf(offset))) {
        return false;
    }
    for (int i = 0; i < this->m_edits.size(); ++i) {
        const Edit & edit = this->m_edits.at(i);
        const bool apart = offset + length <= edit.offset || edit.offset
            + edit.length <= offset;
        const bool sameInsertion = length == 0 && edit.length == 0
            && offset == edit.offset;
        if ( ! apart || sameInsertion) {
            return false;
        }
    }
    this->m_edits.append(Edit(offset, length, text));
    return true;
}

/*!
    Records an issue of @a rule at @a offset that the rule cannot fix on its
    own, with @a message; an issue on a line that is not selected is dropped.
*/
void FormatDocument::report(int offset, const char *rule,
                            const QString & message)
{
    const int line = this->lineOf(offset);
    if ( ! this->isLineSelected(line)) {
        return;
    }
    this->m_issues.append(FormatIssue(line, QByteArray(rule), message));
}

/*!
    Asks the agent (in agent mode, see agentMode()) to handle @a message for
    the @a lineCount lines from the one at @a offset, as the rule @a rule
    names, each line within @a columnLimit columns when it is not 0; a
    request already asked for the same lines is not asked again.

    A request outlives the passes of its rule, unlike an issue, which the
    next pass reports again: it often concerns a change the rule made, which
    no later pass sees to ask about again. Its lines move with the edits that
    later rules make (see applyEdits()).
*/
void FormatDocument::request(
        int offset, const char *rule, const QString & message,
        int lineCount, int columnLimit)
{
    const int line = this->lineOf(offset);
    if ( ! this->m_agentMode || ! this->isLineSelected(line)) {
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
        if (other.line == asked.line && other.rule == asked.rule
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
    Applies the queued edits, moves the selected line ranges and the issues
    along with the lines that the edits add or remove, then tokenizes the new
    text; returns whether any edit changed the text.
*/
bool FormatDocument::applyEdits()
{
    if (this->m_edits.isEmpty()) {
        return false;
    }

    // Applies the edits from the last one to the first one, so that each
    // offset still points into the text it was found in.
    QList<Edit > edits = this->m_edits;
    this->m_edits.clear();
    for (int i = 1; i < edits.size(); ++i) {
        for (int j = i; j > 0 && edits.at(j - 1).offset < edits.at(j).offset; --j) {
            edits.swap(j - 1, j);
        }
    }
    QByteArray text = this->m_text;
    for (int i = 0; i < edits.size(); ++i) {
        const Edit & edit = edits.at(i);
        const int line = this->lineOf(edit.offset);
        const int delta = edit.text.count('\n')
            - this->m_text.mid(edit.offset, edit.length).count('\n');
        text.replace(edit.offset, edit.length, edit.text);
        if (delta == 0) {
            continue;
        }

        // Moves the ranges and the issues below the edit.
        for (int r = 0; r < this->m_ranges.size(); ++r) {
            QPair<int, int > & range = this->m_ranges[r];
            if (line < range.first) {
                range.first += delta;
                range.second += delta;
            } else if (line <= range.second) {
                range.second += delta;
            }
        }
        FormatDocument::shiftIssues(&this->m_issues, line, delta);
        FormatDocument::shiftIssues(&this->m_requests, line, delta);
    }

    const bool changed = text != this->m_text;
    this->m_text = text;
    this->analyze();
    return changed;
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
    regular expression took the token's line, hence the caller skips it.
    Otherwise the result is never null: @a setting itself, or, for a choice
    by regular expression whose pattern matches nothing on that line, its
    fallback (such as `keep-as-is`).

    Only the first pass of a rule (see pass()) runs a replacement, on each
    line whose text the pattern matches; a later pass leaves exactly those
    lines to it. Hence the fallback never changes a replaced line again, and
    a pattern that matches its own result runs once.
*/
FormatSpacing FormatDocument::onSetting(
        const FormatSpacing & setting,
        int index)
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
    if (replaced != decoded) {
        this->replace(this->lineStart(line), text.size(), replaced.toUtf8());
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
        QList<FormatIssue > *issues, int line,
        int delta)
{
    for (int k = 0; k < issues->size(); ++k) {
        FormatIssue & issue = (*issues)[k];
        if (issue.line > line) {
            issue.line += delta;
            issue.endLine += delta;
        } else if (issue.endLine >= line) {
            issue.endLine = qMax(issue.line, issue.endLine + delta);
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
    return suffix.isEmpty() || suffix == QLL("h") || suffix == QLL("hh") || suffix == QLL("hpp")
        || suffix == QLL("hxx") || suffix == QLL("h++");
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
    QRegularExpressionMatchIterator found = include.globalMatch(QString::fromUtf8(this->m_text));
    while (found.hasNext()) {
        names << found.next().captured(1);
    }
    return names;
}
