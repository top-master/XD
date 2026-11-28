// Copyright (C) 2015 The XD Company Ltd.
// SPDX-License-Identifier: Apache-2.0

#include "./format-lexer.h"

namespace {

inline bool isAsciiSpace(char c)
{
    return c == ' ' || c == '\t';
}

inline bool isIdentStart(char c)
{
    return (c >= 'a' && c <= 'z')
        || (c >= 'A' && c <= 'Z')
        || c == '_'
        || c == '$'
        || static_cast<unsigned char >(c) >= 0x80;
}

inline bool isIdentChar(char c)
{
    return isIdentStart(c) || (c >= '0' && c <= '9');
}

inline bool isDigitChar(char c)
{
    return c >= '0' && c <= '9';
}

inline bool isHexChar(char c)
{
    return (c >= '0' && c <= '9')
        || (c >= 'a' && c <= 'f')
        || (c >= 'A' && c <= 'F');
}

inline bool matchKeywordLiteral(
        const char *text,
        int len,
        const char *literal,
        int literalLen)
{
    return len == literalLen && qstrncmp(text, literal, uint(len)) == 0;
}

#define FORMAT_KW(lit, tok) \
    if (matchKeywordLiteral(text, len, lit, int(sizeof(lit) - 1))) return tok;

Token lookupCppKeyword(const char *text, int len)
{
    if (len <= 0) {
        return IDENTIFIER;
    }
    switch (text[0]) {
    case 'Q':
        FORMAT_KW("Q_OBJECT", Q_OBJECT_TOKEN)
        FORMAT_KW("Q_REMOTE", Q_REMOTE_TOKEN)
        FORMAT_KW("Q_GADGET", Q_GADGET_TOKEN)
        FORMAT_KW("Q_DEFAULT", Q_DEFAULT_TOKEN)
        FORMAT_KW("Q_REMOTE_INCLUDE", Q_REMOTE_INCLUDE_TOKEN)
        FORMAT_KW("Q_REMOTE_FORWARD", Q_REMOTE_FORWARD_TOKEN)
        FORMAT_KW("Q_PROPERTY", Q_PROPERTY_TOKEN)
        FORMAT_KW("Q_PLUGIN_METADATA", Q_PLUGIN_METADATA_TOKEN)
        FORMAT_KW("Q_ENUMS", Q_ENUMS_TOKEN)
        FORMAT_KW("Q_ENUM", Q_ENUM_TOKEN)
        FORMAT_KW("Q_FLAGS", Q_FLAGS_TOKEN)
        FORMAT_KW("Q_FLAG", Q_FLAG_TOKEN)
        FORMAT_KW("Q_DECLARE_FLAGS", Q_DECLARE_FLAGS_TOKEN)
        FORMAT_KW("Q_DECLARE_INTERFACE", Q_DECLARE_INTERFACE_TOKEN)
        FORMAT_KW("Q_DECLARE_METATYPE", Q_DECLARE_METATYPE_TOKEN)
        FORMAT_KW("Q_DECLARE_EXTENSION_INTERFACE", Q_DECLARE_INTERFACE_TOKEN)
        FORMAT_KW("Q_SETS", Q_FLAGS_TOKEN)
        FORMAT_KW("Q_CLASSINFO", Q_CLASSINFO_TOKEN)
        FORMAT_KW("Q_INTERFACES", Q_INTERFACES_TOKEN)
        FORMAT_KW("Q_SIGNALS", Q_SIGNALS_TOKEN)
        FORMAT_KW("Q_SLOTS", Q_SLOTS_TOKEN)
        FORMAT_KW("Q_PRIVATE_SLOT", Q_PRIVATE_SLOT_TOKEN)
        FORMAT_KW("QT_MOC_COMPAT", Q_MOC_COMPAT_TOKEN)
        FORMAT_KW("Q_INVOKABLE", Q_INVOKABLE_TOKEN)
        FORMAT_KW("Q_SIGNAL", Q_SIGNAL_TOKEN)
        FORMAT_KW("Q_SLOT", Q_SLOT_TOKEN)
        FORMAT_KW("Q_SCRIPTABLE", Q_SCRIPTABLE_TOKEN)
        FORMAT_KW("Q_PRIVATE_PROPERTY", Q_PRIVATE_PROPERTY_TOKEN)
        FORMAT_KW("Q_REVISION", Q_REVISION_TOKEN)
        break;
    case 'a':
        FORMAT_KW("auto", AUTO)
        FORMAT_KW("asm", ASM)
        break;
    case 'b':
        FORMAT_KW("bool", BOOL)
        FORMAT_KW("break", BREAK)
        FORMAT_KW("bitand", AND)
        FORMAT_KW("bitor", OR)
        break;
    case 'c':
        FORMAT_KW("const", CONST)
        FORMAT_KW("class", CLASS)
        FORMAT_KW("case", CASE)
        FORMAT_KW("catch", CATCH)
        FORMAT_KW("char", CHAR)
        FORMAT_KW("compl", TILDE)
        FORMAT_KW("const_cast", CONST_CAST)
        FORMAT_KW("continue", CONTINUE)
        break;
    case 'd':
        FORMAT_KW("double", DOUBLE)
        FORMAT_KW("default", DEFAULT)
        FORMAT_KW("delete", DELETE)
        FORMAT_KW("do", DO)
        FORMAT_KW("dynamic_cast", DYNAMIC_CAST)
        break;
    case 'e':
        FORMAT_KW("else", ELSE)
        FORMAT_KW("enum", ENUM)
        FORMAT_KW("explicit", EXPLICIT)
        FORMAT_KW("export", EXPORT)
        FORMAT_KW("extern", EXTERN)
        break;
    case 'f':
        FORMAT_KW("float", FLOAT)
        FORMAT_KW("for", FOR)
        FORMAT_KW("friend", FRIEND)
        break;
    case 'g':
        FORMAT_KW("goto", GOTO)
        break;
    case 'i':
        FORMAT_KW("if", IF)
        FORMAT_KW("inline", INLINE)
        FORMAT_KW("int", INT)
        break;
    case 'l':
        FORMAT_KW("long", LONG)
        break;
    case 'm':
        FORMAT_KW("mutable", MUTABLE)
        break;
    case 'n':
        FORMAT_KW("namespace", NAMESPACE)
        FORMAT_KW("new", NEW)
        FORMAT_KW("not", NOT)
        FORMAT_KW("not_eq", NE)
        break;
    case 'o':
        FORMAT_KW("operator", OPERATOR)
        break;
    case 'p':
        FORMAT_KW("private", PRIVATE)
        FORMAT_KW("protected", PROTECTED)
        FORMAT_KW("public", PUBLIC)
        break;
    case 'r':
        FORMAT_KW("register", REGISTER)
        FORMAT_KW("reinterpret_cast", REINTERPRET_CAST)
        FORMAT_KW("return", RETURN)
        break;
    case 's':
        FORMAT_KW("short", SHORT)
        FORMAT_KW("signals", SIGNALS)
        FORMAT_KW("signed", SIGNED)
        FORMAT_KW("sizeof", SIZEOF)
        FORMAT_KW("slots", SLOTS)
        FORMAT_KW("static", STATIC)
        FORMAT_KW("static_cast", STATIC_CAST)
        FORMAT_KW("struct", STRUCT)
        FORMAT_KW("switch", SWITCH)
        break;
    case 't':
        FORMAT_KW("template", TEMPLATE)
        FORMAT_KW("this", THIS)
        FORMAT_KW("throw", THROW)
        FORMAT_KW("try", TRY)
        FORMAT_KW("typedef", TYPEDEF)
        FORMAT_KW("typeid", TYPEID)
        break;
    case 'u':
        FORMAT_KW("union", UNION)
        FORMAT_KW("unsigned", UNSIGNED)
        FORMAT_KW("using", USING)
        break;
    case 'v':
        FORMAT_KW("virtual", VIRTUAL)
        FORMAT_KW("void", VOID)
        FORMAT_KW("volatile", VOLATILE)
        break;
    case 'w':
        FORMAT_KW("wchar", WCHAR)
        FORMAT_KW("while", WHILE)
        break;
    default:
        break;
    }
    return IDENTIFIER;
}

Token lookupDirectiveKeyword(const char *text, int len)
{
    FORMAT_KW("define", PP_DEFINE)
    FORMAT_KW("elif", PP_ELIF)
    FORMAT_KW("else", PP_ELSE)
    FORMAT_KW("endif", PP_ENDIF)
    FORMAT_KW("if", PP_IF)
    FORMAT_KW("ifdef", PP_IFDEF)
    FORMAT_KW("ifndef", PP_IFNDEF)
    FORMAT_KW("import", PP_INCLUDE)
    FORMAT_KW("include", PP_INCLUDE)
    FORMAT_KW("undef", PP_UNDEF)
    return NOTOKEN;
}

#undef FORMAT_KW

const char *scanLineEnding(const char *cur, const char *end)
{
    if (cur < end && *cur == '\r') {
        ++cur;
        if (cur < end && *cur == '\n') {
            ++cur;
        }
        return cur;
    }
    if (cur < end && *cur == '\n') {
        return cur + 1;
    }
    return cur;
}

const char *scanQuotedString(const char *cur, const char *end, char quote)
{
    if (cur < end && *cur == quote) {
        ++cur;
    }
    while (cur < end) {
        const char ch = *cur;
        if (ch == quote) {
            return cur + 1;
        }
        if (ch == '\\') {
            ++cur;
            if (cur >= end) {
                break;
            }
            if (*cur == '\r' && cur + 1 < end && cur[1] == '\n') {
                cur += 2;
                continue;
            }
            ++cur;
            continue;
        }
        ++cur;
    }
    return cur;
}

int countNewlines(const char *from, const char *to)
{
    int count = 0;
    for (const char *p = from; p < to; ++p) {
        if (*p == '\r') {
            ++count;
            if (p + 1 < to && p[1] == '\n') {
                ++p;
            }
        } else if (*p == '\n') {
            ++count;
        }
    }
    return count;
}

const char *tryScanRawString(
        const QByteArray & input, int startOffset, int *linesInside)
{
    const int size = input.size();
    if (startOffset + 2 >= size) {
        return Q_NULLPTR;
    }
    const char *data = input.constData();
    if (data[startOffset] != 'R' || data[startOffset + 1] != '"') {
        return Q_NULLPTR;
    }
    const int delimStart = startOffset + 2;
    int openParen = delimStart;
    while (openParen < size
        && openParen - delimStart <= 16
        && data[openParen] != '('
    ) {
        const char c = data[openParen];
        if (c <= ' ' || c > '~' || c == ')' || c == '\\') {
            return Q_NULLPTR;
        }
        ++openParen;
    }
    if (openParen >= size
        || openParen - delimStart > 16
        || data[openParen] != '('
    ) {
        return Q_NULLPTR;
    }
    const QByteArray delimiter = input.mid(
        delimStart,
        openParen - delimStart
    );
    const QByteArray closing = QByteArray(")") + delimiter + '"';
    const int closePos = input.indexOf(closing, openParen + 1);
    if (closePos < 0) {
        return Q_NULLPTR;
    }
    const char *tokenStart = data + startOffset;
    const char *tokenEnd = data + closePos + closing.size();
    *linesInside = countNewlines(tokenStart, tokenEnd);
    return tokenEnd;
}

const char *tryScanTripleString(
        const char *cur,
        const char *end,
        char quote,
        int *linesInside)
{
    if (cur + 2 >= end
        || cur[0] != quote
        || cur[1] != quote
        || cur[2] != quote
    ) {
        return Q_NULLPTR;
    }
    const char *p = cur + 3;
    while (p < end) {
        if (*p == '\\') {
            if (p + 1 < end
                && p[1] == '\r'
                && p + 2 < end
                && p[2] == '\n'
            ) {
                ++(*linesInside);
                p += 3;
            } else if (p + 1 < end
                && (*p == '\n'
                    || p[1] == '\r'
                    || p[1] == '\n')
            ) {
                if (p[1] == '\n' || p[1] == '\r') {
                    ++(*linesInside);
                }
                p += 2;
            } else {
                p += (p + 1 < end) ? 2 : 1;
            }
            continue;
        }
        if (*p == '\r') {
            ++(*linesInside);
            p += (p + 1 < end && p[1] == '\n') ? 2 : 1;
            continue;
        }
        if (*p == '\n') {
            ++(*linesInside);
            ++p;
            continue;
        }
        if (p + 2 < end
            && p[0] == quote
            && p[1] == quote
            && p[2] == quote
        ) {
            return p + 3;
        }
        ++p;
    }
    return end;
}

Token scanCStyleOperator(const char * & cur,
                         const char *end,
                         bool splitAngleScope)
{
    const char c0 = cur[0];
    const char c1 = (cur + 1 < end) ? cur[1] : '\0';
    const char c2 = (cur + 2 < end) ? cur[2] : '\0';

    if (splitAngleScope && c0 == '<' && c1 == ':' && c2 == ':') {
        cur += 1;
        return LANGLE;
    }
    if (c0 == '.' && c1 == '.' && c2 == '.') {
        cur += 3;
        return ELIPSIS;
    }
    if (c0 == '-' && c1 == '>' && c2 == '*') {
        cur += 3;
        return ARROW_STAR;
    }
    if (c0 == '<' && c1 == '<' && c2 == '=') {
        cur += 3;
        return LTLT_EQ;
    }
    if (c0 == '>' && c1 == '>' && c2 == '=') {
        cur += 3;
        return GTGT_EQ;
    }
    if (c0 == ':' && c1 == ':') {
        cur += 2;
        return SCOPE;
    }
    if (c0 == '-' && c1 == '>') {
        cur += 2;
        return ARROW;
    }
    if (c0 == '.' && c1 == '*') {
        cur += 2;
        return DOTSTAR;
    }
    if (c0 == '+' && c1 == '+') {
        cur += 2;
        return INCR;
    }
    if (c0 == '-' && c1 == '-') {
        cur += 2;
        return DECR;
    }
    if (c0 == '<' && c1 == '<') {
        cur += 2;
        return LTLT;
    }
    if (c0 == '>' && c1 == '>') {
        cur += 2;
        return GTGT;
    }
    if (c0 == '=' && c1 == '=') {
        cur += 2;
        return EQEQ;
    }
    if (c0 == '!' && c1 == '=') {
        cur += 2;
        return NE;
    }
    if (c0 == '<' && c1 == '=') {
        cur += 2;
        return LE;
    }
    if (c0 == '>' && c1 == '=') {
        cur += 2;
        return GE;
    }
    if (c0 == '&' && c1 == '&') {
        cur += 2;
        return ANDAND;
    }
    if (c0 == '|' && c1 == '|') {
        cur += 2;
        return OROR;
    }
    if (c0 == '+' && c1 == '=') {
        cur += 2;
        return PLUS_EQ;
    }
    if (c0 == '-' && c1 == '=') {
        cur += 2;
        return MINUS_EQ;
    }
    if (c0 == '*' && c1 == '=') {
        cur += 2;
        return STAR_EQ;
    }
    if (c0 == '/' && c1 == '=') {
        cur += 2;
        return SLASH_EQ;
    }
    if (c0 == '%' && c1 == '=') {
        cur += 2;
        return PERCENT_EQ;
    }
    if (c0 == '^' && c1 == '=') {
        cur += 2;
        return HAT_EQ;
    }
    if (c0 == '&' && c1 == '=') {
        cur += 2;
        return AND_EQ;
    }
    if (c0 == '|' && c1 == '=') {
        cur += 2;
        return OR_EQ;
    }
    if (c0 == '#' && c1 == '#') {
        cur += 2;
        return PP_HASHHASH;
    }
    if (c0 == '<' && c1 == ':') {
        cur += 2;
        return LBRACK;
    }
    if (c0 == ':' && c1 == '>') {
        cur += 2;
        return RBRACK;
    }
    if (c0 == '<' && c1 == '%') {
        cur += 2;
        return LBRACE;
    }
    if (c0 == '%' && c1 == '>') {
        cur += 2;
        return RBRACE;
    }

    ++cur;
    switch (c0) {
    case '<': return LANGLE;
    case '>': return RANGLE;
    case '(': return LPAREN;
    case ')': return RPAREN;
    case '[': return LBRACK;
    case ']': return RBRACK;
    case '{': return LBRACE;
    case '}': return RBRACE;
    case '=': return EQ;
    case ';': return SEMIC;
    case ':': return COLON;
    case '?': return QUESTION;
    case '.': return DOT;
    case '+': return PLUS;
    case '-': return MINUS;
    case '*': return STAR;
    case '/': return SLASH;
    case '%': return PERCENT;
    case '^': return HAT;
    case '&': return AND;
    case '|': return OR;
    case '~': return TILDE;
    case '!': return NOT;
    case ',': return COMMA;
    case '#': return HASH;
    case '\\': return BACKSLASH;
    case '@': return AT;
    default: return NOTOKEN;
    }
}

Token scanNumberLiteral(const char * & cur, const char *end)
{
    if (*cur == '.' && cur + 1 < end && isDigitChar(cur[1])) {
        ++cur;
        while (cur < end && (isDigitChar(*cur) || *cur == '\'')) {
            ++cur;
        }
        if (cur < end && (*cur == 'e' || *cur == 'E')) {
            ++cur;
            if (cur < end && (*cur == '+' || *cur == '-')) {
                ++cur;
            }
            while (cur < end && (isDigitChar(*cur) || *cur == '\'')) {
                ++cur;
            }
        }
        while (cur < end && isIdentChar(*cur)) {
            ++cur;
        }
        return FLOATING_LITERAL;
    }

    if (*cur == '0' && cur + 1 < end) {
        if (cur[1] == 'x' || cur[1] == 'X') {
            cur += 2;
            while (cur < end && (isHexChar(*cur) || *cur == '\'')) {
                ++cur;
            }
            while (cur < end
                && (*cur == 'u'
                    || *cur == 'U'
                    || *cur == 'l'
                    || *cur == 'L')
            ) {
                ++cur;
            }
            return INTEGER_LITERAL;
        }
        if (cur[1] == 'b' || cur[1] == 'B') {
            cur += 2;
            while (cur < end
                && (*cur == '0'
                    || *cur == '1'
                    || *cur == '\'')
            ) {
                ++cur;
            }
            while (cur < end
                && (*cur == 'u'
                    || *cur == 'U'
                    || *cur == 'l'
                    || *cur == 'L')
            ) {
                ++cur;
            }
            return INTEGER_LITERAL;
        }
    }

    while (cur < end && (isDigitChar(*cur) || *cur == '\'')) {
        ++cur;
    }
    bool isFloat = false;
    if (cur < end && *cur == '.') {
        isFloat = true;
        ++cur;
        while (cur < end && (isDigitChar(*cur) || *cur == '\'')) {
            ++cur;
        }
    }
    if (cur < end && (*cur == 'e' || *cur == 'E')) {
        isFloat = true;
        ++cur;
        if (cur < end && (*cur == '+' || *cur == '-')) {
            ++cur;
        }
        while (cur < end && (isDigitChar(*cur) || *cur == '\'')) {
            ++cur;
        }
    }
    if (isFloat) {
        if (cur < end
            && (*cur == 'f'
                || *cur == 'F'
                || *cur == 'l'
                || *cur == 'L')
        ) {
            ++cur;
        }
        return FLOATING_LITERAL;
    }
    while (cur < end
        && (*cur == 'u'
            || *cur == 'U'
            || *cur == 'l'
            || *cur == 'L')
    ) {
        ++cur;
    }
    return INTEGER_LITERAL;
}

} // namespace

/*!
    Tokenizes @a input according to @a options without shared global state.
*/
Symbols FormatLexer::tokenize(const QByteArray & input, const Options & options)
{
    Symbols symbols;
    if (input.isEmpty()) {
        return symbols;
    }
    symbols.reserve(qMax(16, input.size() / 6));

    const FormatCommentStyle style =
        FormatLanguageInfo::commentStyle(options.language);
    const bool isCFamily = FormatLanguageInfo::supportsCodeFormatting(
        options.language
    );

    const char *begin = input.constData();
    const char *end = begin + input.size();
    const char *cur = begin;
    int lineNum = 1;
    bool atLineStart = true;
    bool inDirective = false;
    bool expectIncludeHeader = false;
    bool lineHasDirectiveToken = false;

    while (cur < end) {
        const char ch = *cur;

        // Horizontal whitespace.
        if (isAsciiSpace(ch)) {
            const char *start = cur;
            while (cur < end && isAsciiSpace(*cur)) {
                ++cur;
            }
            if (options.keepWhitespace) {
                symbols.append(
                    FormatSymbol(
                        lineNum,
                        WHITESPACE,
                        input,
                        int(start - begin),
                        int(cur - start)
                    )
                );
            }
            continue;
        }

        // Line endings.
        if (ch == '\r' || ch == '\n') {
            const char *start = cur;
            cur = scanLineEnding(cur, end);
            if (options.keepNewlines) {
                symbols.append(
                    FormatSymbol(
                        lineNum,
                        inDirective ? PP_NEWLINE : NEWLINE,
                        input,
                        int(start - begin),
                        int(cur - start)
                    )
                );
            }
            ++lineNum;
            atLineStart = true;
            inDirective = false;
            expectIncludeHeader = false;
            lineHasDirectiveToken = false;
            continue;
        }

        // Backslash-newline line continuation inside C-family code/directives.
        if (isCFamily && ch == '\\') {
            const char *peek = cur + 1;
            while (peek < end && isAsciiSpace(*peek)) {
                ++peek;
            }
            if (peek < end && (*peek == '\r' || *peek == '\n')) {
                cur = scanLineEnding(peek, end);
                ++lineNum;
                continue;
            }
        }

        // Comments.
        if (style.supportsSlashLine
            && ch == '/'
            && cur + 1 < end
            && cur[1] == '/'
        ) {
            const char *start = cur;
            cur += 2;
            while (cur < end && *cur != '\r' && *cur != '\n') {
                ++cur;
            }
            if (options.keepComments) {
                symbols.append(
                    FormatSymbol(
                        lineNum,
                        CPP_COMMENT,
                        input,
                        int(start - begin),
                        int(cur - start)
                    )
                );
            }
            atLineStart = false;
            continue;
        }
        if (style.supportsSlashBlock
            && ch == '/'
            && cur + 1 < end
            && cur[1] == '*'
        ) {
            const char *start = cur;
            const int startLine = lineNum;
            cur += 2;
            while (cur < end) {
                if (*cur == '*' && cur + 1 < end && cur[1] == '/') {
                    cur += 2;
                    break;
                }
                if (*cur == '\r') {
                    ++lineNum;
                    cur += (cur + 1 < end && cur[1] == '\n') ? 2 : 1;
                } else if (*cur == '\n') {
                    ++lineNum;
                    ++cur;
                } else {
                    ++cur;
                }
            }
            if (options.keepComments) {
                symbols.append(
                    FormatSymbol(
                        startLine,
                        C_COMMENT,
                        input,
                        int(start - begin),
                        int(cur - start)
                    )
                );
            }
            continue;
        }
        if (style.supportsHashLine && ch == '#') {
            const char *start = cur;
            ++cur;
            while (cur < end && *cur != '\r' && *cur != '\n') {
                ++cur;
            }
            if (options.keepComments) {
                symbols.append(
                    FormatSymbol(
                        lineNum,
                        CPP_COMMENT,
                        input,
                        int(start - begin),
                        int(cur - start)
                    )
                );
            }
            atLineStart = false;
            continue;
        }

        // C-family preprocessor directives at line start.
        if (isCFamily && atLineStart && ch == '#') {
            atLineStart = false;
            inDirective = true;
            lineHasDirectiveToken = true;
            const char *start = cur;
            const char *afterHash = cur + 1;
            while (afterHash < end && isAsciiSpace(*afterHash)) {
                ++afterHash;
            }
            if (afterHash < end && isIdentStart(*afterHash)) {
                const char *identStart = afterHash;
                const char *identEnd = identStart + 1;
                while (identEnd < end && isIdentChar(*identEnd)) {
                    ++identEnd;
                }
                const Token directiveTok = lookupDirectiveKeyword(
                    identStart,
                    int(identEnd - identStart)
                );
                if (directiveTok != NOTOKEN) {
                    cur = identEnd;
                    if (directiveTok == PP_INCLUDE) {
                        expectIncludeHeader = true;
                    }
                    symbols.append(
                        FormatSymbol(
                            lineNum,
                            directiveTok,
                            input,
                            int(start - begin),
                            int(cur - start)
                        )
                    );
                    continue;
                }
            }
            ++cur;
            symbols.append(
                FormatSymbol(lineNum, PP_HASH, input, int(start - begin), 1)
            );
            continue;
        }

        // Angle-bracket header name after `#include` / `#import`.
        if (expectIncludeHeader && ch == '<') {
            const char *start = cur;
            ++cur;
            while (cur < end
                && *cur != '>'
                && *cur != '\r'
                && *cur != '\n'
            ) {
                ++cur;
            }
            if (cur < end && *cur == '>') {
                ++cur;
            }
            expectIncludeHeader = false;
            symbols.append(
                FormatSymbol(
                    lineNum,
                    STRING_LITERAL,
                    input,
                    int(start - begin),
                    int(cur - start)
                )
            );
            continue;
        }
        expectIncludeHeader = false;

        // Python / triple-quoted strings.
        if (style.supportsTripleDocstring && (ch == '"' || ch == '\'')) {
            int linesInside = 0;
            const int startLine = lineNum;
            const char *after = tryScanTripleString(
                cur,
                end,
                ch,
                &linesInside
            );
            if (after) {
                symbols.append(
                    FormatSymbol(
                        startLine,
                        STRING_LITERAL,
                        input,
                        int(cur - begin),
                        int(after - cur)
                    )
                );
                lineNum += linesInside;
                cur = after;
                atLineStart = false;
                continue;
            }
        }

        // C++11 raw string literals.
        if (isCFamily && ch == 'R') {
            int linesInside = 0;
            const int startLine = lineNum;
            const char *after = tryScanRawString(
                input,
                int(cur - begin),
                &linesInside
            );
            if (after) {
                symbols.append(
                    FormatSymbol(
                        startLine,
                        STRING_LITERAL,
                        input,
                        int(cur - begin),
                        int(after - cur)
                    )
                );
                lineNum += linesInside;
                cur = after;
                atLineStart = false;
                continue;
            }
        }

        // Regular double- and single-quoted literals.
        if (ch == '"') {
            const char *start = cur;
            const int startLine = lineNum;
            cur = scanQuotedString(cur, end, '"');
            lineNum += countNewlines(start, cur);
            symbols.append(
                FormatSymbol(
                    startLine,
                    STRING_LITERAL,
                    input,
                    int(start - begin),
                    int(cur - start)
                )
            );
            atLineStart = false;
            continue;
        }
        if (ch == '\'') {
            const char *start = cur;
            const int startLine = lineNum;
            cur = scanQuotedString(cur, end, '\'');
            lineNum += countNewlines(start, cur);
            symbols.append(
                FormatSymbol(
                    startLine,
                    CHARACTER_LITERAL,
                    input,
                    int(start - begin),
                    int(cur - start)
                )
            );
            atLineStart = false;
            continue;
        }

        atLineStart = false;

        // Numeric literals (and `.5`-style floats when `.` precedes a digit).
        if (isDigitChar(ch)
            || (ch == '.'
                && cur + 1 < end
                && isDigitChar(cur[1]))
        ) {
            const char *start = cur;
            const Token tok = scanNumberLiteral(cur, end);
            symbols.append(
                FormatSymbol(
                    lineNum,
                    tok,
                    input,
                    int(start - begin),
                    int(cur - start)
                )
            );
            continue;
        }

        // Identifiers and C++/Qt keywords.
        if (isIdentStart(ch)) {
            const char *start = cur;
            ++cur;
            while (cur < end && isIdentChar(*cur)) {
                ++cur;
            }
            const int len = int(cur - start);
            Token tok = IDENTIFIER;
            if (inDirective) {
                if (len == 7 && qstrncmp(start, "defined", 7) == 0) {
                    tok = PP_DEFINED;
                } else if (len == 3 && qstrncmp(start, "and", 3) == 0) {
                    tok = AND;
                } else if (len == 2 && qstrncmp(start, "or", 2) == 0) {
                    tok = OR;
                } else {
                    tok = lookupCppKeyword(start, len);
                }
            } else if (isCFamily) {
                tok = lookupCppKeyword(start, len);
            }
            symbols.append(
                FormatSymbol(lineNum, tok, input, int(start - begin), len)
            );
            continue;
        }

        // Operators and punctuation.
        const char *start = cur;
        const Token tok = scanCStyleOperator(
            cur,
            end,
            ! inDirective && isCFamily
        );
        symbols.append(
            FormatSymbol(
                lineNum,
                tok,
                input,
                int(start - begin),
                int(cur - start)
            )
        );
    }

    Q_UNUSED(lineHasDirectiveToken);
    return symbols;
}

/*!
    Extracts every comment and Python-style docstring from @a input for @a
    language, marking trailing comments relative to preceding code tokens.
*/
QList<FormatComment > FormatLexer::scanComments(
        const QByteArray & input, FormatLanguage::Id language)
{
    QList<FormatComment > result;
    if (input.isEmpty()) {
        return result;
    }

    const FormatCommentStyle style = FormatLanguageInfo::commentStyle(language);
    Options opts(true, language);
    const Symbols all = FormatLexer::tokenize(input, opts);

    QVector<int > lineStarts;
    lineStarts.append(0);
    for (int i = 0; i < input.size(); ++i) {
        if (input.at(i) == '\n') {
            lineStarts.append(i + 1);
        }
    }

    int previousCodeLine = 0;
    for (int i = 0; i < all.size(); ++i) {
        const FormatSymbol & symbol = all.at(i);
        if (symbol.len <= 0 || symbol.token == NOTOKEN) {
            continue;
        }
        bool isComment = symbol.token == CPP_COMMENT
            || symbol.token == C_COMMENT;
        bool isDocstring = false;
        if (style.supportsTripleDocstring
            && symbol.token == STRING_LITERAL
            && symbol.len >= 6
        ) {
            const char q = input.at(symbol.from);
            if ((q == '"'
                    || q == '\'')
                && input.mid(symbol.from, 3) == QByteArray(3, q)
            ) {
                const bool ownLine = symbol.lineNum > previousCodeLine;
                if (ownLine) {
                    // Only standalone statements (no operator/assignment before
                    // it on the same expression) act as docstrings.
                    isComment = true;
                    isDocstring = true;
                }
            }
        }
        if (isComment) {
            FormatComment comment;
            comment.offset = symbol.from;
            comment.length = symbol.len;
            const QByteArray head = input.mid(symbol.from, 4);
            if (isDocstring) {
                comment.kind = FormatComment::BlockDoc;
            } else if (symbol.token == CPP_COMMENT) {
                const bool doc = head.startsWith("///")
                    || head.startsWith("//!");
                comment.kind = doc && ! head.startsWith("////")
                    ? FormatComment::LineDoc : FormatComment::Line;
            } else {
                const bool doc = head.startsWith("/**")
                    || head.startsWith("/*!");
                comment.kind = doc && head != "/**/" ? FormatComment::BlockDoc
                                                     : FormatComment::Block;
            }
            comment.trailing = previousCodeLine > 0
                && symbol.lineNum == previousCodeLine;
            result.append(comment);
            continue;
        }
        int endLine = symbol.lineNum;
        for (int pos = symbol.from; pos < symbol.from + symbol.len; ++pos) {
            if (input.at(pos) == '\n') {
                ++endLine;
            }
        }
        previousCodeLine = endLine;
    }
    return result;
}
