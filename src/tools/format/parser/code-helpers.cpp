// Copyright (C) 2015 The XD Company Ltd.
// SPDX-License-Identifier: Apache-2.0

#include "./code-helpers.h"

#include <QtCore/QByteArray>
#include <QtCore/QString>

/*!
    Tells whether \a token can end a type's name before a `&` or a `>`: a
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
    Tells whether \a token is a built-in type's keyword.
*/
bool isBuiltinType(Token token)
{
    return token == CHAR || token == WCHAR || token == BOOL || token == SHORT || token == INT
        || token == LONG || token == SIGNED || token == UNSIGNED || token == FLOAT
        || token == DOUBLE || token == VOID;
}

/*!
    Tells whether the gap between the tokens at \a before and \a after of
    \a document holds only spaces and tabs (no newline and no comment).
*/
bool isSameLineGap(const FormatDocument *document, int before, int after)
{
    const int from = document->symbolEnd(before);
    const int to = document->symbols().at(after).from;
    for (int i = from; i < to; ++i) {
        const char c = document->text().at(i);
        if (c != ' ' && c != '\t') {
            return false;
        }
    }
    return true;
}

/*!
    Builds the offset where the 1-based \a line of \a document ends, before
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
    token \a index of \a document, stepping over bracket groups, or -1 when a
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
    capture list the `[` at \a bracket of \a document opens, or -1 when that
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
    if (before != EQ && before != LPAREN && before != COMMA && before != RETURN
        && before != LBRACE && before != SEMIC
    ) {
        return -1;
    }
    int body = document->match(bracket) + 1;
    if (document->token(body) == LPAREN && document->match(body) >= 0) {
        body = document->match(body) + 1;
    }
    while (body < document->symbols().size() && document->token(body) != LBRACE
        && document->token(body) != SEMIC && document->token(body) != RPAREN
    ) {
        ++body;
    }
    if (document->token(body) != LBRACE || document->match(body) < 0) {
        return -1;
    }
    return body;
}

/*!
    Tells whether \a token is a binary operator that may lead a continuation
    line of an expression.
*/
bool isLeadingOperator(Token token)
{
    return token == ANDAND || token == OROR || token == LTLT || token == PLUS || token == MINUS
        || token == OR || token == HAT || token == QUESTION || token == STAR
        || token == SLASH || token == PERCENT || token == EQEQ || token == NE;
}

/*!
    Builds the width of \a text in characters.
*/
int textWidth(const QByteArray & text)
{
    return QString::fromUtf8(text).size();
}
