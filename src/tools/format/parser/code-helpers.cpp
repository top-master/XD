// Copyright (C) 2015 The XD Company Ltd.
// SPDX-License-Identifier: Apache-2.0

#include "./code-helpers.h"

#include <QtCore/QByteArray>
#include <QtCore/QString>

/*!
    Tells whether @a token can end a type's name before a `&` or a `>`: a
    name, a built-in type, a qualifier, or a template's closing bracket.
*/
bool isTypeEnd(Token token)
{
    switch (token) {
    case IDENTIFIER:
    case CONST:
    case VOLATILE:
    case RANGLE:
    case GTGT:
    case STAR:
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
    Tells whether @a token is a built-in type's keyword.
*/
bool isBuiltinType(Token token)
{
    return token == CHAR
        || token == WCHAR
        || token == BOOL
        || token == SHORT
        || token == INT
        || token == LONG
        || token == SIGNED
        || token == UNSIGNED
        || token == FLOAT
        || token == DOUBLE
        || token == VOID;
}

/*!
    Tells whether the gap between the tokens at @a before and @a after of
    @a document holds only spaces, tabs, and unprotected inline comments.
*/
bool isSameLineGap(const FormatDocument *document, int before, int after)
{
    return document->isCodeWhitespace(
        document->symbolEnd(before),
        document->symbols().at(after).from,
        false
    );
}

/*!
    Builds the offset where the 1-based @a line of @a document ends, before
    its carriage return when it has one.
*/
int contentEnd(const FormatDocument *document, int line)
{
    int end = document->lineEnd(line);
    if (end > document->lineStart(line)
        && document->text().at(end - 1) == '\r'
    ) {
        --end;
    }
    return end;
}

/*!
    Builds the index of the semicolon that ends the statement starting at the
    token @a index of @a document, stepping over bracket groups, or -1 when a
    brace or the end of the code comes first.
*/
int statementEnd(const FormatDocument *document, int index)
{
    for (int i = index; i < document->symbols().size(); ++i) {
        const Token token = document->token(i);
        if (token == SEMIC) {
            return i;
        }
        if (token == LPAREN || token == LBRACK) {
            if (document->match(i) < 0) {
                return -1;
            }
            i = document->match(i);
            continue;
        }
        if (token == LBRACE
            || token == RBRACE
            || token == RPAREN
            || token == RBRACK
        ) {
            return -1;
        }
    }
    return -1;
}

/*!
    Builds the index of the `{` that opens the body of the lambda whose
    capture list the `[` at @a bracket of @a document opens, or -1 when that
    `[` opens no lambda: a lambda's `[` stands where an expression starts,
    and its parameters and specifiers, if any, come before its body.
*/
int lambdaBody(const FormatDocument *document, int bracket)
{
    if (document->token(bracket) != LBRACK
        || document->match(bracket) < 0
    ) {
        return -1;
    }
    const Token before = document->token(bracket - 1);
    if (before != EQ
        && before != LPAREN
        && before != COMMA
        && before != RETURN
        && before != LBRACE
        && before != SEMIC
    ) {
        return -1;
    }
    int body = document->match(bracket) + 1;
    if (document->token(body) == LPAREN && document->match(body) >= 0) {
        body = document->match(body) + 1;
    }
    while (body < document->symbols().size()
        && document->token(body) != LBRACE
        && document->token(body) != SEMIC
        && document->token(body) != RPAREN
    ) {
        ++body;
    }
    if (document->token(body) != LBRACE || document->match(body) < 0) {
        return -1;
    }
    return body;
}

/*!
    Tells whether @a token is a binary operator that may lead a continuation
    line of an expression.
*/
bool isLeadingOperator(Token token)
{
    return token == ANDAND
        || token == OROR
        || token == LTLT
        || token == PLUS
        || token == MINUS
        || token == OR
        || token == HAT
        || token == QUESTION
        || token == STAR
        || token == SLASH
        || token == PERCENT
        || token == EQEQ
        || token == NE;
}

/*!
    Tells whether @a token assigns a value, including a compound assignment.
*/
bool isAssignmentOperator(Token token)
{
    return token == EQ
        || token == PLUS_EQ
        || token == MINUS_EQ
        || token == STAR_EQ
        || token == SLASH_EQ
        || token == PERCENT_EQ
        || token == AND_EQ
        || token == OR_EQ
        || token == HAT_EQ
        || token == LTLT_EQ
        || token == GTGT_EQ;
}

/*!
    Tells whether @a token ends an operand, hence an operator right after it
    is a binary one (not a unary minus, say).
*/
bool isOperandEnd(Token token)
{
    return token == IDENTIFIER
        || token == INTEGER_LITERAL
        || token == FLOATING_LITERAL
        || token == CHARACTER_LITERAL
        || token == STRING_LITERAL
        || token == BOOLEAN_LITERAL
        || token == RPAREN
        || token == RBRACK
        || token == THIS;
}

/*!
    Builds how tightly the binary operator @a token binds its operands, as
    C++ ranks it: 1 for `||`, up to 10 for `*`, `/` and `%`; 0 for a token
    that is no operator a chain or a continuation line breaks at.
*/
int operatorPrecedence(Token token)
{
    switch (token) {
    case OROR:
        return 1;
    case ANDAND:
        return 2;
    case OR:
        return 3;
    case HAT:
        return 4;
    case AND:
        return 5;
    case EQEQ:
    case NE:
        return 6;
    case LTLT:
    case GTGT:
        return 8;
    case PLUS:
    case MINUS:
        return 9;
    case STAR:
    case SLASH:
    case PERCENT:
        return 10;
    default:
        return 0;
    }
}

/*!
    Builds the width of @a text in characters.
*/
int textWidth(const QByteArray & text)
{
    return QString::fromUtf8(text).size();
}

/*!
    Tells whether the `&` at @a ampersand of @a document declares a name: the
    type before it holds `const` or a template, or it starts a statement, or
    it is a parameter of a function's declaration or definition (a list whose
    name follows a type, or a constructor's own class name).
*/
bool isReferenceDeclaration(const FormatDocument *document, int ampersand)
{
    // Walks back over the type's tokens.
    int start = ampersand - 1;
    bool hasQualifier = false;
    while (start >= 0) {
        const Token token = document->token(start);
        if (token == CONST
            || token == VOLATILE
            || token == RANGLE
            || token == GTGT
            || isBuiltinType(token)
        ) {
            hasQualifier = true;
        }
        if (token == RANGLE || token == GTGT) {
            // Steps back to the template's `<`.
            int depth = 0;
            for (; start >= 0; --start) {
                const Token inner = document->token(start);
                depth += inner == RANGLE
                    ? 1 : (inner == GTGT ? 2 : (inner == LANGLE ? -1 : 0));
                if (depth <= 0) {
                    break;
                }
            }
            --start;
            continue;
        }
        if (token == IDENTIFIER
            || token == SCOPE
            || token == CONST
            || token == VOLATILE
            || token == STAR
            || isBuiltinType(token)
        ) {
            --start;
            continue;
        }
        break;
    }
    if (hasQualifier) {
        return true;
    }
    const Token before = start >= 0 ? document->token(start) : SEMIC;
    if (before == SEMIC || before == LBRACE || before == RBRACE) {
        return document->token(ampersand + 2) == EQ;
    }
    if (before != LPAREN && before != COMMA) {
        return false;
    }

    // Finds the parameter list's `(`, then what it follows.
    int paren = start;
    while (paren >= 0 && document->token(paren) != LPAREN) {
        const Token token = document->token(paren);
        if (token == RPAREN || token == RBRACK) {
            paren = document->match(paren);
            if (paren < 0) {
                return false;
            }
        } else if (token == SEMIC || token == LBRACE || token == RBRACE) {
            return false;
        }
        --paren;
    }
    if (paren < 1 || document->match(paren) < 0) {
        return false;
    }
    if (document->token(paren - 1) == FOR) {
        return true;
    }
    if (document->token(paren - 1) != IDENTIFIER) {
        return false;
    }
    int name = paren - 1;
    while (name >= 2
        && document->token(name - 1) == SCOPE
        && document->token(name - 2) == IDENTIFIER
    ) {
        name -= 2;
    }
    const Token typeEnd = document->token(name - 1);
    if (isTypeEnd(typeEnd) || typeEnd == AND || typeEnd == TILDE) {
        return true;
    }

    // Takes a constructor: its name repeats its class, or it sits in the class.
    if (name < paren - 1) {
        return document->lexem(paren - 1) == document->lexem(paren - 3);
    }
    return (typeEnd == SEMIC || typeEnd == LBRACE || typeEnd == RBRACE || typeEnd == COLON || typeEnd == EXPLICIT || typeEnd == INLINE)
        && document->scopeOf(paren) == FormatDocument::ClassScope;
}

/*!
    Tells whether the `static` at @a index of @a document declares a
    variable: its type is followed by a name and then by a `;`, `=`, `[`,
    `{` or `(` (which may start the variable's initializer).
*/
bool isStaticVariable(const FormatDocument *document, int index)
{
    int depth = 0;
    for (int k = index + 1; k + 1 < document->symbols().size(); ++k) {
        const Token token = document->token(k);
        if (token == LANGLE) {
            ++depth;
        } else if (token == RANGLE || token == GTGT) {
            depth -= token == GTGT ? 2 : 1;
        } else if (depth <= 0 && token == IDENTIFIER) {
            const Token next = document->token(k + 1);
            if (k > index + 1
                && (next == SEMIC
                    || next == EQ
                    || next == LBRACK
                    || next == LBRACE
                    || next == LPAREN)
            ) {
                return true;
            }
        } else if (depth <= 0
            && token != SCOPE
            && token != STAR
            && token != AND
            && token != CONST
            && token != VOLATILE
            && ! isBuiltinType(token)
        ) {
            return false;
        }
    }
    return false;
}

/*!
    Reports a dereferenced call result and a direct-initialized local in the
    statement that starts at the token @a start of @a document.
*/
void reportStatementShapes(FormatDocument *document, int start)
{
    const FormatDocument::Scope scope = document->scopeOf(start);
    if (scope != FormatDocument::FunctionScope
        && scope != FormatDocument::BlockScope
    ) {
        return;
    }

    // Finds `*<chain>(...) =`.
    if (document->token(start) == STAR) {
        int k = start + 1;
        while (document->token(k) == IDENTIFIER
            && (document->token(k + 1) == DOT
                || document->token(k + 1) == ARROW)
        ) {
            k += 2;
        }
        if (document->token(k) == IDENTIFIER
            && document->token(k + 1) == LPAREN
            && document->match(k + 1) > 0
        ) {
            const Token after = document->token(document->match(k + 1) + 1);
            if (after == EQ
                || after == PLUS_EQ
                || after == MINUS_EQ
                || after == OR_EQ
                || after == AND_EQ
            ) {
                document->report(
                    document->symbols().at(start).from,
                    "code-shape",
                    QString(QLL("store the returned pointer in a named local, "
                        "then dereference that"))
                );
            }
        }
        return;
    }

    // Finds `<Type> <name>(<call>);`.
    int k = start;
    if (document->token(k) == CONST) {
        ++k;
    }
    if (document->token(k) != IDENTIFIER) {
        return;
    }
    ++k;
    while (document->token(k) == SCOPE
        && document->token(k + 1) == IDENTIFIER
    ) {
        k += 2;
    }
    if (document->token(k) == LANGLE) {
        int depth = 0;
        for (; k < document->symbols().size(); ++k) {
            const Token token = document->token(k);
            depth += token == LANGLE
                ? 1 : (token == RANGLE ? -1 : (token == GTGT ? -2 : 0));
            if (depth <= 0 || token == SEMIC || token == LPAREN) {
                break;
            }
        }
        if (depth > 0) {
            return;
        }
        ++k;
    }
    if (document->token(k) != IDENTIFIER
        || document->token(k + 1) != LPAREN
    ) {
        return;
    }
    const int close = document->match(k + 1);
    if (close < 0 || document->token(close + 1) != SEMIC) {
        return;
    }
    // Takes one argument that is a member call, such as `server->url(x)`,
    // and leaves out the types that cannot be copied, and the ones built
    // from a path or a pattern.
    static const char *const uncopyable[] =
        {
            "QFile",
            "QSaveFile",
            "QScopedPointer",
            "QMutexLocker",
            "QReadLocker",
            "QWriteLocker",
            "QDirIterator",
            "QTextStream",
            "QDataStream",
            "QProcess",
            "QSettings",
            "QEventLoop",
            "QFileInfo",
            "QDir",
            "QRegularExpression",
            "QRegExp"
        };
    const QByteArray type = document->lexem(document->token(start) == CONST
        ? start + 1 : start);
    for (size_t u = 0; u < sizeof(uncopyable) / sizeof(uncopyable[0]); ++u) {
        if (type == uncopyable[u]) {
            return;
        }
    }
    const int last = close - 1;
    if (document->token(last) != RPAREN || document->match(last) < k + 2) {
        return;
    }
    bool isMemberCall = false;
    for (int i = k + 2; i < document->match(last); ++i) {
        const Token token = document->token(i);
        if (token == COMMA) {
            return;
        }
        if (token == LPAREN) {
            i = qMax(i, document->match(i));
            continue;
        }
        isMemberCall = isMemberCall || token == DOT || token == ARROW;
    }
    if (isMemberCall
        && document->token(document->match(last) - 1) == IDENTIFIER
    ) {
        document->report(document->symbols().at(start).from, "code-shape", QString(QLL("when the call returns the local's type, copy-initialize "
                                     "it: <Type> %1 = qMove(...);")).arg(QString::fromLatin1(document->lexem(k))));
    }
}

/*!
    Reports an inline body of more than 5 lines: a function body in a class
    body, or one whose head says `inline` or `Q_ALWAYS_INLINE`; @a brace is
    the body's `{` in @a document.
*/
void reportLongInlineBody(FormatDocument *document, int brace)
{
    bool isInline = document->scopeOf(brace) == FormatDocument::ClassScope;
    for (int i = document->headStart(brace); i < brace && ! isInline; ++i) {
        isInline = document->token(i) == INLINE
            || (document->token(i) == IDENTIFIER
                && document->lexem(i) == "Q_ALWAYS_INLINE");
    }
    const int close = document->match(brace);
    if ( ! isInline || close < 0) {
        return;
    }
    int lines = 0;
    for (int line = document->symbolLine(brace) + 1; line < document->symbolLine(close); ++line) {
        if (document->firstSymbolOfLine(line) >= 0) {
            ++lines;
        }
    }
    if (lines > 5) {
        document->report(document->symbols().at(brace).from, "code-shape", QString(QLL("define the function in its source file: the inline "
                                     "body has %1 lines (at most 5)")).arg(lines));
    }
}
