// Copyright (C) 2015 The XD Company Ltd.
// SPDX-License-Identifier: Apache-2.0

#include "./code-shape.h"

#include "../format-document.h"

#include <QtCore/QByteArray>
#include <QtCore/QDir>
#include <QtCore/QFile>
#include <QtCore/QFileInfo>
#include <QtCore/QRegularExpression>
#include <QtCore/QString>

/*!
    @class FormatCodeShapeRule
    @internal

    Reports the code shapes that some supported compilers mishandle, or that
    read poorly once long, and that only a person can rewrite well:

    * a call's returned pointer dereferenced in place (`*call() = value`);
    * a local direct-initialized from a call (`T v(call());`), where
      `T v = qMove(call());` copy-initializes it;
    * an inline function body of more than 5 lines;
    * a member-initializer list kept on a constructor line past 70 columns.
*/

/*!
    Checks each statement start, each function body, and each line.
*/
void FormatCodeShapeRule::run(FormatDocument *document) const
{
    for (int i = 0; i < document->symbols().size(); ++i) {
        if (document->isDirectiveLine(document->symbolLine(i))) {
            continue;
        }
        const Token before = document->token(i - 1);
        if (i == 0
            || before == SEMIC
            || before == LBRACE
            || before == RBRACE
        ) {
            this->checkStatement(document, i);
        }
        if (document->token(i) == LBRACE
            && document->braceScope(i) == FormatDocument::FunctionScope
        ) {
            this->checkBody(document, i);
        }
    }
    this->checkLines(document);
}

/*!
    Reports a dereferenced call result and a direct-initialized local in the
    statement that starts at the token @a start of @a document.
*/
void FormatCodeShapeRule::checkStatement(
        FormatDocument *document,
        int start) const
{
    const FormatDocument::Scope scope = document->scopeOf(start);
    if (scope != FormatDocument::FunctionScope
        && scope != FormatDocument::BlockScope
    ) {
        return;
    }

    // Finds `*<chain>(...) =`.
    if (document->token(start) == STAR) {
        int k = start + 1;
        while (document->token(k) == IDENTIFIER
            && (document->token(k + 1) == DOT || document->token(k + 1) == ARROW)
        ) {
            k += 2;
        }
        if (document->token(k) == IDENTIFIER && document->token(k + 1) == LPAREN
            && document->match(k + 1) > 0
        ) {
            const Token after = document->token(document->match(k + 1) + 1);
            if (after == EQ || after == PLUS_EQ || after == MINUS_EQ || after == OR_EQ
                || after == AND_EQ
            ) {
                document->report(
                    document->symbols().at(start).from,
                    "code-shape",
                    QString(QLL("store the returned pointer in a named local, " "then dereference that"))
                );
            }
        }
        return;
    }

    // Finds `<Type> <name>(<call>);`.
    int k = start;
    if (document->token(k) == CONST) {
        ++k;
    }
    if (document->token(k) != IDENTIFIER) {
        return;
    }
    ++k;
    while (document->token(k) == SCOPE
        && document->token(k + 1) == IDENTIFIER
    ) {
        k += 2;
    }
    if (document->token(k) == LANGLE) {
        int depth = 0;
        for (; k < document->symbols().size(); ++k) {
            const Token token = document->token(k);
            depth += token == LANGLE
                ? 1 : (token == RANGLE ? -1 : (token == GTGT ? -2 : 0));
            if (depth <= 0 || token == SEMIC || token == LPAREN) {
                break;
            }
        }
        if (depth > 0) {
            return;
        }
        ++k;
    }
    if (document->token(k) != IDENTIFIER
        || document->token(k + 1) != LPAREN
    ) {
        return;
    }
    const int close = document->match(k + 1);
    if (close < 0 || document->token(close + 1) != SEMIC) {
        return;
    }
    // Takes one argument that is a member call, such as `server->url(x)`,
    // and leaves out the types that cannot be copied, and the ones built
    // from a path or a pattern.
    static const char *const uncopyable[] = {
        "QFile", "QSaveFile", "QScopedPointer", "QMutexLocker", "QReadLocker",
        "QWriteLocker",
        "QDirIterator",
        "QTextStream",
        "QDataStream",
        "QProcess",
        "QSettings",
        "QEventLoop",
        "QFileInfo",
        "QDir",
        "QRegularExpression",
        "QRegExp"
    };
    const QByteArray type = document->lexem(document->token(start) == CONST ? start + 1 : start);
    for (size_t u = 0; u < sizeof(uncopyable) / sizeof(uncopyable[0]); ++u) {
        if (type == uncopyable[u]) {
            return;
        }
    }
    const int last = close - 1;
    if (document->token(last) != RPAREN || document->match(last) < k + 2) {
        return;
    }
    bool isMemberCall = false;
    for (int i = k + 2; i < document->match(last); ++i) {
        const Token token = document->token(i);
        if (token == COMMA) {
            return;
        }
        if (token == LPAREN) {
            i = qMax(i, document->match(i));
            continue;
        }
        isMemberCall = isMemberCall || token == DOT || token == ARROW;
    }
    if (isMemberCall
        && document->token(document->match(last) - 1) == IDENTIFIER
    ) {
        document->report(document->symbols().at(start).from, "code-shape",
                         QString(QLL("when the call returns the local's type, copy-initialize "
                                     "it: <Type> %1 = qMove(...);"))
                         .arg(QString::fromLatin1(document->lexem(k))));
    }
}

/*!
    Reports an inline body of more than 5 lines: a function body in a class
    body, or one whose head says `inline` or `Q_ALWAYS_INLINE`; @a brace is
    the body's `{` in @a document.
*/
void FormatCodeShapeRule::checkBody(FormatDocument *document, int brace) const
{
    bool isInline = document->scopeOf(brace) == FormatDocument::ClassScope;
    for (int i = document->headStart(brace); i < brace && ! isInline; ++i) {
        isInline = document->token(i) == INLINE
            || (document->token(i) == IDENTIFIER && document->lexem(i) == "Q_ALWAYS_INLINE");
    }
    const int close = document->match(brace);
    if ( ! isInline || close < 0) {
        return;
    }
    int lines = 0;
    for (int line = document->symbolLine(brace) + 1; line < document->symbolLine(close); ++line) {
        if (document->firstSymbolOfLine(line) >= 0) {
            ++lines;
        }
    }
    if (lines > 5) {
        document->report(document->symbols().at(brace).from, "code-shape",
                         QString(QLL("define the function in its source file: the inline "
                                     "body has %1 lines (at most 5)")).arg(lines));
    }
}

/*!
    Reports a member-initializer list that stays on a constructor line
    longer than 70 columns.
*/
void FormatCodeShapeRule::checkLines(FormatDocument *document) const
{
    for (int line = 1; line <= document->lineCount(); ++line) {
        const int width = QString::fromUtf8(document->lineText(line)).size();
        const int first = document->firstSymbolOfLine(line);
        if (width <= 70 || first < 0 || document->isDirectiveLine(line)) {
            continue;
        }
        const int last = document->lastSymbolOfLine(line);
        for (int i = first; i < last; ++i) {
            if (document->token(i) == RPAREN && document->token(i + 1) == COLON
                && document->match(i) >= first && document->scopeOf(i) != FormatDocument::FunctionScope
                && document->scopeOf(i) != FormatDocument::BlockScope
                && document->symbolLine(i + 1) == line && i + 2 <= last
            ) {
                document->report(
                    document->lineStart(line),
                    "code-shape",
                    QString(QLL("break the member-initializer list onto lines " "of its own"))
                );
                break;
            }
        }
    }
}
