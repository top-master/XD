// Copyright (C) 2015 The XD Company Ltd.
// SPDX-License-Identifier: Apache-2.0

#include "./class-helpers.h"
#include "./code-helpers.h"

#include <QtCore/QByteArray>
#include <QtCore/QFile>
#include <QtCore/QList>

/*!
    Tells whether @a token can end the type in front of a declared name.
*/
bool endsType(Token token)
{
    switch (token) {
    case IDENTIFIER:
    case CONST:
    case RANGLE:
    case GTGT:
    case STAR:
    case AND:
    case AUTO:
    case CHAR:
    case WCHAR:
    case BOOL:
    case SHORT:
    case INT:
    case LONG:
    case SIGNED:
    case UNSIGNED:
    case FLOAT:
    case DOUBLE:
    case VOID:
        return true;
    default:
        return false;
    }
}

/*!
    Tells whether @a name is spelled the way a macro is: three or more
    upper-case letters, digits and underscores (a one- or two-letter name is
    more often a template parameter, such as `T`).
*/
bool isMacroName(const QByteArray & name)
{
    if (name.size() < 3) {
        return false;
    }
    for (int i = 0; i < name.size(); ++i) {
        const char c = name.at(i);
        if ( ! (c >= 'A'
                && c <= 'Z')
            && ! (c >= '0'
                && c <= '9')
            && c != '_'
        ) {
            return false;
        }
    }
    return true;
}

/*!
    Builds the indexes of the tokens that sit directly in the class body
    whose `{` is at @a brace of @a document, leaving out the
    bodies nested in it.
*/
QList<int > classBodyTokens(const FormatDocument *document, int brace)
{
    QList<int > tokens;
    const int close = document->match(brace);
    for (int i = brace + 1; i < close; ++i) {
        if (document->token(i) == LBRACE && document->match(i) > i) {
            tokens.append(i);
            i = document->match(i) - 1;
            continue;
        }
        tokens.append(i);
    }
    return tokens;
}

/*!
    Reads, from the head of the class body whose `{` is at @a brace of
    @a document, whether it is a template and the names of its base classes
    into @a members: each base's last name, without its namespace or class
    qualifiers and without its template arguments (`QHash` of
    `public QHash<K, V >`).
*/
static void readClassHead(const FormatDocument *document, int brace,
                          FormatClassMembers *members)
{
    const int start = document->headStart(brace);
    int colon = -1;
    for (int k = start; k < brace; ++k) {
        const Token token = document->token(k);
        if (token == TEMPLATE) {
            members->isTemplate = true;
        } else if (token == COLON && colon < 0) {
            colon = k;
        }
    }
    if (colon < 0) {
        return;
    }

    // Takes the name before each `,` or `<` at the head's own depth.
    int depth = 0;
    QByteArray last;
    for (int k = colon + 1; k <= brace; ++k) {
        const Token token = document->token(k);
        if (token == LANGLE) {
            if (depth == 0 && ! last.isEmpty()) {
                members->bases << last;
                last.clear();
            }
            ++depth;
        } else if (token == RANGLE) {
            --depth;
        } else if (token == GTGT) {
            depth -= 2;
        } else if (depth == 0 && token == IDENTIFIER) {
            last = document->lexem(k);
        } else if (depth == 0
            && (token == COMMA
                || token == LBRACE)
            && ! last.isEmpty()
        ) {
            members->bases << last;
            last.clear();
        }
    }
}

/*!
    Lists the members of the class body whose `{` is at @a brace of
    @a document, named @a className, with its bases and whether it is a
    template (see FormatClassMembers).
*/
FormatClassMembers collectMembers(
        const FormatDocument *document, int brace,
        const QByteArray & className)
{
    FormatClassMembers members;
    readClassHead(document, brace, &members);
    const QList<int > tokens = classBodyTokens(document, brace);
    bool isStatic = false;
    bool skip = false;
    for (int t = 0; t < tokens.size(); ++t) {
        const int i = tokens.at(t);
        const Token token = document->token(i);
        if (token == SEMIC
            || token == RBRACE
            || (token == LBRACE
                && document->match(i) > i)
            || document->isAccessLabel(i)
        ) {
            isStatic = false;
            skip = false;
            continue;
        }
        if (token == STATIC) {
            isStatic = true;
        } else if (token == TYPEDEF
            || token == USING
            || token == FRIEND
            || token == ENUM
            || token == CLASS
            || token == STRUCT
            || token == UNION
        ) {
            skip = true;
        }
        if (skip || token != IDENTIFIER) {
            continue;
        }
        const QByteArray name = document->lexem(i);
        const Token after = document->token(i + 1);
        const bool declares = after == LPAREN || after == SEMIC || after == EQ || after == LBRACK
            || after == COLON || after == COMMA;
        if ( ! declares
            || name == className
            || isMacroName(name)
            || ! endsType(document->token(i - 1))
            || document->token(i - 1) == TILDE
        ) {
            continue;
        }
        if (isStatic) {
            members.statics.insert(name);
        } else {
            members.names.insert(name);
        }
        if (after == LPAREN && document->match(i + 1) > 0) {
            // Steps over the parameters, whose names are no members.
            while (t + 1 < tokens.size()
                && tokens.at(t + 1) <= document->match(i + 1)
            ) {
                ++t;
            }
        }
    }
    return members;
}

/*!
    Tells whether the token at @a index of @a document sits in the arguments
    of a macro (a call whose name is spelled the way a macro is), up to the
    start of its statement; a macro may paste or quote the name, such as
    `SIGNAL(valueChanged(int))` or `QT_PREPEND_NAMESPACE(qHash)`.
*/
bool isMacroArgument(const FormatDocument *document, int index)
{
    for (int k = index - 1; k >= 0; --k) {
        const Token token = document->token(k);
        if ((token == RPAREN
                || token == RBRACK)
            && document->match(k) >= 0
        ) {
            k = document->match(k);
            continue;
        }
        if (token == SEMIC || token == LBRACE || token == RBRACE) {
            return false;
        }
        if (token == LPAREN
            && k > 0
            && isMacroName(document->lexem(k - 1))
        ) {
            return true;
        }
    }
    return false;
}

/*!
    Finds the body of the class named @a className in @a document, and builds
    the index of its `{`, or -1 when the document defines no such class.
*/
int findClassBody(const FormatDocument *document,
                  const QByteArray & className)
{
    for (int i = 0; i < document->symbols().size(); ++i) {
        if (document->token(i) == LBRACE
            && document->braceScope(i) == FormatDocument::ClassScope
            && classNameOf(document, i) == className
        ) {
            return i;
        }
    }
    return -1;
}

/*!
    Builds the index of the `{` of the innermost class body of @a document
    that holds the token at @a index, or -1 when none does.
*/
int enclosingClassBody(const FormatDocument *document, int index)
{
    int depth = 0;
    for (int k = index - 1; k >= 0; --k) {
        const Token token = document->token(k);
        if (token == RBRACE) {
            ++depth;
        } else if (token == LBRACE) {
            if (depth > 0) {
                --depth;
            } else if (document->braceScope(k) == FormatDocument::ClassScope) {
                return k;
            }
        }
    }
    return -1;
}

/*!
    Builds the name of the class whose body's `{` is at @a brace of
    @a document, after any export macro (`class Q_CORE_EXPORT QFile`); empty
    for an unnamed class.
*/
QByteArray classNameOf(const FormatDocument *document, int brace)
{
    int templateDepth = 0;
    for (int k = document->headStart(brace); k + 1 < brace; ++k) {
        const Token token = document->token(k);
        if (token == LANGLE) {
            ++templateDepth;
        } else if (token == RANGLE) {
            --templateDepth;
        } else if (token == GTGT) {
            templateDepth -= 2;
        } else if (templateDepth == 0 && (token == CLASS
                || token == STRUCT)
            && document->token(k + 1) == IDENTIFIER
        ) {
            int name = k + 1;
            while (document->token(name + 1) == IDENTIFIER) {
                ++name;
            }
            return document->lexem(name);
        }
    }
    return QByteArray();
}

/*!
    Builds the offset where the line of the token at @a index of @a document
    ends, before its carriage return when it has one.
*/
int contentEndOf(const FormatDocument *document, int index)
{
    const int line = document->symbolEndLine(index);
    int end = document->lineEnd(line);
    if (end > document->lineStart(line)
        && document->text().at(end - 1) == '\r'
    ) {
        --end;
    }
    return end;
}

/*!
    Adds `inline` in front of the declaration that starts at the token
    @a start of @a document, and moves its head's later lines right as far,
    up to the line that closes its parameters; hence the parameters stay
    lined up under the `(`.
*/
void addInlineKeyword(
        FormatDocument *document,
        int start)
{
    const QByteArray keyword("inline ");
    document->replace(document->symbols().at(start).from, 0, keyword);
    const int line = document->symbolLine(start);
    int open = start;
    while (open < document->symbols().size()
        && document->token(open) != LPAREN
    ) {
        ++open;
    }
    const int close = open < document->symbols().size()
        ? document->match(open) : -1;
    const int last = close >= 0 ? document->symbolLine(close) : line;
    const int indent = textWidth(document->indentOf(line));
    for (int l = line + 1; l <= last; ++l) {
        if (textWidth(document->indentOf(l)) > indent) {
            document->replace(document->lineStart(l), 0, QByteArray(
                keyword.size(), ' '
            ));
        }
    }
}

/*!
    Puts @a marker in place of the `override` or `Q_DECL_OVERRIDE` at
    @a overrideToken of @a document; an empty @a marker removes that marker
    together with the space before it.
*/
void setOverrideMarker(
        FormatDocument *document,
        int overrideToken, const QString & marker)
{
    const int end = document->symbolEnd(overrideToken);
    if (marker.isEmpty()) {
        const int from = document->symbolEnd(overrideToken - 1);
        document->replace(from, end - from, QByteArray());
        return;
    }
    const int from = document->symbols().at(overrideToken).from;
    document->replace(from, end - from, marker.toLatin1());
}

/*!
    Builds the index of the first token of the declaration whose body opens
    at @a body of @a document (after an access specifier and a template
    head, and the macros that lead it), and sets @a hasInline when it says
    `inline` or names a macro with `INLINE` in it (such as `Q_ALWAYS_INLINE`),
    @a exempt for a `friend` or `constexpr` one, which needs no keyword, and
    @a overrideToken to the index of an `override` or `Q_DECL_OVERRIDE`
    after its parameters.
*/
int functionDeclarationStart(
        const FormatDocument *document, int body,
        bool *hasInline, bool *exempt, int *overrideToken)
{
    int start = document->headStart(body);

    // Leaves a head that a directive cuts short: its start is not known.
    if (start > 0
        && document->isDirectiveLine(document->symbolLine(start - 1))
    ) {
        return -1;
    }
    // Steps over what leads the declaration, in any order, until nothing
    // more does: access specifiers, template heads, and macros.
    for (;;) {
        const int before = start;
        for (;;) {
            const Token token = document->token(start);
            const bool isAccess = token == PUBLIC || token == PRIVATE
                || token == PROTECTED;
            int label = start + (isAccess ? 1 : 0);
            const Token kind = document->token(label);
            if (kind == SLOTS
                || kind == SIGNALS
                || kind == Q_SLOTS_TOKEN
                || kind == Q_SIGNALS_TOKEN
            ) {
                ++label;
            }
            if (label > start && document->token(label) == COLON) {
                start = label + 1;
            } else if (token == TEMPLATE
                && document->token(start + 1) == LANGLE
            ) {
                int depth = 0;
                for (++start; start < body; ++start) {
                    const Token inner = document->token(start);
                    depth += inner == LANGLE
                        ? 1
                        : (inner == RANGLE ? -1 : (inner == GTGT ? -2 : 0));
                    if (depth <= 0) {
                        break;
                    }
                }
                ++start;
            } else {
                break;
            }
        }
        // Steps over the macros that lead the declaration (moc gives some of
        // them tokens of their own): one that expands to a template head, and a
        // statement-like macro call that ends its line (such as
        // `Q_DECLARE_FLAGS(...)`, which takes no `;`).
        while (start < body && isMacroName(document->lexem(start))
            && document->token(start + 1) != SCOPE
        ) {
            const QByteArray name = document->lexem(start);
            if (document->token(start + 1) == LPAREN) {
                const int close = document->match(start + 1);
                if (close < 0
                    || close + 1 >= body
                    || document->symbolLine(close + 1)
                        == document->symbolLine(close)
                ) {
                    break;
                }
                start = close + 1;
                continue;
            }
            if (name.contains("INLINE")) {
                *hasInline = true;
            }
            ++start;
        }
        if (start == before) {
            break;
        }
    }
    int parameters = -1;
    for (int i = start; i < body; ++i) {
        const Token token = document->token(i);
        if (token == LPAREN) {
            parameters = i;
            break;
        }
        const QByteArray name = token == IDENTIFIER
            ? document->lexem(i) : QByteArray();
        if (token == INLINE || name.contains("INLINE")) {
            *hasInline = true;
        }
        if (token == FRIEND || name == "constexpr" || name == "Q_DECL_CONSTEXPR"
            || name == "Q_DECL_RELAXED_CONSTEXPR"
        ) {
            *exempt = true;
        }
    }

    // Looks for `override` between the parameters and the body.
    const int close = parameters >= 0 ? document->match(parameters) : -1;
    for (int i = close + 1; close > 0 && i < body; ++i) {
        const QByteArray name = document->lexem(i);
        if (name == "override" || name == "Q_DECL_OVERRIDE") {
            *overrideToken = i;
        }
    }
    return start < body ? start : -1;
}

/*!
    Tells whether the lambda capture list that the `[` at \a bracket of
    \a document opens captures `this`: it names `this` or `*this`, or it
    starts with the default capture `=` or `&`.
*/
static bool capturesThis(const FormatDocument *document, int bracket)
{
    const int close = document->match(bracket);
    const Token first = document->token(bracket + 1);
    if (first == EQ
        || (first == AND
            && (bracket + 2 == close || document->token(bracket + 2) == COMMA))
    ) {
        return true;
    }
    for (int k = bracket + 1; k < close; ++k) {
        if (document->token(k) != THIS) {
            continue;
        }
        const int start = document->token(k - 1) == STAR ? k - 1 : k;
        const Token before = document->token(start - 1);
        if (before == LBRACK || before == COMMA) {
            return true;
        }
    }
    return false;
}


/*!
    Tells whether a token is inside a lambda that does not capture `this`.
*/
bool insideUncapturedLambda(const FormatDocument *document, int body, int index)
{
    for (int token = body + 1; token < index; ++token) {
        const int lambda = lambdaBody(document, token);
        if (lambda >= 0 && ! capturesThis(document, token)) {
            const int close = document->match(lambda);
            if (close >= index) {
                return true;
            }
            if (close > token) {
                token = close;
            }
        }
    }
    return false;
}

/*!
    Builds the names that the tokens from \a from up to \a to of \a document
    declare: a name right after a type, followed by what may end a
    declaration (`;`, `=`, `,`, `)`, `(`, `[`, or a range-for's `:`), and
    the names that declaration lists after commas.
*/
QSet<QByteArray > declaredLocalNames(
        const FormatDocument *document, int from,
        int to)
{
    QSet<QByteArray > names;
    for (int i = from; i < to; ++i) {
        if (document->token(i) != IDENTIFIER
            || ! endsType(document->token(i - 1))
        ) {
            continue;
        }
        const Token before = document->token(i - 2);
        if (document->token(i - 1) == IDENTIFIER
            && (before == DOT
                || before == ARROW)
        ) {
            continue;
        }
        const Token after = document->token(i + 1);
        if (after != SEMIC
            && after != EQ
            && after != COMMA
            && after != RPAREN
            && after != LPAREN
            && after != LBRACK
            && after != COLON
        ) {
            continue;
        }
        names.insert(document->lexem(i));

        // Takes the names the same declaration lists after commas, such as
        // `m` and `d` of `int y, m, d;`.
        for (int k = i + 1; k < to; ++k) {
            const Token token = document->token(k);
            if (token == LPAREN || token == LBRACK || token == LBRACE) {
                k = qMax(k, document->match(k));
                continue;
            }
            if (token == SEMIC || token == RPAREN || token == RBRACE) {
                break;
            }
            if (token != COMMA) {
                continue;
            }
            int name = k + 1;
            while (document->token(name) == STAR
                || document->token(name) == AND
            ) {
                ++name;
            }
            if (document->token(name) == IDENTIFIER) {
                names.insert(document->lexem(name));
            }
        }
    }
    return names;
}
