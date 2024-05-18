//20180318
#pragma once

typedef struct {
    PyObject_HEAD
    PyObject *func_code;
    PyObject *func_globals;
    PyObject *func_defaults;
    PyObject *func_closure;
    PyObject *func_doc;
    PyObject *func_name;
    PyObject *func_dict;
    PyObject *func_weakreflist;
} PyFunctionObject;

extern PyTypeObject PyFunction_Type;
#define PyFunction_Check(op) ((op)->ob_type == &PyFunction_Type)
extern PyObject * PyFunction_New(PyObject *, PyObject *);
extern PyObject * PyFunction_GetCode(PyObject *);
extern PyObject * PyFunction_GetGlobals(PyObject *);
extern PyObject * PyFunction_GetDefaults(PyObject *);
extern int PyFunction_SetDefaults(PyObject *, PyObject *);
extern PyObject * PyFunction_GetClosure(PyObject *);
extern int PyFunction_SetClosure(PyObject *, PyObject *);
#define PyFunction_GET_CODE(func) (((PyFunctionObject *)func) -> func_code)
#define PyFunction_GET_GLOBALS(func) (((PyFunctionObject *)func) -> func_globals)
#define PyFunction_GET_DEFAULTS(func) (((PyFunctionObject *)func) -> func_defaults)
#define PyFunction_GET_CLOSURE(func) (((PyFunctionObject *)func) -> func_closure)
extern PyTypeObject PyClassMethod_Type;
extern PyTypeObject PyStaticMethod_Type;
extern PyObject * PyClassMethod_New(PyObject *);
extern PyObject * PyStaticMethod_New(PyObject *);
