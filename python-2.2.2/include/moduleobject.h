//20180318
#pragma once

extern PyTypeObject PyModule_Type;
#define PyModule_Check(op) PyObject_TypeCheck(op, &PyModule_Type)
#define PyModule_CheckExact(op) ((op)->ob_type == &PyModule_Type)
extern PyObject * PyModule_New(char *);
extern PyObject * PyModule_GetDict(PyObject *);
extern char * PyModule_GetName(PyObject *);
extern char * PyModule_GetFilename(PyObject *);
extern void _PyModule_Clear(PyObject *);
