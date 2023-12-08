#include "udebug.h"


#include <stdio.h> //defines FILENAME_MAX

#ifdef Q_OS_WIN
    #include <qt_windows.h>
    //#define DBGHELP_TRANSLATE_TCHAR //force Unicode version of DbgHelp functions
    //#pragma comment( lib, "dbghelp.lib" )
    #include <DbgHelp.h>
#endif //Q_OS_WIN

static bool uDebugAt = false;
static quint32 uDebugCount = 0;

bool xdAtEnabled() {
    return uDebugCount > 0;
}
bool setXdAtEnabled(bool enabled) {
    bool tmp = uDebugAt;
    uDebugAt = enabled;

    if(enabled)
        uDebugCount++;
    else if(uDebugCount > 0)
        uDebugCount--;

    return tmp != enabled;
}

void UDebug::init()
{
#ifdef QMAKE_WATCH
#  ifdef Q_OS_WIN
    HANDLE process = ::GetCurrentProcess();
    //get App directory as search-path
    wchar_t path[FILENAME_MAX];
    DWORD size = ::GetModuleFileNameW(NULL, path, sizeof(path)/sizeof(path[0]));
    if(size > 0) {
        while (size > 0 && path[size-1] != '\\')
            --size;
        path[size] = 0;
    }
    //prepare debug help
    DWORD options = ::SymGetOptions();
    options |= SYMOPT_LOAD_LINES;
    ::SymSetOptions(options);
    ::SymInitializeW( process, path, TRUE );
#  endif
#endif
}

#define UDEBUG_NO_STACK 1

void UDebug::printStack(const QString &msg, int skipLatest)
{
#ifndef QMAKE_WATCH
    Q_UNUSED(msg) Q_UNUSED(skipLatest)
#else
#  ifdef Q_OS_WIN
    HANDLE process = GetCurrentProcess();

    void *stack[ 100 ];
    unsigned short frames= CaptureStackBackTrace( 0, 100, stack, NULL );

    //reserve memory for line
    IMAGEHLP_LINE *line = reinterpret_cast<IMAGEHLP_LINE *>(calloc( sizeof(IMAGEHLP_LINE), 1 ));
    line->SizeOfStruct = sizeof(IMAGEHLP_LINE);
    DWORD dwDisplacement;

    if(msg.size() > 0) {
        if(skipLatest >= frames)
            skipLatest = frames - 1;
        if(skipLatest >= 0) {
            SymGetLineFromAddr(process, (DWORD)(stack[skipLatest]), &dwDisplacement, line);
        }
        if(line->FileName) {
            this->printLocation(xdTranslatePath(line->FileName), line->LineNumber);
            this->stream->buffer += msg;
        } else {
            stream->buffer += QLatin1String("WARNING: ");
            stream->buffer += msg;
        }
        if(stream->buffer.endsWith(QLatin1Char(' ')))
            stream->buffer.chop(1);
    }

#    if UDEBUG_NO_STACK
    //Skip stack-trace
    free(line);
    return;
#    else
    //Separate message-log from stack-trace
    this->stream->buffer += '\n';
    //reserve memory for a single symbol
    SYMBOL_INFO  *symbol = reinterpret_cast<SYMBOL_INFO *>(calloc( sizeof(SYMBOL_INFO) + 256 * sizeof(char), 1 ));
    symbol->MaxNameLen   = 255;
    symbol->SizeOfStruct = sizeof( SYMBOL_INFO );

    for(unsigned int i = skipLatest; i < frames; ++i) {
        //source-code file-names should not contain any unicode
        SymGetLineFromAddr(process, (DWORD)(stack[i]), &dwDisplacement, line);
        SymFromAddr( process, reinterpret_cast<DWORD64>( stack[i] ), 0, symbol);

        if(strcmp(symbol->Name, "main") == 0)
            break;

        this->stream->buffer += QString::fromPrintf("%i: %s - 0x%08X "
                , frames - i - 1, symbol->Name, symbol->Address
            );
        if(line->FileName) {
            int size = strlen(line->FileName);
            if(size > 0) {
                while (size > 0 && line->FileName[size-1] != '\\')
                    --size;
                this->stream->buffer += xdTranslatePath(line->FileName + size);
                this->stream->buffer += " : ";
                this->stream->buffer += QString::number(line->LineNumber, 10);
            }
        }
        this->stream->buffer += '\n';
    }
    //dellocate buffers
    free(line);
    free(symbol);
#    endif //UDEBUG_NO_STACK
#  endif //Q_OS_WIN
#endif //QMAKE_WATCH
}
