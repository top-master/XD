// Copyright (C) 2015 The XD Company Ltd.
// SPDX-License-Identifier: Apache-2.0

#include "./config-escape.h"

#include <QtCore/QByteArray>

/*!
    @class Config::Escape
    @internal

    Decodes the escapes a C++ string literal takes, for a setting written in
    double quotes on the command line: `\'`, `\"`, `\?`, `\\`, `\a`, `\b`,
    `\f`, `\n`, `\r`, `\t`, `\v`, an octal `\ooo` (one to three digits),
    a hexadecimal `\x<digits>`, and the code points `\u<4 digits>` and
    `\U<8 digits>` (four and eight hexadecimal digits).
*/

/*!
    The code point past the last one Unicode has, U+10FFFF; a value that
    reaches it names no character.
*/
static const uint pastLastCodePoint = uint(QChar::LastValidCodePoint) + 1;

/*!
    @fn int Config::Escape::digitOf(QChar c, int base)

    Builds the value of @a c as a digit of @a base (8 or 16), or -1 for a
    character that is no such digit; a letter digit counts in either case,
    as `code | 0x20` turns `A` to `F` into `a` to `f`.
*/

/*!
    Builds the value of the @a count digits of @a base (8 or 16) of @a text
    at @a from (all of its digits there, when @a count is 0) into @a value,
    and the number of digits it read into @a read; returns false when a
    digit is missing. A value past U+10FFFF stays right past it, however
    many digits follow, rather than wrap (see isCodePoint()).
*/
bool Config::Escape::readDigits(
        const QString & text, int from, int base,
        int count, uint *value, int *read)
{
    *value = 0;
    *read = 0;
    while (from + *read < text.size() && (count == 0 || *read < count)) {
        const int digit = Config::Escape::digitOf(
            text.at(from + *read),
            base
        );
        if (digit < 0) {
            break;
        }
        *value = qMin(*value * uint(base) + uint(digit), pastLastCodePoint);
        ++*read;
    }
    return *read > 0 && (count == 0 || *read == count);
}

/*!
    Tells whether @a value names a character of Unicode, U+10FFFF at most.
*/
bool Config::Escape::isCodePoint(uint value)
{
    return value < pastLastCodePoint;
}

/*!
    Tells whether @a text is one text in double quotes, whose closing quote
    ends it (a `\` taking the character after it along); a text of two
    quoted parts, such as `"a" "b"`, is not.
*/
bool Config::Escape::isQuoted(const QString & text)
{
    if (text.size() < 2 || ! text.startsWith(QLatin1Char('"'))) {
        return false;
    }
    int i = 1;
    while (i < text.size() && text.at(i) != QLatin1Char('"')) {
        i += text.at(i) == QLatin1Char('\\') ? 2 : 1;
    }
    return i == text.size() - 1;
}

/*!
    Builds into @a value the text in double quotes @a text stands for, its
    escapes decoded (see decode()), or @a text as it is when it is no such
    text (see isQuoted()); returns false, with @a error, for a bad escape.
*/
bool Config::Escape::unquote(
        const QString & text, QString *value,
        QString *error)
{
    if ( ! Config::Escape::isQuoted(text)) {
        *value = text;
        return true;
    }
    return Config::Escape::decode(
        text.mid(1, text.size() - 2),
        value,
        error
    );
}

/*!
    Builds into @a value the character the one-character escape `\<name>`
    stands for, which both a C++ string literal and an `.ini` value take:
    `\'`, `\"`, `\?`, `\\`, `\a`, `\b`, `\f`, `\n`, `\r`, `\t`, and
    `\v`; returns false for any other @a name.
*/
bool Config::Escape::simple(char name, char *value)
{
    static const char names[] = "'\"?\\abfnrtv";
    static const char values[] = "'\"?\\\a\b\f\n\r\t\v";
    const int place = name != 0
        ? QByteArray::fromRawData(names, sizeof(names) - 1).indexOf(name) : -1;
    if (place < 0) {
        return false;
    }
    *value = values[place];
    return true;
}

/*!
    Appends the character of @a codePoint to @a text, as a surrogate pair
    past U+FFFF.
*/
void Config::Escape::appendCodePoint(QString *text, uint codePoint)
{
    if (QChar::requiresSurrogates(codePoint)) {
        *text += QChar(QChar::highSurrogate(codePoint));
        *text += QChar(QChar::lowSurrogate(codePoint));
    } else {
        *text += QChar(ushort(codePoint));
    }
}

/*!
    Decodes the escapes of @a text into @a decoded; returns false, with
    @a error naming the escape, for one a C++ string literal does not take.
*/
bool Config::Escape::decode(const QString & text, QString *decoded,
                            QString *error)
{
    decoded->clear();
    QString out;
    for (int i = 0; i < text.size(); ++i) {
        const QChar c = text.at(i);
        if (c != QLatin1Char('\\')) {
            out += c;
            continue;
        }
        if (i + 1 >= text.size()) {
            *error = QString(QLL("%1: a lone \\ ends it")).arg(text);
            return false;
        }
        const QChar next = text.at(++i);
        const char name = next.toLatin1();

        // Decodes a one-character escape.
        char simple = 0;
        if (Config::Escape::simple(name, &simple)) {
            out += QLatin1Char(simple);
            continue;
        }

        // Decodes an octal escape of one to three digits.
        if (Config::Escape::digitOf(next, 8) >= 0) {
            uint value = 0;
            int read = 0;
            Config::Escape::readDigits(
                text.mid(i, 3),
                0,
                8,
                0,
                &value,
                &read
            );
            out += QChar(ushort(value));
            i += read - 1;
            continue;
        }

        // Decodes a hexadecimal escape, or a code point.
        const int count = name == 'x' ? 0 : name == 'u' ? 4 : name == 'U' ? 8 : -1;
        if (count < 0) {
            *error = QString(QLL("%1: \\%2 is not a C++ escape")).arg(
                text,
                QString(next)
            );
            return false;
        }
        uint value = 0;
        int read = 0;
        if ( ! Config::Escape::readDigits(
            text,
            i + 1,
            16,
            count,
            &value,
            &read
        )) {
            *error = count == 0
                ? QString(QLL("%1: \\x needs hexadecimal digits")).arg(text)
                : QString(QLL("%1: \\%2 needs %3 hexadecimal digits"))
                    .arg(text, QString(next), QString::number(count));
            return false;
        }
        if ( ! Config::Escape::isCodePoint(value)) {
            *error = QString(QLL("%1: \\%2%3 is past U+10FFFF")).arg(
                text,
                QString(next),
                text.mid(i + 1, read)
            );
            return false;
        }
        appendCodePoint(&out, value);
        i += read;
    }
    *decoded = out;
    return true;
}
