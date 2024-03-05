/****************************************************************************
**
** Copyright (C) 2015 The XD Company Ltd.
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing/
**
** This file is part of the QtCore module of the Qt Toolkit.
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

#include "qsharedmemory.h"
#include "qsharedmemory_p.h"
#include "qsystemsemaphore.h"
#include <qdebug.h>
#include <qt_windows.h>

QT_BEGIN_NAMESPACE

#ifndef QT_NO_SHAREDMEMORY

#include <Sddl.h> //for ConvertStringSecurityDescriptorToSecurityDescriptorW

//#define QSHAREDMEMORY_DEBUG

QSharedMemoryPrivate::QSharedMemoryPrivate() : QObjectPrivate(),
        memory(0), size(0), error(QSharedMemory::NoError),
           systemSemaphore(QString()), lockedByMe(false), hand(0)
{
}

void QSharedMemoryPrivate::setErrorString(QLatin1String function)
{
    DWORD windowsError = GetLastError();
    if (windowsError == 0)
        return;
    switch (windowsError) {
    case ERROR_ALREADY_EXISTS:
        error = QSharedMemory::AlreadyExists;
        errorString = QSharedMemory::tr("%1: already exists").arg(function);
    break;
    case ERROR_FILE_NOT_FOUND:
#if defined(Q_OS_WINCE) || (defined(Q_OS_WINRT) && _MSC_VER < 1900)
        // This happens on CE only if no file is present as CreateFileMappingW
        // bails out with this error code
    case ERROR_INVALID_PARAMETER:
#endif
        error = QSharedMemory::NotFound;
        errorString = QSharedMemory::tr("%1: doesn't exist").arg(function);
        break;
    case ERROR_COMMITMENT_LIMIT:
        error = QSharedMemory::InvalidSize;
        errorString = QSharedMemory::tr("%1: invalid size").arg(function);
        break;
    case ERROR_NO_SYSTEM_RESOURCES:
    case ERROR_NOT_ENOUGH_MEMORY:
        error = QSharedMemory::OutOfResources;
        errorString = QSharedMemory::tr("%1: out of resources").arg(function);
        break;
    case ERROR_ACCESS_DENIED:
        error = QSharedMemory::PermissionDenied;
        errorString = QSharedMemory::tr("%1: permission denied").arg(function);
        break;
    default:
        errorString = QSharedMemory::tr("%1: unknown error %2").arg(function).arg(qt_error_string(windowsError));
        error = QSharedMemory::UnknownError;
#ifdef QSHAREDMEMORY_DEBUG
        qDebug() << errorString << "key" << key;
#endif
        break;
    }
}

HANDLE QSharedMemoryPrivate::handle()
{
    if (!hand) {
        // Don't allow making handles on empty keys.
        const QLatin1String function("QSharedMemory::handle");
        if (nativeKey.isEmpty()) {
            error = QSharedMemory::KeyError;
            errorString = QSharedMemory::tr("%1: unable to make key").arg(function);
            return 0;
        }
#if defined(Q_OS_WINPHONE)
        Q_UNIMPLEMENTED();
        hand = 0;
#elif defined(Q_OS_WINRT)
#if _MSC_VER >= 1900
        hand = OpenFileMappingFromApp(FILE_MAP_ALL_ACCESS, FALSE, reinterpret_cast<PCWSTR>(nativeKey.utf16()));
#else
        hand = CreateFileMappingFromApp(INVALID_HANDLE_VALUE, 0, PAGE_READWRITE, 0, (PCWSTR)nativeKey.utf16());
#endif
#elif defined(Q_OS_WINCE)
        // This works for opening a mapping too, but always opens it with read/write access in
        // attach as it seems.
        hand = CreateFileMapping(INVALID_HANDLE_VALUE, 0, PAGE_READWRITE, 0, 0, (wchar_t*)nativeKey.utf16());
#else
        hand = OpenFileMapping(FILE_MAP_ALL_ACCESS, false, (wchar_t*)nativeKey.utf16());
#endif
        if (!hand) {
            setErrorString(function);
            return 0;
        }
    }
    return hand;
}

bool QSharedMemoryPrivate::cleanHandle()
{
    if (hand != 0 && !CloseHandle(hand)) {
        hand = 0;
        setErrorString(QLatin1Literal("QSharedMemory::cleanHandle"));
        return false;
    }
    hand = 0;
    return true;
}

/// String notation in SDDL format.
const wchar_t STR_ALLOW_ANYTHING[] =
        L"D:P" //means use DACL instead of a SACL (we may rarely use SACL's...)
        //
        //Following ACE (i.e. access control entry) strings control who gets access.
        //  Each one is A (allow) and allows for object and contains inheritance (OICI).
        //  The first two do grant all access (GA - grant all) to System (SY) and Administrators (BA - built-in administratos).
        //
        L"(A;OICI;GA;;;SY)" //SDDL_LOCAL_SYSTEM: Local system. The corresponding RID is SECURITY_LOCAL_SYSTEM_RID.
        L"(A;OICI;GA;;;BA)" //SDDL_BUILTIN_ADMINISTRATORS: Built-in administrators. The corresponding RID is DOMAIN_ALIAS_RID_ADMINS.
        //
        //below we allow limited user's to modify the shared-memory created by service
        //  IU: SDDL_INTERACTIVE: users actually logged-on to a session interactively (with user-name and password).
        //  This is a group identifier added to the token of a process when it was logged on interactively.
        //  The corresponding logon type is LOGON32_LOGON_INTERACTIVE. The corresponding RID is SECURITY_INTERACTIVE_RID.
        L"(A;OICI;GA;;;IU)";

bool QSharedMemoryPrivate::create(int size)
{
    const QLatin1String function("QSharedMemory::create");
    if (nativeKey.isEmpty()) {
        error = QSharedMemory::KeyError;
        errorString = QSharedMemory::tr("%1: key error").arg(function);
        return false;
    }

#if defined(Q_OS_WINPHONE)
    Q_UNIMPLEMENTED();
    Q_UNUSED(size)
    hand = 0;
#else

    if( false == nativeKey.startsWith(QLL("Global\\")) ) {
        // Create the file mapping.
#if defined(Q_OS_WINRT)
        hand = CreateFileMappingFromApp(INVALID_HANDLE_VALUE, 0, PAGE_READWRITE, size, (PCWSTR)nativeKey.wcharCast());
#else
        hand = CreateFileMapping(INVALID_HANDLE_VALUE, 0, PAGE_READWRITE, 0, size, nativeKey.wcharCast());
#endif
    } else {
        // Create Global-File mapping between service-processes and user-processes.
        // note: Below only works for Service-Processes and always returns zero for Normal-User-Processes
        // since normal-processes don't have the create-global-privilege.
        //
        // If you really need normal-processes to create the shared-memory
        // and not the service then you could have an IPC scheme like below:
        // 1. your normal-process code sends a message to the service containing the file mapping handle it created,
        // 2. the service would then call DuplicateHandle(...) to get a reference to it.
        // 3. but this would require your service to run with the debug-privilege.

        SECURITY_ATTRIBUTES attributes;
        ZeroMemory(&attributes, sizeof(attributes));
        attributes.nLength = sizeof(attributes);
        //below method is taking string in SDDL format
        ConvertStringSecurityDescriptorToSecurityDescriptorW
            (
                STR_ALLOW_ANYTHING
                , SDDL_REVISION_1
                , &attributes.lpSecurityDescriptor
                , NULL
            );

#if defined(Q_OS_WINRT)
        hand = CreateFileMappingFromApp(INVALID_HANDLE_VALUE, &attribute, PAGE_READWRITE, size, (PCWSTR)nativeKey.wcharCast());
#else
        hand = CreateFileMappingW(INVALID_HANDLE_VALUE, &attributes
                , PAGE_READWRITE, 0, size, nativeKey.wcharCast());
#endif

        LocalFree(attributes.lpSecurityDescriptor);
    }
#endif // !Q_OS_WINPHONE

    setErrorString(function);

    // hand is valid when it already exists unlike unix so explicitly check
    if (error == QSharedMemory::AlreadyExists || !hand)
        return false;

    return true;
}

bool QSharedMemoryPrivate::attach(QSharedMemory::AccessMode mode)
{
    // Grab a pointer to the memory block
    int permissions = (mode == QSharedMemory::ReadOnly ? FILE_MAP_READ : FILE_MAP_ALL_ACCESS);
#if defined(Q_OS_WINPHONE)
    Q_UNIMPLEMENTED();
    Q_UNUSED(mode)
    Q_UNUSED(permissions)
    memory = 0;
#elif defined(Q_OS_WINRT)
    memory = (void *)MapViewOfFileFromApp(handle(), permissions, 0, 0);
#else
    memory = (void *)MapViewOfFile(handle(), permissions, 0, 0, 0);
#endif
    if (0 == memory) {
        setErrorString(QLatin1String("QSharedMemory::attach"));
        cleanHandle();
        return false;
    }

    // Grab the size of the memory we have been given (a multiple of 4K on windows)
    MEMORY_BASIC_INFORMATION info;
    if (!VirtualQuery(memory, &info, sizeof(info))) {
        // Windows doesn't set an error code on this one,
        // it should only be a kernel memory error.
        error = QSharedMemory::UnknownError;
        errorString = QSharedMemory::tr("%1: size query failed").arg(QLatin1String("QSharedMemory::attach: "));
        return false;
    }
    size = info.RegionSize;

    return true;
}

bool QSharedMemoryPrivate::detach()
{
    // umap memory
#if defined(Q_OS_WINPHONE)
    Q_UNIMPLEMENTED();
    return false;
#else
    if (!UnmapViewOfFile(memory)) {
        setErrorString(QLatin1String("QSharedMemory::detach"));
        return false;
    }
#endif
    memory = 0;
    size = 0;

    // close handle
    return cleanHandle();
}

#endif //QT_NO_SHAREDMEMORY


QT_END_NAMESPACE
