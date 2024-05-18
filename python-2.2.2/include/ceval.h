//20180318
#pragma once

PyObject * PyEval_CallObjectWithKeywords(PyObject *, PyObject *, PyObject *);
#undef PyEval_CallObject
PyObject * PyEval_CallObject(PyObject *, PyObject *);
#define PyEval_CallObject(func,arg) PyEval_CallObjectWithKeywords(func, arg, (PyObject *)NULL)
PyObject * PyEval_CallFunction(PyObject *obj, char *format, ...);
PyObject * PyEval_CallMethod(PyObject *obj, char *methodname, char *format, ...);
void PyEval_SetProfile(Py_tracefunc, PyObject *);
void PyEval_SetTrace(Py_tracefunc, PyObject *);
PyObject * PyEval_GetBuiltins();
PyObject * PyEval_GetGlobals();
PyObject * PyEval_GetLocals();
PyObject * PyEval_GetOwner();
PyObject * PyEval_GetFrame();
int PyEval_GetRestricted();
int PyEval_MergeCompilerFlags(PyCompilerFlags *cf);

int Py_FlushLine();
int Py_AddPendingCall(int (*func)(void *), void *arg);
int Py_MakePendingCalls();
void Py_SetRecursionLimit(int);
int Py_GetRecursionLimit();

char * PyEval_GetFuncName(PyObject *);
char * PyEval_GetFuncDesc(PyObject *);
extern PyThreadState * PyEval_SaveThread();
extern void PyEval_RestoreThread(PyThreadState *);
extern void PyEval_InitThreads();
extern void PyEval_AcquireLock();
extern void PyEval_ReleaseLock();
extern void PyEval_AcquireThread(PyThreadState *tstate);
extern void PyEval_ReleaseThread(PyThreadState *tstate);
extern void PyEval_ReInitThreads();

extern int _PyEval_SliceIndex(PyObject *, int *);
