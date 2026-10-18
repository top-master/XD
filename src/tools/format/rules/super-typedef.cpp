// Copyright (C) 2015 The XD Company Ltd.
// SPDX-License-Identifier: Apache-2.0

#include "./super-typedef.h"

#include "../format-document.h"
#include "../parser/code-helpers.h"

#include <QtCore/QByteArray>
#include <QtCore/QString>

/*!
    \class FormatSuperTypedefRule
    \internal

    Gives each class that has a base a private `typedef <Base> super;` as the
    first declaration of its body, right after `Q_OBJECT` or `Q_GADGET` when
    the class opens with one. A `struct`, whose members start
    public, gets a report instead.
*/

/*!
    Adds the typedef to each class body that lacks one.
*/
void FormatSuperTypedefRule::run(FormatDocument *document) const
{
    for (int i = 0; i < document->symbols().size(); ++i) {
        if (document->token(i) != LBRACE
            || document->braceScope(i) != FormatDocument::ClassScope
        ) {
            continue;
        }
        const int close = document->match(i);
        bool isStruct = false;
        const QByteArray base = this->baseName(document, i, &isStruct);
        if (close <= i + 1
            || base.isEmpty()
            || this->hasSuper(document, i)
        ) {
            continue;
        }
        if (isStruct) {
            document->report(document->symbols().at(i).from, "super-typedef",
                             QString(QLL("give the struct a private typedef %1 super;"))
                             .arg(QString::fromUtf8(base)));
            continue;
        }

        // Places the typedef after an opening `Q_OBJECT`, else first.
        const int first = i + 1;
        const int firstLine = document->symbolLine(first);
        if (firstLine == document->symbolLine(i)) {
            continue;
        }
        const Token token = document->token(first);
        const QByteArray line = "typedef " + base + " super;";
        if (token == Q_OBJECT_TOKEN || token == Q_GADGET_TOKEN) {
            document->replace(contentEnd(document, firstLine), 0,
                              "\n" + document->indentOf(firstLine) + line);
            continue;
        }
        const bool isAccess = token == PUBLIC || token == PRIVATE
            || token == PROTECTED;
        const QByteArray indent = isAccess
            ? document->indentOf(document->symbolLine(i)) + "    "
                : document->indentOf(firstLine);
        document->replace(document->lineStart(firstLine), 0, indent + line + "\n");
    }
}

/*!
    Builds the text of the first base class of the class whose body opens at
    \a brace of \a document, or an empty one when it has no base or the base
    spans lines; \a isStruct tells whether it is a `struct`.
*/
QByteArray FormatSuperTypedefRule::baseName(const FormatDocument *document, int brace,
                                            bool *isStruct) const
{
    // Finds the class key, then the `:` of the base list after it.
    int key = -1;
    for (int i = document->headStart(brace); i < brace && key < 0; ++i) {
        const Token token = document->token(i);
        if (token == CLASS || token == STRUCT) {
            key = i;
            *isStruct = token == STRUCT;
        }
    }
    int colon = -1;
    int depth = 0;
    for (int i = key + 1; key >= 0 && i < brace; ++i) {
        const Token token = document->token(i);
        depth += token == LANGLE
            ? 1 : (token == RANGLE ? -1 : (token == GTGT ? -2 : 0));
        if (token == COLON && depth == 0) {
            colon = i;
            break;
        }
    }
    if (colon < 0) {
        return QByteArray();
    }
    int first = colon + 1;
    while (document->token(first) == PUBLIC || document->token(first) == PRIVATE
        || document->token(first) == PROTECTED || document->token(first) == VIRTUAL
    ) {
        ++first;
    }
    int last = first;
    depth = 0;
    for (int i = first; i < brace; ++i) {
        const Token token = document->token(i);
        depth += token == LANGLE
            ? 1 : (token == RANGLE ? -1 : (token == GTGT ? -2 : 0));
        if (token == COMMA && depth == 0) {
            break;
        }
        last = i;
    }
    if (first >= brace
        || document->symbolLine(first) != document->symbolEndLine(last)
    ) {
        return QByteArray();
    }
    const int from = document->symbols().at(first).from;
    return document->text().mid(from, document->symbolEnd(last) - from);
}

/*!
    Tells whether the class body that opens at \a brace of \a document
    already declares `super` (with `typedef` or `using`), leaving out the
    bodies nested in it.
*/
bool FormatSuperTypedefRule::hasSuper(const FormatDocument *document,
                                      int brace) const
{
    const int close = document->match(brace);
    for (int i = brace + 1; i < close; ++i) {
        if (document->token(i) == LBRACE && document->match(i) > i) {
            i = document->match(i);
            continue;
        }
        if (document->token(i) == IDENTIFIER && document->lexem(i) == "super"
            && (document->token(i + 1) == SEMIC || document->token(i + 1) == EQ)
        ) {
            return true;
        }
    }
    return false;
}
