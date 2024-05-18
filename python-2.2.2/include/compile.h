//20180318
#pragma once

typedef struct {
    PyObject_HEAD
    int co_argcount;
    int co_nlocals;
    int co_stacksize;
    int co_flags;
    PyObject *co_code;
    PyObject *co_consts;
    PyObject *co_names;
    PyObject *co_varnames;
    PyObject *co_freevars;
    PyObject *co_cellvars;
    PyObject *co_filename;
    PyObject *co_name;
    int co_firstlineno;
    PyObject *co_lnotab;
} PyCodeObject;

#define CO_OPTIMIZED 0x0001
#define CO_NEWLOCALS 0x0002
#define CO_VARARGS 0x0004
#define CO_VARKEYWORDS 0x0008
#define CO_NESTED 0x0010
#define CO_GENERATOR 0x0020

#define CO_GENERATOR_ALLOWED    0x1000
#define CO_FUTURE_DIVISION    	0x2000

extern PyTypeObject PyCode_Type;
#define PyCode_Check(op) ((op)->ob_type == &PyCode_Type)
#define PyCode_GetNumFree(op) (PyTuple_GET_SIZE((op)->co_freevars))

#define CO_MAXBLOCKS 20

struct _node;
PyCodeObject * PyNode_Compile(struct _node *, char *);
PyCodeObject * PyCode_New(int, int, int, int, PyObject *, PyObject *, PyObject *, PyObject *, PyObject *, PyObject *, PyObject *, PyObject *, int, PyObject *); 
int PyCode_Addr2Line(PyCodeObject *, int);

typedef struct {
    int ff_found_docstring;
    int ff_last_lineno;
    int ff_features;
} PyFutureFeatures;

PyFutureFeatures * PyNode_Future(struct _node *, char *);
PyCodeObject * PyNode_CompileFlags(struct _node *, char *, PyCompilerFlags *);

#define FUTURE_NESTED_SCOPES "nested_scopes"
#define FUTURE_GENERATORS "generators"
#define FUTURE_DIVISION "division"

#define _PyCode_GETCODEPTR(co, pp) ((*(co)->co_code->ob_type->tp_as_buffer->bf_getreadbuffer)((co)->co_code, 0, (void **)(pp)))

