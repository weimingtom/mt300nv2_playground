//20180318
#pragma once

typedef struct {
    PyObject_HEAD
    long ob_ival;
} PyIntObject;

extern PyTypeObject PyInt_Type;
#define PyInt_Check(op) PyObject_TypeCheck(op, &PyInt_Type)
#define PyInt_CheckExact(op) ((op)->ob_type == &PyInt_Type)
extern PyObject * PyInt_FromString(char*, char**, int);
extern PyObject * PyInt_FromUnicode(Py_UNICODE*, int, int);
extern PyObject * PyInt_FromLong(long);
extern long PyInt_AsLong(PyObject *);
extern long PyInt_GetMax();
extern PyIntObject _Py_ZeroStruct, _Py_TrueStruct;
#define Py_False ((PyObject *) &_Py_ZeroStruct)
#define Py_True ((PyObject *) &_Py_TrueStruct)
#define PyInt_AS_LONG(op) (((PyIntObject *)(op))->ob_ival)

extern unsigned long PyOS_strtoul(char *, char **, int);
extern long PyOS_strtol(char *, char **, int);
