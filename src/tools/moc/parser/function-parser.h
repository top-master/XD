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

#ifndef FUNCTIONPARSER_H
#define FUNCTIONPARSER_H

#include "moc.h"

class FunctionParser {
public:
    inline FunctionParser(Moc *owner, FunctionDef *definition)
        : moc(owner), def(definition)
        , isTest(false), isInMacro(false)
        , isBodyParsed(true)
    {}

    bool parseFunction(bool inMacro) {
        this->isTest = false;
        this->isInMacro = inMacro;
        this->isBodyParsed = true;
        return this->run();
    }

    bool parseMaybeFunction(const ClassDef *cdef) {
        this->isTest = true;
        this->isBodyParsed = false;
        this->className = cdef->classname;
        return this->run();
    }

private:
    bool run() {
        def->isVirtual = false;
        def->isStatic = false;
        bool templateFunction = false;
        //skip modifiers and attributes from beginning
        forever {
            Token t = moc->next();
            switch (t) {
            default: // Prevents compile-warning.
                break;
            case INLINE:
            case EXPLICIT:
                continue;
            case STATIC:
                def->isStatic = true; //mark as static
                continue;
            case VIRTUAL:
                def->isVirtual = true; //mark as virtual
                continue;
            case TEMPLATE:
                templateFunction = true;
                continue;
            }
            moc->prev(); //restore last token, unhandled by above

            if(moc->testFunctionAttribute(def))
                continue;
            if(moc->testFunctionRevision(def))
                continue;
            break;
        }

        bool tilde = moc->test(TILDE);
        def->type = moc->parseType();

        if (def->type.name.isEmpty()) {
            if(this->isTest)
                return false;

            if (templateFunction) {
                moc->error("Template function as signal or slot");
            } else
                moc->error();
        }

        bool scopedFunctionName = false;
        if (moc->test(LPAREN)) {
           def->name = def->type.name;
           scopedFunctionName = def->type.isScoped;
           if (def->name == this->className) {
               def->isDestructor = tilde;
               def->isConstructor = !tilde;
               def->type = Type();
           } else {
               def->type = Type("int");
           }
        } else {
            Type tempType = moc->parseType();;
            while (!tempType.name.isEmpty() && moc->lookup() != LPAREN) {
                if (moc->testFunctionAttribute(def->type.firstToken, def)) {
                    ; // fine
                } else if (def->type.firstToken == Q_SIGNALS_TOKEN /*|| def->type.name == "Q_SIGNAL"*/) {
                    if(this->isTest)
                        def->isSignal = true;
                    else
                        moc->error();
                } else if (def->type.firstToken == Q_SLOTS_TOKEN /*|| def->type.name == "Q_SLOT"*/) {
                    if(this->isTest)
                        def->isSlot = true;
                    else
                        moc->error();
                } else {
                    if (!def->tag.isEmpty())
                        def->tag += ' ';
                    def->tag += def->type.name;
                }
                def->type = tempType;
                tempType = moc->parseType();
            }
            if (this->isTest) {
                if(qNot( moc->test(LPAREN) ))
                    return false;
            } else
                moc->next(LPAREN, "Not a signal or slot declaration");

            def->name = tempType.name;
            scopedFunctionName = tempType.isScoped;
        }

        // we don't support references as return types, it's too dangerous
        if (def->type.referenceType == Type::Reference) {
            QByteArray rawName = def->type.rawName;
            QRange rawClass = def->type.rawClass;
            //reset to void
            def->type = Type("void");
            //restore rawName
            def->type.rawName = rawName;
            def->type.rawClass = rawClass;
        }

        def->type.renormalize();

        if ( ! moc->test(RPAREN)) {
            this->parseArguments();
            if(this->isTest) {
                if (qNot( moc->test(RPAREN) ))
                    return false;
            } else
                moc->next(RPAREN);
        }

        //support and skip optional macros with compiler specific options
        forever {
            Token t = moc->next();
            switch (t) {
            default: // Prevents compile-warning.
                break;
            case IDENTIFIER:
                continue;
            case CONST:
                def->isConst = true; //mark as const
                continue;
            }
            moc->prev(); //restore last token, unhandled by above

            if(moc->testFunctionDefault(def))
                continue;

            break;
        }

        //parse until end
        if(this->isBodyParsed) {
            if (this->isInMacro) {
                if(this->isTest) {
                    if(qNot( moc->test(RPAREN) ))
                        return false; //expected macro end
                } else {
                    moc->next(RPAREN);
                    moc->prev();
                }
            } else {
                parseBody();
            }
        }

        if (scopedFunctionName) {
            if(qNot(this->isTest) || (def->isSignal || def->isSlot || def->isInvokable)) {
                QByteArray msg("Function declaration ");
                msg += def->name;
                msg += " contains extra qualification. Ignoring as signal or slot.";
                moc->warning(msg.constData());
                return false;
            }
        }
        return true;
    }

    void parseArguments() //parseFunctionArguments(FunctionDef *def)
    {
        while (moc->hasNext()) {
            ArgumentDef  arg;
            arg.type = moc->parseType();
            if (arg.type.name == "void")
                break;
            if (moc->test(IDENTIFIER))
                arg.name = moc->lexem();
            while (moc->test(LBRACK)) {
                arg.rightType += moc->lexemUntil(RBRACK);
            }
            if (moc->test(CONST) || moc->test(VOLATILE)) {
                arg.rightType += ' ';
                arg.rightType += moc->lexem();
            }

            // No longer need to do:
            // arg.normalizedType = ...
            // arg.typeNameForCast = ...

            if (moc->test(EQ))
                arg.isDefault = true;
            def->arguments += arg;
            if (!moc->until(COMMA))
                break;
        }

        if (!def->arguments.isEmpty()
            && def->arguments.last().normalizedType() == "QPrivateSignal") {
            def->arguments.removeLast();
            def->isPrivateSignal = true;
        }
    }
    void parseBody()
    {
        if (moc->test(THROW)) {
            moc->next(LPAREN);
            moc->until(RPAREN);
        }
        if (moc->test(SEMIC))
            ;
        else if ((def->inlineCode = moc->test(LBRACE)))
            moc->until(RBRACE);
        else if ((def->isAbstract = moc->test(EQ)))
            moc->until(SEMIC);
        else
            moc->error("could not find function end-semicolon (;) or begin-brace ({)");

        moc->testFunctionDefault(def);
    }

private:
    Moc *moc;
    FunctionDef *def;
    bool isTest;
    bool isInMacro;
    bool isBodyParsed;
    QByteArray className;
};

#endif //FUNCTIONPARSER_H
