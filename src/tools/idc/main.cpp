/****************************************************************************
**
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

#include <QDir>
#include <QFile>
#include <QDir>
#include <QScopedArrayPointer>
#include <qt_windows.h>
#include <io.h>

//normally do not use "QProcess" for speed but
//  now we use "QProcess" only for "ForwardedChannels" feature
#define USE_QPROCESS 0

#if USE_QPROCESS
#include <QProcess>
#include <QLibraryInfo>
#endif

QT_BEGIN_NAMESPACE

#define PRINT_CAST(x) ((const char*)qPrintable(x))


static QString quotePath(const QString &s)
{
    if (!s.startsWith(QLatin1Char('\"')) && s.contains(QLatin1Char(' ')))
        return QLatin1Char('\"') + s + QLatin1Char('\"');
    return s;
}

static bool hasFileExtension(const QString &filePath, const QString &extension)
{
    return filePath.endsWith(extension, Qt::CaseInsensitive);
}

static bool hasExeExtension(const QString &filePath)
{
    return hasFileExtension(filePath, QStringLiteral(".exe"));
}

static bool hasDllExtension(const QString &filePath)
{
    return hasFileExtension(filePath, QStringLiteral(".dll"));
}

#if USE_QPROCESS

static bool runWithQtInEnvironment(const QString &cmd)
{
    QProcess proc;

    // prepend the qt binary directory to the path
    QStringList env = QProcess::systemEnvironment();
    for (int i=0; i<env.count(); ++i) {
        QString var = env.at(i);
        int setidx = var.indexOf(QLatin1Char('='));
        if (setidx != -1) {
            QString varname = var.left(setidx).trimmed().toUpper();
            if (varname == QLatin1String("PATH")) {
                var = var.mid(setidx + 1);
                var = QLatin1String("PATH=") +
                    QLibraryInfo::location(QLibraryInfo::BinariesPath) +
                    QLatin1Char(';') + var;
                env[i] = var;
                break;
            }
        }
    }

    proc.setEnvironment(env);
    //proc.setProcessChannelMode(QProcess::ForwardedChannels);
    proc.start(cmd);
    proc.waitForFinished(-1);

    return (proc.exitCode() == 0);
}

#else

// Prepend the Qt binary directory to PATH.
static bool prependPath()
{
    enum { maxEnvironmentSize = 32767 };
    wchar_t buffer[maxEnvironmentSize];
    if (!GetModuleFileName(NULL, buffer, maxEnvironmentSize))
        return false;
    wchar_t *ptr = wcsrchr(buffer, L'\\');
    if (!ptr)
        return false;
    *ptr++ = L';';
    const wchar_t pathVariable[] = L"PATH";
    if (!GetEnvironmentVariable(pathVariable, ptr, DWORD(maxEnvironmentSize - (ptr - buffer)))
        || !SetEnvironmentVariable(pathVariable, buffer)) {
        return false;
    }
    return true;
}

static QString errorString(DWORD errorCode)
{
    wchar_t *resultW = 0;
    FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER|FORMAT_MESSAGE_FROM_SYSTEM,
                  NULL, errorCode, MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                  Q_PTR_CAST(LPWSTR, &resultW), 0, NULL);
    const QString result = QString::fromWCharArray(resultW);
    LocalFree(Q_PTR_CAST(HLOCAL, resultW));
    return result;
}

static bool runWithQtInEnvironment(const QString &cmd)
{
    enum { timeOutMs = 30000 };
    static const bool pathSet = prependPath();
    if (!pathSet)
        return false;

    STARTUPINFO si;
    ZeroMemory(&si, sizeof(si));
    si.cb = sizeof(si);

    STARTUPINFO myInfo;
    GetStartupInfo(&myInfo);
    si.hStdInput = myInfo.hStdInput;
    si.hStdOutput = myInfo.hStdOutput;
    si.hStdError = myInfo.hStdError;

    PROCESS_INFORMATION pi;
    ZeroMemory(&pi, sizeof(PROCESS_INFORMATION));

    QScopedArrayPointer<wchar_t> commandLineW(new wchar_t[cmd.size() + 1]);
    cmd.toWCharArray(commandLineW.data());
    commandLineW[cmd.size()] = 0;
    if (!CreateProcessW(0, commandLineW.data(), 0, 0, /* InheritHandles */ TRUE, 0, 0, 0, &si, &pi)) {
        fprintf(stderr, "Unable to execute \"%s\": %s\n", PRINT_CAST(cmd),
                PRINT_CAST(errorString(GetLastError())));
        return false;
    }

    DWORD exitCode = 1;
    switch (WaitForSingleObject(pi.hProcess, timeOutMs)) {
    case WAIT_OBJECT_0:
        GetExitCodeProcess(pi.hProcess, &exitCode);
        break;
    case WAIT_TIMEOUT:
        fprintf(stderr, "Timed out after %d ms out waiting for \"%s\".\n",
                int(timeOutMs), PRINT_CAST(cmd));
        TerminateProcess(pi.hProcess, 1);
        break;
    default:
        fprintf(stderr, "Error waiting for \"%s\": %s\n",
                PRINT_CAST(cmd), PRINT_CAST(errorString(GetLastError())));
        TerminateProcess(pi.hProcess, 1);
        break;
    }
    CloseHandle(pi.hThread);
    CloseHandle(pi.hProcess);
    if (exitCode)
        fprintf(stderr, "\"%s\" returned exit code: %lu (0x%lx)\n", PRINT_CAST(cmd), exitCode, exitCode);
    return exitCode == 0;
}

#endif // USE_QPROCESS

static bool attachResource(const QString &applicationName, const wchar_t *type, int resource, const QByteArray &data, QString *errorMessage, const QString &typeInfo = QLatin1String("resource"))
{
    HANDLE hExe = BeginUpdateResource((const wchar_t *)applicationName.utf16(), false);
    if (hExe == 0) {
        if (errorMessage)
            *errorMessage = QString::fromLatin1("Failed to attach %1 to binary %2 - could not open file.").arg(typeInfo, applicationName);
        return false;
    }
    if (!UpdateResource(hExe, type, MAKEINTRESOURCE(resource), 0, 
                        Q_PTR_CAST(void*, data.data()), data.count())) {
        EndUpdateResource(hExe, true);
        if (errorMessage)
            *errorMessage = QString::fromLatin1("Failed to attach %1 to binary %2 - could not update file.").arg(typeInfo, applicationName);
        return false;
    }

    if (!EndUpdateResource(hExe,false)) {
        if (errorMessage)
            *errorMessage = QString::fromLatin1("Failed to attach %1 to binary %2 - could not write file.").arg(typeInfo, applicationName);
        return false;
    }

    if (errorMessage)
        *errorMessage = QString::fromLatin1("The %1 attached to %2.").arg(typeInfo, applicationName);
    return true;
}

static inline bool attachTypeLibrary(const QString &applicationName, int resource, const QByteArray &data, QString *errorMessage)
{
    return attachResource(applicationName, L"TYPELIB", resource, data, errorMessage, QLatin1String("type library"));
}
static inline bool attachManifest(const QString &applicationName, int resource, const QByteArray &data, QString *errorMessage)
{
    return attachResource(applicationName, L"MANIFEST", resource, data, errorMessage, QLatin1String("manifest"));
}


// Manually add defines that are missing from pre-VS2012 compilers
#ifndef LOAD_LIBRARY_SEARCH_DLL_LOAD_DIR
#  define LOAD_LIBRARY_SEARCH_DLL_LOAD_DIR 0x00000100
#endif
#ifndef LOAD_LIBRARY_SEARCH_DEFAULT_DIRS
#  define LOAD_LIBRARY_SEARCH_DEFAULT_DIRS 0x00001000
#endif

static HMODULE loadLibraryCompat(const QString &input)
{
    const wchar_t *inputC = reinterpret_cast<const wchar_t *>(input.utf16());
    if (QSysInfo::windowsVersion() < QSysInfo::WV_VISTA)
        return LoadLibrary(inputC); // fallback for Windows XP and older

    // Load DLL with the folder containing the DLL temporarily added to the search path when loading dependencies
    HMODULE result =
        LoadLibraryEx(inputC, NULL, LOAD_LIBRARY_SEARCH_DLL_LOAD_DIR | LOAD_LIBRARY_SEARCH_DEFAULT_DIRS);
    // If that fails, call with flags=0 to get LoadLibrary() behavior (search %PATH%).
    if (!result)
        result = LoadLibraryEx(inputC, NULL, 0);
    return result;
}

static bool registerServer(const QString &input)
{
    bool ok = false;
    if (hasExeExtension(input)) {
        ok = runWithQtInEnvironment(quotePath(input) + QLatin1String(" -regserver"));
    } else {
        HMODULE hdll = loadLibraryCompat(input);
        if (!hdll) {
            fprintf(stderr, "Couldn't load library file %s\n", PRINT_CAST(input));
            return false;
        }
        typedef HRESULT(__stdcall* RegServerProc)();
        RegServerProc DllRegisterServer = Q_PTR_CAST(RegServerProc, GetProcAddress(hdll, "DllRegisterServer"));
        if (!DllRegisterServer) {
            fprintf(stderr, "Library file %s doesn't appear to be a COM library\n", PRINT_CAST(input));
            return false;
        }
        ok = DllRegisterServer() == S_OK;
    }
    return ok;
}

static bool unregisterServer(const QString &input)
{
    bool ok = false;
    if (hasExeExtension(input)) {
        ok = runWithQtInEnvironment(quotePath(input) + QLatin1String(" -unregserver"));
    } else {
        HMODULE hdll = loadLibraryCompat(input);
        if (!hdll) {
            fprintf(stderr, "Couldn't load library file %s\n", PRINT_CAST(input));
            return false;
        }
        typedef HRESULT(__stdcall* RegServerProc)();
        RegServerProc DllUnregisterServer = Q_PTR_CAST(RegServerProc, GetProcAddress(hdll, "DllUnregisterServer"));
        if (!DllUnregisterServer) {
            fprintf(stderr, "Library file %s doesn't appear to be a COM library\n", PRINT_CAST(input));
            return false;
        }
        ok = DllUnregisterServer() == S_OK;
    }
    return ok;
}

static HRESULT dumpIdl(const QString &input, const QString &idlfile, const QString &version)
{
    HRESULT res = E_FAIL;

    if (hasExeExtension(input)) {
        const QString command = quotePath(input) + QLatin1String(" -dumpidl ")
            + quotePath(idlfile) + QLatin1String(" -version ") + version;
        if (runWithQtInEnvironment(command))
            res = S_OK;
    } else {
        HMODULE hdll = loadLibraryCompat(input);
        if (!hdll) {
            fprintf(stderr, "Couldn't load library file %s\n", PRINT_CAST(input));
            return 3;
        }
        typedef HRESULT(__stdcall* DumpIDLProc)(const QString&, const QString&);
        DumpIDLProc DumpIDL = Q_PTR_CAST(DumpIDLProc, GetProcAddress(hdll, "DumpIDL"));
        if (!DumpIDL) {
            fprintf(stderr, "Couldn't resolve 'DumpIDL' symbol in %s\n", PRINT_CAST(input));
            return 3;
        }
        res = DumpIDL(idlfile, version);
        FreeLibrary(hdll);
    }

    return res;
}

const char usage[] =
"Usage: idc [options] [input_file]\n"
"Interface Description Compiler " QT_VERSION_STR "\n\n"
"Options:\n"
"  -?, /h, -h, -help                 Displays this help.\n"
"  /v, -v                            Displays version information.\n"
"  /version, -version <version>      Specify the interface version.\n"
"  /idl, -idl <file>                 Specify the interface definition file.\n"
"  /tlb, -tlb <file>                 Specify the type library file.\n"
"  /manifest <file>                  Specify the manifest file.\n"
"  /regserver, -regserver            Register server.\n"
"  /unregserver, -unregserver        Unregister server.\n\n"
"Examples:\n"
"idc -regserver l.dll                Register the COM server l.dll\n"
"idc -unregserver l.dll              Unregister the COM server l.dll\n"
"idc l.dll -idl l.idl -version 2.3   Writes the IDL of the server dll to the file idl.\n"
"                                    The type library will have version 2.3\n"
"idc l.dll -tlb l.tlb                Replaces the type library in l.dll with l.tlb\n";

enum Mode { RegisterServer, UnregisterServer, Other };

int runIdc(int argc, char **argv)
{
    QString error;
    QString tlbfile;
    QString idlfile;
    QString manfile;
    QString input;
    QString version = QLatin1String("1.0");
    Mode mode = Other;

    int i = 1;
    while (i < argc) {
        QString p = QString::fromLocal8Bit(argv[i]).toLower();

        if (p == QLatin1String("/idl") || p == QLatin1String("-idl")) {
            ++i;
            if (i > argc) {
                error = QLatin1String("Missing name for interface definition file!");
                break;
            }
            idlfile = QFile::decodeName(argv[i]).trimmed();
        } else if (p == QLatin1String("/version") || p == QLatin1String("-version")) {
            ++i;
            if (i > argc)
                version = QLatin1String("1.0");
            else
                version = QLatin1String(argv[i]);
        } else if (p == QLatin1String("/tlb") || p == QLatin1String("-tlb")) {
            ++i;
            if (i > argc) {
                error = QLatin1String("Missing name for type library file!");
                break;
            }
            tlbfile = QFile::decodeName(argv[i]).trimmed();
        } else if (p == QLatin1String("/manifest") || p == QLatin1String("-manifest")) {
            ++i;
            if (i > argc) {
                error = QLatin1String("Missing name for manifest file!");
                break;
            }
            manfile = QFile::decodeName(argv[i]).trimmed();
        } else if (p == QLatin1String("/v") || p == QLatin1String("-v")) {
            fprintf(stdout, "Qt Interface Definition Compiler version 1.2\n");
            return 0;
        } else if (p == QLatin1String("/h") || p == QLatin1String("-h") || p == QLatin1String("-?") || p == QLatin1String("/?")) {
            fprintf(stdout, "%s\n", usage);
            return 0;
        } else if (p == QLatin1String("/regserver") || p == QLatin1String("-regserver")) {
            mode = RegisterServer;
        } else if (p == QLatin1String("/unregserver") || p == QLatin1String("-unregserver")) {
            mode = UnregisterServer;
        } else if (p[0] == QLatin1Char('/') || p[0] == QLatin1Char('-')) {
            error = QLatin1String("Unknown option \"") + p + QLatin1Char('\"');
            break;
        } else {
            input = QFile::decodeName(argv[i]).trimmed();
            // LoadLibraryEx requires a fully qualified path when used together with LOAD_LIBRARY_SEARCH_DLL_LOAD_DIR
            input = QFileInfo(input).absoluteFilePath();
        }
        i++;
    }
    if (!error.isEmpty()) {
        fprintf(stderr, "%s\n", PRINT_CAST(error));
        return 5;
    }
    if (input.isEmpty()) {
        fprintf(stderr, "No input file specified!\n\n%s\n", usage);
        return 1;
    }

    switch (mode) {
    case RegisterServer:
        if (!registerServer(input)) {
            fprintf(stderr, "Failed to register server!\n");
            return 1;
        }
        fprintf(stderr, "Server registered successfully!\n");
        return 0;
    case UnregisterServer:
        if (!unregisterServer(input)) {
            fprintf(stderr, "Failed to unregister server!\n");
            return 1;
        }
        fprintf(stderr, "Server unregistered successfully!\n");
        return 0;
    case Other:
        break;
    }

    if (manfile.isEmpty()) {
        if (hasExeExtension(input) && tlbfile.isEmpty() && idlfile.isEmpty()) {
            fprintf(stderr, "No type output file specified!\n");
            return 2;
        }
        if (hasDllExtension(input) && idlfile.isEmpty() && tlbfile.isEmpty()) {
            fprintf(stderr, "No interface definition file and no type library file specified!\n");
            return 3;
        }
    }
    input = QDir::toNativeSeparators(input);     
    if (!tlbfile.isEmpty() || !manfile.isEmpty()) {
        tlbfile = QDir::toNativeSeparators(manfile.isEmpty() ? tlbfile : manfile);
        QFile file(tlbfile);
        if (!file.open(QIODevice::ReadOnly)) {
            fprintf(stderr, "Couldn't open %s for read: %s\n", PRINT_CAST(tlbfile), PRINT_CAST(file.errorString()));
            return 4;
        }
        QByteArray data = file.readAll();
        QString error;
        bool ok;
        if(manfile.isEmpty())
            ok = attachTypeLibrary(input, 1, data, &error);
        else
            ok = attachManifest(input, 1, data, &error);
        fprintf(stderr, "idc: %s\n", PRINT_CAST(error));
        return ok ? 0 : 4;
    } else if (!idlfile.isEmpty()) {
        idlfile = QDir::toNativeSeparators(idlfile);
        idlfile = quotePath(idlfile);
        fprintf(stderr, "\n\n%s\n\n", PRINT_CAST(idlfile));
        quotePath(input);
        HRESULT res = dumpIdl(input, idlfile, version);

        switch (res) {
        case S_OK:
            break;
        case E_FAIL:
            fprintf(stderr, "IDL generation failed trying to run program %s!\n", PRINT_CAST(input));
            return res;
        case -1:
            fprintf(stderr, "Couldn't open %s for writing!\n", PRINT_CAST(idlfile));
            return res;
        case 1:
            fprintf(stderr, "Malformed appID value in %s!\n", PRINT_CAST(input));
            return res;
        case 2:
            fprintf(stderr, "Malformed typeLibID value in %s!\n", PRINT_CAST(input));
            return res;
        case 3:
            fprintf(stderr, "Class has no metaobject information (error in %s)!\n", PRINT_CAST(input));
            return res;
        case 4:
            fprintf(stderr, "Malformed classID value in %s!\n", PRINT_CAST(input));
            return res;
        case 5:
            fprintf(stderr, "Malformed interfaceID value in %s!\n", PRINT_CAST(input));
            return res;
        case 6:
            fprintf(stderr, "Malformed eventsID value in %s!\n", PRINT_CAST(input));
            return res;

        default:
            fprintf(stderr, "Unknown error writing IDL from %s\n", PRINT_CAST(input));
            return 7;
        }
    }
    return 0;
}

QT_END_NAMESPACE

int main(int argc, char **argv)
{
    return QT_PREPEND_NAMESPACE(runIdc)(argc, argv);
}
