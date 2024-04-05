/****************************************************************************
**
** Copyright (C) 2015 The XD Company Ltd.
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing/
**
** This file is part of the tools applications of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL21$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see http://www.qt.io/terms-conditions. For further
** information use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file. Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** As a special exception, The Qt Company gives you certain additional
** rights. These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef PARSER_H
#define PARSER_H

#include <qstack.h>
#include "symbols.h"

QT_BEGIN_NAMESPACE

class Parser
{
public:
    Parser():index(0), displayWarnings(true), displayNotes(true) {}
    virtual ~Parser();

    static bool forceLinuxFormatLogs;
    Symbols symbols;
    int index;
    bool displayWarnings;
    bool displayNotes;

    struct IncludePath
    {
        inline explicit IncludePath(const QByteArray &_path)
            : path(_path), isFrameworkPath(false) {}
        QByteArray path;
        bool isFrameworkPath;
    };
    QList<IncludePath> includes;

    QStack<QByteArray> currentFilenames;
    /// @return \c true when parser is in input-file
    /// (which we get from command-line).
    inline bool isRootFile() const { return currentFilenames.size() <= 1; }

    inline bool hasNext() const { return (index < symbols.size()); }

    // MARK: Methods to Move forward or backward.

    /// @return next-token and sets that as current-symbol to be handled.
    inline Token next();
    /// Same as next(), but does not move forward if current-symbol mismatches @p possible.
    /// @return @c true if moved forward.
    bool test(Token possible);
    void next(Token expected);
    void next(Token expected, const char *msg);
    /// Handles the case that "not" is defined as macro name.
    void nextIdentifier();
    inline void prev() {--index;}

    // MARK: current-symbol info helpers.

    /// @return current-tokent from current-symbol.
    inline Token token() const { return symbols.at(index-1).token; }
    inline const Symbol &symbol() const { return symbols.at(index-1); }
    /// @return current-symbols index.
    inline int offset() const { return index - 1; }
    inline int nextOffset() const { return index; }
    /// Sets symbol at index "offset" as current-symbol (or current-token).
    inline void seek(int offset) { index = offset + 1; }
    /// lookup(1) returns next-token like "next()" would (but without setting that as current-symbol)
    /// lookup(0) returns current-token (which was returned by last call to "next()")
    /// lookup(-1) returns last and already handled token
    inline Token lookup(int distance = 1) const;

    inline const Symbol &symbol_lookup(int distance = 1) const { return symbols.at(index-1+distance); }
    inline QByteArray lexem() { return symbols.at(index-1).lexem();}
    inline QByteArray unquotedLexem() { return symbols.at(index-1).unquotedLexem();}

    // MARK: notification log prints.

    void error(int rollback);
    void error(const char *msg = 0);
    void errorToken(Token expected);
    void warning(const char * = 0);
    void note(const char * = 0);

    virtual int onExit(int code);
};

Token Parser::next() {
    if(index >= symbols.size())
        return NOTOKEN;
    return symbols.at(index++).token;
}

inline bool Parser::test(Token possible)
{
    // TRACE/moc improve: multiline condition, to be debuggable.
    if (index < symbols.size()) {
        const Symbol &symbol = symbols.at(index);
#ifdef DEBUG_MOC
        const QByteArray &lexem = symbol.lexem();
        Q_UNUSED(lexem)
#endif
        if (symbol.token == possible) {
            ++index;
            return true;
        }
    }
    return false;
}

inline Token Parser::lookup(int distance) const
{
    const int l = index - 1 + distance;
    return l < symbols.size() ? symbols.at(l).token : NOTOKEN;
}

inline void Parser::next(Token expected)
{
    if (!test(expected)) {
        errorToken(expected);
    }
}

inline void Parser::next(Token token, const char *msg)
{
    if (!test(token))
        error(msg);
}

QT_END_NAMESPACE

#endif
