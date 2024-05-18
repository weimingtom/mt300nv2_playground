//20180324
#pragma once

struct _ts; 
struct _is; 

typedef struct _is {
    struct _is *next;
    struct _ts *tstate_head;
    PyObject *modules;
    PyObject *sysdict;
    PyObject *builtins;
    int checkinterval;
} PyInterpreterState;

struct _frame; 

typedef int (*Py_tracefunc)(PyObject *, struct _frame *, int, PyObject *);

#define PyTrace_CALL 0
#define PyTrace_EXCEPTION 1
#define PyTrace_LINE 2
#define PyTrace_RETURN 3

typedef struct _ts {
    struct _ts *next;
    PyInterpreterState *interp;
    struct _frame *frame;
    int recursion_depth;
    int ticker;
    int tracing;
    int use_tracing;
    Py_tracefunc c_profilefunc;
    Py_tracefunc c_tracefunc;
    PyObject *c_profileobj;
    PyObject *c_traceobj;
    PyObject *curexc_type;
    PyObject *curexc_value;
    PyObject *curexc_traceback;
    PyObject *exc_type;
    PyObject *exc_value;
    PyObject *exc_traceback;
    PyObject *dict;
    int tick_counter;
} PyThreadState;

PyInterpreterState * PyInterpreterState_New();
void PyInterpreterState_Clear(PyInterpreterState *);
void PyInterpreterState_Delete(PyInterpreterState *);
PyThreadState * PyThreadState_New(PyInterpreterState *);
void PyThreadState_Clear(PyThreadState *);
void PyThreadState_Delete(PyThreadState *);
void PyThreadState_DeleteCurrent();
PyThreadState * PyThreadState_Get();
PyThreadState * PyThreadState_Swap(PyThreadState *);
PyObject * PyThreadState_GetDict();
extern PyThreadState * _PyThreadState_Current;
#ifdef _DEBUG
#define PyThreadState_GET() PyThreadState_Get()
#else
#define PyThreadState_GET() (_PyThreadState_Current)
#endif
PyInterpreterState * PyInterpreterState_Head();
PyInterpreterState * PyInterpreterState_Next(PyInterpreterState *);
PyThreadState * PyInterpreterState_ThreadHead(PyInterpreterState *);
PyThreadState * PyThreadState_Next(PyThreadState *);
extern unaryfunc _PyThreadState_GetFrame;
