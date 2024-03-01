/****************************************************************************
**
** Copyright (C) 2015 The XD Company Ltd.
**
** This file is part of the QtExtras module of the XD Toolkit.
**
** $QT_BEGIN_LICENSE:APACHE2$
**
** Licensed under the Apache License, Version 2.0 (the "License");
** you may not use this file except in compliance with the License.
** You may obtain a copy of the License at
**
**     http://www.apache.org/licenses/LICENSE-2.0
**
** Unless required by applicable law or agreed to in writing, software
** distributed under the License is distributed on an "AS IS" BASIS,
** WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
** See the License for the specific language governing permissions and
** limitations under the License.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef EXTRAS_CUSTOM_TYPES_H
#define EXTRAS_CUSTOM_TYPES_H

#include <QtCore/qvector.h>
#include <QtCore/qbytearray.h>
#include <QtCore/qreadwritelock.h>


class CustomTypes;


struct AbstractTypeInfo {
    typedef void *(*Constructor)(const void *, void *);

    inline AbstractTypeInfo() : typeName(), constructorPtr(0) {}

    inline bool isNull() const { return typeName.isEmpty() && !constructorPtr; }
    inline bool isEmpty() const { return typeName.isEmpty() || !constructorPtr; }

    inline void *construct(const void *copy = Q_NULLPTR, void *where = Q_NULLPTR) const;

    QByteArray typeName;
    quint32 size;
    Constructor constructorPtr;
};

void *AbstractTypeInfo::construct(const void *copy, void *where) const
    { return constructorPtr(copy, !where ? ::malloc(size) : where); }


struct TypeInfo : public AbstractTypeInfo {
    inline TypeInfo();
    inline TypeInfo(const AbstractTypeInfo *super, int id) : AbstractTypeInfo(*super), id(id) {}

    int id;
};

//class AbstractCustomTypeModel : AbstractTypeInfo {
//public:
//    virtual int append() {}
//};

namespace CustomTypePrivate {

enum AutoRegisterFlag {
    flag_Default = 0,
    flag_NoCopy = 1
};

struct TypeInfoEntry : AbstractTypeInfo {
    int alias;
};

// Forward declaration.
template <typename T, int>
struct Modify {
    static int Register(CustomTypes *ct, const char *typeName);
};

} // namespace CustomTypePrivate

class CustomTypes {
public:
    enum { InvalidType = 0, FirstType = InvalidType+1 };

    //size: "sizeof(TYPE)" (i.e. required for shared memory support)
    int registerType(const char *typeName, quint32 size, TypeInfo::Constructor constructor);

    enum AutoRegisterFlag {
        flag_Default = 0,
        flag_NoCopy = 1
    };

    // simple/automatic register using templates and flags
    template <typename T>
    Q_INLINE_TEMPLATE int registerType(const char *typeName)
        { return CustomTypePrivate::Modify<T, flag_Default>::Register(this, typeName); }

    template <typename T, int flags>
    Q_INLINE_TEMPLATE int registerType(const char *typeName)
        { return CustomTypePrivate::Modify<T, flags>::Register(this, typeName); }

    /// Returns @p aliasId if it is valid and @p typeName is not registered,
    /// but if @p typeName is registered returns its id.
    int registerTypedef(const char* typeName, int aliasId);

    int type(const char *typeName) const;

    const char *typeName(int type) const;

    TypeInfo typeInfo(const char *typeName, int length = -1) const;

    TypeInfo typeInfo(int type) const;

    bool isRegistered(int type) const;

    inline const QVector< CustomTypePrivate::TypeInfoEntry > &toVector() const
        { return this->entries; }

//    void *construct(int type, const void *copy = 0, void *where = 0) const {
//        const TypeInfo &inf = typeInfo(type);
//        if(inf.isValid())
//            return inf.construct(copy, where);
//        return 0;
//    }

protected:
    int typeUnlocked(const char *typeName, int length) const;
    mutable QReadWriteLock customTypesLock;
    QVector< CustomTypePrivate::TypeInfoEntry > entries;
};


namespace CustomTypePrivate {

template <typename T>
struct Functions {
    enum { Size = sizeof(T) };

    static void Destruct(void *t)
    {
        Q_UNUSED(t) // Silences MSVC warning for POD types.
        static_cast<T*>(t)->~T();
    }

    /// @warning Caller should allocate memory.
    static void *Construct(const void *t, void *where)
    {
        Q_UNUSED(t)
        return new (where) T();
    }

    /// @warning Caller should allocate memory.
    static void *ConstructOrCopy(const void *t, void *where)
    {
        if (t)
            return new (where) T(*static_cast<const T*>(t));
        return new (where) T();
    }

#ifndef QT_NO_DATASTREAM
    static void Save(QDataStream &stream, const void *t)
    {
        stream << *static_cast<const T*>(t);
    }

    static void Load(QDataStream &stream, void *t)
    {
        stream >> *static_cast<T*>(t);
    }
#endif // QT_NO_DATASTREAM
};

#ifndef Q_MOC_RUN
template <typename T>
struct Modify<T, CustomTypes::flag_Default > {
    static int Register(CustomTypes *ct, const char *typeName) {
        return ct->registerType(typeName, Functions<T>::Size, Functions<T>::ConstructOrCopy);
    }
};
template <typename T>
struct Modify<T, CustomTypes::flag_NoCopy > {
    static int Register(CustomTypes *ct, const char *typeName) {
        return ct->registerType(typeName, Functions<T>::Size, Functions<T>::Construct);
    }
};
#endif //Q_MOC_RUN

} // namespace CustomTypePrivate

TypeInfo::TypeInfo() : id(CustomTypes::InvalidType) {}

template <typename T>
struct CustomTypeInfo
{
    enum { Defined = false };
    static const char *name() { return Q_NULLPTR; }
    static Q_INLINE_TEMPLATE int id() { return CustomTypes::InvalidType; }
};

/// @note Uses QBasicAtomicInt only to increase performance
/// (else @ref CustomTypes::registerType has responsibility to be thread-safe).
#define Q_DECLARE_CUSTOM_TYPE_FLAGS(TYPE, CONTAINER_PTR, FLAGS) \
    template <>                                                 \
    struct CustomTypeInfo< TYPE >                               \
    {                                                           \
        enum { Defined = 1 };                                   \
        static const char *name() { return #TYPE; }             \
        static int id() {                                       \
            static QBasicAtomicInt idGuard = Q_BASIC_ATOMIC_INITIALIZER(0); \
            int current = idGuard.loadAcquire();                \
            if ( ! current) {                                   \
                current = (CONTAINER_PTR)->registerType< TYPE , FLAGS >(name()); \
                idGuard.storeRelease(current);                  \
            }                                                   \
            return current;                                     \
        }                                                       \
    };

#define Q_DECLARE_CUSTOM_TYPE(TYPE, CONTAINER_PTR) \
    Q_DECLARE_CUSTOM_TYPE_FLAGS(TYPE, CONTAINER_PTR, CustomTypes::flag_Default)

#endif //EXTRAS_CUSTOM_TYPES_H
