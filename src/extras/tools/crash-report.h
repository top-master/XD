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

#ifndef CRASH_REPORTER_H
#define CRASH_REPORTER_H

#include <QtCore/QCoreApplication>
#include <QtCore/QElapsedTimer>

#if defined(Q_OS_WIN)
    // Excludes rarely used content from the Windows headers.
#   ifndef WIN32_LEAN_AND_MEAN
#       define WIN32_LEAN_AND_MEAN
#       include <windows.h>
#       undef WIN32_LEAN_AND_MEAN
#   else
#       include <windows.h>
#   endif
#   include <tchar.h>
#   include <DbgHelp.h>
#   include <crtdbg.h>
#endif

#include <unordered_map>
#include <exception>
#include <sstream>


#ifndef USE_RECOVERY_CALLBACK
#   define USE_RECOVERY_CALLBACK 0
#endif

/// @warning Supports Windows only, at least tested versions (XP, 7, 10).
///
/// @todo Support more platforms.
class CrashRecovery {
public:
    inline CrashRecovery() {
        HMODULE hModule = LoadLibraryA("Kernel32.dll");
        if (hModule != NULL) {
            #if USE_RECOVERY_CALLBACK
                registerRecoveryCallback = reinterpret_cast<RegisterApplicationRecoveryCallbackFunc>(GetProcAddress(hModule, "RegisterApplicationRecoveryCallback"));
                recoveryInProgress = reinterpret_cast<ApplicationRecoveryInProgressFunc>(GetProcAddress(hModule, "ApplicationRecoveryInProgress"));
                recoveryFinished = reinterpret_cast<ApplicationRecoveryFinishedFunc>(GetProcAddress(hModule, "ApplicationRecoveryFinished"));
                unregisterRecoveryCallback = reinterpret_cast<UnregisterApplicationRecoveryCallbackFunc>(GetProcAddress(hModule, "UnregisterApplicationRecoveryCallback"));

                //if (registerRecoveryCallback) {
                //    registerRecoveryCallback(ApplicationRecoveryCallback, qApp, 0, 0);
                //}
            #endif
            registerRestart = reinterpret_cast<RegisterApplicationRestartFunc>(GetProcAddress(hModule, "RegisterApplicationRestart"));
        } else {
            #if USE_RECOVERY_CALLBACK
                registerRecoveryCallback = Q_NULLPTR;
                recoveryInProgress = Q_NULLPTR;
                recoveryFinished = Q_NULLPTR;
                unregisterRecoveryCallback = Q_NULLPTR;
            #endif
            registerRestart = Q_NULLPTR;
        }
    }

private:
#if USE_RECOVERY_CALLBACK
    // Application-defined callback function used to save data and application state information
    // in the event the application encounters an unhandled exception or becomes unresponsive.
    static DWORD WINAPI ApplicationRecoveryCallback( PVOID pvParameter )
    {
        // TODO: SAVE SETTINGS HERE
    }
#endif

private:
    typedef HRESULT (WINAPI *RegisterApplicationRecoveryCallbackFunc)(
        _In_  APPLICATION_RECOVERY_CALLBACK pRecoveyCallback,
        _In_opt_  PVOID pvParameter,
        _In_ DWORD dwPingInterval,
        _In_ DWORD dwFlags
        );
    typedef HRESULT (WINAPI *ApplicationRecoveryInProgressFunc)(
        _Out_ PBOOL pbCancelled
        );
    typedef VOID (WINAPI *ApplicationRecoveryFinishedFunc)(
        _In_ BOOL bSuccess
        );
    typedef HRESULT (WINAPI *UnregisterApplicationRecoveryCallbackFunc)(void);
    typedef HRESULT (WINAPI *GetApplicationRecoveryCallbackFunc)(
        _In_  HANDLE hProcess,
        _Out_ APPLICATION_RECOVERY_CALLBACK* pRecoveryCallback,
        _Deref_opt_out_opt_ PVOID* ppvParameter,
        _Out_opt_ PDWORD pdwPingInterval,
        _Out_opt_ PDWORD pdwFlags
        );

    typedef HRESULT (WINAPI *RegisterApplicationRestartFunc)(
        _In_opt_ PCWSTR pwzCommandline,
        _In_ DWORD dwFlags
        );
    typedef HRESULT (WINAPI *UnregisterApplicationRestartFunc)(void);
    typedef HRESULT (WINAPI *GetApplicationRestartSettingsFunc)(
        _In_ HANDLE hProcess,
        /*_Out_ecount_opt_(*pcchSize)*/ PWSTR pwzCommandline,
        _Inout_ PDWORD pcchSize,
        _Out_opt_ PDWORD pdwFlags
        );

public:
#if USE_RECOVERY_CALLBACK
    RegisterApplicationRecoveryCallbackFunc registerRecoveryCallback;
    ApplicationRecoveryInProgressFunc recoveryInProgress;
    ApplicationRecoveryFinishedFunc recoveryFinished;
    UnregisterApplicationRecoveryCallbackFunc unregisterRecoveryCallback;
#endif
    RegisterApplicationRestartFunc registerRestart;
};

/// @warning Supports Windows only, at least versions we tested (XP, 7, 10).
///
/// @todo Support more platforms.
class CrashReporter {
public:
    inline CrashReporter() { attach(); }
    inline ~CrashReporter() { detach(); }

    inline static void attach() {
        if (m_lastExceptionFilter != NULL) {
            qDebug("CrashReporter: is already registered");
        }
        SetErrorMode(SEM_FAILCRITICALERRORS);
        // Ensures UnHandledExceptionFilter is called before App dies.
        m_lastExceptionFilter = ::SetUnhandledExceptionFilter(
                CrashReporter::UnHandledExceptionFilter
            );
    }
    inline static void detach() {
        ::SetUnhandledExceptionFilter(m_lastExceptionFilter);
    }

    // Below methods are just not meant to be called and were
    // used to test crash dump and/or recovery feature.
    inline static void simulateCrash() {
        *static_cast<volatile int *>(NULL) = 0; //causes crush by trying to change memory at null
    }
    /// Will cause crush by trying to increment iterator pointing to end.
    static inline void simulateCrashDeep() {
        qDebug("CrashRecovery::simulateCrashDeep");
        typedef std::unordered_map<quint32, quint32> Hash;
        Hash list;
        list[0xC001] = 0xDEAD;
        Hash::iterator it = list.begin();
        it = list.erase(it);
        ++it; // should crash here
        if (it != list.end())
            it->second = 0xCAFE; // but just to be sure
    }

    /// Waits one minute and then will cause crash
    /// (Windows will not provide restart option for Apps that crash sooner).
    static void simulateCrashLater();

    static void prepare();

private:
    /// Ensures Debug-Logs are not used unless Debugger is attached
    /// (e.g. prevents "OutputDebugStringA(...)" from being called).
    static void MsgHandler(QtMsgType t, const char* str);

#ifdef Q_OS_WIN
    static void InvalidParameterHandler(const wchar_t* expression
        , const wchar_t* function
        , const wchar_t* file
        , unsigned int line
        , uintptr_t /*pReserved*/)
    {
        std::stringstream errMsg;
        errMsg << "Invalid parameter error in Function " << function
            << "\n\nFile: " << file << " Line: " << line
            << "\nExpression: " << expression;
        throw std::runtime_error(errMsg.str());
    }
#endif //Q_OS_WIN

private:
    static QtMsgHandler defaultHandler;
    static LPTOP_LEVEL_EXCEPTION_FILTER m_lastExceptionFilter;
    static LONG WINAPI UnHandledExceptionFilter(_EXCEPTION_POINTERS *);
};


#endif // CRASH_REPORTER_H
