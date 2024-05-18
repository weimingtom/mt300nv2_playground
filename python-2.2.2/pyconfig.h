//20180406
#pragma once

#define MS_NO_COREDLL

#if defined(_MSC_VER)
#include <io.h>
#else
#include <unistd.h>
#endif
#define HAVE_LIMITS_H
#define HAVE_SYS_UTIME_H
#define HAVE_HYPOT
#define HAVE_TEMPNAM
#define HAVE_TMPFILE
#define HAVE_TMPNAM
#define DONT_HAVE_SIG_ALARM
#define DONT_HAVE_SIG_PAUSE
#define LONG_BIT	32
#define PREFIX ""
#define EXEC_PREFIX ""

#define NT	
#define MS_WIN32
#define MS_WINDOWS

#define COMPILER "[MSC 32 bit (Intel)]"

#define PYTHONPATH ".\\DLLs;.\\lib;.\\lib\\plat-win;.\\lib\\lib-tk"
typedef int pid_t;
#define WORD_BIT 32
#include <stdio.h>
#define HAVE_CLOCK
#define HAVE_STRFTIME
#define HAVE_STRERROR
#define NT_THREADS
#define WITH_THREAD
#define USE_SOCKET

#define HAVE_LONG_LONG 1
#if defined(_MSC_VER)
#define LONG_LONG __int64
#else
#define LONG_LONG long long
#endif


#define PLATFORM "win32"
#define SIZEOF_VOID_P 4
#define SIZEOF_TIME_T 4
#define SIZEOF_OFF_T 4
#define SIZEOF_FPOS_T 8
#define SIZEOF_HKEY 4

#define SIZEOF_SHORT 2
#define SIZEOF_INT 4
#define SIZEOF_LONG 4
#define SIZEOF_LONG_LONG 8

#define HAVE_TZNAME
#define RETSIGTYPE void
#define socklen_t int
#define STDC_HEADERS 1
#define BAD_STATIC_FORWARD 1
#define HAVE_PUTENV
#define HAVE_PROTOTYPES
#define Py_USING_UNICODE
#define PY_UNICODE_TYPE unsigned short
#define Py_UNICODE_SIZE SIZEOF_SHORT
#define HAVE_USABLE_WCHAR_T
#define WITH_CYCLE_GC 1
#define HAVE_DYNAMIC_LOADING
#define HAVE_FTIME
#define HAVE_GETPEERNAME
#define HAVE_GETPID
#define HAVE_MKTIME
#define HAVE_SETVBUF
#define HAVE_FCNTL_H 1
#define HAVE_SIGNAL_H 1
#define HAVE_STDARG_H 1
#define HAVE_STDARG_PROTOTYPES
#define HAVE_STDDEF_H 1
#define HAVE_STDLIB_H 1
#define HAVE_LIBNSL 1
#define HAVE_LIBSOCKET 1
