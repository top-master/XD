#include "manualskip.h"
#include "library/udebug.h"

#include "qplatformdefs.h"
#include <shlwapi.h>

/**
 WARNING: once no longer needed,
 caller should `FreeLibrary((HMODULE)hLib)`.
 */
FARPROC GetAddress(const wchar_t *pLibFileName, const char *pProcName, HINSTANCE *phLib=NULL);
FARPROC GetAddress(const wchar_t *pLibFileName, const char *pProcName, HINSTANCE *phLib) {
    if (phLib) *phLib = NULL;
    HINSTANCE hLib = ::LoadLibrary(pLibFileName);
    if (hLib < (HINSTANCE)32) {
        // Failed to load DLL.
        return NULL;
    }
    //char Filename[500];
    //GetModuleFileName((HMODULE)hLib, Filename, sizeof(Filename));
    FARPROC pAddress = GetProcAddress((HMODULE)hLib, pProcName);
    if (!pAddress) {
        FreeLibrary((HMODULE)hLib);
        hLib = 0;
    }
    if (phLib) *phLib = hLib;
    return pAddress;
    //...FreeLibrary((HMODULE)hLib);
}

typedef short (WINAPI*GetKeyStateTD)(int nVirtKey);

bool isKeyDown(int vKey)
{
    static GetKeyStateTD GetKeyStatePtr = (GetKeyStateTD) ::GetAddress(L"user32", "GetKeyState");

    int lR = GetKeyStatePtr(vKey);
    if ( ! (lR == 0 || lR == 1)) {
        return true;
    }
    return false;
}


bool skipOnKey(const QString &str)
{
#ifdef Q_OS_WIN
    if(isKeyDown(keyPause)) {
        xd("qmake exited since you holded PauseBreak key ").noQuotes() << str;
        return true;
    }
    if(isKeyDown(keySnapshot)) {
        xd("qmake exited since you holded PrintScreen key ").noQuotes() << str;
        return true;
    }
#endif
    return false;
}
