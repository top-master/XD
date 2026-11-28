// Copyright (C) 2015 The XD Company Ltd.
// SPDX-License-Identifier: Apache-2.0

#include "./format-language.h"

#include <QtCore/QFileInfo>

namespace {

FormatLanguage::Id languageFromExtension(const QString & suffix)
{
    const QString lower = suffix.toLower();
    if (lower == QLL("c")
        || lower == QLL("cc")
        || lower == QLL("cpp")
        || lower == QLL("cxx")
        || lower == QLL("c++")
        || lower == QLL("h")
        || lower == QLL("hh")
        || lower == QLL("hpp")
        || lower == QLL("hxx")
        || lower == QLL("h++")
        || lower == QLL("inl")
        || lower == QLL("tcc")
        || lower == QLL("txx")
    ) {
        return FormatLanguage::Cpp;
    }
    if (lower == QLL("m") || lower == QLL("mm")) {
        return FormatLanguage::ObjectiveC;
    }
    if (lower == QLL("py") || lower == QLL("pyi") || lower == QLL("pyw")) {
        return FormatLanguage::Python;
    }
    if (lower == QLL("sh")
        || lower == QLL("bash")
        || lower == QLL("zsh")
        || lower == QLL("ksh")
    ) {
        return FormatLanguage::Shell;
    }
    return FormatLanguage::Unknown;
}

FormatLanguage::Id languageFromShebang(const QByteArray & text)
{
    if ( ! text.startsWith("#!")) {
        return FormatLanguage::Unknown;
    }
    const int newline = text.indexOf('\n');
    const QByteArray line = (newline >= 0 ? text.left(newline) : text).toLower();
    if (line.contains("python")) {
        return FormatLanguage::Python;
    }
    if (line.contains("/sh")
        || line.contains("bash")
        || line.contains("zsh")
        || line.contains("ksh")
    ) {
        return FormatLanguage::Shell;
    }
    return FormatLanguage::Unknown;
}

} // namespace

/*!
    Detects the source language of the file at @a path from its suffix and, when
    the path has no known suffix, from any shebang line in @a text, falling back
    to C++ when no foreign script signature is present.
*/
FormatLanguage::Id FormatLanguageInfo::detect(
        const QString & path, const QByteArray & text)
{
    if ( ! path.isEmpty()) {
        const QFileInfo info(path);
        const FormatLanguage::Id bySuffix =
            languageFromExtension(info.suffix());
        if (bySuffix != FormatLanguage::Unknown) {
            return bySuffix;
        }
        const QString fileName = info.fileName().toLower();
        if (fileName == QLL("cmakelists.txt")
            || fileName.endsWith(QLL(".cmake"))
            || fileName == QLL("makefile")
            || fileName == QLL(".gitignore")
        ) {
            return FormatLanguage::Shell;
        }
    }
    const FormatLanguage::Id byShebang = languageFromShebang(text);
    if (byShebang != FormatLanguage::Unknown) {
        return byShebang;
    }
    return FormatLanguage::Cpp;
}

/*!
    Returns the canonical display name for @a language.
*/
QString FormatLanguageInfo::name(FormatLanguage::Id language)
{
    switch (language) {
    case FormatLanguage::Cpp:
        return QString(QLL("C++"));
    case FormatLanguage::ObjectiveC:
        return QString(QLL("Objective-C"));
    case FormatLanguage::Python:
        return QString(QLL("Python"));
    case FormatLanguage::Shell:
        return QString(QLL("Shell"));
    case FormatLanguage::Generic:
        return QString(QLL("Generic"));
    case FormatLanguage::Unknown:
        break;
    }
    return QString(QLL("Unknown"));
}

/*!
    Describes the comment delimiters recognized for @a language.
*/
FormatCommentStyle FormatLanguageInfo::commentStyle(FormatLanguage::Id language)
{
    FormatCommentStyle style;
    style.supportsSlashLine = false;
    style.supportsSlashBlock = false;
    style.supportsHashLine = false;
    style.supportsTripleDocstring = false;

    switch (language) {
    case FormatLanguage::Cpp:
    case FormatLanguage::ObjectiveC:
        style.supportsSlashLine = true;
        style.supportsSlashBlock = true;
        break;
    case FormatLanguage::Python:
        style.supportsHashLine = true;
        style.supportsTripleDocstring = true;
        break;
    case FormatLanguage::Shell:
        style.supportsHashLine = true;
        break;
    case FormatLanguage::Generic:
    case FormatLanguage::Unknown:
        style.supportsSlashLine = true;
        style.supportsSlashBlock = true;
        style.supportsHashLine = true;
        break;
    }
    return style;
}

/*!
    Tells whether C-family AST layout rules apply to @a language.
*/
bool FormatLanguageInfo::supportsCodeFormatting(FormatLanguage::Id language)
{
    return language == FormatLanguage::Cpp
        || language == FormatLanguage::ObjectiveC;
}
