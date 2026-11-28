// Copyright (C) 2015 The XD Company Ltd.
// SPDX-License-Identifier: Apache-2.0

#ifndef LEXER_FORMAT_LANGUAGE_H
#define LEXER_FORMAT_LANGUAGE_H

#include <QtCore/QByteArray>
#include <QtCore/QString>

struct FormatLanguage
{
    enum Id {
        Unknown = 0,
        Cpp,
        ObjectiveC,
        Python,
        Shell,
        Generic
    };
};

struct FormatCommentStyle
{
    bool supportsSlashLine;
    bool supportsSlashBlock;
    bool supportsHashLine;
    bool supportsTripleDocstring;
};

class FormatLanguageInfo
{
public:

    static FormatLanguage::Id detect(
            const QString & path, const QByteArray & text = QByteArray());
    static QString name(FormatLanguage::Id language);
    static FormatCommentStyle commentStyle(FormatLanguage::Id language);
    static bool supportsCodeFormatting(FormatLanguage::Id language);
};

#endif // LEXER_FORMAT_LANGUAGE_H
