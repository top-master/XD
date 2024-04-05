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

#include "parser.h"
#include "utils.h"
#include <stdio.h>
#include <stdlib.h>

QT_BEGIN_NAMESPACE

#ifdef USE_LEXEM_STORE
Symbol::LexemStore Symbol::lexemStore;
#endif

static const char *error_msg = 0;

bool Parser::forceLinuxFormatLogs = false;

Parser::~Parser() {
    // Nothing to do (but required).
}

void Parser::error(int rollback) {
    index -= rollback;
    error();
}
void Parser::error(const char *msg) {
    if (msg || error_msg) {
        qWarning(ErrorFormatStringByOption("Error: %s"),
                 currentFilenames.top().constData(), symbol().lineNum, msg?msg:error_msg);
    } else {
        UDebug dbg;
        dbg.printLocation(QLatin1String(currentFilenames.top().constData()), symbol().lineNum);
        dbg.nospace() << "moc: Parse error at \""
                      << symbol().lexem().data() << '"';
    }
    xd("moc: exit");
    this->onExit(EXIT_FAILURE);
}

void Parser::errorToken(Token expected)
{
#if defined(DEBUG_MOC)
    UDebug dbg; dbg << "Parser expected" << tokenTypeName(expected)
            << "but got:" << symbol().lexem().data();
    error( qPrintable(dbg.toString()) );
#else
    Q_UNUSED(expected)
    error();
#endif
}

void Parser::warning(const char *msg) {
    if (displayWarnings && msg)
        fprintf(stderr, ErrorFormatStringByOption("Warning: %s\n"),
                currentFilenames.top().constData(), qMax(0, index > 0 ? symbol().lineNum : 0), msg);
}

void Parser::note(const char *msg) {
    if (displayNotes && msg)
        fprintf(stderr, ErrorFormatStringByOption("Note: %s\n"),
                currentFilenames.top().constData(), qMax(0, index > 0 ? symbol().lineNum : 0), msg);
}

int Parser::onExit(int code)
{
    if (true) // Supresses warnings.
        exit(code);

    return code;
}

void Parser::nextIdentifier()
{
    Token t = next();
    const Symbol &definedOrNotDefined = symbol();
    if(t != PP_IDENTIFIER) {
        const QByteArray &word = definedOrNotDefined.lexem();
        if(qNot((t == PP_AND && word == "and")
                || (t == PP_NOT && word == "not")
                || (t == PP_OR && word == "or")))
        {
            errorToken(PP_IDENTIFIER);
        }
    }
}

QT_END_NAMESPACE
