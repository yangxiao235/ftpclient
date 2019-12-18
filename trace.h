#ifndef __TRACE_H__850CE873
#define __TRACE_H__850CE873

// TRACE macro for win32
#ifdef _DEBUG
#include <crtdbg.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <stdlib.h>
#include <wchar.h>

#pragma warning( push )
#pragma warning( disable : 4996)
static void TraceWImpl(int line, const wchar_t *file,
                      const wchar_t* format, ...)
{
    va_list args, argsBackup;
    va_start(args, format);
    va_copy(argsBackup, args);

    int elemsInBuf = 0;
    elemsInBuf = _vsnwprintf(nullptr, 0, format, args);
    assert(elemsInBuf >= 0);
    wchar_t *buf = (wchar_t *)calloc(elemsInBuf + 1, sizeof(wchar_t));
    assert(buf);
    _vsnwprintf_s(buf, elemsInBuf + 1, elemsInBuf, format, argsBackup);
    
    va_end(args);
    va_end(argsBackup);

    fwprintf(stderr, L"-----------------------------------------------------------\n");
    fwprintf(stderr, L"TRACE: %s, line %d, in file %s\n", buf, line, file);
    fwprintf(stderr, L"-----------------------------------------------------------\n");
    free(buf);
    buf = 0;
}

static void TraceAImpl(int line, const char *file,
    const char* format, ...)
{
    va_list args, argsBackup;
    va_start(args, format);
    va_copy(argsBackup, args);

    int elemsInBuf = 0;
    elemsInBuf = vsnprintf(nullptr, 0, format, args);
    assert(elemsInBuf >= 0);
    char *buf = (char *)calloc(elemsInBuf + 1, sizeof(char));
    assert(buf);
    _vsnprintf_s(buf, elemsInBuf + 1, elemsInBuf, format, argsBackup);

    va_end(args);
    va_end(argsBackup);

    fprintf(stderr, "-----------------------------------------------------------\n");
    fprintf(stderr, "TRACE: %s, line %d, in file %s\n", buf, line, file);
    fprintf(stderr, "-----------------------------------------------------------\n");
    free(buf);
    buf = 0;
}
#pragma warning( pop ) 

#define _L(x)  L""##x
#define TRACEW(format, ...) TraceWImpl(__LINE__, _L(__FILE__), (format), __VA_ARGS__)
#define TRACEA(format, ...) TraceAImpl(__LINE__, __FILE__, (format), __VA_ARGS__)
#else
// Remove for release mode
#define TRACE  ((void)0)
#define TRACEF ((void)0)
#endif

#endif // __TRACE_H__850CE873