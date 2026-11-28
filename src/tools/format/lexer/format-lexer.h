// Copyright (C) 2015 The XD Company Ltd.
// SPDX-License-Identifier: Apache-2.0

#ifndef LEXER_FORMAT_LEXER_H
#define LEXER_FORMAT_LEXER_H

#include "./format-language.h"
#include "./format-token.h"
#include "../format-comment.h"

#include <QtCore/QByteArray>
#include <QtCore/QList>

class FormatLexer
{
public:

    struct Options
    {
        inline explicit Options(
                bool keepCommentTokens = false,
                FormatLanguage::Id lang = FormatLanguage::Cpp)
            : language(lang)
            , keepComments(keepCommentTokens)
            , keepWhitespace(false)
            , keepNewlines(false)
        {
        }

        FormatLanguage::Id language;
        bool keepComments;
        bool keepWhitespace;
        bool keepNewlines;
    };

    static Symbols tokenize(const QByteArray & input,
                            const Options & options = Options());
    static QList<FormatComment > scanComments(
            const QByteArray & input,
            FormatLanguage::Id language = FormatLanguage::Cpp);
};

#endif // LEXER_FORMAT_LEXER_H
