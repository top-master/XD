// Copyright (C) 2015 The XD Company Ltd.
// SPDX-License-Identifier: Apache-2.0

#ifndef LEXER_FORMAT_TOKEN_H
#define LEXER_FORMAT_TOKEN_H

#include <QtCore/QByteArray>
#include <QtCore/QVector>

#define FORMAT_FOR_ALL_TOKENS(F) \
    F(NOTOKEN) \
    F(IDENTIFIER) \
    F(INTEGER_LITERAL) \
    F(CHARACTER_LITERAL) \
    F(STRING_LITERAL) \
    F(BOOLEAN_LITERAL) \
    F(HEADER_NAME) \
    F(LANGLE) \
    F(RANGLE) \
    F(LPAREN) \
    F(RPAREN) \
    F(ELIPSIS) \
    F(LBRACK) \
    F(RBRACK) \
    F(LBRACE) \
    F(RBRACE) \
    F(EQ) \
    F(SCOPE) \
    F(SEMIC) \
    F(COLON) \
    F(DOTSTAR) \
    F(QUESTION) \
    F(DOT) \
    F(DYNAMIC_CAST) \
    F(STATIC_CAST) \
    F(REINTERPRET_CAST) \
    F(CONST_CAST) \
    F(TYPEID) \
    F(THIS) \
    F(TEMPLATE) \
    F(THROW) \
    F(TRY) \
    F(CATCH) \
    F(TYPEDEF) \
    F(FRIEND) \
    F(CLASS) \
    F(NAMESPACE) \
    F(ENUM) \
    F(STRUCT) \
    F(UNION) \
    F(VIRTUAL) \
    F(PRIVATE) \
    F(PROTECTED) \
    F(PUBLIC) \
    F(EXPORT) \
    F(AUTO) \
    F(REGISTER) \
    F(EXTERN) \
    F(MUTABLE) \
    F(ASM) \
    F(USING) \
    F(INLINE) \
    F(EXPLICIT) \
    F(STATIC) \
    F(CONST) \
    F(VOLATILE) \
    F(OPERATOR) \
    F(SIZEOF) \
    F(NEW) \
    F(DELETE) \
    F(PLUS) \
    F(MINUS) \
    F(STAR) \
    F(SLASH) \
    F(PERCENT) \
    F(HAT) \
    F(AND) \
    F(OR) \
    F(TILDE) \
    F(NOT) \
    F(PLUS_EQ) \
    F(MINUS_EQ) \
    F(STAR_EQ) \
    F(SLASH_EQ) \
    F(PERCENT_EQ) \
    F(HAT_EQ) \
    F(AND_EQ) \
    F(OR_EQ) \
    F(LTLT) \
    F(GTGT) \
    F(GTGT_EQ) \
    F(LTLT_EQ) \
    F(EQEQ) \
    F(NE) \
    F(LE) \
    F(GE) \
    F(ANDAND) \
    F(OROR) \
    F(INCR) \
    F(DECR) \
    F(COMMA) \
    F(ARROW_STAR) \
    F(ARROW) \
    F(CHAR) \
    F(WCHAR) \
    F(BOOL) \
    F(SHORT) \
    F(INT) \
    F(LONG) \
    F(SIGNED) \
    F(UNSIGNED) \
    F(FLOAT) \
    F(DOUBLE) \
    F(VOID) \
    F(CASE) \
    F(DEFAULT) \
    F(IF) \
    F(ELSE) \
    F(SWITCH) \
    F(WHILE) \
    F(DO) \
    F(FOR) \
    F(BREAK) \
    F(CONTINUE) \
    F(GOTO) \
    F(SIGNALS) \
    F(SLOTS) \
    F(RETURN) \
    F(Q_OBJECT_TOKEN) \
    F(Q_REMOTE_TOKEN) \
    F(Q_REMOTE_INCLUDE_TOKEN) \
    F(Q_REMOTE_FORWARD_TOKEN) \
    F(Q_GADGET_TOKEN) \
    F(Q_PROPERTY_TOKEN) \
    F(Q_DEFAULT_TOKEN) \
    F(Q_PLUGIN_METADATA_TOKEN) \
    F(Q_ENUMS_TOKEN) \
    F(Q_ENUM_TOKEN) \
    F(Q_FLAGS_TOKEN) \
    F(Q_FLAG_TOKEN) \
    F(Q_DECLARE_FLAGS_TOKEN) \
    F(Q_DECLARE_INTERFACE_TOKEN) \
    F(Q_DECLARE_METATYPE_TOKEN) \
    F(Q_CLASSINFO_TOKEN) \
    F(Q_INTERFACES_TOKEN) \
    F(Q_SIGNALS_TOKEN) \
    F(Q_SLOTS_TOKEN) \
    F(Q_SIGNAL_TOKEN) \
    F(Q_SLOT_TOKEN) \
    F(Q_PRIVATE_SLOT_TOKEN) \
    F(Q_MOC_COMPAT_TOKEN) \
    F(Q_INVOKABLE_TOKEN) \
    F(Q_SCRIPTABLE_TOKEN) \
    F(Q_PRIVATE_PROPERTY_TOKEN) \
    F(Q_REVISION_TOKEN) \
    F(SPECIAL_TREATMENT_MARK) \
    F(CPP_COMMENT) \
    F(C_COMMENT) \
    F(FLOATING_LITERAL) \
    F(HASH) \
    F(QUOTE) \
    F(SINGLEQUOTE) \
    F(LANGLE_SCOPE) \
    F(DIGIT) \
    F(CHARACTER) \
    F(NEWLINE) \
    F(WHITESPACE) \
    F(BACKSLASH) \
    F(INCOMPLETE) \
    F(PP_DEFINE) \
    F(PP_UNDEF) \
    F(PP_IF) \
    F(PP_IFDEF) \
    F(PP_IFNDEF) \
    F(PP_ELIF) \
    F(PP_ELSE) \
    F(PP_ENDIF) \
    F(PP_INCLUDE) \
    F(PP_HASHHASH) \
    F(PP_HASH) \
    F(PP_DEFINED) \
    F(PP_INCOMPLETE) \
    F(AT)

#ifndef FOR_ALL_TOKENS
#define FOR_ALL_TOKENS(F) FORMAT_FOR_ALL_TOKENS(F)
#endif

enum Token {
#define FORMAT_CREATE_TOKEN(Name) Name,
    FORMAT_FOR_ALL_TOKENS(FORMAT_CREATE_TOKEN)
#undef FORMAT_CREATE_TOKEN

    PP_NOTOKEN = NOTOKEN,
    PP_IDENTIFIER = IDENTIFIER,
    PP_INTEGER_LITERAL = INTEGER_LITERAL,
    PP_FLOATING_LITERAL = FLOATING_LITERAL,
    PP_CHARACTER_LITERAL = CHARACTER_LITERAL,
    PP_STRING_LITERAL = STRING_LITERAL,
    PP_LANGLE = LANGLE,
    PP_RANGLE = RANGLE,
    PP_LPAREN = LPAREN,
    PP_RPAREN = RPAREN,
    PP_COMMA = COMMA,
    PP_PLUS = PLUS,
    PP_MINUS = MINUS,
    PP_STAR = STAR,
    PP_SLASH = SLASH,
    PP_PERCENT = PERCENT,
    PP_HAT = HAT,
    PP_AND = AND,
    PP_OR = OR,
    PP_TILDE = TILDE,
    PP_NOT = NOT,
    PP_LTLT = LTLT,
    PP_GTGT = GTGT,
    PP_EQEQ = EQEQ,
    PP_NE = NE,
    PP_LE = LE,
    PP_GE = GE,
    PP_ANDAND = ANDAND,
    PP_OROR = OROR,
    PP_QUESTION = QUESTION,
    PP_COLON = COLON,
    PP_QUOTE = QUOTE,
    PP_SINGLEQUOTE = SINGLEQUOTE,
    PP_DIGIT = DIGIT,
    PP_CHARACTER = CHARACTER,
    PP_WHITESPACE = WHITESPACE,
    PP_NEWLINE = NEWLINE,
    PP_CPP_COMMENT = CPP_COMMENT,
    PP_C_COMMENT = C_COMMENT,
    PP_BACKSLASH = BACKSLASH
};

struct FormatSymbol
{
    inline FormatSymbol()
        : lineNum(-1), token(NOTOKEN), from(0), len(-1)
    {
    }

    inline FormatSymbol(int line, Token tk)
        : lineNum(line), token(tk), from(0), len(-1)
    {
    }

    inline FormatSymbol(
            int line,
            Token tk,
            const QByteArray & input,
            int offset,
            int length)
        : lineNum(line), token(tk), source(input), from(offset), len(length)
    {
    }

    inline bool operator==(const FormatSymbol & other) const
    {
        return this->lexem() == other.lexem();
    }

    inline QByteArray lexem() const
    {
        if (this->len <= 0
            || this->from < 0
            || this->from >= this->source.size()
        ) {
            return QByteArray();
        }
        return this->source.mid(this->from, this->len);
    }

    inline QByteArray unquotedLexem() const
    {
        if (this->len >= 2
            && this->source.size() >= this->from + this->len
        ) {
            return this->source.mid(this->from + 1, this->len - 2);
        }
        return this->lexem();
    }

    int lineNum;
    Token token;
    QByteArray source;
    int from;
    int len;
};

Q_DECLARE_TYPEINFO(FormatSymbol, Q_MOVABLE_TYPE);

typedef FormatSymbol Symbol;
typedef QVector<FormatSymbol > Symbols;

#endif // LEXER_FORMAT_TOKEN_H
