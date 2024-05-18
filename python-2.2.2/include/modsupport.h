//20180318
#pragma once

#include <stdarg.h>

extern int PyArg_Parse(PyObject *, char *, ...);
extern int PyArg_ParseTuple(PyObject *, char *, ...);
extern int PyArg_ParseTupleAndKeywords(PyObject *, PyObject *, char *, char **, ...);
extern int PyArg_UnpackTuple(PyObject *, char *, int, int, ...);
extern PyObject * Py_BuildValue(char *, ...);
extern int PyArg_VaParse(PyObject *, char *, va_list);
extern PyObject * Py_VaBuildValue(char *, va_list);

extern int PyModule_AddObject(PyObject *, char *, PyObject *);
extern int PyModule_AddIntConstant(PyObject *, char *, long);
extern int PyModule_AddStringConstant(PyObject *, char *, char *);

#define PYTHON_API_VERSION 1011
#define PYTHON_API_STRING "1011"

#ifdef _DEBUG
#define Py_InitModule4 Py_InitModule4TraceRefs
#endif

extern PyObject * Py_InitModule4(char *name, PyMethodDef *methods, char *doc, PyObject *self, int apiver);
#define Py_InitModule(name, methods) Py_InitModule4(name, methods, (char *)NULL, (PyObject *)NULL, PYTHON_API_VERSION)
#define Py_InitModule3(name, methods, doc) Py_InitModule4(name, methods, doc, (PyObject *)NULL, PYTHON_API_VERSION)
extern char * _Py_PackageContext;
