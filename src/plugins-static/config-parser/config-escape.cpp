// Copyright (C) 2015 The XD Company Ltd.
// SPDX-License-Identifier: Apache-2.0

#include "./config-escape.h"

#include <QtCore/QByteArray>

/*!
    \class Config::Escape
    \internal

    Decodes the escapes a C++ string literal takes, for a setting written in
    double quotes on the command line: `\'`, `\"`, `\?`, `\\`, `\a`, `\b`,
    `\f`, `\n`, `\r`, `\t`, `\v`, an octal `\ooo` (one to three digits),
    a hexadecimal `\x<digits>`, and the code points `\u<4 digits>` and
    `\U<8 digits>` (four and eight hexadecimal digits).
*/

/*!
    Builds the value of the hexadecimal digit \a c, or -1 for a character
    that is no such digit.
*/
static int hexDigit(QChar c)
{
    const ushort code = c.unicode();
    if (code >= '0' && code <= '9') {
        return code - '0';
    }
    if (code >= 'a' && code <= 'f') {
        return code - 'a' + 10;
    }
    if (code >= 'A' && code <= 'F') {
        return code - 'A' + 10;
    }
    return -1;
}

/*!
    Builds the hexadecimal value of the \a count digits of \a text at
    \a from (all of the hexadecimal digits there, when \a count is 0) into
    \a value, and the number of digits it read into \a read; returns false
    when a digit is missing.
*/
static bool readHex(const QString & text, int from, int count, uint *value, int *read)
{
    *value = 0;
    *read = 0;
    while (from + *read < text.size() && (count == 0 || *read < count)) {
        const int digit = hexDigit(text.at(from + *read));
        if (digit < 0) {
            break;
        }
        *value = *value * 16 + uint(digit);
        ++*read;
    }
    return *read > 0 && (count == 0 || *read == count);
}

/*!
    Decodes the escapes of \a text into \a decoded; returns false, with
    \a error naming the escape, for one a C++ string literal does not take.
*/
bool Config::Escape::decode(const QString & text, QString *decoded, QString *error)
{
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
        const char simple = next.toLatin1();

        // Decodes a one-character escape, by its place in `names`.
        static const char names[] = "'\"?\\abfnrtv";
        static const char values[] = "'\"?\\\a\b\f\n\r\t\v";
        const int place = simple != 0
            ? QByteArray::fromRawData(names, sizeof(names) - 1).indexOf(simple) : -1;
        if (place >= 0) {
            out += QLatin1Char(values[place]);
            continue;
        }

        // Decodes an octal escape of one to three digits.
        if (next >= QLatin1Char('0') && next <= QLatin1Char('7')) {
            uint value = 0;
            int read = 0;
            while (read < 3 && i + read < text.size() && text.at(i + read) >= QLatin1Char('0')
                   && text.at(i + read) <= QLatin1Char('7')
            ) {
                value = value * 8 + uint(text.at(i + read).unicode() - '0');
                ++read;
            }
            out += QChar(ushort(value));
            i += read - 1;
            continue;
        }

        // Decodes a hexadecimal escape, or a code point.
        const int count = simple == 'x' ? 0 : simple == 'u' ? 4 : simple == 'U' ? 8 : -1;
        uint value = 0;
        int read = 0;
        if (count < 0 || !readHex(text, i + 1, count, &value, &read) || value > 0x10FFFF) {
            *error = QString(QLL("%1: \\%2 is not a C++ escape")).arg(text, QString(next));
            return false;
        }
        if (QChar::requiresSurrogates(value)) {
            out += QChar(QChar::highSurrogate(value));
            out += QChar(QChar::lowSurrogate(value));
        } else {
            out += QChar(ushort(value));
        }
        i += read;
    }
    *decoded = out;
    return true;
}
