//20180318
#pragma once

typedef struct {
    PyObject_HEAD
    double ob_fval;
} PyFloatObject;

extern PyTypeObject PyFloat_Type;
#define PyFloat_Check(op) PyObject_TypeCheck(op, &PyFloat_Type)
#define PyFloat_CheckExact(op) ((op)->ob_type == &PyFloat_Type)
extern PyObject * PyFloat_FromString(PyObject*, char** junk);
extern PyObject * PyFloat_FromDouble(double);
extern double PyFloat_AsDouble(PyObject *);
#define PyFloat_AS_DOUBLE(op) (((PyFloatObject *)(op))->ob_fval)
extern void PyFloat_AsReprString(char*, PyFloatObject *v);
extern void PyFloat_AsString(char*, PyFloatObject *v);
