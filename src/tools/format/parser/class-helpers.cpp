// Copyright (C) 2015 The XD Company Ltd.
// SPDX-License-Identifier: Apache-2.0

#include "./class-helpers.h"

#include <QtCore/QByteArray>
#include <QtCore/QFile>
#include <QtCore/QList>

/*!
    Tells whether \a token can end the type in front of a declared name.
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
    Tells whether \a name is spelled the way a macro is: three or more
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
    whose `{` is at \a brace of \a document, leaving out the
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
    Reads, from the head of the class body whose `{` is at \a brace of
    \a document, whether it is a template and the names of its base classes
    into \a members: each base's last name, without its namespace or class
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
    Lists the members of the class body whose `{` is at \a brace of
    \a document, named \a className, with its bases and whether it is a
    template (see FormatClassMembers).
*/
FormatClassMembers collectMembers(const FormatDocument *document, int brace,
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
        if (token == SEMIC || token == RBRACE || (token == LBRACE && document->match(i) > i)
            || (token == COLON && (document->token(i - 1) == PUBLIC
                || document->token(i - 1) == PRIVATE
                || document->token(i - 1) == PROTECTED
                || document->token(i - 1) == SLOTS
                || document->token(i - 1) == SIGNALS))
        ) {
            isStatic = false;
            skip = false;
            continue;
        }
        if (token == STATIC) {
            isStatic = true;
        } else if (token == TYPEDEF || token == USING || token == FRIEND || token == ENUM
            || token == CLASS || token == STRUCT || token == UNION
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
        if ( ! declares || name == className || isMacroName(name)
            || ! endsType(document->token(i - 1)) || document->token(i - 1) == TILDE
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
    Tells whether the token at \a index of \a document sits in the arguments
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
    Finds the body of the class named \a className in \a document, and builds
    the index of its `{`, or -1 when the document defines no such class.
*/
int findClassBody(const FormatDocument *document,
                  const QByteArray & className)
{
    for (int i = 0; i < document->symbols().size(); ++i) {
        if (document->token(i) == LBRACE && document->braceScope(i) == FormatDocument::ClassScope
            && classNameOf(document, i) == className
        ) {
            return i;
        }
    }
    return -1;
}

/*!
    Builds the index of the `{` of the innermost class body of \a document
    that holds the token at \a index, or -1 when none does.
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
    Builds the name of the class whose body's `{` is at \a brace of
    \a document, after any export macro (`class Q_CORE_EXPORT QFile`); empty
    for an unnamed class.
*/
QByteArray classNameOf(const FormatDocument *document, int brace)
{
    for (int k = document->headStart(brace); k + 1 < brace; ++k) {
        const Token token = document->token(k);
        if ((token == CLASS
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
    Builds the offset where the line of the token at \a index of \a document
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
