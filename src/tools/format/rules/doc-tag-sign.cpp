// Copyright (C) 2015 The XD Company Ltd.
// SPDX-License-Identifier: Apache-2.0

#include "./doc-tag-sign.h"

#include "../format-document.h"

#include <QtCore/QByteArray>
#include <QtCore/QList>

/*!
    @class FormatDocTagSignRule
    @internal

    Writes the commands of a doc-comment (`\a`, `@p`, `\class`, `@return`,
    and the other Doxygen and QDoc commands) with one sign, `@` or a
    backslash, so that one file never mixes the two:

    * A doc-comment whose lines are all among the lines to format, while
      some lines of the file are not (a new one, with `-L` or `--git`),
      takes `[Comments] docTagSign` (`@` by default).
    * Any other doc-comment (an edited one, or each one when the whole file
      is formatted) takes the sign that most of the file's doc-comment
      commands use; with no command yet, or as many of each, it takes
      `[Comments] docTagSign`.

    A sign inside backticks stays as it is, and so does one before a word
    that names no command.
*/

/*!
    Names the commands a sign may start; a sign before any other word stays.
*/
static const char *const commandNames[] = {
    "a",
    "b",
    "c",
    "e",
    "p",
    "em",
    "brief",
    "class",
    "struct",
    "union",
    "namespace",
    "enum",
    "value",
    "omitvalue",
    "fn",
    "macro",
    "var",
    "property",
    "typedef",
    "internal",
    "param",
    "tparam",
    "return",
    "returns",
    "retval",
    "sa",
    "see",
    "since",
    "note",
    "warning",
    "deprecated",
    "obsolete",
    "relates",
    "reimp",
    "overload",
    "ref",
    "link",
    "endlink",
    "l",
    "code",
    "endcode",
    "snippet",
    "include",
    "list",
    "li",
    "endlist",
    "inmodule",
    "module",
    "file",
    "threadsafe",
    "reentrant",
    "nonreentrant",
    "target",
    "keyword", "title", "page", "todo", "pre", "post", "invariant", "throws",
    "exception",
    "details",
    "image",
    "table",
    "row",
    "header",
    "endtable",
    "tt",
    "sub",
    "sup",
    "underline",
    "bold",
    "legalese",
    "endlegalese",
    "quotation",
    "endquotation",
    "dontinclude", "headerfile", "inheaderfile"
};

/*!
    Tells whether @p word names a command that commandNames lists.
*/
static bool isCommand(const QByteArray & word)
{
    for (size_t i = 0; i < sizeof(commandNames) / sizeof(commandNames[0]); ++i) {
        if (word == commandNames[i]) {
            return true;
        }
    }
    return false;
}

/*!
    Rewrites the command signs of each doc-comment of @p group that differ
    from the sign the class picks for it.
*/
void FormatDocTagSignRule::runGroup(
        FormatDocument *document,
        const QList<int > & group) const
{
    // Picks the sign: the setting's for a doc-comment whose lines are all
    // selected while others are not, else the file's.
    bool isNew = true;
    bool isRanged = false;
    const QList<FormatComment > & comments = document->comments();
    for (int k = 0; k < group.size(); ++k) {
        if ( ! comments.at(group.at(k)).isDoc()) {
            return;
        }
        const int from = document->commentLine(group.at(k));
        const int to = document->commentEndLine(group.at(k));
        for (int line = from; line <= to; ++line) {
            isNew = isNew && document->isLineSelected(line);
        }
    }
    for (int line = 1; line <= document->lineCount() && ! isRanged; ++line) {
        isRanged = ! document->isLineSelected(line);
    }
    const char setting = document->settings().docTagSign;
    const char sign = isNew && isRanged
        ? setting : this->fileSign(document, setting);

    // Writes each command of the group with that sign.
    for (int k = 0; k < group.size(); ++k) {
        const QList<int > offsets = this->tags(document, group.at(k));
        for (int t = 0; t < offsets.size(); ++t) {
            if (document->text().at(offsets.at(t)) != sign) {
                document->replace(offsets.at(t), 1, QByteArray(1, sign));
            }
        }
    }
}

/*!
    Lists the offsets, in the text of @p document, of the signs that start a
    command in the doc-comment at @p comment: a `@` or a backslash after a
    space, an opening bracket, or the comment's marker, outside of
    backticks, before a word that isCommand() takes.
*/
QList<int > FormatDocTagSignRule::tags(
        const FormatDocument *document, int comment) const
{
    QList<int > offsets;
    const FormatComment & entry = document->comments().at(comment);
    const QByteArray & text = document->text();
    bool inTicks = false;
    for (int i = entry.offset; i < entry.offset + entry.length; ++i) {
        const char c = text.at(i);
        if (c == '\n') {
            inTicks = false;
            continue;
        }
        if (c == '`') {
            inTicks = ! inTicks;
            continue;
        }
        if (inTicks || (c != '@' && c != '\\')) {
            continue;
        }
        const char before = i > entry.offset ? text.at(i - 1) : ' ';
        if (before != ' ' && before != '\t' && before != '(' && before != '!'
            && before != '*' && before != '/'
        ) {
            continue;
        }
        int end = i + 1;
        while (end < entry.offset + entry.length
            && ((text.at(end) >= 'a' && text.at(end) <= 'z')
                || (text.at(end) >= '0' && text.at(end) <= '9'))
        ) {
            ++end;
        }
        if (isCommand(text.mid(i + 1, end - i - 1))) {
            offsets << i;
        }
    }
    return offsets;
}

/*!
    Builds the sign that most of the commands of the doc-comments of
    @p document use; @p fallback when they use none, or as many of each.
*/
char FormatDocTagSignRule::fileSign(
        const FormatDocument *document, char fallback) const
{
    int ats = 0;
    int backslashes = 0;
    const QList<FormatComment > & comments = document->comments();
    for (int i = 0; i < comments.size(); ++i) {
        if ( ! comments.at(i).isDoc()) {
            continue;
        }
        const QList<int > offsets = this->tags(document, i);
        for (int t = 0; t < offsets.size(); ++t) {
            if (document->text().at(offsets.at(t)) == '@') {
                ++ats;
            } else {
                ++backslashes;
            }
        }
    }
    if (ats == backslashes) {
        return fallback;
    }
    return ats > backslashes ? '@' : '\\';
}
