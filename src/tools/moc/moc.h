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

#ifndef MOC_H
#define MOC_H

#include "moc-config.h"
#include "parser.h"
#include "moc-file.h"
#include <qstringlist.h>
#include <qmap.h>
#include <qpair.h>
#include <qrange.h>
#include <qjsondocument.h>
#include <qjsonarray.h>
#include <qjsonobject.h>
#include <qfile.h>
#include <stdio.h>
#include <ctype.h>

QT_BEGIN_NAMESPACE

struct QMetaObject;
struct ClassDef;
class Moc;

struct Type
{
    enum ReferenceType { NoReference, Reference, RValueReference, Pointer };

    inline Type()
        : isConst(false), isVolatile(false), isScoped(false)
        , firstToken(NOTOKEN), referenceType(NoReference)
    {}
    inline explicit Type(const QByteArray &_name)
        : name(_name), rawName(name)
        , isConst(false), isVolatile(false), isScoped(false)
        , firstToken(NOTOKEN), referenceType(NoReference)
    {}

    static QByteArray normalize(const QByteArray &s, bool fixScope = false);
    inline static QByteArray normalize(const char *s, bool fixScope = false) { return normalize(QByteArray(s), fixScope); }

    inline const QByteArray &toNormalized() const {
        if (Q_UNLIKELY(m_normalized.isNull())) {
            const_cast<Type *>(this)->m_normalized = Type::normalize(this->name);
        }
        return m_normalized;
    }

    inline const QByteArray &renormalize() {
        m_normalized.clear();
        return toNormalized();
    }

    /// @return The pure-type-name extracted from "rawName",
    /// like "QSharedPointer" from "const QSharedPointer<QObject> &"
    inline QByteArray getClass() const { return rawName.mid(rawClass.start, rawClass.length); }
    inline QByteArray getClassRef() const { return QByteArray::fromRawData(rawName.constData() + rawClass.start, rawClass.length); }
    bool setClass(const QByteArray &clazz);
    inline void appendClass(const QByteArray &clazz) {
        if(rawClass.isNull())
            rawClass.start = name.size();
        rawClass.length += clazz.size();
        name.append(clazz);
    }

    /// @return \c true if mapped/changed.
    bool mapTo(ClassDef *target, const QByteArray &localName);

    Q_ALWAYS_INLINE bool isVoid() const
    {
        // Is empty for constructors, hence considered `void`.
        return this->toNormalized().size() <= 0
                || m_normalized == "void";
    }
    Q_ALWAYS_INLINE bool isNotVoid() const { return ! this->isVoid(); }

public:
    QByteArray name;
    //When used as a return type, the type name may be modified to remove the references.
    // rawName is the type as found in the function signature
    QByteArray rawName;
    /// Location of Pure-Type-Name in "rawName"
    /// (e.g. "const int" equals `QRange(6, 3)`).
    QRange rawClass;

    uint isConst : 1;
    uint isVolatile : 1;
    uint isScoped : 1;
    Token firstToken;
    ReferenceType referenceType;

private:
    QByteArray m_normalized;
};

struct EnumDef
{
    EnumDef() : isEnumClass(false) {}
    QByteArray name;
    QList<QByteArray> values;
    bool isEnumClass; // c++11 enum class
};

struct ArgumentDef
{
    ArgumentDef() : isDefault(false) {}
    Type type;
    QByteArray rightType, name;
    QByteArray normalizedType() const;
    QByteArray typeNameForCast() const; // type name to be used in cast from void * in metacall
    bool isDefault;
};

struct FunctionDef
{
    FunctionDef()
        : returnTypeIsVolatile(false), access(Private), isConst(false)
        , isStatic(false), isVirtual(false), isAbstract(false), inlineCode(false)
        , isClone(false), isCloneRequired(false), isCompat(false)
        , isInvokable(false), isScriptable(false), isSlot(false), isSignal(false), isPrivateSignal(false)
        , isConstructor(false), isDestructor(false)
        , revision(0)
    {}

    /// Return-value's type.
    Type type;
    /// Default return-value used by QT_REMOTE.
    QByteArray defaultValue;
    QByteArray tag;
    QByteArray name;
    bool returnTypeIsVolatile;

    QList<ArgumentDef> arguments;

    enum Access { Private, Protected, Public };
    static const char *stringFromAccess(Access v);
    Access access;
    bool isConst;
    bool isStatic;
    bool isVirtual;
    bool isAbstract;
    bool inlineCode;

    // If both "isClone" and "isCloneRequired" are true this is a clone
    // but should be included in generated header for Q_REMOTE_CONTROLLER
    bool isClone;
    bool isCloneRequired;
    inline bool isRemoteClone() const { return (isClone && isCloneRequired); }

    /// @return \c true if this is NOT directly callable,
    /// hence should get excluded from "qt_static_metacall(...)".
    inline bool isUnreal() const { return (isClone != isCloneRequired); }
    /// @return \c true when generated files should not define this.
    inline bool isIgnored() const { return this->isUnreal() || isAbstract;  }

    QByteArray inPrivateClass;
    bool isCompat;
    bool isInvokable;
    bool isScriptable;
    bool isSlot;
    bool isSignal;
    bool isPrivateSignal;
    bool isConstructor;
    bool isDestructor;

    int revision;

#define MOC_INCLUDE_SIGNATURE 0

    struct MetaData {
        void set(const FunctionDef &, int type);

#if MOC_INCLUDE_SIGNATURE
        QByteArray sig;
        QByteArray arguments;
#endif
        unsigned char flags;
        QByteArray comment;
    };
};

class FunctionList : public QVector<FunctionDef>
{
public:
    inline void clone(FunctionDef *funcDef) {
        while (funcDef->arguments.size() > 0 && funcDef->arguments.last().isDefault) {
            funcDef->isClone = true;
            funcDef->arguments.removeLast();
            this->append(*funcDef);
        }
    }
};

struct PropertyDef
{
    PropertyDef():notifyId(-1), constant(false), final(false), gspec(ValueSpec), revision(0){}
    QByteArray name, type, member, read, write, reset, designable, scriptable, editable, stored, user, notify, inPrivateClass;
    int notifyId;
    bool constant;
    bool final;
    enum Specification  { ValueSpec, ReferenceSpec, PointerSpec };
    Specification gspec;
    bool stdCppSet() const {
        QByteArray s("set");
        s += toupper(name[0]);
        s += name.mid(1);
        return (s == write);
    }
    int revision;
};


struct ClassInfoDef
{
    QByteArray name;
    QByteArray value;
};

struct Superclass {
    inline Superclass() {}
    inline Superclass(const QByteArray &typeName, const FunctionDef::Access &access) : className(typeName), access(access) {}
    inline Superclass(const QPair<QByteArray, FunctionDef::Access> &pair) : className(pair.first), access(pair.second) {}

    /// Ignores any @c namespace prefix.
    inline bool hasName(const QByteArray &name) const {
        const QByteArray &ownName = this->className;
        return ownName.endsWith(name)
            && (ownName.length() == name.length()
                || (ownName.length() >= name.length()
                    && ':' == ownName.at(ownName.length() - 1 - name.length())
                )
            );
    }

    /// @warning Contains namespace, even the `::` global-namespace prefix, but
    /// only if original-header file contains said things.
    QByteArray className;

    FunctionDef::Access access;
};
typedef QList<Superclass> SuperClasses;

class EnumList : public QList<EnumDef> {
    typedef QList<EnumDef> super;
public:
    inline EnumList() {}

    inline super::const_iterator find(QByteArray name) const {
        super::const_iterator it = constBegin();
        const super::const_iterator end = constEnd();
        for(; it != end; ++it) {
            if(it->name == name) {
                break;
            }
        }
        return it;
    }

    inline bool contains(QByteArray name) const {
        return find(name) != constEnd();
    }
};

struct BraceScope {
    inline BraceScope() : begin(0), end(0) {}

    int begin;
    int end;
    /// WARNING: does NOT consider LBRACE or RBRACE as inside of class.
    inline bool contains(int offset) const {
        return offset > this->begin && offset < this->end - 1;
    }
};

struct ClassBasicDef {
    ClassBasicDef()
        : isStruct(false)
        , isUsed(false)
    {}

    /// @warning May not be prefixed by "::" sign.
    inline QByteArray toNamespace() const {
        return qualified.mid(0, qualified.length() - classname.length() - 2);
    }

    inline bool isGlobalNamespaced() const {
        return qualified.length() == classname.length();
    }

    QByteArray classname;
    QByteArray qualified; //full_namespace::and_name
    bool isStruct;
    bool isUsed; //only required by QT_REMOTE
};

struct ClassDef : public ClassBasicDef, public BraceScope {
    ClassDef()
        : hasQObject(false), hasQRemote(false), isRemote(false)
        , hasQGadget(false), notifyableProperties(0)
        , revisionedMethods(0), revisionedProperties(0)
    {}

    SuperClasses superclassList;

    bool inheritsFrom(const QByteArray &name);

    struct Interface
    {
        inline explicit Interface(const QByteArray &_className)
            : className(_className) {}
        QByteArray className;
        QByteArray interfaceId;
    };
    QList<QList<Interface> >interfaceList;

    bool hasQObject;
    bool hasQRemote, isRemote;
    bool hasQGadget;

    struct PluginData {
        QByteArray iid;
        QMap<QString, QJsonArray> metaArgs;
        QJsonDocument metaData;
    } pluginData;

    FunctionList constructorList;
    FunctionList signalList, slotList, methodList,
                 publicList, members;

    FunctionList::iterator findMember(const QByteArray &name);

    int notifyableProperties;
    QList<PropertyDef> propertyList;
    QList<ClassInfoDef> classInfoList;
    QMap<QByteArray, bool> enumDeclarations;
    EnumList enumList;
    /// Filters out undeclared enumerators and sets.
    void filterEnumList();
    QSet<QByteArray> classList;
    QMap<QByteArray, QByteArray> flagAliases;
    int revisionedMethods;
    int revisionedProperties;

};

struct NamespaceDef : BraceScope {
    QByteArray name;
};

struct ParseState {
    inline ParseState() : templateClass(false) {}
    QList<NamespaceDef> namespaceList;
    bool templateClass;

    QByteArray namespacePrefix(int offset, const QByteArray &className = QByteArray()) const;
};

class Moc : public Parser
{
    typedef Parser super;
    static Moc *m_instance;
public:
    inline Moc()
        : noInclude(false), generatedCode(false), mustIncludeQPluginH(false)
    {
        if ( ! m_instance) {
            m_instance = this;
        } else if (this != m_instance) {
            // Note: instead of passing instance to each and everything,
            // we force having no more than a single instance.
            qFatal("Duplicate Moc-class instances.");
        }
    }

    virtual ~Moc();

    static QByteArray filename;
    static QByteArray outputFileName;
    QVector<MocFile> createdFiles;

    typedef QList<QByteArray> IncludeList;

#if QT_REMOTE
    static bool requiresRemote; //this is set in "parse()"
    static bool isRemoteGen; //this is set in "parse()"
    static const QLatin1Literal REMOTE_SUFFIX;
    //key="class name", value="include path"
    typedef QHash<QByteArray, QByteArray> RemoteFileMap;
    RemoteFileMap knownRemoteFiles;
    IncludeList remoteIncludes;
    typedef QHash<QByteArray, ClassBasicDef> ForwardList;
    ForwardList remoteForwards;
#endif //QT_REMOTE

    bool noInclude;
    bool generatedCode;
    bool mustIncludeQPluginH;
    QByteArray includePath;
    QList<QByteArray> includeFiles;
    QList<ClassDef> classList;
    QMap<QByteArray, QByteArray> interface2IdMap;
    QList<QByteArray> metaTypes;
    // map from class name to fully qualified name
    QSet<QByteArray> knownQObjectClasses;
    QSet<QByteArray> knownGadgets;
    QMap<QString, QJsonArray> metaArgs;

    bool parseUntilClass(Token t, ParseState *state);
    bool parseQObject(Token *t, const ParseState &state);

    void parse();
    void generate(FILE *out);

    FILE *rawOpenForOutput(const QString &path, const QByteArray &pathEncoded);
    inline FILE *openForOutput(const QString &path) { return rawOpenForOutput(path, QFile::encodeName(path)); }
    inline FILE *openForOutput(const QByteArray &path) { return rawOpenForOutput(QFile::decodeName(path), path); }
    void closeOutput(FILE *);

    QByteArray printHeader(FILE *out);
    void printInclude(FILE *out);

#if QT_REMOTE
    QByteArray remote_headerPath();
    QByteArray remote_sourcePath();
    void remote_includeFromLocal(const QByteArray &localPath);
    void remote_include(const QByteArray &path);
    bool remote_generate();
    void remote_generateHeader(FILE *out);
#endif // QT_REMOTE

    bool parseClassHead(ClassDef *def);
    bool parseClassForward(ClassBasicDef *def, const ParseState &state);
    /// Depricated: use "def->contains(this->nextOffset())" instead of "inClass(def)".
    inline bool inClass(const ClassDef *def) const {
        return index > def->begin && index < def->end - 1;
    }

    /// Depricated: use "def->contains(this->nextOffset())" instead of "inNamespace(def)".
    inline bool inNamespace(const NamespaceDef *def) const {
        return index > def->begin && index < def->end - 1;
    }

    Type parseType();

    bool parseEnum(EnumDef *def);

    bool parseFunction(FunctionDef *def, bool inMacro = false);
    bool parseMaybeFunction(const ClassDef *cdef, FunctionDef *def);

    void parseSlots(ClassDef *def, FunctionDef::Access access);
    void parseSignals(ClassDef *def);
    void parseProperty(ClassDef *def);
    void parsePluginData(ClassDef *def);
    void createPropertyDef(PropertyDef &def);
    void parseEnumOrFlag(ClassDef *def, bool isFlag);
    void parseFlag(ClassDef *def);
    void parseClassInfo(ClassDef *def);
    void parseInterfaces(ClassDef *def);
    void parseDeclareInterface();
    void parseDeclareMetatype();
    void parseSlotInPrivate(ClassDef *def, FunctionDef::Access access);
    void parsePrivateProperty(ClassDef *def);

    QByteArray lexemUntil(Token);

    /// Skips tokens until `target` gets current-token
    /// (which "lookup(0)" would return).
    bool until(Token target);

    // test for Q_INVOCABLE, Q_SCRIPTABLE, etc. and set the flags
    // in FunctionDef accordingly
    bool testFunctionAttribute(FunctionDef *def);
    bool testFunctionAttribute(Token tok, FunctionDef *def);
    bool testFunctionRevision(FunctionDef *def);
    bool testFunctionRevisionRaw(int *revisionPtr);
    bool testFunctionDefault(FunctionDef *def);

    /// Same as next() if macro-expand enabled, otherwise, helps skip macros.
    ///
    /// @returns \c true if caller should skip.
    inline bool skipUnless(Token expected, Token skipUntil MOC_TOKEN_SCOPE(= RPAREN));

    void checkSuperClasses(ClassDef *def);
    void checkProperties(ClassDef* cdef);

    int onExit(int code) Q_DECL_OVERRIDE;

    static inline void onExitStatic(int code) {
        if (m_instance) {
            m_instance->onExit(code);
        } else {
            exit(code);
        }
    }
};

inline QByteArray noRef(const QByteArray &type)
{
    if (type.endsWith('&')) {
        if (type.endsWith("&&"))
            return type.left(type.length()-2);
        return type.left(type.length()-1);
    }
    return type;
}

inline bool Moc::skipUnless(Token expected, Token skipUntil) {
#if QT_MOC_MACRO_EXPAND
    next(expected);
    Q_UNUSED(skipUntil)
#else
    if ( ! test(expected)) {
        if (skipUntil != NOTOKEN) {
            until(skipUntil);
        }
        warning("Ignoring, MOC was built with macro-expand being disabled.");
        return true;
    }
#endif
    return false;
}

QT_END_NAMESPACE

#endif // MOC_H
