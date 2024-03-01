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

#include "customtypes.h"

#include <QMetaObject>

int CustomTypes::registerType(const char *typeName, quint32 size, AbstractTypeInfo::Constructor constructor) {
    QByteArray normalizedTypeName = QMetaObject::normalizedType(typeName);
    QWriteLocker locker(&customTypesLock);
    int idx = typeUnlocked(normalizedTypeName.constData(),
                           normalizedTypeName.size());
    if (idx <= InvalidType) {
        CustomTypePrivate::TypeInfoEntry inf;
        inf.typeName = normalizedTypeName;
        inf.size = size;
        inf.constructorPtr = constructor;
        inf.alias = InvalidType;
        idx = entries.count() + FirstType;
        entries.append(inf);
        //qDebug("CustomTypes::registerType: %li as %s(%p)", idx, inf.typeName.data(), constructor);
    }
    return idx;
}

int CustomTypes::registerTypedef(const char *typeName, int aliasId)
{
    if ( ! typeName || aliasId <= InvalidType)
        return InvalidType;

    QByteArray normalizedTypeName = QMetaObject::normalizedType(typeName);

    QWriteLocker locker(&customTypesLock);
    int idx = typeUnlocked(normalizedTypeName.constData(),
                           normalizedTypeName.size());
    if (idx > InvalidType)
        return idx; //typeName already registered

    if ( entries.count() > (aliasId - FirstType) ) {
        CustomTypePrivate::TypeInfoEntry inf;
        inf.typeName = normalizedTypeName;
        inf.alias = aliasId;
        entries.append(inf);
        return aliasId;
    }
    return InvalidType;
}

int CustomTypes::type(const char *typeName) const
{
    int length = qstrlen(typeName);
    if (!length)
        return InvalidType;

    QReadLocker locker(&customTypesLock);
    int type = typeUnlocked(typeName, length);
    if (type <= InvalidType) {
        const QByteArray normalizedTypeName = QMetaObject::normalizedType(typeName);
        type = typeUnlocked(normalizedTypeName.constData(),
                            normalizedTypeName.size());
    }
    return type;
}

const char *CustomTypes::typeName(int type) const {
    if (type > InvalidType) {
        QReadLocker locker(&customTypesLock);
        if( entries.count() > (type - FirstType) ) {
            const CustomTypePrivate::TypeInfoEntry &inf = entries.at(type - FirstType);
            if(!inf.typeName.isEmpty())
                return inf.typeName;
        }
    }
    return 0;
}

TypeInfo CustomTypes::typeInfo(int type) const {
    if(type > InvalidType) {
        QReadLocker locker(&customTypesLock);
        if( entries.count() > (type - FirstType) )
            return TypeInfo( &entries.at(type - FirstType), type);
    }
    return TypeInfo();
}

bool CustomTypes::isRegistered(int type) const {
    if (type > InvalidType) {
        QReadLocker locker(&customTypesLock);
        return ( entries.count() > (type - FirstType)
                 && !entries.at(type - FirstType).typeName.isEmpty());
    }
    return false;
}

int CustomTypes::typeUnlocked(const char *typeName, int length) const {
    for (int v = 0; v < entries.count(); ++v) {
        const CustomTypePrivate::TypeInfoEntry *customInfo = &entries.at(v);
        if ((length == customInfo->typeName.size())
            && ! strcmp(typeName, customInfo->typeName.constData())
        ) {
            while (customInfo->alias > InvalidType) {
                v = customInfo->alias - FirstType;
                customInfo = &entries.at(v);
            }
            return v + FirstType;
        }
    }
    return InvalidType;
}

TypeInfo CustomTypes::typeInfo(const char *typeName, int length) const {
    if(length < 0) length = qstrlen(typeName);
    if (length > 0) {
        QReadLocker locker(&customTypesLock);
        int type = typeUnlocked(typeName, length);
        if (type <= InvalidType) {
            const QByteArray normalizedTypeName = QMetaObject::normalizedType(typeName);
            type = typeUnlocked(normalizedTypeName.constData(),
                                normalizedTypeName.size());
        }
        if(type > InvalidType)
            return TypeInfo( &entries.at(type - FirstType), type);
    }
    return TypeInfo();
}
