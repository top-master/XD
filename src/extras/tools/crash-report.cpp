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

#include "crash-report.h"

#include <stdio.h>


// MARK: declarations.

LPTOP_LEVEL_EXCEPTION_FILTER CrashReporter::m_lastExceptionFilter = NULL;

typedef BOOL (WINAPI *MiniDumpWriteDumpFunc)(HANDLE hProcess, DWORD ProcessId
        , HANDLE hFile
        , MINIDUMP_TYPE DumpType
        , const MINIDUMP_EXCEPTION_INFORMATION *ExceptionInfo
        , const MINIDUMP_USER_STREAM_INFORMATION *UserStreamInfo
        , const MINIDUMP_CALLBACK_INFORMATION *Callback
    );


// MARK: definitions.

LONG WINAPI CrashReporter::UnHandledExceptionFilter(struct _EXCEPTION_POINTERS *exceptionPtr)
{
    // we load DbgHelp.dll dynamically, to support Windows 2000.
    HMODULE	hModule = ::LoadLibraryA("DbgHelp.dll");
    if (hModule) {
        MiniDumpWriteDumpFunc dumpFunc = reinterpret_cast<MiniDumpWriteDumpFunc>(
                ::GetProcAddress(hModule, "MiniDumpWriteDump")
            );
        if (dumpFunc) {            
            // Chooses path for dump-file based on App-path.
            wchar_t dumpFilePath[MAX_PATH]; {
                DWORD size = ::GetModuleFileNameW(NULL, dumpFilePath, sizeof(dumpFilePath)/sizeof(*dumpFilePath));
                // Fetch system time for dump-file name.
                SYSTEMTIME	SystemTime;
                ::GetLocalTime(&SystemTime);
                // Combines App-path with date and time.
                ::_snwprintf_s(dumpFilePath + size
                        , sizeof(dumpFilePath)/sizeof(*dumpFilePath) - size
                        , _TRUNCATE // -1
                        , L"_%04d-%d-%02d_%d-%02d-%02d.dmp"
                        , SystemTime.wYear, SystemTime.wMonth, SystemTime.wDay
                        , SystemTime.wHour, SystemTime.wMinute, SystemTime.wSecond
                    );
            }

            // Creates and opens the dump-file.
            HANDLE hFile = ::CreateFileW( dumpFilePath, GENERIC_WRITE
                    , FILE_SHARE_WRITE
                    , NULL
                    , CREATE_ALWAYS
                    , FILE_ATTRIBUTE_NORMAL | FILE_ATTRIBUTE_HIDDEN
                    , NULL
                );

            if (hFile != INVALID_HANDLE_VALUE) {
                _MINIDUMP_EXCEPTION_INFORMATION exceptionInfo;
                exceptionInfo.ThreadId          = ::GetCurrentThreadId();
                exceptionInfo.ExceptionPointers = exceptionPtr;
                exceptionInfo.ClientPointers    = NULL;
                // At last write crash-dump to file.
                bool ok = dumpFunc(::GetCurrentProcess(), ::GetCurrentProcessId()
                        , hFile, MiniDumpNormal
                        , &exceptionInfo, NULL, NULL
                    );
                // Dump-data is written, and we can close the file.
                ::CloseHandle(hFile);
                if (ok) {
                    // Returns from UnhandledExceptionFilter and executes the associated exception handler,
                    // where said handler usually results in process termination.
                    return EXCEPTION_EXECUTE_HANDLER;
                }
            }
        }
    }
    // Proceed with normal execution of UnhandledExceptionFilter,
    // which means obeying the `SetErrorMode` flags,
    // or invoking the Application Error pop-up and/or message-box.
    return EXCEPTION_CONTINUE_SEARCH;
}

void CrashReporter::simulateCrashLater() {
    QElapsedTimer timer;
    timer.start();
    while (timer.timeLeft(64000)) {
        QCoreApplication::processEvents();
    }
    *((int *)NULL) = 0; // Causes crash by trying to change memory at `nullptr`.
}

void CrashReporter::prepare() {
    // Ensures Debug-Logs are not used unless Debugger is attached.
    CrashReporter::defaultHandler = qInstallMsgHandler(CrashReporter::MsgHandler);
#ifdef Q_OS_WIN
#  ifdef _DEBUG
    // Enables debugging for memory leaks.
    //  was turned off for Release-builds.
    HANDLE hLogFile = CreateFileA("memory-leak.log"
            , GENERIC_WRITE, FILE_SHARE_WRITE, NULL
            , CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL
        );
    (void) hLogFile;
    _CrtSetDbgFlag(_CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF);
    _CrtSetReportMode(_CRT_WARN, _CRTDBG_MODE_FILE);
    _CrtSetReportFile(_CRT_WARN, hLogFile);
    _CrtSetReportMode(_CRT_ERROR, _CRTDBG_MODE_FILE);
    _CrtSetReportFile(_CRT_ERROR, hLogFile);
    _CrtSetReportMode(_CRT_ASSERT, _CRTDBG_MODE_FILE);
    _CrtSetReportFile(_CRT_ASSERT, hLogFile);
#  endif
    // Replaces debug error-dialogs with error-throws to enable crash-dump
    // (windows-services are not allowed to show any message-box,
    // except by using "WTSSendMessage" method)
    _set_invalid_parameter_handler(static_cast<_invalid_parameter_handler>(
            CrashReporter::InvalidParameterHandler
        ));
#endif //Q_OS_WIN
}

QtMsgHandler CrashReporter::defaultHandler = Q_NULLPTR;

void CrashReporter::MsgHandler(QtMsgType t, const char *str)
{
#ifndef QT_DEBUG
    Q_UNUSED(t)
#elif defined(Q_OS_WIN)
    if(IsDebuggerPresent()) {
        defaultHandler(t, str);
        return;
    }
#endif

    // Use default console prints.
    fprintf(stderr, "%s\n", str);
    // Flush to prevent any unexpected "\n" after every 4,095 bytes.
    fflush(stderr);
}
