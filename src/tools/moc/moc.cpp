/****************************************************************************
**
** Copyright (C) 2015 The XD Company Ltd.
** Copyright (C) 2015 The Qt Company Ltd.
** Copyright (C) 2016 Olivier Goffart <ogoffart@woboq.com>
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

#include "moc.h"
#include "parser/function-parser.h"
#include "parser/function-mapper.h"

#include "generator.h"
#include "qdatetime.h"
#include "utils.h"
#include "outputrevision.h"
#include "udebug.h"
#include <QtCore/qfile.h>
#include <QtCore/qfileinfo.h>
#include <QtCore/qdir.h>

// for normalizeTypeInternal
#include <private/qmetaobject_moc_p.h>
#include <private/qmetaobject_p.h> //for the flags.

const char STR_QOBJECT[] = "QObject";
const char STR_REMOTE_OBJECT[] = "QObjectRemote";

QT_BEGIN_NAMESPACE

Moc *Moc::m_instance = Q_NULLPTR;

QByteArray Moc::filename = QByteArray();
QByteArray Moc::outputFileName = QByteArray();
#if QT_REMOTE
bool Moc::requiresRemote = false;
bool Moc::isRemoteGen = false;
const QLatin1Literal Moc::REMOTE_SUFFIX = QLL("Remote");
#endif

// only moc needs this function
static QByteArray normalizeType(const QByteArray &ba, bool fixScope = false)
{
    const char *s = ba.constData();
    int len = ba.size();
    char stackbuf[64];
    char *buf = (len >= 64 ? new char[len + 1] : stackbuf);
    char *d = buf;
    char last = 0;
    while(*s && is_space(*s))
        s++;
    while (*s) {
        while (*s && !is_space(*s))
            last = *d++ = *s++;
        while (*s && is_space(*s))
            s++;
        if (*s && ((is_ident_char(*s) && is_ident_char(last))
                   || ((*s == ':') && (last == '<')))) {
            last = *d++ = ' ';
        }
    }
    *d = '\0';
    QByteArray result = normalizeTypeInternal(buf, d, fixScope);
    if (buf != stackbuf)
        delete [] buf;
    return result;
}

Moc::~Moc() {
    if (this == m_instance) {
        m_instance = Q_NULLPTR;
    }
}

bool Moc::parseClassHead(ClassDef *def)
{
    // figure out whether this is a class declaration, or only a
    // forward or variable declaration.
    int i = 0;
    Token token;
    do {
        token = lookup(i++);
        if (token == COLON || token == LBRACE)
            break;
        if (token == SEMIC || token == RANGLE)
            return false;
    } while (token);

    // Stores type, not really required by the way.
    def->isStruct = (lookup(0) == STRUCT);

    if (!test(IDENTIFIER)) // typedef struct { ... }
        return false;
    QByteArray name = lexem();

    // support "class IDENT name" and "class IDENT(IDENT) name"
    // also support "class IDENT name (final|sealed|Q_DECL_FINAL)"
    if (test(LPAREN)) {
        until(RPAREN);
        if (!test(IDENTIFIER))
            return false;
        name = lexem();
    } else  if (test(IDENTIFIER)) {
        const QByteArray lex = lexem();
        if (lex != "final" && lex != "sealed" && lex != "Q_DECL_FINAL")
            name = lex;
    }

    def->qualified += name;
    while (test(SCOPE)) {
        def->qualified += lexem();
        if (test(IDENTIFIER)) {
            name = lexem();
            def->qualified += name;
        }
    }
    def->classname = name;

    if (test(IDENTIFIER)) {
        const QByteArray lex = lexem();
        if (lex != "final" && lex != "sealed" && lex != "Q_DECL_FINAL")
            return false;
    }

    if (test(COLON)) {
        do {
            test(VIRTUAL);
            FunctionDef::Access access = FunctionDef::Public;
            if (test(PRIVATE))
                access = FunctionDef::Private;
            else if (test(PROTECTED))
                access = FunctionDef::Protected;
            else
                test(PUBLIC);
            test(VIRTUAL);
            const QByteArray type = parseType().name;
            // ignore the 'class Foo : BAR(Baz)' case
            if (test(LPAREN)) {
                until(RPAREN);
            } else {
                const Superclass &superclass = Superclass(type, access);
                def->superclassList += superclass;
                if (superclass.hasName("QObjectDecor")) {
                    ClassInfoDef info;
                    info.name = "Decor";
                    info.value = info.name;
                    def->classInfoList += info;
                }
            }
        } while (test(COMMA));

        if (!def->superclassList.isEmpty()
            && knownGadgets.contains(def->superclassList.first().className)) {
            // Q_GADGET subclasses are treated as Q_GADGETs
            knownGadgets.insert(def->classname);
            knownGadgets.insert(def->qualified);
        }
    }
    if (!test(LBRACE))
        return false;
    def->begin = index - 1;
    bool foundRBrace = until(RBRACE);
    def->end = index;
    index = def->begin + 1;
    return foundRBrace;
}

bool Moc::parseClassForward(ClassBasicDef *def, const ParseState &state)
{
    Token type = next();
    if((type == CLASS || type == STRUCT)
            && test(IDENTIFIER) && test(SEMIC))
    {
        def->isStruct = (type == STRUCT);

        const Symbol &id = symbol_lookup(-1);
        def->classname = id.lexem();
        def->qualified = state.namespacePrefix(this->nextOffset(), def->classname);
        return true;
    }
    return false;
}

Type Moc::parseType()
{
    Type type;
    bool hasSignedOrUnsigned = false;
    bool isVoid = false;
    type.firstToken = lookup();
    for (;;) {
        Token t = next();
        switch (t) {
            case SIGNED:
            case UNSIGNED:
                hasSignedOrUnsigned = true;
                // fall through
            case CONST:
            case VOLATILE:
                type.name += lexem();
                type.name += ' ';
                if (t == CONST)
                    type.isConst = true;
                else if(t == VOLATILE)
                    type.isVolatile = true;
                continue;
            case Q_MOC_COMPAT_TOKEN:
            case Q_INVOKABLE_TOKEN:
            case Q_SCRIPTABLE_TOKEN:
            case Q_SIGNALS_TOKEN:
            case Q_SLOTS_TOKEN:
            case Q_SIGNAL_TOKEN:
            case Q_SLOT_TOKEN:
                type.name += lexem();
                return type;
            case NOTOKEN:
                return type;
            default:
                prev();
                break;
        }
        break;
    }
    test(ENUM) || test(CLASS) || test(STRUCT);
    for(;;) {
        switch (next()) {
        case IDENTIFIER:
            // void mySlot(unsigned myArg)
            if (hasSignedOrUnsigned) {
                prev();
                break;
            }
        case CHAR:
        case SHORT:
        case INT:
        case LONG:
            type.appendClass(lexem());
            // preserve '[unsigned] long long', 'short int', 'long int', 'long double'
            if (test(LONG) || test(INT) || test(DOUBLE)) {
                type.appendClass(QByteArray::fromRawData(" ", 1));
                prev();
                continue;
            }
            break;
        case FLOAT:
        case DOUBLE:
        case VOID:
        case BOOL:
            type.appendClass(lexem());
            isVoid |= (lookup(0) == VOID);
            break;
        case NOTOKEN:
            return type;
        default:
            prev();
            ;
        }
        if (test(LANGLE)) {
            if (type.name.isEmpty()) {
                // In C/C++ the '<' char cannot start a type.
                return type;
            }

            // Adds template's `<...>` part to `Type::name`,
            // but additionally emits spaces where required to help old compilers.
            QByteArray templ = lexemUntil(RANGLE);
            type.name.reserve(type.name.size() + templ.size() * 1.2);
            int depth = 0;
            for (int i = 0; i < templ.size(); ++i) {
                const char current = templ.at(i);
                type.name += current;
                if (current == '(') {
                    depth++;
                    continue;
                } else if (current == ')') {
                    depth--;
                    continue;
                }
                if (depth == 0 && i+1 < templ.size()) {
                    const char next = templ.at(i+1);
                    if (current == '<' && next == ':') {
                        type.name += ' ';
                    } else if (current == '>' && next == '>'
                        // Unless is a binary `>>` operator.
                        && i+2 < templ.size() && ! is_digit_char(templ.at(i+2))
                    ) {
                        type.name += ' ';
                    }
                }
            }
        }
        if (test(SCOPE)) {
            type.name += lexem();
            type.isScoped = true;
        } else {
            break;
        }
    }
    while (test(CONST) || test(VOLATILE) || test(SIGNED) || test(UNSIGNED)
           || test(STAR) || test(AND) || test(ANDAND)) {
        type.name += ' ';
        type.name += lexem();
        if (lookup(0) == AND)
            type.referenceType = Type::Reference;
        else if (lookup(0) == ANDAND)
            type.referenceType = Type::RValueReference;
        else if (lookup(0) == STAR)
            type.referenceType = Type::Pointer;
    }
    type.rawName = type.name;
    // transform stupid things like 'const void' or 'void const' into 'void'
    if (isVoid && type.referenceType == Type::NoReference) {
        type.name = "void";
    }
    return type;
}

bool Moc::parseEnum(EnumDef *def)
{
    bool isTypdefEnum = false; // typedef enum { ... } Foo;

    if (test(CLASS))
        def->isEnumClass = true;

    if (test(IDENTIFIER)) {
        def->name = lexem();
    } else {
        if (lookup(-1) != TYPEDEF)
            return false; // anonymous enum
        isTypdefEnum = true;
    }
    if (test(COLON)) { // C++11 strongly typed enum
        // enum Foo : unsigned long { ... };
        parseType(); //ignore the result
    }
    if (!test(LBRACE))
        return false;
    do {
        if (lookup() == RBRACE) // accept trailing comma
            break;
        next(IDENTIFIER);
        def->values += lexem();
    } while (test(EQ) ? until(COMMA) : test(COMMA));
    next(RBRACE);
    if (isTypdefEnum) {
        if (!test(IDENTIFIER))
            return false;
        def->name = lexem();
    }
    return true;
}

bool Moc::testFunctionAttribute(FunctionDef *def)
{
    if (index < symbols.size() && testFunctionAttribute(symbols.at(index).token, def)) {
        ++index;
        return true;
    }
    return false;
}

bool Moc::testFunctionAttribute(Token tok, FunctionDef *def)
{
    switch (tok) {
        case Q_MOC_COMPAT_TOKEN:
            def->isCompat = true;
            return true;
        case Q_INVOKABLE_TOKEN:
            def->isInvokable = true;
            return true;
        case Q_SIGNAL_TOKEN:
            def->isSignal = true;
            return true;
        case Q_SLOT_TOKEN:
            def->isSlot = true;
            return true;
        case Q_SCRIPTABLE_TOKEN:
            def->isInvokable = def->isScriptable = true;
            return true;
        default: break;
    }
    return false;
}

bool Moc::testFunctionRevisionRaw(int *revision)
{
    if (test(Q_REVISION_TOKEN)) {
        next(LPAREN);
        QByteArray arguments = lexemUntil(RPAREN);
        //remove LPAREN and RPAREN
        arguments.remove(0, 1);
        arguments.chop(1);
        bool ok = false;
        *revision = arguments.toInt(&ok);
        if (!ok || *revision < 0)
            error("Invalid revision");
        return true;
    }

    return false;
}

bool Moc::testFunctionRevision(FunctionDef *def)
{
    return testFunctionRevisionRaw(&def->revision);
}

bool Moc::testFunctionDefault(FunctionDef *def)
{
    if (test(Q_DEFAULT_TOKEN)) {
        next(LPAREN);
        def->defaultValue = lexemUntil(RPAREN);
        //remove LPAREN and RPAREN
        def->defaultValue.remove(0, 1);
        def->defaultValue.chop(1);
        return true;
    }
    return false;
}

// returns false if the function should be ignored
bool Moc::parseFunction(FunctionDef *def, bool inMacro)
{
    FunctionParser parser = FunctionParser(this, def);
    return parser.parseFunction(inMacro);
}

// like parseFunction, but never aborts with an error
bool Moc::parseMaybeFunction(const ClassDef *cdef, FunctionDef *def)
{
    FunctionParser parser = FunctionParser(this, def);
    return parser.parseMaybeFunction(cdef);
}


bool Moc::parseUntilClass(Token t, ParseState *state)
{
    switch (t) {
        case NAMESPACE: {
            int rewind = index;
            if (test(IDENTIFIER)) {
                if (test(EQ)) {
                    // namespace Foo = Bar::Baz;
                    until(SEMIC);
                } else if (!test(SEMIC)) {
                    NamespaceDef def;
                    def.name = lexem();
                    // TRACE moc/parse: support Android's "namespace name IDENT(IDENT)"
                    //next(LBRACE); //old code
                    if(!test(LBRACE))
                        until(LBRACE);
                    def.begin = index - 1;
                    until(RBRACE);
                    def.end = index;
                    index = def.begin + 1;
                    state->namespaceList += def;
                    index = rewind;
                }
            }
            break;
        }
        case SEMIC:
        case RBRACE:
            state->templateClass = false;
            break;
        case TEMPLATE:
            state->templateClass = true;
            break;
        case MOC_INCLUDE_BEGIN:
            // TRACE moc Add: unquoted Lexem If Any.
            currentFilenames.push(symbol().unquotedLexemIfAny());
            break;
        case MOC_INCLUDE_END:
            currentFilenames.pop();
            break;
        case Q_DECLARE_INTERFACE_TOKEN:
            parseDeclareInterface();
            break;
        case Q_DECLARE_METATYPE_TOKEN:
            parseDeclareMetatype();
            break;
        case USING:
            if (test(NAMESPACE)) {
                while (test(SCOPE) || test(IDENTIFIER))
                    ;
                next(SEMIC);
            }
            break;
        case Q_REMOTE_INCLUDE_TOKEN:
            next(LPAREN);
            if (currentFilenames.size() <= 1) {
                QByteArray path = lexemUntil(RPAREN);
                //remove LPAREN,QUOTE and QUOTE,RPAREN
                path.remove(0, 2);
                path.chop(2);
                //resolve path relative to input file's directory
                QFileInfo inFileInfo(QDir::current(), QFile::decodeName(filename));
                QString decodedPath = QFile::decodeName(path);
                decodedPath = inFileInfo.dir().absoluteFilePath(decodedPath);
                //at last encode and append to includes
                this->remote_include( QFile::encodeName(decodedPath) );
            } else
                until(RPAREN);
            break;
#if QT_REMOTE
        case Q_REMOTE_FORWARD_TOKEN: {
            int lastIndex = this->index;
            ClassBasicDef cdef;
            if(parseClassForward(&cdef, *state)) {
                cdef.isUsed = this->isRootFile();
                remoteForwards.insert(cdef.classname, cdef);
            } else {
                this->index = lastIndex;
                this->warning("Expected forward declaration.");
            }
        } break;
#endif
        case CLASS:
        case STRUCT: {
            //found class/struct
            if (this->isRootFile())
                break; //was outside moc include break to do full parsing
            //was inside moc include do fast parsing
            //  although the class is just included by the input-file (which we get from command-line)
            //  we need for latter to know at least which classes are valid "QObject" sub-classes
            ClassDef def;
            if (qNot( parseClassHead(&def) )) {
                return false;
            }

            while (def.contains(this->nextOffset()) && hasNext()) {
                Token t = next();
                if (t == Q_OBJECT_TOKEN) {
                    def.hasQObject = true;
                    break;
                } else if(t == Q_GADGET_TOKEN) {
                   def.hasQGadget = true;
                   break;
                } else if(t == Q_REMOTE_TOKEN) {
                    def.hasQObject = true;
                    def.hasQRemote = true;
                    break;
                }
            }

            if (!def.hasQObject && !def.hasQGadget) {
                return false;
            }

            def.qualified = state->namespacePrefix(this->nextOffset(), def.qualified);

            QSet<QByteArray> &classHash = def.hasQObject ? knownQObjectClasses : knownGadgets;
            classHash.insert(def.classname);
            classHash.insert(def.qualified);

#if QT_REMOTE
            if(def.hasQRemote) {
                knownRemoteFiles.insert(def.classname + Moc::REMOTE_SUFFIX.data(), currentFilenames.last());
                knownRemoteFiles.insert(def.qualified + Moc::REMOTE_SUFFIX.data(), currentFilenames.last());
            }
#endif
            return false; }
        default: break;
    }
    return true;
}

bool Moc::parseQObject(Token *t, const ParseState &state)
{
    int remoteMacroOffset = -1;

    //skip if not class/struct or is inside moc file
    if ((*t != CLASS && *t != STRUCT) || currentFilenames.size() > 1)
        return false;

    ClassDef def;
    if (parseClassHead(&def)) {
        FunctionDef::Access access = FunctionDef::Private;
        def.qualified = state.namespacePrefix(this->nextOffset(), def.qualified);
        while (def.contains(this->nextOffset()) && hasNext()) {
            switch ((*t = next())) {
            case PRIVATE:
                access = FunctionDef::Private;
                if (test(Q_SIGNALS_TOKEN))
                    error("Signals cannot have access specifier");
                break;
            case PROTECTED:
                access = FunctionDef::Protected;
                if (test(Q_SIGNALS_TOKEN))
                    error("Signals cannot have access specifier");
                break;
            case PUBLIC:
                access = FunctionDef::Public;
                if (test(Q_SIGNALS_TOKEN))
                    error("Signals cannot have access specifier");
                break;
            case STRUCT:
            case CLASS: {
                ClassDef nestedDef;
                if (parseClassHead(&nestedDef)) {
                    while (nestedDef.contains(this->nextOffset()) && def.contains(this->nextOffset())) {
                        *t = next();
                        if (*t >= Q_META_TOKEN_BEGIN && *t < Q_META_TOKEN_END)
                            error("Meta object features not supported for nested classes");
                    }
                    def.classList.insert(nestedDef.classname);
                }
            } break;
            case Q_SIGNALS_TOKEN:
                parseSignals(&def);
                break;
            case Q_SLOTS_TOKEN:
                switch (lookup(-1)) {
                case PUBLIC:
                case PROTECTED:
                case PRIVATE:
                    parseSlots(&def, access);
                    break;
                default:
                    error("Missing access specifier for slots");
                }
                break;
#if QT_REMOTE
            case Q_REMOTE_TOKEN: {
                //just required to remember QObject has "Q_REMOTE" macro
                ClassInfoDef infoDef;
                infoDef.name = "Q_REMOTE";
                def.classInfoList.prepend(infoDef);
                def.hasQRemote = true;
                requiresRemote = true;
                remoteMacroOffset = this->offset();
            } //do not break just continue to "Q_OBJECT_TOKEN"
#endif // QT_REMOTE
            case Q_OBJECT_TOKEN:
                def.hasQObject = true;
                if (state.templateClass)
                    error("Template classes not supported by Q_OBJECT");
                if (def.classname != "Qt" && def.classname != STR_QOBJECT && def.superclassList.isEmpty())
                    error("Class contains Q_OBJECT macro but does not inherit from QObject");
                break;
            case Q_GADGET_TOKEN:
                def.hasQGadget = true;
                if (state.templateClass)
                    error("Template classes not supported by Q_GADGET");
                break;
            case Q_PROPERTY_TOKEN:
                parseProperty(&def);
                break;
            case Q_PLUGIN_METADATA_TOKEN:
                parsePluginData(&def);
                break;
            case Q_ENUMS_TOKEN:
            case Q_ENUM_TOKEN:
                parseEnumOrFlag(&def, false);
                break;
            case Q_FLAGS_TOKEN:
            case Q_FLAG_TOKEN:
                parseEnumOrFlag(&def, true);
                break;
            case Q_DECLARE_FLAGS_TOKEN:
                parseFlag(&def);
                break;
            case Q_CLASSINFO_TOKEN:
                parseClassInfo(&def);
                break;
            case Q_INTERFACES_TOKEN:
                parseInterfaces(&def);
                break;
            case Q_PRIVATE_SLOT_TOKEN:
                parseSlotInPrivate(&def, access);
                break;
            case Q_PRIVATE_PROPERTY_TOKEN:
                parsePrivateProperty(&def);
                break;
            case ENUM: {
                EnumDef enumDef;
                if (parseEnum(&enumDef))
                    def.enumList += enumDef;
            } break;
            case SEMIC:
            case COLON:
                break;
            default:
                FunctionDef funcDef;
                funcDef.access = access;
                const int rewind = index--;
                if (parseMaybeFunction(&def, &funcDef)) {
                    if (funcDef.isConstructor) {
                        if ((access == FunctionDef::Public) && funcDef.isInvokable) {
                            def.constructorList += funcDef;
                            def.constructorList.clone(&funcDef);
                        }
                    } else if (funcDef.isDestructor) {
                        // don't care about destructors
                    } else {
                        def.members += funcDef;
                        if (access == FunctionDef::Public)
                            def.publicList += funcDef;
                        if (funcDef.isSlot) {
                            def.slotList += funcDef;
                            def.slotList.clone(&funcDef);
                            if (funcDef.revision > 0)
                                ++def.revisionedMethods;
                        } else if (funcDef.isSignal) {
                            def.signalList += funcDef;
                            def.signalList.clone(&funcDef);
                            if (funcDef.revision > 0)
                                ++def.revisionedMethods;
                        } else if (funcDef.isInvokable) {
                            def.methodList += funcDef;
                            def.methodList.clone(&funcDef);
                            if (funcDef.revision > 0)
                                ++def.revisionedMethods;
                        }
                    }
                } else {
                    index = rewind;
                }
            }
        }

        next(RBRACE);

        if (!def.hasQObject && !def.hasQGadget && def.signalList.isEmpty() && def.slotList.isEmpty()
            && def.propertyList.isEmpty() && def.enumDeclarations.isEmpty())
            return false; // no meta object code required


        if (!def.hasQObject && !def.hasQGadget)
            error("Class declaration lacks Q_OBJECT macro.");

        // Add meta tags to the plugin meta data:
        if (!def.pluginData.iid.isEmpty())
            def.pluginData.metaArgs = metaArgs;

        checkSuperClasses(&def);
        checkProperties(&def);

        //prefix types which are defined inside of local-class braces
        //  with local-class as namesapce
        const QByteArray localName = def.classname; {
            NamespaceMapper handler(&def, localName);
            handler.mapList(&def.signalList);
            handler.mapList(&def.slotList);
        }

        classList += def;
        QSet<QByteArray> &classHash = def.hasQObject ? knownQObjectClasses : knownGadgets;
        classHash.insert(def.classname);
        classHash.insert(def.qualified);

#if QT_REMOTE
        // Generate Q_REMOTE_CONTROLLER class.
        if(def.hasQRemote) {
            def.isRemote = true;
            //change class-name but keep backup
            def.classname.append(REMOTE_SUFFIX.data(), REMOTE_SUFFIX.size());
            def.qualified.append(REMOTE_SUFFIX.data(), REMOTE_SUFFIX.size());

            SuperClasses::iterator it = def.superclassList.begin();
            SuperClasses::iterator end = def.superclassList.end();
            //suffix super-classes with "Remote"
            //  like from "QObject" into "QObjectRemote"
            it = def.superclassList.begin();
            for(; it != end; it++) {
                Superclass *sc = &*it;
                if(sc->className == STR_REMOTE_OBJECT) {
                    this->seek(remoteMacroOffset);
                    this->error("service should not inherit QObjectRemote");
                }
                sc->className.append(REMOTE_SUFFIX.data(), REMOTE_SUFFIX.size());
            }
            //remove super-classes that do not have Q_REMOTE macro
            //  but include required header for super-classes which have it
            SuperClasses newSuperClasses;
            it = def.superclassList.begin();
            if(it != end) {
                if((*it).className == STR_REMOTE_OBJECT)
                    newSuperClasses += *it++;
            }
            for(; it != end; it++) {
                const Superclass *sc = &*it;
                RemoteFileMap::iterator iFile = knownRemoteFiles.find(sc->className);
                if(iFile != knownRemoteFiles.end()) {
                    this->remote_includeFromLocal(iFile.value());
                    newSuperClasses += *sc;
                }
            }
            def.superclassList = newSuperClasses;

            // Excludes nested types (enums and inner-classes),
            // but we allow enums (so about 1KB extra per class).
            #if 0
                def.enumDeclarations.clear();
                def.enumList.clear();
            #endif
            def.classList.clear();

            //wraps types which have Q_REMOTE macro into "QRef"
            //  also includes any header required for the new type
            RemoteMapper wrapper(this);
            wrapper.mapList(&def.signalList);
            wrapper.mapList(&def.slotList);

            //at last register in known Class list and Remote files
            classList += def;
            knownRemoteFiles.insert(def.classname, currentFilenames.last());
            knownRemoteFiles.insert(def.qualified, currentFilenames.last());
        }
#endif // QT_REMOTE
        return true;
    }
    return false;
}

void Moc::parse()
{
    ParseState state;
    while (hasNext()) {
        Token t = next();

        if(parseUntilClass(t, &state))
            parseQObject(&t, state);
    }
}

static void findRequiredContainers(ClassDef *cdef, QSet<QByteArray> *requiredQtContainers)
{
    static const QVector<QByteArray> candidates = QVector<QByteArray>()
#define STREAM_SMART_POINTER(SMART_POINTER) << #SMART_POINTER
        QT_FOR_EACH_AUTOMATIC_TEMPLATE_SMART_POINTER(STREAM_SMART_POINTER)
#undef STREAM_SMART_POINTER
#define STREAM_1ARG_TEMPLATE(TEMPLATENAME) << #TEMPLATENAME
        QT_FOR_EACH_AUTOMATIC_TEMPLATE_1ARG(STREAM_1ARG_TEMPLATE)
#undef STREAM_1ARG_TEMPLATE
        ;

    for (int i = 0; i < cdef->propertyList.count(); ++i) {
        const PropertyDef &p = cdef->propertyList.at(i);
        foreach (const QByteArray &candidate, candidates) {
            if (p.type.contains(candidate + "<"))
                requiredQtContainers->insert(candidate);
        }
    }

    QVector<FunctionDef> allFunctions = cdef->slotList + cdef->signalList + cdef->methodList;

    for (int i = 0; i < allFunctions.count(); ++i) {
        const FunctionDef &f = allFunctions.at(i);
        foreach (const ArgumentDef &arg, f.arguments) {
            foreach (const QByteArray &candidate, candidates) {
                if (arg.normalizedType().contains(candidate + "<"))
                    requiredQtContainers->insert(candidate);
            }
        }
    }
}

QByteArray Moc::printHeader(FILE *out)
{
    QDateTime dt = QDateTime::currentDateTime();
    QByteArray dstr = dt.toString().toLatin1();
    QByteArray fn = filename;
    int i = filename.length()-1;
    while (i>0 && filename[i-1] != '/' && filename[i-1] != '\\')
        --i;                                // skip path
    if (i >= 0)
        fn = filename.mid(i);
    fprintf(out, "/****************************************************************************\n"
                 "** Meta object code from reading C++ file '%s'\n**\n" , fn.constData());
    fprintf(out, "** Created: %s\n"
                 "**      by: The XD Meta Object Compiler version %d (Qt %s)\n"
                 "**\n" , dstr.constData(), mocOutputRevision, QT_VERSION_STR);
    fprintf(out, "** WARNING! All changes made in this file will be lost!\n"
                 "*****************************************************************************/\n\n");
    return fn;
}

void Moc::printInclude(FILE *out)
{
    if (!noInclude) {
        if (includePath.size() && !includePath.endsWith('/'))
            includePath += '/';
        if (includePath == "./")
            includePath = QByteArray();

        // Maybe required info.
        QFileInfo inputInfo(QFile::decodeName(filename));
        const QString &inputBaseName = inputInfo.fileName();

        for (int i = 0; i < includeFiles.size(); ++i) {
            QByteArray inc = includeFiles.at(i);
            QByteArray link;
            if (inc[0] != '<' && inc[0] != '"') {
                if (includePath.size())
                    inc.prepend(includePath);
                link = linkToParent(inc, outputFileName);
                // Prefers quotes (never "<" and ">" syntax, since
                // path should be either relative or absolute).
                inc = '\"' + link + '\"';
            }

            // Skip including input-file (if generating remote-controller).
            if (isRemoteGen && ! inputBaseName.isEmpty()) {
                if (link.isEmpty()) {
                    link = linkToParent(QByteArray::fromRawData(
                        inc.constData(), inc.length() - 2
                    ), outputFileName);
                }
                const QString &linkActual = QFile::decodeName(link);
                // Link would be same as `completeBaseName` (since remote-controller-file is
                // placed beside input-file).
                if (linkActual.compare(inputBaseName, Q_FS_CASE) == 0) {
                    continue;
                }
            }

            // Finally, emit the include-line.
            fprintf(out, "#include %s\n", inc.constData());
        }
    }
}

void Moc::generate(FILE *out)
{
    const QByteArray &fn = printHeader(out);

#if QT_REMOTE
    if (requiresRemote) {
        if (isRemoteGen == true) {
            // Includes remote-controller's header into its source-code (e.g.
            // file with "*_remote.h" suffix into file with "*_remote.cpp" suffix).
            fprintf(out, "#include \"%s\"\n\n", linkToParent(remote_headerPath(), outputFileName).data() );
        }
    }
#endif

    printInclude(out);

    if (classList.size() && classList.first().classname == "Qt")
        fprintf(out, "#include <QtCore/qobject.h>\n");

    fprintf(out, "#include <QtCore/qbytearray.h>\n"); // For QByteArrayData
    fprintf(out, "#include <QtCore/qmetatype.h>\n");  // For QMetaType::Type
    if (mustIncludeQPluginH)
        fprintf(out, "#include <QtCore/qplugin.h>\n");

    QSet<QByteArray> requiredQtContainers;
    for (int i = 0; i < classList.size(); ++i) {
        findRequiredContainers(&classList[i], &requiredQtContainers);
    }

    // after finding the containers, we sort them into a list to avoid
    // non-deterministic behavior which may cause rebuilds unnecessarily.
    QList<QByteArray> requiredContainerList = requiredQtContainers.toList();
    std::sort(requiredContainerList.begin(), requiredContainerList.end());

    foreach (const QByteArray &qtContainer, requiredContainerList) {
        fprintf(out, "#include <QtCore/%s>\n", qtContainer.constData());
    }


#if QT_REMOTE
    if (requiresRemote) {
        if(isRemoteGen == false) {
            // Includes remote-controller source-code (e.g. file with "*_remote.cpp" suffix)
            // into service-interface (i.e. file with "moc_*.cpp" or "*.moc" suffix)
            fprintf(out, "\n#define MOC_SERVICE_BUILD");
            fprintf(out, "\n#include \"%s\"\n", linkToParent(remote_sourcePath(), outputFileName).data() );
            fprintf(out, "\n#undef MOC_SERVICE_BUILD\n");
        }
    }
#endif

    fprintf(out, "\n");
    fprintf(out, "#if !defined(Q_MOC_OUTPUT_REVISION)\n"
            "#error \"The header file '%s' doesn't include <QObject>.\"\n", fn.constData());
    fprintf(out, "#elif Q_MOC_OUTPUT_REVISION != %d\n", mocOutputRevision);
    fprintf(out, "#error \"This file was generated using the moc from %s."
            " It\"\n#error \"cannot be used with the include files from"
            " this version of Qt.\"\n#error \"(The moc has changed too"
            " much.)\"\n", QT_VERSION_STR);
    fprintf(out, "#endif\n\n");

    fprintf(out, "QT_BEGIN_MOC_NAMESPACE\n");

    for (int i = 0; i < classList.size(); ++i) {
        ClassDef *c = &classList[i];
#if QT_REMOTE
        if(c->isRemote != isRemoteGen)
            continue;
#endif
        Generator generator(*this, c, metaTypes, out);
        generator.generateCode();
    }

    fprintf(out, "QT_END_MOC_NAMESPACE\n");
}

FILE *Moc::rawOpenForOutput(const QString &filePath, const QByteArray &pathEncoded) {
    const bool wasExisting = QFileInfo::exists(filePath);

    const char *path = pathEncoded.constData();
    FILE *out = 0;
#if defined(_MSC_VER) && _MSC_VER >= 1400
    if (fopen_s(&out, path, "w"))
#else
    out = fopen(path, "w"); // create output file
    if (!out)
#endif
    {
        return Q_NULLPTR;
    }

    if ( ! wasExisting) {
        createdFiles << MocFile(out, filePath);
    }
    return out;
}

void Moc::closeOutput(FILE *out)
{
    bool unmanaged = true;

    const int count = createdFiles.count();
    for(int i = 0; i < count; ++i) {
        MocFile entry = createdFiles.at(i);
        if (entry.raw == out) {
            entry.close();
            unmanaged = false;
        }
    }

    if (unmanaged) {
        fclose(out);
    }
}

QString filePathExtension(const QByteArray &filename, const QLatin1String &suffix) {
    QString path;
    path.reserve(filename.size() * 2 + 22);
    QFileInfo outFileInfo(QDir::current(), QFile::decodeName(filename));
    QString dir = outFileInfo.dir().canonicalPath();
    path.append(dir);
    path.append(QLatin1Char('/'));
    path.append(outFileInfo.completeBaseName());
    path.append(suffix);
    return path;
}

QByteArray Moc::remote_headerPath()
{
    return QFile::encodeName( filePathExtension(filename, QLatin1Literal("_remote.h")) );
}
QByteArray Moc::remote_sourcePath()
{
    return QFile::encodeName( filePathExtension(filename, QLatin1Literal("_remote.cpp")) );
}

void Moc::remote_includeFromLocal(const QByteArray &path)
{
    //convert header path from "*.h" to "*_remote.h"
    const QByteArray &inc = QFile::encodeName( filePathExtension(path, QLatin1Literal("_remote.h")) );
    remote_include(inc);
}

void Moc::remote_include(const QByteArray &path)
{
    QByteArray inc = path;
    inc.replace('\\', '/');
    //add header to output-file if not same as itself and not duplicate
    if(!remoteIncludes.contains(inc) && inc != outputFileName) {
        //xd("include" << QFile::decodeName(rfi.value()));
        remoteIncludes.append(inc);
    }
}

bool Moc::remote_generate()
{
    outputFileName = remote_headerPath();
    FILE *remoteOut = openForOutput(outputFileName);
    if(!remoteOut) return false;
    //we need to call "generateRemoteHeader" before "generate"
    //  since it changes "ClassDef"
    isRemoteGen = true;
    remote_generateHeader(remoteOut);
    isRemoteGen = false;
    closeOutput(remoteOut);

    //generate remote object *.cpp source
    outputFileName = remote_sourcePath();
    remoteOut = openForOutput(outputFileName);
    if(!remoteOut) return false;
    isRemoteGen = true;
    generate(remoteOut);
    isRemoteGen = false;
    closeOutput(remoteOut);

    return true;
}

void Moc::remote_generateHeader(FILE *out)
{
    const QByteArray &fn = printHeader(out);

    //replace anything but "a" to "z" and "0" to "9" with "_"
    QByteArray fid = QString::fromUtf8(fn)
            .replace(QRegExp(QLatin1String("[^a-z0-9]"), Qt::CaseInsensitive), QLatin1String("_")).toUpper().toUtf8();
    fid.prepend("MOC_");
    fid.append("_REMOTE");

    // TRACE/moc BugFix 1: added start/end macro to support multiple includes #1.
    fprintf(out, "\n#ifndef %s", fid.constData());
    fprintf(out, "\n#define %s\n\n", fid.constData());

    //include Service-Interface header required for "typedef MyService LocalType;"
    fprintf(out, "#include \"%s\"\n", linkToParent(filename, outputFileName).data());
    foreach(const QByteArray &inc, this->remoteIncludes) {
        fprintf(out, "#include \"%s\"\n", linkToParent(inc, outputFileName).data());
    }

    // We include "qremoteobject" at last to allow user-headers to include Qt-headers with any required-order
    fprintf(out, "\n#include <QtRemote/qremoteobject.h>\n\n");

    //create list of forward-declarations
    ForwardList::const_iterator it = this->remoteForwards.constBegin();
    ForwardList::const_iterator end = this->remoteForwards.constEnd();
    for (; it != end; ++it) {
        const ClassBasicDef &forward = it.value();
        if(forward.isUsed) {
            fprintf(out, "\n%s %s%s;\n"
                    , forward.isStruct ? "struct" : "class"
                    , forward.classname.constData()
                    , REMOTE_SUFFIX.data()
                );
        }
    }

    //at last generate all Q_REMOTE_CONTROLLER classes
    for (int i = 0; i < classList.size(); ++i) {
        ClassDef *c = &classList[i];
        if(!c->isRemote)
            continue;

        Generator generator(*this, c, metaTypes, out);
        generator.remote_generateClass();
    }

    // TRACE/moc BugFix 1: added start/end macro to support multiple includes #2.
    fprintf(out, "\n\n#endif //%s\n", fid.constData());
}

void Moc::parseSlots(ClassDef *def, FunctionDef::Access access)
{
    int defaultRevision = -1;
    testFunctionRevisionRaw(&defaultRevision);

    next(COLON);
    while (def->contains(this->nextOffset()) && hasNext()) {
        switch (next()) {
        case PUBLIC:
        case PROTECTED:
        case PRIVATE:
        case Q_SIGNALS_TOKEN:
        case Q_SLOTS_TOKEN:
            prev();
            return;
        case SEMIC:
            continue;
        case FRIEND:
            until(SEMIC);
            continue;
        case USING:
            error("'using' directive not supported in 'slots' section");
        default:
            prev();
        }

        FunctionDef funcDef;
        funcDef.access = access;
        if (!parseFunction(&funcDef))
            continue;
        if (funcDef.revision > 0) {
            ++def->revisionedMethods;
        } else if (defaultRevision != -1) {
            funcDef.revision = defaultRevision;
            ++def->revisionedMethods;
        }
        def->members += funcDef;
        def->slotList += funcDef;
        def->slotList.clone(&funcDef);
    }
}

void Moc::parseSignals(ClassDef *def)
{
    int defaultRevision = -1;
    testFunctionRevisionRaw(&defaultRevision);

    next(COLON);
    while (def->contains(this->nextOffset()) && hasNext()) {
        switch (next()) {
        case PUBLIC:
        case PROTECTED:
        case PRIVATE:
        case Q_SIGNALS_TOKEN:
        case Q_SLOTS_TOKEN:
            prev();
            return;
        case SEMIC:
            continue;
        case FRIEND:
            until(SEMIC);
            continue;
        case USING:
            error("'using' directive not supported in 'signals' section");
        default:
            prev();
        }
        FunctionDef funcDef;
        funcDef.access = FunctionDef::Public;
        parseFunction(&funcDef);
        if (funcDef.isVirtual)
            warning("Signals cannot be declared virtual");
        if (funcDef.inlineCode)
            error("Not a signal declaration");
        if (funcDef.revision > 0) {
            ++def->revisionedMethods;
        } else if (defaultRevision != -1) {
            funcDef.revision = defaultRevision;
            ++def->revisionedMethods;
        }
        def->members += funcDef;
        def->signalList += funcDef;
        def->signalList.clone(&funcDef);
    }
}

void Moc::createPropertyDef(PropertyDef &propDef)
{
    QByteArray type = parseType().name;
    if (type.isEmpty())
        error();
    propDef.designable = propDef.scriptable = propDef.stored = "true";
    propDef.user = "false";
    /*
      The Q_PROPERTY construct cannot contain any commas, since
      commas separate macro arguments. We therefore expect users
      to type "QMap" instead of "QMap<QString, QVariant>". For
      coherence, we also expect the same for
      QValueList<QVariant>, the other template class supported by
      QVariant.
    */
    type = Type::normalize(type);
    if (type == "QMap")
        type = "QMap<QString,QVariant>";
    else if (type == "QValueList")
        type = "QValueList<QVariant>";
    else if (type == "LongLong")
        type = "qlonglong";
    else if (type == "ULongLong")
        type = "qulonglong";

    propDef.type = type;

    next();
    propDef.name = lexem();
    while (test(IDENTIFIER)) {
        QByteArray l = lexem();
        if (l[0] == 'C' && l == "CONSTANT") {
            propDef.constant = true;
            continue;
        } else if(l[0] == 'F' && l == "FINAL") {
            propDef.final = true;
            continue;
        }

        QByteArray v, v2;
        if (test(LPAREN)) {
            v = lexemUntil(RPAREN);
            v = v.mid(1, v.length() - 2); // removes the '(' and ')'
        } else if (test(INTEGER_LITERAL)) {
            v = lexem();
            if (l != "REVISION")
                error(1);
        } else {
            next(IDENTIFIER);
            v = lexem();
            if (test(LPAREN))
                v2 = lexemUntil(RPAREN);
            else if (v != "true" && v != "false")
                v2 = "()";
        }
        switch (l[0]) {
        case 'M':
            if (l == "MEMBER")
                propDef.member = v;
            else
                error(2);
            break;
        case 'R':
            if (l == "READ")
                propDef.read = v;
            else if (l == "RESET")
                propDef.reset = v + v2;
            else if (l == "REVISION") {
                bool ok = false;
                propDef.revision = v.toInt(&ok);
                if (!ok || propDef.revision < 0)
                    error(1);
            } else
                error(2);
            break;
        case 'S':
            if (l == "SCRIPTABLE")
                propDef.scriptable = v + v2;
            else if (l == "STORED")
                propDef.stored = v + v2;
            else
                error(2);
            break;
        case 'W': if (l != "WRITE") error(2);
            propDef.write = v;
            break;
        case 'D': if (l != "DESIGNABLE") error(2);
            propDef.designable = v + v2;
            break;
        case 'E': if (l != "EDITABLE") error(2);
            propDef.editable = v + v2;
            break;
        case 'N': if (l != "NOTIFY") error(2);
            propDef.notify = v;
            break;
        case 'U': if (l != "USER") error(2);
            propDef.user = v + v2;
            break;
        default:
            error(2);
        }
    }
    if (propDef.read.isNull() && propDef.member.isNull()) {
        QByteArray msg;
        msg += "Property declaration ";
        msg += propDef.name;
        msg += " has no READ accessor function or associated MEMBER variable. The property will be invalid.";
        warning(msg.constData());
    }
    if (propDef.constant && !propDef.write.isNull()) {
        QByteArray msg;
        msg += "Property declaration ";
        msg += propDef.name;
        msg += " is both WRITEable and CONSTANT. CONSTANT will be ignored.";
        propDef.constant = false;
        warning(msg.constData());
    }
    if (propDef.constant && !propDef.notify.isNull()) {
        QByteArray msg;
        msg += "Property declaration ";
        msg += propDef.name;
        msg += " is both NOTIFYable and CONSTANT. CONSTANT will be ignored.";
        propDef.constant = false;
        warning(msg.constData());
    }
}

void Moc::parseProperty(ClassDef *def)
{
    next(LPAREN);
    PropertyDef propDef;
    createPropertyDef(propDef);
    next(RPAREN);

    if(!propDef.notify.isEmpty())
        def->notifyableProperties++;
    if (propDef.revision > 0)
        ++def->revisionedProperties;
    def->propertyList += propDef;
}

void Moc::parsePluginData(ClassDef *def)
{
    next(LPAREN);
    QByteArray metaData;
    while (test(IDENTIFIER)) {
        QByteArray l = lexem();
        if (l == "IID") {
            next(STRING_LITERAL);
            def->pluginData.iid = unquotedLexem();
        } else if (l == "FILE") {
            next(STRING_LITERAL);
            QByteArray metaDataFile = unquotedLexem();
            QFileInfo fi(QFileInfo(QString::fromLocal8Bit(currentFilenames.top().constData())).dir(), QString::fromLocal8Bit(metaDataFile.constData()));
            for (int j = 0; j < includes.size() && !fi.exists(); ++j) {
                const IncludePath &p = includes.at(j);
                if (p.isFrameworkPath)
                    continue;

                fi.setFile(QString::fromLocal8Bit(p.path.constData()), QString::fromLocal8Bit(metaDataFile.constData()));
                // try again, maybe there's a file later in the include paths with the same name
                if (fi.isDir()) {
                    fi = QFileInfo();
                    continue;
                }
            }
            if (!fi.exists()) {
                QByteArray msg;
                msg += "Plugin Metadata file ";
                msg += lexem();
                msg += " does not exist. Declaration will be ignored";
                error(msg.constData());
                return;
            }
            QFile file(fi.canonicalFilePath());
            file.open(QFile::ReadOnly);
            metaData = file.readAll();
        }
    }

    if (!metaData.isEmpty()) {
        def->pluginData.metaData = QJsonDocument::fromJson(metaData);
        if (!def->pluginData.metaData.isObject()) {
            QByteArray msg;
            msg += "Plugin Metadata file ";
            msg += lexem();
            msg += " does not contain a valid JSON object. Declaration will be ignored";
            warning(msg.constData());
            def->pluginData.iid = QByteArray();
            return;
        }
    }

    mustIncludeQPluginH = true;
    next(RPAREN);
}

void Moc::parsePrivateProperty(ClassDef *def)
{
    next(LPAREN);
    PropertyDef propDef;
    next(IDENTIFIER);
    propDef.inPrivateClass = lexem();
    while (test(SCOPE)) {
        propDef.inPrivateClass += lexem();
        next(IDENTIFIER);
        propDef.inPrivateClass += lexem();
    }
    // also allow void functions
    if (test(LPAREN)) {
        next(RPAREN);
        propDef.inPrivateClass += "()";
    }

    next(COMMA);

    createPropertyDef(propDef);

    if(!propDef.notify.isEmpty())
        def->notifyableProperties++;
    if (propDef.revision > 0)
        ++def->revisionedProperties;

    def->propertyList += propDef;
}

void Moc::parseEnumOrFlag(ClassDef *def, bool isFlag)
{
    next(LPAREN);
    QByteArray identifier;
    while (test(IDENTIFIER)) {
        identifier = lexem();
        while (test(SCOPE) && test(IDENTIFIER)) {
            identifier += "::";
            identifier += lexem();
        }
        def->enumDeclarations[identifier] = isFlag;
    }
    next(RPAREN);
}

void Moc::parseFlag(ClassDef *def)
{
    next(LPAREN);
    QByteArray flagName, enumName;
    while (test(IDENTIFIER)) {
        flagName = lexem();
        while (test(SCOPE) && test(IDENTIFIER)) {
            flagName += "::";
            flagName += lexem();
        }
    }
    next(COMMA);
    while (test(IDENTIFIER)) {
        enumName = lexem();
        while (test(SCOPE) && test(IDENTIFIER)) {
            enumName += "::";
            enumName += lexem();
        }
    }

    def->flagAliases.insert(enumName, flagName);
    next(RPAREN);
}

void Moc::parseClassInfo(ClassDef *def)
{
    next(LPAREN);
    ClassInfoDef infoDef;

    if (skipUnless(STRING_LITERAL)) return;
    infoDef.name = symbol().unquotedLexem();
    if (skipUnless(COMMA)) return;
    if (test(STRING_LITERAL)) {
        infoDef.value = symbol().unquotedLexem();
    } else {
        // support Q_CLASSINFO("help", QT_TR_NOOP("blah"))
        next(IDENTIFIER);
        if (skipUnless(LPAREN)) return;
        if (skipUnless(STRING_LITERAL)) return;
        infoDef.value = symbol().unquotedLexem();
        if (skipUnless(RPAREN)) return;
    }
    if (skipUnless(RPAREN)) return;
    def->classInfoList += infoDef;
}

void Moc::parseInterfaces(ClassDef *def)
{
    next(LPAREN);
    while (test(IDENTIFIER)) {
        QList<ClassDef::Interface> iface;
        iface += ClassDef::Interface(lexem());
        while (test(SCOPE)) {
            iface.last().className += lexem();
            next(IDENTIFIER);
            iface.last().className += lexem();
        }
        while (test(COLON)) {
            next(IDENTIFIER);
            iface += ClassDef::Interface(lexem());
            while (test(SCOPE)) {
                iface.last().className += lexem();
                next(IDENTIFIER);
                iface.last().className += lexem();
            }
        }
        // resolve from classnames to interface ids
        for (int i = 0; i < iface.count(); ++i) {
            const QByteArray iid = interface2IdMap.value(iface.at(i).className);
            if (iid.isEmpty())
                error("Undefined interface");

            iface[i].interfaceId = iid;
        }
        def->interfaceList += iface;
    }
    next(RPAREN);
}

void Moc::parseDeclareInterface()
{
    next(LPAREN);
    QByteArray interface;
    next(IDENTIFIER);
    interface += lexem();
    while (test(SCOPE)) {
        interface += lexem();
        next(IDENTIFIER);
        interface += lexem();
    }
    next(COMMA);
    QByteArray iid;
    if (test(STRING_LITERAL)) {
        iid = lexem();
    } else {
        next(IDENTIFIER);
        iid = lexem();
    }
    interface2IdMap.insert(interface, iid);
    next(RPAREN);
}

void Moc::parseDeclareMetatype()
{
    next(LPAREN);
    QByteArray typeName = lexemUntil(RPAREN);
    typeName.remove(0, 1);
    typeName.chop(1);
    metaTypes.append(typeName);
}

void Moc::parseSlotInPrivate(ClassDef *def, FunctionDef::Access access)
{
    next(LPAREN);
    FunctionDef funcDef;
    next(IDENTIFIER);
    funcDef.inPrivateClass = lexem();
    // also allow void functions
    if (test(LPAREN)) {
        next(RPAREN);
        funcDef.inPrivateClass += "()";
    }
    next(COMMA);
    funcDef.access = access;
    parseFunction(&funcDef, true);
    def->members += funcDef;
    def->slotList += funcDef;
    def->slotList.clone(&funcDef);
    if (funcDef.revision > 0)
        ++def->revisionedMethods;

}

QByteArray Moc::lexemUntil(Token target)
{
    int from = index;
    until(target);
    QByteArray s;
    while (from <= index) {
        QByteArray n = symbols.at(from++-1).lexem();
        if (s.size() && n.size()) {
            char prev = s.at(s.size()-1);
            char next = n.at(0);
            if ((is_ident_char(prev) && is_ident_char(next))
                || (prev == '<' && next == ':')
                || (prev == '>' && next == '>'))
                s += ' ';
        }
        s += n;
    }
    return s;
}

bool Moc::until(Token target) {
    int braceCount = 0;
    int brackCount = 0;
    int parenCount = 0;
    int angleCount = 0;
    if (index) {
        switch(symbols.at(index-1).token) {
        case LBRACE: ++braceCount; break;
        case LBRACK: ++brackCount; break;
        case LPAREN: ++parenCount; break;
        case LANGLE: ++angleCount; break;
        default: break;
        }
    }

    //when searching commas within the default argument, we should take care of template depth (anglecount)
    // unfortunatelly, we do not have enough semantic information to know if '<' is the operator< or
    // the beginning of a template type. so we just use heuristics.
    int possible = -1;

    while (index < symbols.size()) {
        Token t = symbols.at(index++).token;
        switch (t) {
        case LBRACE: ++braceCount; break;
        case RBRACE: --braceCount; break;
        case LBRACK: ++brackCount; break;
        case RBRACK: --brackCount; break;
        case LPAREN: ++parenCount; break;
        case RPAREN: --parenCount; break;
        case LANGLE:
            if (parenCount == 0 && braceCount == 0 && parenCount == 0)
                ++angleCount;
          break;
        case RANGLE:
            if (parenCount == 0 && braceCount == 0)
                --angleCount;
          break;
        case GTGT:
            if (parenCount == 0 && braceCount == 0) {
                angleCount -= 2;
                t = RANGLE;
            }
            break;
        default: break;
        }
        if (t == target
            && braceCount <= 0
            && brackCount <= 0
            && parenCount <= 0
            && (target != RANGLE || angleCount <= 0)) {
            if (target != COMMA || angleCount <= 0)
                return true;
            possible = index;
        }

        if (target == COMMA && t == EQ && possible != -1) {
            index = possible;
            return true;
        }

        if (braceCount < 0 || brackCount < 0 || parenCount < 0
            || (target == RANGLE && angleCount < 0)) {
            --index;
            break;
        }

        if (braceCount <= 0 && t == SEMIC) {
            // Abort on semicolon. Allow recovering bad template parsing (QTBUG-31218)
            break;
        }
    }

    if(target == COMMA && angleCount != 0 && possible != -1) {
        index = possible;
        return true;
    }

    return false;
}

void Moc::checkSuperClasses(ClassDef *def)
{
    const QByteArray firstSuperclass = def->superclassList.value(0).className;

    if (!knownQObjectClasses.contains(firstSuperclass)) {
        // enable once we /require/ include paths
#if 0
        QByteArray msg;
        msg += "Class ";
        msg += def->className;
        msg += " contains the Q_OBJECT macro and inherits from ";
        msg += def->superclassList.value(0);
        msg += " but that is not a known QObject subclass. You may get compilation errors.";
        warning(msg.constData());
#endif
        return;
    }
    for (int i = 1; i < def->superclassList.count(); ++i) {
        const QByteArray superClass = def->superclassList.at(i).className;
        if (knownQObjectClasses.contains(superClass)) {
            QByteArray msg;
            msg += "Class ";
            msg += def->classname;
            msg += " inherits from two QObject subclasses ";
            msg += firstSuperclass;
            msg += " and ";
            msg += superClass;
            msg += ". This is not supported!";
            warning(msg.constData());
        }

        if (interface2IdMap.contains(superClass)) {
            bool registeredInterface = false;
            for (int i = 0; i < def->interfaceList.count(); ++i)
                if (def->interfaceList.at(i).first().className == superClass) {
                    registeredInterface = true;
                    break;
                }

            if (!registeredInterface) {
                QByteArray msg;
                msg += "Class ";
                msg += def->classname;
                msg += " implements the interface ";
                msg += superClass;
                msg += " but does not list it in Q_INTERFACES. qobject_cast to ";
                msg += superClass;
                msg += " will not work!";
                warning(msg.constData());
            }
        }
    }
}

void Moc::checkProperties(ClassDef *cdef)
{
    //
    // specify get function, for compatibiliy we accept functions
    // returning pointers, or const char * for QByteArray.
    //
    QSet<QByteArray> definedProperties;
    for (int i = 0; i < cdef->propertyList.count(); ++i) {
        PropertyDef &p = cdef->propertyList[i];
        if (p.read.isEmpty() && p.member.isEmpty())
            continue;
        if (definedProperties.contains(p.name)) {
            QByteArray msg = "The property '" + p.name + "' is defined multiple times in class " + cdef->classname + ".";
            warning(msg.constData());
        }
        definedProperties.insert(p.name);

        for (int j = 0; j < cdef->publicList.count(); ++j) {
            const FunctionDef &f = cdef->publicList.at(j);
            if (f.name != p.read)
                continue;
            if (!f.isConst) // get  functions must be const
                continue;
            if (f.arguments.size()) // and must not take any arguments
                continue;
            PropertyDef::Specification spec = PropertyDef::ValueSpec;
            QByteArray tmp = f.type.toNormalized();
            if (p.type == "QByteArray" && tmp == "const char *")
                tmp = "QByteArray";
            if (tmp.left(6) == "const ")
                tmp = tmp.mid(6);
            if (p.type != tmp && tmp.endsWith('*')) {
                tmp.chop(1);
                spec = PropertyDef::PointerSpec;
            } else if (f.type.name.endsWith('&')) { // raw type, not normalized type
                spec = PropertyDef::ReferenceSpec;
            }
            if (p.type != tmp)
                continue;
            p.gspec = spec;
            break;
        }
        if(!p.notify.isEmpty()) {
            int notifyId = -1;
            for (int j = 0; j < cdef->signalList.count(); ++j) {
                const FunctionDef &f = cdef->signalList.at(j);
                if(f.name != p.notify) {
                    continue;
                } else {
                    notifyId = j /* Signal indexes start from 0 */;
                    break;
                }
            }
            p.notifyId = notifyId;
            if (notifyId == -1) {
                QByteArray msg = "NOTIFY signal '" + p.notify + "' of property '" + p.name
                        + "' does not exist in class " + cdef->classname + ".";
                error(msg.constData());
            }
        }
    }
}

int Moc::onExit(int code)
{
    if (code != 0) {
        const int count = createdFiles.count();
        for(int i = 0; i < count; ++i) {
            createdFiles.ptr(i)->remove();
        }
    }
    return super::onExit(code);
}

const char *FunctionDef::stringFromAccess(FunctionDef::Access v)
{
    switch (v) {
    case Private: return "private";
    case Protected: return "protected";
    case Public: return "public";
    }
    return 0;
}

bool ClassDef::inheritsFrom(const QByteArray &name)
{
    const int count = superclassList.count();
    for (int i = 0; i < count; ++i) {
        const Superclass &base = superclassList.at(i);
        if (base.hasName(name)) {
            return true;
        }
    }

    return false;
}

FunctionList::iterator ClassDef::findMember(const QByteArray &name)
{
    QByteArray key = name;
    {
        int pos = key.indexOf('(');
        if( pos >= 0 )
            key.chop(key.length() - pos);
    }

    for (int i = 0; i < members.count(); ++i) {
        FunctionDef *def = &members[i];
        if(def->name == key)
            return members.begin() + i;
    }
    return members.end();
}

void ClassDef::filterEnumList()
{
    EnumList enumList;
    for (int i = 0; i < this->enumList.count(); ++i) {
        EnumDef def = this->enumList.at(i);
        if (this->enumDeclarations.contains(def.name)) {
            enumList += def;
        }
        QByteArray alias = this->flagAliases.value(def.name);
        if (this->enumDeclarations.contains(alias)) {
            def.name = alias;
            enumList += def;
        }
    }
    this->enumList = enumList;
}

QByteArray ParseState::namespacePrefix(int offset, const QByteArray &className) const {
    QByteArray r = className;
    for (int i = this->namespaceList.size() - 1; i >= 0; --i) {
        const NamespaceDef &current = this->namespaceList.at(i);
        if (current.contains(offset))
            r.prepend(current.name + "::");
    }
    return r;
}

// only moc needs this function
QByteArray Type::normalize(const QByteArray &b, bool fixScope)
{
#ifdef QT_DEBUG
    const char *bData = b.constData();
    Q_UNUSED(bData) // Debug porpuse only.
#endif
    const QByteArray &result = normalizeType(b, fixScope);
    //Q_ASSERT(!result.isEmpty());
    return result;
}

bool Type::setClass(const QByteArray &clazz) {
    if(rawClass.isValid(1)) {
        QByteArray r;
        r.reserve(rawName.size() - rawClass.length + clazz.size());
        r.append(rawName.constData(), rawClass.start);
        r.append(clazz);
        r.append(rawName.constData() + rawClass.end(), rawName.size() - rawClass.end());

        if(name == rawName) //update "name" only when unchanged (not hard-coded)
            name = r;
        rawName = r;
        rawClass.length = clazz.size();
        return true;
    }
    return false; //class was never appended or was removed
}

bool Type::mapTo(ClassDef *target, const QByteArray &localName)
{
    bool isChanged = false;

    QByteArray clazz = this->getClass();
    bool isNested = target->enumList.contains(clazz)
            || target->classList.contains(clazz);
    if(isNested) {
        clazz.prepend("::", 2);
        clazz.prepend(localName);
        isChanged = this->setClass(clazz);
    }
    return isChanged;
}

QByteArray ArgumentDef::normalizedType() const
{
    return Type::normalize(QByteArray(this->type.name + ' ' + this->rightType));
}

QByteArray ArgumentDef::typeNameForCast() const
{
    return Type::normalize(QByteArray(noRef(this->type.name) + "(*)" + this->rightType));
}


void FunctionDef::MetaData::set(const FunctionDef &f, int type)
{
#if MOC_INCLUDE_SIGNATURE
    sig = f.name + '(';
    arguments.clear();

    for (int j = 0; j < f.arguments.count(); ++j) {
        const ArgumentDef &a = f.arguments.at(j);
        if (j) {
            sig += ',';
            arguments += ',';
        }
        sig += a.normalizedType();
        arguments += a.name;
    }
    sig += ')';
#endif

    comment = QByteArray();
    flags = type;
    if (f.access == FunctionDef::Private) {
        flags |= AccessPrivate;
        comment.append("Private");
    } else if (f.access == FunctionDef::Public) {
        flags |= AccessPublic;
        comment.append("Public");
    } else if (f.access == FunctionDef::Protected) {
        flags |= AccessProtected;
        comment.append("Protected");
    }

    if (f.isCompat) {
        flags |= MethodCompatibility;
        comment.append(" | MethodCompatibility");
    }
    if (f.isClone) {
        flags |= MethodCloned;
        comment.append(" | MethodCloned");
    }
    if (f.isScriptable) {
        flags |= MethodScriptable;
        comment.append(" | MethodScriptable");
    }

    if (f.revision > 0) {
        flags |= MethodRevisioned;
        comment.append(" | MethodRevisioned");
    }
}


QT_END_NAMESPACE
