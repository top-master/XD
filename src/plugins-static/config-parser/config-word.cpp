// Copyright (C) 2015 The XD Company Ltd.
// SPDX-License-Identifier: Apache-2.0

#include "./config-word.h"

/*!
    @class Config::Word
    @internal

    Matches one fixed word, in any case, and keeps it in the spelling the
    schema gives (such as `keep-as-is`).
*/

/*!
    Builds the matcher of the word @a word.
*/
Config::Word::Word(const QString & word)
    : m_word(word)
{
}

/*!
    Builds a shared matcher of the word @a word, for a key's matchers.
*/
QSharedPointer<const Config::ValueMatcher > Config::Word::create(
        const QString & word)
{
    return QSharedPointer<const Config::ValueMatcher >(new Config::Word(word));
}

/*!
    Matches @a value when it is the word, in any case.
*/
bool Config::Word::match(const QString & value, QString *matched) const
{
    if (value.compare(this->m_word, Qt::CaseInsensitive) != 0) {
        return false;
    }
    *matched = this->m_word;
    return true;
}

/*!
    Builds the word in double quotes.
*/
QString Config::Word::describe() const
{
    return QLatin1Char('"') + this->m_word + QLatin1Char('"');
}
