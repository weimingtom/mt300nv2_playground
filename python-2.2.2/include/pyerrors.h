//20180324
#pragma once

void PyErr_SetNone(PyObject *);
void PyErr_SetObject(PyObject *, PyObject *);
void PyErr_SetString(PyObject *, const char *);
PyObject * PyErr_Occurred();
void PyErr_Clear();
void PyErr_Fetch(PyObject **, PyObject **, PyObject **);
void PyErr_Restore(PyObject *, PyObject *, PyObject *);
int PyErr_GivenExceptionMatches(PyObject *, PyObject *);
int PyErr_ExceptionMatches(PyObject *);
void PyErr_NormalizeException(PyObject**, PyObject**, PyObject**);

extern PyObject * PyExc_Exception;
extern PyObject * PyExc_StopIteration;
extern PyObject * PyExc_StandardError;
extern PyObject * PyExc_ArithmeticError;
extern PyObject * PyExc_LookupError;
extern PyObject * PyExc_AssertionError;
extern PyObject * PyExc_AttributeError;
extern PyObject * PyExc_EOFError;
extern PyObject * PyExc_FloatingPointError;
extern PyObject * PyExc_EnvironmentError;
extern PyObject * PyExc_IOError;
extern PyObject * PyExc_OSError;
extern PyObject * PyExc_ImportError;
extern PyObject * PyExc_IndexError;
extern PyObject * PyExc_KeyError;
extern PyObject * PyExc_KeyboardInterrupt;
extern PyObject * PyExc_MemoryError;
extern PyObject * PyExc_NameError;
extern PyObject * PyExc_OverflowError;
extern PyObject * PyExc_RuntimeError;
extern PyObject * PyExc_NotImplementedError;
extern PyObject * PyExc_SyntaxError;
extern PyObject * PyExc_IndentationError;
extern PyObject * PyExc_TabError;
extern PyObject * PyExc_ReferenceError;
extern PyObject * PyExc_SystemError;
extern PyObject * PyExc_SystemExit;
extern PyObject * PyExc_TypeError;
extern PyObject * PyExc_UnboundLocalError;
extern PyObject * PyExc_UnicodeError;
extern PyObject * PyExc_ValueError;
extern PyObject * PyExc_ZeroDivisionError;
extern PyObject * PyExc_WindowsError;
extern PyObject * PyExc_MemoryErrorInst;
extern PyObject * PyExc_Warning;
extern PyObject * PyExc_UserWarning;
extern PyObject * PyExc_DeprecationWarning;
extern PyObject * PyExc_SyntaxWarning;
extern PyObject * PyExc_OverflowWarning;
extern PyObject * PyExc_RuntimeWarning;

extern int PyErr_BadArgument();
extern PyObject * PyErr_NoMemory();
extern PyObject * PyErr_SetFromErrno(PyObject *);
extern PyObject * PyErr_SetFromErrnoWithFilename(PyObject *, char *);
extern PyObject * PyErr_Format(PyObject *, const char *, ...);
extern PyObject * PyErr_SetFromWindowsErrWithFilename(int, const char *);
extern PyObject * PyErr_SetFromWindowsErr(int);
extern void PyErr_BadInternalCall();
extern void _PyErr_BadInternalCall(char *filename, int lineno);
#define PyErr_BadInternalCall() _PyErr_BadInternalCall(__FILE__, __LINE__)
PyObject * PyErr_NewException(char *name, PyObject *base, PyObject *dict);
extern void PyErr_WriteUnraisable(PyObject *);
extern int PyErr_Warn(PyObject *, char *);
extern int PyErr_WarnExplicit(PyObject *, char *, char *, int, char *, PyObject *);
extern int PyErr_CheckSignals();
extern void PyErr_SetInterrupt();
extern void PyErr_SyntaxLocation(char *, int);
extern PyObject * PyErr_ProgramText(char *, int);
#if defined(_MSC_VER)
#define vsnprintf _vsnprintf
#endif
#include <stdarg.h>
extern int PyOS_snprintf(char *str, size_t size, const char  *format, ...);
extern int PyOS_vsnprintf(char *str, size_t size, const char  *format, va_list va);
