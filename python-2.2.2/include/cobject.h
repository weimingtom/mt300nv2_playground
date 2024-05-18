//20180318
#pragma once

extern PyTypeObject PyCObject_Type;
#define PyCObject_Check(op) ((op)->ob_type == &PyCObject_Type)
extern PyObject * PyCObject_FromVoidPtr(void *cobj, void (*destruct)(void*));
extern PyObject * PyCObject_FromVoidPtrAndDesc(void *cobj, void *desc, void (*destruct)(void*, void*));
extern void * PyCObject_AsVoidPtr(PyObject *);
extern void * PyCObject_GetDesc(PyObject *);
extern void * PyCObject_Import(char *module_name, char *cobject_name);
