

// Copyright (C) 2015 The XD Company Ltd.
// Copyright (C) 2015 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-2.1-only OR LGPL-3.0-only

#ifndef EXTERNAL_TESTS_APP
#error "Building directly is not supported, use QExternalTest class."
#endif

#include <QtCore/qglobal.h>
#include <stdlib.h>
#include <stddef.h>

extern volatile bool globalDontOptimizeHelper = true;
#ifdef NO_OPTIMIZE
#  undef NO_OPTIMIZE
#endif
#define NO_OPTIMIZE if ((globalDontOptimizeHelper = true))

void q_external_test_user_code()
{
#include "user_code.cpp"
}

#ifdef Q_OS_WIN
#  include <windows.h>
#  if defined(Q_CC_MSVC) && !defined(Q_OS_WINCE)
#    include <crtdbg.h>
#  endif
static void q_test_setup()
{
    SetErrorMode(SEM_FAILCRITICALERRORS | SEM_NOGPFAULTERRORBOX | SEM_NOOPENFILEERRORBOX);
}
static int __cdecl CrtDbgHook(int /*reportType*/, char * /*message*/, int *returnValue)
{
    if (returnValue)
        *returnValue = TRUE;
    return TRUE;
}
#else
static void q_test_setup() { }
#endif
int main(int argc, char **argv)
{
    // Ensures methods like `qFatal(...)` cause crash instead of ignoring.
#if defined(Q_CC_MSVC)
#  if defined(QT_DEBUG) && defined(_DEBUG) && defined(_CRT_ERROR) && !defined(Q_OS_WINCE)
    {
        int result = _CrtSetReportHook2(_CRT_RPTHOOK_INSTALL, CrtDbgHook);
        fprintf(stderr, "Running {_CrtSetReportHook2(...)} result: %d.\n", result);
    }
#  else
    fprintf(stderr, "Skipped {_CrtSetReportHook2(...)}.\n");
#  endif
#endif
    fflush(stderr);

#ifdef EXTERNAL_TESTS_APP
    EXTERNAL_TESTS_APP
#else
    (void)argc; (void)argv;
#endif

    q_test_setup();
    q_external_test_user_code();
    return 0;
}
