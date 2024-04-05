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

#include "function-mapper.h"


FunctionMapper::FunctionMapper()
{
}

FunctionMapper::~FunctionMapper()
{
}

bool FunctionMapper::mapList(FunctionList *list)
{
    bool isChanged = false;
    //detaching manually since we use "QVector::ptr(...)"
    list->detach();
    for (int i = list->count() - 1; i >= 0; --i) {
        if( this->mapFunction(list->ptr(i), &i) )
            isChanged = true;
    }
    return isChanged;
}

bool FunctionMapper::mapFunction(FunctionDef *func, int *)
{
    bool isChanged = this->mapType(&func->type);
    if(isChanged)
        func->type.renormalize();

    for (int i = 0; i < func->arguments.count(); i++) {
        bool r = this->mapType(&func->arguments[i].type);
        if(r)
            isChanged = true;
    } //next i
    return isChanged;
}

bool FunctionMapper::mapType(Type *)
{
    return false; //nothing was changed
}

NamespaceMapper::NamespaceMapper(ClassDef *target, const QByteArray &localName)
    : super()
    , m_target(target), m_localName(localName)
{
}

NamespaceMapper::~NamespaceMapper()
{
}

bool NamespaceMapper::mapType(Type *t)
{
    return t->mapTo(m_target, m_localName);
}

#if QT_REMOTE

RemoteMapper::RemoteMapper(Moc *owner)
    : super(), moc(owner)
{
}

RemoteMapper::~RemoteMapper()
{}

bool RemoteMapper::mapList(FunctionList *v)
{
    m_list = v;
    return super::mapList(v);
}

bool RemoteMapper::mapFunction(FunctionDef *funcPtr, int *index)
{
#if defined(QT_DEBUG) && 0
    const char *strDebug = 0;
    if(funcPtr->name == "networkAdapter") {
        UDebug() << moc->knownRemoteFiles;
    }
#endif

    FunctionDef cloned = *funcPtr;
    if( super::mapFunction(&cloned, index) ) {
        if(funcPtr->isClone == false) {
            cloned.isClone = true;
            // Ensures including in header even though is a clone.
            cloned.isCloneRequired = true;
            // Ensures original gets excluded from header.
            funcPtr->isCloneRequired = true;
        }
        m_list->insert(funcPtr + 1, cloned);
        return true;
    }
    return false;
}

//will change type for example from "QRef<MyService>" into "QRef<MyServiceRemote>"
bool RemoteMapper::mapType(Type *input)
{
    if(input->referenceType != Type::NoReference)
        return false;
    else if(input->getClassRef() != "QRef")
        return false;

    //get remote-controller name like "QRef<MyServiceRemote>" from "QRef<MyService>"
    QByteArray clazzMapped; {
        clazzMapped.reserve(input->name.size() + Moc::REMOTE_SUFFIX.size() + 1);
        clazzMapped.append(input->name);
        clazzMapped.chop(1); //remove '>' from end
        clazzMapped.append(Moc::REMOTE_SUFFIX.data(), Moc::REMOTE_SUFFIX.size());
        clazzMapped.append('>');
    }
    int prefixSize = sizeof("QRef"); //includes "<" (since "sizeof" counts trailing-null)
    if(input->isConst)
        prefixSize += sizeof("const"); //includes " "

    //get raw service name like: MyService
    const QByteArray &clazz = QByteArray::fromRawData(
            clazzMapped.constData() + prefixSize,
            clazzMapped.size() - prefixSize - Moc::REMOTE_SUFFIX.size() - 1
        );
    //get raw remote-controller name like: MyServiceRemote
    const QByteArray &clazzSuffixed = QByteArray::fromRawData(
            clazzMapped.constData() + prefixSize,
            clazz.size() + Moc::REMOTE_SUFFIX.size()
        );

    //hash through all QObject sub-classes with Q_REMOTE macro to find mathing type
    Moc::RemoteFileMap::iterator it = moc->knownRemoteFiles.find(clazzSuffixed);
    if(it != moc->knownRemoteFiles.end()) {
        moc->remote_includeFromLocal(it.value());
        goto posChange;
    } else {
        //hash through all class/struct forward-declarations with Q_REMOTE_FORWARD
        Moc::ForwardList::iterator it = moc->remoteForwards.find(clazz);
        if(it != moc->remoteForwards.end()) {
            it.value().isUsed = true;
            goto posChange;
        }
    }
    return false;

posChange:
    //at last change class
    const bool wasConst = input->isConst;
    *input = Type(clazzMapped);
    input->isConst = wasConst;
    return true;
}

#endif // QT_REMOTE
