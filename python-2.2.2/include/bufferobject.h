//20180318
#pragma once

extern PyTypeObject PyBuffer_Type;
#define PyBuffer_Check(op) ((op)->ob_type == &PyBuffer_Type)
#define Py_END_OF_BUFFER (-1)
extern PyObject * PyBuffer_FromObject(PyObject *base, int offset, int size);
extern PyObject * PyBuffer_FromReadWriteObject(PyObject *base, int offset, int size);
extern PyObject * PyBuffer_FromMemory(void *ptr, int size);
extern PyObject * PyBuffer_FromReadWriteMemory(void *ptr, int size);
extern PyObject * PyBuffer_New(int size);
